#include <stdint.h>
#include <stm32f101xb.h>


static inline void spin(volatile uint32_t count) {
  while (count--) (void) 0;
}

static inline void turnOnLed() {
  // active low , so we set BR13 (reset bit port 13) to turn on
  GPIOC->BSRR &= ~(GPIO_BSRR_BS13);
  GPIOC->BSRR |= GPIO_BSRR_BR13;
}

static inline void turnOffLed() {
  // active low , so we set BS13 (set bit port 13) to turn off
  GPIOC->BSRR &= ~(GPIO_BSRR_BR13);
  GPIOC->BSRR |= GPIO_BSRR_BS13;
}


static inline void setLedPortOutput() {
  // LED located on PC13
  // Set Mode 11: Output mode 50 MHz
  // set CRF 01 : OpenDrain Output
  GPIOC->CRH |= (GPIO_CRH_MODE13 | GPIO_CRH_CNF13_0);
}

static inline void enablePeripheralClockCMSIS() {
  // set bit on register apb2enr enable clock peripheral for port c
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
}

int main() {
    enablePeripheralClockCMSIS();
    setLedPortOutput();
    while(1){
      turnOffLed();
      spin(999999);
      turnOnLed();
      spin(999999);
    }
    return 0;
}

static inline void selectSystemClock() {
  // Enable HSI
  RCC->CR |= RCC_CR_HSION;

  // magic number from SystemInit stm32f1xx for clock register CFGR : 0xF0FF0000U
  RCC->CFGR |= 0xF0FF0000U;
}

extern void _estack(void);

// startup code
__attribute__((naked, noreturn)) void _reset(void) {
  extern long _sbss, _ebss, _sdata, _edata, _sidata;
  for (long *dst = &_sbss; dst < &_ebss; dst++) *dst=0;
  for (long *dst = &_sdata, *src = &_sidata; dst < &_edata;) *dst++ = *src++;  
  selectSystemClock();
  main();
  for (;;) (void) 0;
}

// 16 standard and 91 STM32-specific handlers
__attribute__((section(".vectors"))) void (*const tab[16 + 91])(void) = {
  _estack, _reset
};