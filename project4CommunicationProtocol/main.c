#include <stdint.h>
#include <stm32f101xb.h>
#include "clk.h"
#include "spi.h"
#include "sdcard.h"
#include "sdcard_spi.h"
#include "queue.h"
#include "led.h"
#include "uart.h"
#include "i2c.h"
#include <string.h>
#include "vl53l1x/core/VL53L1X_api.h"

volatile uint32_t mticks = 0;

void SysTick_Handler(void) {
    mticks++; // Increment every 1ms
}

void Delay(uint32_t ms) {
    uint32_t start =   mticks;
    while((mticks - start) < ms);
}

uint8_t checkArray(int rxArr[], int txArr[], int size) {
    for(int i=0; i<3;i++){
        if(rxArr[i] != txArr[i]) {
            return 0;
        }
    }
    return 1;
}

void HardFault_Handler() {
    uint32_t active_irq  = SCB->ICSR & 0x1FF;  // bits[8:0] = active exception number
    // Subtract 16 to get the peripheral IRQ number
    // e.g. active_irq=22 → IRQ #6 → TIM1_UP_IRQn on STM32F103

    uint32_t ipending0 = NVIC->ISPR[0]; // pending IRQs  0-31
    uint32_t ipending1 = NVIC->ISPR[1]; // pending IRQs 32-63
    uint32_t iactive0  = NVIC->IABR[0]; // active IRQs   0-31
    uint32_t iactive1  = NVIC->IABR[1]; // active IRQs  32-63
}

int main() {
    //     /* Freeze peripherals when core is halted by debugger */
    // DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP    /* Freeze IWDG */
    //            |  DBGMCU_CR_DBG_WWDG_STOP    /* Freeze WWDG */
    //         //    |  DBGMCU_CR_DBG_TIM1_STOP    /* Freeze TIM1 */
    //            |  DBGMCU_CR_DBG_TIM2_STOP    /* Freeze TIM2 */
    //            |  DBGMCU_CR_DBG_TIM3_STOP    /* Freeze TIM3 */
    //            |  DBGMCU_CR_DBG_TIM4_STOP;   /* Freeze TIM4 */
    CLK_Init();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000);
    Init_SPI1(); 
    Uart* uartInstance = NULL;
    uartInstance =  UartCreate();
    if (uartInstance != NULL){
        init_uart(uartInstance, (uint32_t)115200);
        uart_transmit_string(uartInstance,(uint8_t*)"UART RESET");
        uart_transmit(uartInstance, '\n');
        uart_transmit(uartInstance, '\r');
    }
    Led* led = LedCreate();
    if(led != NULL)
        init_led_gpio(led,PORTC,OUTPUT_PUSHPULL);
    uint8_t initStatus = 0;
    uint32_t timeout = 40;
    Sdcard sdcard1;
    do {
        initStatus = init_sdcard(&sdcard1);
        if (initStatus == 0x01) break;
        // Delay(10);
    } while (--timeout);

    uint8_t buff[512] = {};
    memset(buff, 0x0, sizeof(buff));

    Q_T sdcardQueue;
    Q_Init(&sdcardQueue);

    // dummy data to write
    char dummy[] = "hello, world";
    char step[] = "indian speed";
    Data temp;
    uint8_t read_res;
    uint8_t count_error=0;
    uint8_t i2cDataToBeSent = 0x01;
    I2C_Init(0);
    VL53L1X_SensorInit((uint16_t)0x29);

    while(1){
        if(initStatus == 0x01) {
            I2C_Write(0x29,&i2cDataToBeSent,1);
            read_res = read_single_block(0x00, buff);
            if(read_res != 0) {
                uart_transmit_string(uartInstance, (uint8_t *)"Error Read");
                uart_transmit(uartInstance, '\n');
                uart_transmit(uartInstance, '\r');
                memset(buff,0,40);
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            if (buff[0] != 0)
                Q_Enqueue(&sdcardQueue, buff, 40);
            
            if(Q_Dequeue(&sdcardQueue, &temp)){
                for(int i = 0 ; i < 50 ; i++){
                    uart_transmit(uartInstance, temp.data[i]);
                }
            }
            uart_transmit(uartInstance, '\n');
            uart_transmit(uartInstance, '\r');
            if(write_single_block(1024, (uint16_t*)dummy)){
                uart_transmit_string(uartInstance, (uint8_t *) "Error Write");
                uart_transmit(uartInstance, '\n');
                uart_transmit(uartInstance, '\r');
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            blink_fast(led);
            read_res = read_single_block(1024, buff); 
            if (read_res != 0) {
                uart_transmit_string(uartInstance, (uint8_t *) "Error Read");
                uart_transmit(uartInstance, '\n');
                uart_transmit(uartInstance, '\r');
                memset(buff,0,40);
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            if (buff[0] != 0)
                Q_Enqueue(&sdcardQueue, buff, 40);
            
            if(Q_Dequeue(&sdcardQueue, &temp)) {
                for(int i = 0 ; i < 50 ; i++){
                    uart_transmit(uartInstance, temp.data[i]);
                }
            }

            uart_transmit(uartInstance, '\n');
            uart_transmit(uartInstance, '\r');
            if(write_single_block(0x00, (uint16_t*)step)) {
                uart_transmit_string(uartInstance, (uint8_t *) "Error Write");
                uart_transmit(uartInstance, '\n');
                uart_transmit(uartInstance, '\r');
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            blink_fast(led);
        } else {
            blink_slow(led);
            NVIC_SystemReset();
        }

    }

    return 0;
}