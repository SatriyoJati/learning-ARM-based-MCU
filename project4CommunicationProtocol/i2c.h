#ifndef I2C_H
#define I2C_H
#include "stdint.h"

typedef struct {
    uint8_t i2c_port;
    uint8_t freq;
} I2C_Instance;

uint8_t I2C_Init(uint8_t isFastMode);

uint8_t I2C_Write_Address(uint8_t devAddr, uint8_t isRead);

uint8_t I2C_Write(uint8_t devAddr, uint8_t *data, uint16_t len);

uint8_t I2C_Write_NoStop(uint8_t devAddr, uint8_t *data, uint16_t len);

uint8_t I2C_Read(uint8_t devAddr, uint16_t index,  uint8_t *buf,  uint16_t len);

void I2C_RepeatStart(void);

#endif /* I2C_H*/