#include <stdint.h>
#include <stm32f101xb.h>
#include "clk.h"
#include "spi.h"
#include "sdcard.h"
#include "sdcard_spi.h"

volatile uint32_t mticks = 0;

void SysTick_Handler(void) {
    mticks++; // Increment every 1ms
}

void Delay(uint32_t ms) {
    uint32_t start =   mticks;
    while((mticks - start) < ms);
}

void init_led_gpio() {
    // 1. Enable Clock for GPIOC (Bit 4 in RCC_APB2ENR)
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);

    GPIOC->CRH |= (GPIO_CRH_MODE13_1);
}

uint8_t checkArray(int rxArr[], int txArr[], int size) {
    for(int i=0; i<3;i++){
        if(rxArr[i] != txArr[i]) {
            return 0;
        }
    }
    return 1;
}

int main() {
    CLK_Init();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000);
    init_led_gpio();
    // Init_LoopBack_SPI_Test();  
    Init_SPI1(); 
    uint8_t tx = 0x85;
    uint8_t rx;
    uint8_t txArr[] = {0x55, 0x11, 0x10};
    uint8_t rxArr[3];
    uint8_t initStatus = 0;
    uint32_t timeout = 40;
    Sdcard sdcard1;
    do {
        initStatus = init_sdcard(&sdcard1);
        if (initStatus == 0x01) break;
        // Delay(10);
    } while (--timeout);
    // dataout = init_sdcard();
    // SD_CardInfo sdcardinfo;
    // dataout = SD_Init(&sdcardinfo);

    while(1){
        // SPI_Transmit_Receive_MultiByte(txArr,3,rxArr,3);
        // dataout = spi1_transfer_single(tx);
        if(initStatus == 0x01) {
            GPIOC->BSRR |= (GPIO_BSRR_BS13);
            Delay(100);
            GPIOC->BSRR |= (GPIO_BSRR_BR13);
            Delay(100);
        } else {
            GPIOC->BSRR |= (GPIO_BSRR_BS13);
            Delay(1000);
            GPIOC->BSRR |= (GPIO_BSRR_BR13);
            Delay(1000);

        }

    }

    return 0;
}