#include <stm32f101xb.h>
#include <stddef.h>
#include "spi.h"

void SPI_Init_Slave() {
    SPI1->CR1 |= SPI_CR1_DFF;
    SPI1->CR1 |= SPI_CR1_CPOL | SPI_CR1_CPHA;

    SPI1->CR1 |= SPI_CR1_LSBFIRST;
    SPI1->CR1 |= SPI_CR1_SSM;
    SPI1->CR1 &= ~SPI_CR1_MSTR;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void SPI_Transmit_Interrupt() {
    SPI1->CR2 |= SPI_CR2_TXEIE;

    while(!(SPI1->SR & SPI_SR_TXE));

    uint16_t data = SPI1->DR;
}

uint8_t SPI_Receive_Interrupt(){
    uint8_t data;

    while(!(SPI1->SR & SPI_SR_RXNE))
    data = SPI1->DR ;
    return data;
}

void SPI_Transmit_Receive_MultiByte(uint8_t *dataIn , size_t size, uint8_t* dataOut, size_t sizeout) {
    SPI1->DR = dataIn[0];

    for(size_t i = 1; i < size && i < sizeout ; i++) {
        while(!(SPI1->SR & SPI_SR_RXNE));
        dataOut[i-1] = SPI1->DR;

        while(!(SPI1->SR & SPI_SR_TXE));
        SPI1->DR = dataIn[i];

    }
    while(!(SPI1->SR & SPI_SR_RXNE));
    dataOut[sizeout-1] = SPI1->DR;
}


/*
    SPI1 NSS    PA4
    SPI1 SCK    PA5
    SPI1 MISO   PA6
    SPI1 MOSI   PA7
*/
void Init_LoopBack_SPI_Test() {
    // use SPI1 and GPIOA 
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // Clear Bits first
    GPIOA->CRL &= ~( GPIO_CRL_CNF4  | GPIO_CRL_MODE4  |
                 GPIO_CRL_CNF5  | GPIO_CRL_MODE5  |
                 GPIO_CRL_CNF6  | GPIO_CRL_MODE6  |
                 GPIO_CRL_CNF7  | GPIO_CRL_MODE7  );

    //mosi
    GPIOA->CRL |= GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7;

    //sck
    GPIOA->CRL |= GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5;

    //miso
    GPIOA->CRL |=  GPIO_CRL_CNF6_1;    /* CNF6 = 10 → input with pull */
    GPIOA->ODR |=  GPIO_ODR_ODR6;      

    //nss
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOA->CRL |= GPIO_CRL_CNF4_0;
    GPIOA->CRL &= ~( GPIO_CRL_MODE4);


    SPI1->CR1 = 0;

    SPI1->CR1 |= SPI_CR1_MSTR;     // Master mode
    SPI1->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM; // Software slave management

    SPI1->CR1 |= SPI_CR1_BR;     // Baud rate (e.g. fPCLK/8)

    SPI1->CR1 |= SPI_CR1_SPE;      // Enable SPI
}

void init_SPI1_AIVer() {
    /* ── Enable clocks ─────────────────────────────────────────── */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN   /* GPIOA clock */
                 |  RCC_APB2ENR_AFIOEN;  /* AFIO  clock */

    /* ── PA4 : CS — general-purpose output, push-pull, 10 MHz ─── */
    /* CRL bits [19:16] for PA4 → MODE=01 (out 10MHz), CNF=00 (GP push-pull) */
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);    
    GPIOA->CRL |=  GPIO_CRL_MODE4_0;   /* MODE4 = 01 → 10 MHz output */
    /* CNF4 = 00 → general purpose push-pull (default after mask) */

    /* CS high (deselected) */
    GPIOA->BSRR = GPIO_BSRR_BS4;

    /* ── PA5 : SCK — AF push-pull, 50 MHz ─────────────────────── */
    /* CRL bits [23:20] for PA5 → MODE=11 (out 50MHz), CNF=10 (AF push-pull)*/
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
    GPIOA->CRL |=  GPIO_CRL_CNF5_1     /* CNF5 = 10 → AF push-pull */
               |   GPIO_CRL_MODE5_0    /* MODE5 = 11 → 50 MHz */
               |   GPIO_CRL_MODE5_1;

    /* ── PA6 : MISO — input with pull-up ──────────────────────── */
    /* CRL bits [27:24] for PA6 → MODE=00 (input), CNF=10 (input pull-up/dn)*/
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL |=  GPIO_CRL_CNF6_1;    /* CNF6 = 10 → input with pull */
    GPIOA->ODR |=  GPIO_ODR_ODR6;      /* ODR=1 → pull-up enabled */

    /* ── PA7 : MOSI — AF push-pull, 50 MHz ────────────────────── */
    /* CRL bits [31:28] for PA7 → MODE=11 (out 50MHz), CNF=10 (AF push-pull)*/
    GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |=  GPIO_CRL_CNF7_1     /* CNF7 = 10 → AF push-pull */
               |   GPIO_CRL_MODE7_0    /* MODE7 = 11 → 50 MHz */
               |   GPIO_CRL_MODE7_1;
}

void Init_SPI1() {
        // use SPI1 and GPIOA 
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // Clear Bits first
    GPIOA->CRL &= ~( GPIO_CRL_CNF4  | GPIO_CRL_MODE4  |
                 GPIO_CRL_CNF5  | GPIO_CRL_MODE5  |
                 GPIO_CRL_CNF6  | GPIO_CRL_MODE6  |
                 GPIO_CRL_CNF7  | GPIO_CRL_MODE7  );

    //mosi
    GPIOA->CRL |= GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7; // cnf 10 mode 11

    //miso // cnf 10 mosi 00
    GPIOA->CRL |= GPIO_CRL_CNF6_1 ; 
    GPIOA->ODR |=  GPIO_ODR_ODR6;

    //sck cnf 10 mode 11
    GPIOA->CRL |= GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5;

    //nss
    GPIOA->CRL &= ~(GPIO_CRL_CNF4);
    GPIOA->CRL &= ~( GPIO_CRL_MODE4);
    GPIOA->CRL |= GPIO_CRL_MODE4_0;



    SPI1->CR1 = 0;

    SPI1->CR1 |= SPI_CR1_MSTR;     // Master mode
    SPI1->CR1 |= SPI_CR1_SSI ;
    SPI1->CR1 |= SPI_CR1_SSM;
    // SPI1->CR1 &= ~(SPI_CR1_SSM);
    // SPI1->CR2 &= ~(SPI_CR2_SSOE);

    SPI1->CR1 |= SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0;     // Baud rate (e.g. fPCLK/8)

    SPI1->CR1 |= SPI_CR1_SPE;      // Enable SPI
}

void spi_transfer_multiple(uint8_t* data, uint8_t length) {

    // *((uint8_t *) & (SPI1->DR)) = data[0];
    uint8_t rev = 0 ;
    for(int i =0; i < length;i++) {
        while(!(SPI1->SR & SPI_SR_TXE));
        *((uint8_t *) & (SPI1->DR)) = data[i];
        while(!(SPI1->SR & SPI_SR_RXNE));
        rev = (uint8_t) SPI1->DR;
    }
}

void spi_receive_multiple(uint8_t *dataout)
{
    for(int i=0 ; i < 10; i++ ) {
        while(!(SPI1->SR & SPI_SR_RXNE));
        dataout[i] = (uint8_t) SPI1->DR;
    } 
}

void spi_receive_single(uint8_t out) {
    while(!(SPI1->SR & SPI_SR_RXNE));
    out = (uint8_t) SPI1->DR;
}

uint8_t spi1_transfer_single(uint8_t data) {
    uint8_t revdata = 0;
    while (!(SPI1->SR & SPI_SR_TXE));

    *((uint8_t *) & (SPI1->DR)) = data;
    
    while(!(SPI1->SR & SPI_SR_RXNE));
    revdata = (uint8_t) SPI1->DR;
    return revdata;
}
