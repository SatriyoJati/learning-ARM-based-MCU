/**
 * @file    sdcard_spi.h
 * @brief   SD Card SPI Driver for STM32F103 using CMSIS
 *
 * Hardware Configuration (SPI1):
 *   PA4  -> SD_CS   (Chip Select, GPIO Output)
 *   PA5  -> SPI1_SCK
 *   PA6  -> SPI1_MISO
 *   PA7  -> SPI1_MOSI
 *
 * SD Card SPI Mode Initialization Sequence:
 *   1. Power-on delay (>74 clock cycles with CS high)
 *   2. CMD0  -> GO_IDLE_STATE  (R1 response: 0x01)
 *   3. CMD8  -> SEND_IF_COND   (R7 response, check voltage)
 *   4. ACMD41-> SD_SEND_OP_COND (poll until R1 = 0x00)
 *   5. CMD58 -> READ_OCR       (check CCS bit for SDHC/SDXC)
 *   6. CMD16 -> SET_BLOCKLEN   (set 512 bytes for SDSC)
 */

#ifndef SDCARD_SPI_H
#define SDCARD_SPI_H

#include <stm32f101xb.h>
#include <stdint.h>

/* ─── SD Card Types ────────────────────────────────────────────────────────── */
typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_MMC     = 1,   /* MMC v3 */
    SD_TYPE_SD1     = 2,   /* SD v1 (SDSC ≤ 2GB) */
    SD_TYPE_SD2     = 4,   /* SD v2 (SDSC ≤ 2GB) */
    SD_TYPE_SDHC    = 8,   /* SDHC / SDXC (block addressed) */
} SD_CardType;

/* ─── Return Codes ─────────────────────────────────────────────────────────── */
typedef enum {
    SD_OK           = 0x00,
    SD_ERROR        = 0x06,
    SD_TIMEOUT      = 0x02,
    SD_NOT_PRESENT  = 0x03,
    SD_UNSUPPORTED  = 0x04,
    SD_CRC_ERROR    = 0x05,
    SD_IDLE_STATE   = 0x01, /* R1 bit: in idle state */
} SD_Status;

/* ─── SD Card Info ─────────────────────────────────────────────────────────── */
typedef struct {
    SD_CardType type;
    uint32_t    capacity_blocks; /* Total blocks (512 bytes each) */
    uint32_t    capacity_mb;     /* Capacity in MB */
    uint8_t     csd[16];         /* Raw CSD register */
    uint8_t     cid[16];         /* Raw CID register */
} SD_CardInfo;

/* ─── SPI / GPIO Configuration ────────────────────────────────────────────── */
#define SD_SPI              SPI1
#define SD_SPI_CLK          RCC_APB2Periph_SPI1

#define SD_GPIO_PORT        GPIOA
#define SD_GPIO_CLK         RCC_APB2Periph_GPIOA

#define SD_PIN_SCK          GPIO_Pin_5    /* PA5 */
#define SD_PIN_MISO         GPIO_Pin_6    /* PA6 */
#define SD_PIN_MOSI         GPIO_Pin_7    /* PA7 */
#define SD_PIN_CS           GPIO_Pin_4    /* PA4 */

/* ─── SPI Speed Presets ────────────────────────────────────────────────────── */
/* Init phase: ≤400 kHz. PCLK2=72MHz: Div256=281kHz, Div128=562kHz */
#define SPI_BAUD_INIT       SPI_BaudRatePrescaler_256   /* ~281 kHz */
#define SPI_BAUD_FAST       SPI_BaudRatePrescaler_4     /* ~18 MHz  */

/* ─── Timeouts ─────────────────────────────────────────────────────────────── */
#define SD_TIMEOUT_CMD      500   /* Command response timeout (iterations) */
#define SD_TIMEOUT_INIT     2000  /* ACMD41 polling timeout (iterations)   */
#define SD_TIMEOUT_DATA     5000  /* Data token wait timeout (iterations)  */

/* ─── SD Commands ──────────────────────────────────────────────────────────── */
#define CMD0    (0x40 | 0)    /* GO_IDLE_STATE */
#define CMD1    (0x40 | 1)    /* SEND_OP_COND (MMC) */
#define CMD8    (0x40 | 8)    /* SEND_IF_COND */
#define CMD9    (0x40 | 9)    /* SEND_CSD */
#define CMD10   (0x40 | 10)   /* SEND_CID */
#define CMD12   (0x40 | 12)   /* STOP_TRANSMISSION */
#define CMD16   (0x40 | 16)   /* SET_BLOCKLEN */
#define CMD17   (0x40 | 17)   /* READ_SINGLE_BLOCK */
#define CMD18   (0x40 | 18)   /* READ_MULTIPLE_BLOCK */
#define CMD23   (0x40 | 23)   /* SET_BLOCK_COUNT (MMC) */
#define CMD24   (0x40 | 24)   /* WRITE_BLOCK */
#define CMD25   (0x40 | 25)   /* WRITE_MULTIPLE_BLOCK */
#define CMD41   (0x40 | 41)   /* APP_SEND_OP_COND (ACMD) */
#define CMD55   (0x40 | 55)   /* APP_CMD */
#define CMD58   (0x40 | 58)   /* READ_OCR */
#define CMD59   (0x40 | 59)   /* CRC_ON_OFF */

/* ─── Public API ───────────────────────────────────────────────────────────── */
SD_Status SD_Init(SD_CardInfo *card_info);
SD_Status SD_ReadBlock(uint32_t block_addr, uint8_t *buf);
SD_Status SD_WriteBlock(uint32_t block_addr, const uint8_t *buf);
uint8_t   SD_GetStatus(void);

/* Low-level SPI helpers (also useful for debugging) */
void    SPI1_Init_Slow(void);
void    SPI1_SetFast(void);
uint8_t SPI1_Transfer(uint8_t data);
void    SD_CS_Low(void);
void    SD_CS_High(void);

#endif /* SDCARD_SPI_H */
