#include "i2c.h"
#include "stm32f100xb.h"

uint8_t I2C_Init(uint8_t isFastMode)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    // I2C1->CR1 &= ~(I2C_CR1_PE);

    GPIOB->CRL &= ~(0xFF << 24);
    // GPIOB->CRL |= ( 0b11 << GPIO_CRL_CNF6_Pos  |
    //             0b11 << GPIO_CRL_CNF7_Pos  |
    //             0b11 << GPIO_CRL_MODE6_Pos |
    //             0b11 << GPIO_CRL_MODE7_Pos );
    GPIOB->CRL |= (0xEE << 24);

    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~(I2C_CR1_SWRST);

    if(isFastMode != 1) {
        I2C1->CR2 = 32;         //32 MHz
        I2C1->CCR = 160;        //
        
        I2C1->TRISE = 0xA;
    } else {
        I2C1->CR2 = 32;
        I2C1->CCR = I2C_CCR_FS | 27;
        
        I2C1->TRISE = 0xA;
    }

    I2C1->CR1 |= I2C_CR1_PE;

    return 0;
}

uint8_t I2C_Write_Address(uint8_t devAddr, uint8_t isRead)
{
    while (I2C1->SR2 & I2C_SR2_BUSY);

    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    while (!(I2C1->SR1 & I2C_SR1_SB));      // Wait SB=1

    if (!isRead)
        I2C1->DR = devAddr << 1;
    else
        I2C1->DR = (devAddr << 1)| 1;;
        
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    return 0;
}

uint8_t I2C_Write(uint8_t devAddr, uint8_t *data, uint16_t len)
{
    while (I2C1->SR2 & I2C_SR2_BUSY);

    I2C_Write_Address(devAddr,0);
    for(uint8_t i = 0; i < len ; i++){
        while (!(I2C1->SR1 & I2C_SR1_TXE));
        I2C1->DR = data[i];
    }

    while(!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

uint8_t I2C_Read(uint8_t devAddr, uint8_t *buf, uint16_t len)
{
    while (I2C1->SR2 & I2C_SR2_BUSY);
    I2C1->CR1 |= I2C_CR1_ACK;

    I2C_Write_Address(devAddr,1);

    for (uint8_t i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C1->CR1 &= ~I2C_CR1_ACK;  // NACK on last byte
            I2C1->CR1 |=  I2C_CR1_STOP;
        }
        while (!(I2C1->SR1 & I2C_SR1_RXNE));
        buf[i] = I2C1->DR;
    }

    return 0;
}
