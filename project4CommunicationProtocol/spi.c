#include <stm32f101xb.h>
#include <stddef.h>

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
    GPIOA->CRL |= GPIO_CRL_CNF6_0 ;
    GPIOA->CRL &= ~(GPIO_CRL_MODE6) ;

    //nss
    GPIOA->CRL |= GPIO_CRL_CNF4_0;
    GPIOA->CRL &= ~( GPIO_CRL_MODE4);


    SPI1->CR1 = 0;

    SPI1->CR1 |= SPI_CR1_MSTR;     // Master mode
    SPI1->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM; // Software slave management

    SPI1->CR1 |= SPI_CR1_BR_1;     // Baud rate (e.g. fPCLK/8)

    SPI1->CR1 |= SPI_CR1_SPE;      // Enable SPI
}


uint8_t spi1_transfer(uint8_t data) {
    uint8_t revdata = 0;
    while (!(SPI1->SR & SPI_SR_TXE));

    *((uint8_t *) & (SPI1->DR)) = data;
    
    while(!(SPI1->SR & SPI_SR_RXNE));
    revdata = (uint8_t) SPI1->DR;
    return revdata;
}
