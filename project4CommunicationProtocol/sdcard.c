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
                res = waitR1();
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

