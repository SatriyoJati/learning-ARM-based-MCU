#include "timer.h"
#include "stm32f103xb.h"

int g_is_tim2_init = 0;


int TIM2_Init(void) {

    if (g_is_tim2_init) {
        return 0;
    }
    // Enable clock for TIM2 peripheral
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    // Set Prescaler: 72MHz / 72 = 1MHz (1 tick = 1 microsecond)
    TIM2->PSC = (SystemCoreClock / 1000000) - 1;
    
    // Set Auto-Reload to max value
    TIM2->ARR = 0xFFFF;
    
    // Generate update event to load prescaler value immediately
    TIM2->EGR |= TIM_EGR_UG;
    
    TIM2->CNT = 0;  // ← add before TIM_CR1_CEN

    // Enable the counter
    TIM2->CR1 |= TIM_CR1_CEN;
    g_is_tim2_init = 1;

    return 0;
}

int DelayUs(uint32_t us) {
    if (!g_is_tim2_init) {
        return 1;
    }
    uint32_t start = (uint16_t) TIM2->CNT;

    while (((uint16_t)TIM2->CNT - start) < (uint16_t) us);
    return 0;
}
