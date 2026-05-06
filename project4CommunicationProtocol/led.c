#include "led.h"
#include <stm32f101xb.h>
#include <stddef.h>

static Led leds[MAXPOOL];
static Led* free_stack[MAXPOOL];
static int top = -1;
static uint8_t initialized = 0;

static void leds_pool_init() {
    for(int i=0; i< MAXPOOL; i++) {
        free_stack[++top] = &leds[i];
    }
    initialized = 1;
}

Led* LedCreate() {
    if(!initialized) leds_pool_init();

    if (top < 0) return NULL;

    Led* led = free_stack[top--];
    return led;
}

uint8_t init_led_gpio(Led * led , uint32_t port, uint32_t mode) {
    // 1. Enable Clock for GPIOC (Bit 4 in RCC_APB2ENR)
    RCC->APB2ENR |= port;
    led->port = port;

    led->pinNumber=13;
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);

    switch (mode)
    {
    case OUTPUT_PUSHPULL:
        /* code */
        GPIOC->CRH |= (GPIO_CRH_MODE13_1);
        led->mode= OUTPUT_PUSHPULL;
        break;
    
    case OUTPUT_ODRAIN:
        GPIOC->CRH |= (GPIO_CRH_MODE13_0 | GPIO_CRH_CNF13_0);
        led->mode= OUTPUT_ODRAIN;
    default:
        break;
    }  

    led->initStatus=1;
    return 0;
}


uint8_t blink_fast(Led* led) {
    if(!led->initStatus)
        return 1;
    GPIOC->BSRR |= (GPIO_BSRR_BS13);
    Delay(100);
    GPIOC->BSRR |= (GPIO_BSRR_BR13);
    Delay(100);

    return 0;
}

uint8_t blink_slow(Led* led) {
    if (!led->initStatus)
        return 1;
    GPIOC->BSRR |= (GPIO_BSRR_BS13);
    Delay(1000);
    GPIOC->BSRR |= (GPIO_BSRR_BR13);
    Delay(1000);
    return 0;
}
