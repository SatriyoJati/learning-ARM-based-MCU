#include <stdint.h>
#include <stddef.h>
#ifndef SPI_HANDLE_H
#define SPI_HANDLE_H

void Init_LoopBack_SPI_Test();

uint8_t spi1_transfer_single(uint8_t data);
void spi_transfer_multiple(uint8_t* data, uint8_t length);
void spi_receive_multiple(uint8_t* dataout);
void spi_receive_single(uint8_t out);
void SPI_Transmit_Receive_MultiByte(uint8_t *dataIn , size_t size, uint8_t* dataOut, size_t sizeout);
#endif