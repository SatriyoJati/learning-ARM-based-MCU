#include <stdint.h>
#include <stddef.h>

#ifndef SDCARD_SPI
#define  SDCARD_SPI
typedef struct  {
    uint8_t sdcard_capacity;
    uint8_t sdcard_voltage;
    uint8_t sdcard_version;
}Sdcard;
// typedef struct Sdcard Sdcard;

uint8_t init_sdcard(Sdcard* sdcard);

#endif //SDCARD_SPI