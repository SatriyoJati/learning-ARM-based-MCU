#include "sdcard.h"
#include "spi.h"
#include <stm32f101xb.h>

#define CMD_LENGTH (6)

// #define CMD0 (0x40 | 0x0)
#define INIT_SDCARD_SUCCEED 1


#define SDCARD_SDHC_SDXC 0x1
#define SDCARD_SDSC 0x0


#define SDCARD_V2 0x1
#define SDCARD_V1 0x0

#define SDCARD_VOLT_NORMAL 0x08


#define SD_MAX_READ_ATTEMPTS 100

typedef enum {
    IDLE_STATE,
    ERASE_RESET,
    ILLEGAL_COMMAND,
    COM_CRC_ERR,
    ERASE_SEQ_ERR,
    ADDR_ERR,
    PARAM_ERR,
    UNUSABLE_CARD
} R1_Response;

uint8_t CMD0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
uint8_t CMD8[] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
uint8_t CMD58[] = {0x7A, 0x00, 0x00, 0x00, 0x00, 0xFF};
uint8_t ACMD41[] = {0x69, 0x40, 0x00, 0x00, 0x00, 0x00};
uint8_t CMD55[] = {0x77,0x00, 0x00, 0x00, 0x00,0x00};

// read write command
uint8_t CMD17[] = {0x51, 0x00,0x00,0x00,0x00, 0x00};
uint8_t CMD18[] = {0x52, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t CMD12[] = {0x4C, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t CMD24[] = {0x58,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t CMD25[] = {0x59,0x00,0x00,0x00,0x00,0x00,0x00};

void sd_CS_Low() {
    GPIOA->BSRR = GPIO_BSRR_BR4;
}

void sd_CS_High() {
    GPIOA->BSRR = GPIO_BSRR_BS4;
    spi1_transfer_single(0xFF);
}

uint8_t waitR1() {
    uint8_t r1;
    uint16_t timeout = 500;
    do {
        r1 = spi1_transfer_single(0xFF);
        if (r1 == 0x01) return r1;
    } while (--timeout);    
    return 0xFF;
}

uint8_t waitR1SuccessInit() {
    uint8_t r1;
    uint16_t timeout = 500;
    do {
        r1 = spi1_transfer_single(0xFF);
        if (r1 == 0x00) return r1;
    } while (--timeout);    
    return 0xFF;
}

uint8_t waitR1EightClocks() {
    uint8_t r1 = 0x04;
    for (int i=0; i < 8 && r1 & 0x80 ; i++ ) {
        r1 =  spi1_transfer_single(0xFF);
    }
    return r1;
}

/** Busy-loop microsecond delay (approximate at 72 MHz) */
static void delay_us(uint32_t us)
{
    /* Each iteration ≈ 1/72MHz * ~14 cycles ≈ 0.19 µs; scale factor ~5 */
    volatile uint32_t cnt = us * 6;
    while (cnt--) { __NOP(); }
}

static void delay_ms(uint32_t ms)
{
    while (ms--) { delay_us(1000); }
}

uint8_t init_sdcard(Sdcard* sdcard)
{
    uint8_t res = 0;
    uint8_t v2_card = 0;
    sd_CS_High();
    delay_ms(2000);  /* Wait for card power stabilisation */
    for(int i=0 ; i < 50 ; i++) {
        spi1_transfer_single(0xFF);
    };
    sd_CS_Low();
    spi_transfer_multiple(CMD0, CMD_LENGTH);

    res = waitR1();
    sd_CS_High();

    if (res != 0x01) {
        return UNUSABLE_CARD; // error sdcard
    }

    // CMD8 
    //------------------------------------------
    uint8_t len = sizeof(CMD8)/sizeof(CMD8[0]);
    sd_CS_Low();
    spi_transfer_multiple(CMD8, len); 
    res = waitR1();

    uint8_t r7[4];

    if (res == 0x01) {
        for(int i=0; i < 4; i++) {
            r7[i] = spi1_transfer_single(0xFF);
        }
        sd_CS_High();
        // bit 11-8 voltage accepted, 7-0 check pattern AA.
        if ((r7[2] & 0x0F) == 0x01 && r7[3] == 0xAA) { 
            v2_card = 1;
            sdcard->sdcard_version = SDCARD_V2;
            sdcard->sdcard_voltage = SDCARD_VOLT_NORMAL;
        } else {
            return UNUSABLE_CARD;
        }
    } else if (res & 0x04) {
        sd_CS_High();
        v2_card = 0;
        sdcard->sdcard_version = 0x0;
        return  ILLEGAL_COMMAND;// return illegal command
    } else {
        return UNUSABLE_CARD; //return sdcard
    }

    sd_CS_High();

    // CMD55 to make card interpret next command is ACMD
    sd_CS_Low();
    spi_transfer_multiple(CMD55,CMD_LENGTH);
    res = waitR1();
    sd_CS_High();
    if (res != 0x1) {
        return UNUSABLE_CARD;
    }
    
    // ACMD41
    sd_CS_Low();
    uint32_t timeout = 1500;
    do{
        spi_transfer_multiple(ACMD41, CMD_LENGTH);
        res = waitR1SuccessInit();
        if (res == 0x00) {
            // CMD58
            sd_CS_High();
            uint8_t r3[4];
            if (v2_card && (res == 0x00)) {
                sd_CS_Low();
                spi_transfer_multiple(CMD58,CMD_LENGTH);
                res = waitR1SuccessInit();
                sd_CS_High();
            }

            if(res == 0x00) {
                for(int i=0; i < 4 ; i++) {
                    r3[i] = spi1_transfer_single(0xFF);
                }
                if ((r3[3] && 0xF0) == 0xC0) {
                    //  ccs = 1, 
                    sdcard->sdcard_capacity = 0x1;
                } else if ((r3[3] && 0xF0) == 0x40) {
                    sdcard->sdcard_capacity = 0x0;
                }
            }
            break;
        } 
        delay_ms(100);
    } while (timeout--);
    sd_CS_High();

    return INIT_SDCARD_SUCCEED;
}

uint8_t read_single_block(uint16_t addr, uint8_t *buff) {
    uint8_t res, data;
    uint8_t readAttempts = 0;

    CMD17[1] = (uint8_t)(addr >> 24);
    CMD17[2] = (uint8_t)(addr >> 16);
    CMD17[3] = (uint8_t)(addr >> 8);
    CMD17[4] = (uint8_t)(addr >> 0);

    sd_CS_Low();
    spi_transfer_multiple(CMD17, CMD_LENGTH);
    res = waitR1SuccessInit();
    if (res == 0x0) {
        while(++readAttempts != SD_MAX_READ_ATTEMPTS) {
            if((data = spi1_transfer_single(0xFF)) != 0xFF) break;
        }

        if (data == 0xFE) {
            for(uint16_t i = 0 ; i < 512 ; i++) {
                *buff++ = spi1_transfer_single(0xFF);
            }
            spi1_transfer_single(0xFF);
            spi1_transfer_single(0xFF);
        } else if ((data & 0x0F)) {
            res = 2; // DATA ERROR
        }
    } else {
        res = 1; // RESPONSE R1 ERROR
    }

    spi1_transfer_single(0xFF);
    sd_CS_High();
    spi1_transfer_single(0xFF);
    return res;
}

uint8_t read_multi_blocks(uint16_t addr, uint32_t *buff, uint16_t nSector)
{
    uint8_t res, data;
    uint8_t readAttempts;

    CMD17[1] = (uint8_t)(addr >> 24);
    CMD17[2] = (uint8_t)(addr >> 16);
    CMD17[3] = (uint8_t)(addr >> 8);
    CMD17[4] = (uint8_t)(addr >> 0);

    sd_CS_Low();
    spi_transfer_multiple(CMD18,CMD_LENGTH);
    res = waitR1SuccessInit();

    if (res == 0x0) {

        for ( uint8_t i=0; i < nSector; i++) {
            while(++readAttempts != SD_MAX_READ_ATTEMPTS) {
                if((data = spi1_transfer_single(0xFF)) != 0xFF) break;
            }

            if (data == 0xFE) {
                for(uint16_t i = 0 ; i < 512 ; i++) {
                    *buff++ = spi1_transfer_single(0xFF);
                }

                // discard crc
                spi1_transfer_single(0xFF);
                spi1_transfer_single(0xFF);
            }
        }
        spi_transfer_multiple(CMD12,CMD_LENGTH);
        res = waitR1SuccessInit();
        if (res == 0x0){
            return 1;
        }
        spi1_transfer_single(0xFF);
        sd_CS_High();
        spi1_transfer_single(0xFF);
    }

    spi1_transfer_single(0xFF);
    sd_CS_High();
    spi1_transfer_single(0xFF);
    return 0;
}

uint8_t write_single_block(uint16_t addr, uint16_t *dataString) {

    uint8_t res;
    uint8_t dataResponseToken;

    CMD24[1] = (uint8_t)(addr >> 24);
    CMD24[2] = (uint8_t)(addr >> 16);
    CMD24[3] = (uint8_t)(addr >> 8);
    CMD24[4] = (uint8_t)(addr >> 0);

    sd_CS_Low();
    spi_transfer_multiple(CMD24,CMD_LENGTH);
    res = waitR1SuccessInit();
    if (res != 0x0){
        sd_CS_High();
        return 1;
    }

    uint8_t byteArray[512] ;
    memset(byteArray,0x0,sizeof(byteArray));
    memcpy(byteArray, dataString, strlen(dataString) + 1);

    spi1_transfer_single(0xFE);

    for ( uint16_t i=0; i < 512 ; i++)
        spi1_transfer_single(byteArray[i]);
    
    // CRC
    spi1_transfer_single(0xFF);
    spi1_transfer_single(0xFF);
    
    uint16_t timeout = 1500;
    while( timeout-- ) {
        dataResponseToken = spi1_transfer_single(0xFF);
        uint8_t status = dataResponseToken & 0x1F;
        if ( status == 0x5) {
            break; // data accepted
        } else if (status == 0xB) {
            sd_CS_High();
            return 1; // crc error
        } else if (status == 0xD) {
            sd_CS_High();
            return 1; // write error
        } else {
            sd_CS_High();
            return 1;
        };
    }

    while(spi1_transfer_single(0xFF) == 0x00);

    sd_CS_High();
    return 0;
}

uint8_t write_multi_blocks(uint16_t addr, uint16_t *dataString, uint16_t nSector) {
    uint8_t res;
    uint8_t dataResponseToken;

    CMD25[1] = (uint8_t)(addr >> 24);
    CMD25[2] = (uint8_t)(addr >> 16);
    CMD25[3] = (uint8_t)(addr >> 8);
    CMD25[4] = (uint8_t)(addr >> 0);

    sd_CS_Low();
    spi_transfer_multiple(CMD25, CMD_LENGTH);
    res = waitR1SuccessInit();
    if (res != 0x0){
        sd_CS_High();
        return 1;
    }

    spi1_transfer_single(0xFC); 
    for (uint16_t i=0; i < nSector ; i++){

        for(uint16_t j=0;j < 512 ; j++) {
            spi1_transfer_single(dataString[i * 512 + j]);
        }

        spi1_transfer_single(0xFF);
        spi1_transfer_single(0xFF);

        uint16_t timeout = 1500;
        while(timeout--) {
            dataResponseToken = spi1_transfer_single(0xFF);
            uint8_t status = dataResponseToken & 0x1F;
            if ( status == 0x5) {
                break; // data accepted
            } else if (status == 0xB) {
                sd_CS_High();
                return 1; // crc error
            } else if (status == 0xD) {
                sd_CS_High();
                return 1; // write error
            } else {
                sd_CS_High();
                return 1;
            };
        }
    }

    spi1_transfer_single(0xFD); // Stop Tran
    spi1_transfer_single(0xFF);

    while(spi1_transfer_single(0xFF) == 0x00);
    return 0;
}