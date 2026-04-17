#include <stm32f101xb.h>

void CLK_Init(){
    // enable HSE
    RCC->CR |= RCC_CR_HSEON;
    // wait until stable
    while(!(RCC->CR & RCC_CR_HSERDY));

    // use HSE
    RCC->CFGR |= RCC_CFGR_PLLSRC;
    // PLL MUL x9
    RCC->CFGR |= RCC_CFGR_PLLMULL9;
    // enabling PLL
    RCC->CR |= RCC_CR_PLLON;

    while(!(RCC->CR & RCC_CR_PLLRDY));

    // now sysclk use HSE (8KHz) x9 PLLMUL , we get 72MHz
    // AHB prescaler div 1
    RCC->CFGR |= RCC_CFGR_HPRE_0;
    // APB2 prescaler div 1
    RCC->CFGR |= RCC_CFGR_PPRE2_0;
    // APB1 prescaler div 2 36 MHz
    RCC->CFGR |= RCC_CFGR_PPRE1_0;

    // Select PLL as clock source
    RCC->CFGR |= (RCC_CFGR_SW_PLL);
}