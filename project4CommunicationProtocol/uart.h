#ifndef UART_H
#define UART_H
#include "stdint.h"

typedef struct {
    uint32_t bdrate;
    uint32_t stopbit;
    uint32_t datalen;
    uint8_t initStatus; 
} Uart;

Uart* UartCreate();
uint8_t init_uart(Uart *uart, uint32_t bdRate);
uint8_t uart_transmit(Uart* uart, uint8_t data);
uint8_t uart_receive(Uart *uart);
uint8_t uart_transmit_string(Uart* uart, uint32_t* pstr);
uint8_t uart_receive_string(Uart* uart, uint32_t* pstr);

#endif //UART_H

