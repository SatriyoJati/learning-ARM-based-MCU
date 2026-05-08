#include "uart.h"
#include "stm32f100xb.h"
#include "stdint.h"
#include "stddef.h"

#define MAXUART 4

Uart uarts[MAXUART];
static int top = -1;
static Uart* free_stack[MAXUART] = {NULL};
static uint8_t initialized = 0;

static void UartInstancePoolInit() {
    for(int i=0; i< MAXUART; i++) {
        free_stack[++top] = &uarts[i];
    }
    initialized = 1;
}

Uart* UartCreate() {
    if(!initialized) UartInstancePoolInit();

    if (top < 0) return NULL;

    Uart* uart = free_stack[top--];
    return uart;
}

static uint32_t get_apb2_clk() {
    uint32_t apb2scaler = 0;
    uint32_t usart1_clock = 0;

    uint32_t prescaler2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> (RCC_CFGR_PPRE2_Pos);

    if (prescaler2 < 4) {
        apb2scaler = 1 ;
    } else {
        apb2scaler = 1 << (prescaler2 - 3);
    }

    usart1_clock = SystemCoreClock / apb2scaler;

    return usart1_clock;
}

static void  set_uart_gpio() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN;

    // reset gpio tx rx
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
    GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);

    //TX : af out 50 MHz
    GPIOA->CRH |= (GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1);

    //RX : float input, input mode
    GPIOA->CRH |= GPIO_CRH_CNF10_0;
}

uint8_t init_uart(Uart *uart, uint32_t bdRate)
{
    uint32_t usart1_clk = get_apb2_clk();
    set_uart_gpio();

    USART1->CR1 |= USART_CR1_UE;
    USART1->CR1 |= USART_CR1_M ;

    // USART1->CR2 |= USART_CR2_STOP_1;

    // for now only suport 72Mhz
    if (usart1_clk != 72000000)
        return 1;

    USART1->BRR = 0x271;
    
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    return 0;
}

uint8_t uart_transmit(Uart *uart, uint8_t data)
{
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = data;
    return 0;
}

uint8_t uart_receive(Uart *uart)
{
    while(!(USART1->SR & USART_SR_RXNE));
    return (uint8_t) (USART1->DR);
}

uint8_t uart_transmit_string(Uart *uart, uint8_t *pstr)
{
    while(*pstr){
        uart_transmit(uart, *pstr);
        pstr++;
    }

    return 0;
}

uint8_t uart_receive_string(Uart *uart, uint8_t *buff)
{
    while(*buff){
        *buff = uart_receive(uart);
        buff++;
    }
    return 0;
}
