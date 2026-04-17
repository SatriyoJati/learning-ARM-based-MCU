#include <stdint.h>
#include <stddef.h>
#ifndef SPI_HANDLE_H
#define SPI_HANDLE_H

void Init_LoopBack_SPI_Test();

uint8_t spi1_transfer(uint8_t data);
void SPI_Transmit_Receive_MultiByte(uint8_t *dataIn , size_t size, uint8_t* dataOut, size_t sizeout);
#endif