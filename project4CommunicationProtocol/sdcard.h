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


uint8_t read_single_block(uint16_t addr, uint8_t *buff);
uint8_t read_multi_blocks(uint16_t addr, uint32_t *buff, uint16_t nSector);
uint8_t write_single_block(uint16_t addr, uint16_t *dataString);
uint8_t write_multi_blocks(uint16_t addr, uint16_t *dataString, uint16_t nSector);

#endif //SDCARD_SPI