#include "i2c.h"
#include "stm32f100xb.h"

#define I2C_TIMEOUT_MAX   50000 // Simple loop iteration cutoff limit

static void I2C_ClearADDR(void) {
    (void)I2C1->SR1; // Read SR1 then SR2 sequentially to clear ADDR flag
    (void)I2C1->SR2;
}

void I2C_RepeatStart(void) 
{
    uint32_t timeout = I2C_TIMEOUT_MAX;

    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    timeout = I2C_TIMEOUT_MAX;
    while (!(I2C1->SR1 & I2C_SR1_SB)){
        if (--timeout == 0) return 1;
    };
}

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
    uint32_t timeout = I2C_TIMEOUT_MAX;
    // while (I2C1->SR2 & I2C_SR2_BUSY);

    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    while (!(I2C1->SR1 & I2C_SR1_SB)){
        if(--timeout == 0) return 1;
    };      // Wait SB=1

    if (!isRead)
        I2C1->DR = devAddr << 1;
    else
        I2C1->DR = (devAddr << 1)| 1;;
        
    timeout = I2C_TIMEOUT_MAX;
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) {
        if (I2C1->SR1 & I2C_SR1_AF){
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return 1;
        }
        if (--timeout == 0) return 1;
    };
    (void)I2C1->SR1; 
    (void)I2C1->SR2;

    return 0;
}

uint8_t I2C_Write(uint8_t devAddr, uint8_t *data, uint16_t len)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (I2C1->SR2 & I2C_SR2_BUSY){
        if (--timeout == 0) return 1;
    };

    if(I2C_Write_Address(devAddr,0) != 0) return 1;
    for(uint16_t i = 0; i < len ; i++){
        timeout = I2C_TIMEOUT_MAX;
        while (!(I2C1->SR1 & I2C_SR1_TXE)){
            if (I2C1->SR1 & I2C_SR1_AF) { I2C1->CR1 |= I2C_CR1_STOP; return 1; }
            if (--timeout == 0) return 1;
        };
        I2C1->DR = data[i];
    }

    timeout = I2C_TIMEOUT_MAX;
    while(!(I2C1->SR1 & I2C_SR1_BTF)){
        if (--timeout == 0) return 1;
    };
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

uint8_t I2C_Read(uint8_t devAddr, uint16_t index,  uint8_t *buf,  uint16_t len)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (I2C1->SR2 & I2C_SR2_BUSY){
        if(--timeout == 0) return 1;
    };

    // --- PHASE 1: WRITE REGISTER ADDRESS ---
    I2C1->CR1 |= I2C_CR1_START; 
    while (!(I2C1->SR1 & I2C_SR1_SB)){ if(--timeout == 0) return 1; };
    
    I2C1->DR = (devAddr << 1); 
    while (!(I2C1->SR1 & I2C_SR1_ADDR)){
        if (I2C1->SR1 & I2C_SR1_AF) { I2C1->CR1 |= I2C_CR1_STOP; return 1; }
        if (--timeout == 0) return 1;
    };
    I2C_ClearADDR();
    
    // // Transmit Register MSB
    // while (!(I2C1->SR1 & I2C_SR1_TXE)){ if (--timeout == 0) return 1; };
    // I2C1->DR = (uint8_t)(index >> 8);
    
    // Transmit Register LSB
    while (!(I2C1->SR1 & I2C_SR1_TXE)){ if (--timeout == 0) return 1; };
    I2C1->DR = index ;
    
    // Wait for address byte to clear transmission window completely
    while (!(I2C1->SR1 & I2C_SR1_BTF)){ if (--timeout == 0) return 1; }; 

    I2C1->CR1 |= I2C_CR1_STOP;  // Generate STOP

    // --- PHASE 2: REPEATED START ---
    I2C1->CR1 |= I2C_CR1_START;
    timeout = I2C_TIMEOUT_MAX;
    while (!(I2C1->SR1 & I2C_SR1_SB)){ if (--timeout == 0) return 1; };
    
    I2C1->CR1 |= I2C_CR1_ACK; // Ensure ACK is primed
    I2C1->DR = (devAddr << 1) | 1; 
    while (!(I2C1->SR1 & I2C_SR1_ADDR)){
        if (I2C1->SR1 & I2C_SR1_AF) { I2C1->CR1 |= I2C_CR1_STOP; return 1; }
        if (--timeout == 0) return 1;
    };
    
    // --- PHASE 3: RECEIVE DATA (Strict Hardware Sequencing) ---
    if (len == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;  // Clear ACK
        I2C1->CR1 |= I2C_CR1_STOP;  // Generate STOP
        
        while (!(I2C1->SR1 & I2C_SR1_RXNE)){ if (--timeout == 0) return 1; };
        buf[0] = I2C1->DR;
    } 
    else if (len == 2) {
        while (!(I2C1->SR1 & I2C_SR1_RXNE)){ if (--timeout == 0) return 1; };
        I2C1->CR1 &= ~I2C_CR1_ACK;  // Clear ACK
        I2C1->CR1 |= I2C_CR1_STOP;            
        
        buf[0] = I2C1->DR;          // Read first byte
        while (!(I2C1->SR1 & I2C_SR1_RXNE)){ if (--timeout == 0) return 1; };
        
        buf[1] = I2C1->DR;          // Read second byte
    } 
    else { // len > 2
        I2C_ClearADDR();
        for (uint16_t i = 0; i < len; i++) {
            // Wait until 3 bytes are left in total stream
            if (i == (len - 3)) {
                while (!(I2C1->SR1 & I2C_SR1_BTF)){ if (--timeout == 0) return 1; };
                I2C1->CR1 &= ~I2C_CR1_ACK; // Clear ACK
                buf[i++] = I2C1->DR;       // Read N-2
                
                while (!(I2C1->SR1 & I2C_SR1_BTF)){ if (--timeout == 0) return 1; };
                I2C1->CR1 |= I2C_CR1_STOP; // Generate STOP
                buf[i++] = I2C1->DR;       // Read N-1
                
                while (!(I2C1->SR1 & I2C_SR1_RXNE)){ if (--timeout == 0) return 1; };
                buf[i] = I2C1->DR;         // Read N
                break;
            }
            while (!(I2C1->SR1 & I2C_SR1_RXNE)){ if (--timeout == 0) return 1; };
            buf[i] = I2C1->DR;
        }
    }
    return 0;
}

uint8_t I2C_Write_NoStop(uint8_t devAddr, uint8_t *data, uint16_t len)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (I2C1->SR2 & I2C_SR2_BUSY){
        if (--timeout == 0) return 1;
    };

    if(I2C_Write_Address(devAddr,0) != 0) return 1;
    for(uint16_t i = 0; i < len ; i++){
        timeout = I2C_TIMEOUT_MAX;
        while (!(I2C1->SR1 & I2C_SR1_TXE)){
            if (I2C1->SR1 & I2C_SR1_AF) { I2C1->CR1 |= I2C_CR1_STOP; return 1; }
            if (--timeout == 0) return 1;
        };
        I2C1->DR = data[i];
    }

    timeout = I2C_TIMEOUT_MAX;
    while(!(I2C1->SR1 & I2C_SR1_BTF)){
        if (--timeout == 0) return 1;
    };
    // I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

// FIX: Combined Write-Index + Repeated Start Read transaction
uint8_t I2C_Read_RepeatedStart(uint8_t devAddr, uint8_t regAddr, uint8_t *buf, uint16_t len)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (I2C1->SR2 & I2C_SR2_BUSY){
        if(--timeout == 0) return 1;
    };

    // 1. Write Phase: Send target register address
    timeout = I2C_TIMEOUT_MAX;
    I2C1->CR1 |= I2C_CR1_START; 
    while (!(I2C1->SR1 & I2C_SR1_SB)){
        if(--timeout == 0) return 1;
    };
    
    I2C1->DR = (devAddr << 1); // Write Address allocation
    while (!(I2C1->SR1 & I2C_SR1_ADDR)){
        if (I2C1->SR1 & I2C_SR1_AF) { I2C1->CR1 |= I2C_CR1_STOP; return 1; }
        if (--timeout == 0) return 1;
    };
    I2C_ClearADDR();
    
    timeout = I2C_TIMEOUT_MAX;
    while (!(I2C1->SR1 & I2C_SR1_TXE)){
        if (--timeout == 0) return 1;
    };
    I2C1->DR = regAddr;

    timeout = I2C_TIMEOUT_MAX;
    while (!(I2C1->SR1 & I2C_SR1_BTF)){
        if (--timeout == 0) return 1;
    }; // Wait for register byte to transmit completely

    // 2. REPEATED START PHASE: Turn bus around without issuing a STOP command
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    timeout = I2C_TIMEOUT_MAX;
    while (!(I2C1->SR1 & I2C_SR1_SB)){
        if (--timeout == 0) return 1;
    };
    
    I2C1->DR = (devAddr << 1) | 1; // Read Address allocation
    while (!(I2C1->SR1 & I2C_SR1_ADDR)){
        if (I2C1->SR1 & I2C_SR1_AF) { I2C1->CR1 |= I2C_CR1_STOP; return 1; }
        if (--timeout == 0) return 1;
    };
    
    // Handle NACK and STOP setup proactively for single-byte responses
    if (len == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        I2C_ClearADDR();
        I2C1->CR1 |= I2C_CR1_STOP;
    } else {
        I2C_ClearADDR();
    }

    // 3. Sequential Streaming Read Phase
    for (uint16_t i = 0; i < len; i++) {
        if (len > 1 && i == len - 1) {
            timeout = I2C_TIMEOUT_MAX;
            while (!(I2C1->SR1 & I2C_SR1_RXNE)){ if (--timeout == 0) return 1; };
            I2C1->CR1 &= ~I2C_CR1_ACK;  // NACK the final incoming payload byte
            I2C1->CR1 |= I2C_CR1_STOP; // Arm hardware STOP generation
        }
        timeout = I2C_TIMEOUT_MAX;
        while (!(I2C1->SR1 & I2C_SR1_RXNE)){
            if (--timeout == 0) return 1;
        };
        buf[i] = I2C1->DR;
    }

    return 0;
}
