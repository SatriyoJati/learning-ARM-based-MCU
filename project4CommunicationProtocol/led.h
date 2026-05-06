#ifndef LED_NOTIF
#define LED_NOTIF
#include "stdint.h"
#include <stddef.h>

#define MAXPOOL 3
#define PORTA   RCC_APB2ENR_IOPAEN
#define PORTB   RCC_APB2ENR_IOPBEN
#define PORTC   RCC_APB2ENR_IOPCEN
#define PORTD   RCC_APB2ENR_IOPDEN


typedef enum  {
    OUTPUT_PUSHPULL,
    OUTPUT_ODRAIN
}LedMode;

typedef struct {
    uint32_t port;
    uint32_t mode;
    uint32_t pinNumber;
    uint32_t initStatus;
}Led;

Led* LedCreate();

uint8_t init_led_gpio(Led * led , uint32_t port, uint32_t mode);

uint8_t blink_fast(Led* led);

uint8_t blink_slow(Led* led);

#endif