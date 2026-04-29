/**
 * @file    sdcard_spi.c
 * @brief   SD Card SPI1 Driver — STM32F103 CMSIS Implementation
 *
 * ┌─────────────────────────────────────────────────────────────┐
 * │  STM32F103          SD Card (SPI Mode)                      │
 * │                                                             │
 * │  PA4  ──────────►  Pin 1  CS   / DAT3                       │
 * │  PA7  ──────────►  Pin 2  MOSI / CMD                        │
 * │  3.3V ──────────►  Pin 3  VDD                               │
 * │  GND  ──────────►  Pin 4  VSS                               │
 * │  PA5  ──────────►  Pin 5  SCK  / CLK                        │
 * │  GND  ──────────►  Pin 6  VSS2                              │
 * │  PA6  ◄──────────  Pin 7  MISO / DAT0                       │
 * │                                                             │
 * │  Note: 10kΩ pull-up on MISO recommended                     │
 * └─────────────────────────────────────────────────────────────┘
 *
 * SPI Mode 0 (CPOL=0, CPHA=0), MSB first, 8-bit frames
 *
 * Initialization Flow:
 *   1. Configure GPIO (PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI)
 *   2. Configure SPI1 slow (~281 kHz, CPOL=0, CPHA=0)
 *   3. Power-on: send ≥74 dummy clocks with CS=HIGH
 *   4. CMD0  → assert SPI mode (R1 = 0x01)
 *   5. CMD8  → detect SD v2 cards (R7 response)
 *   6. ACMD41 → wait for card ready (R1 = 0x00)
 *   7. CMD58 → read OCR, detect SDHC by CCS bit
 *   8. CMD16 → set block length to 512 (SDSC only)
 *   9. CMD59 → disable CRC (optional, default off in SPI)
 *  10. Switch SPI to fast clock (~18 MHz)
 */

#include "sdcard_spi.h"

/* ═══════════════════════════════════════════════════════════════
 *  Internal helpers
 * ═══════════════════════════════════════════════════════════════ */

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

/* ═══════════════════════════════════════════════════════════════
 *  GPIO / SPI Hardware Init
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief  Initialise SPI1 GPIO pins
 *         PA4  → CS   (push-pull output, default HIGH)
 *         PA5  → SCK  (alternate function push-pull)
 *         PA6  → MISO (input with pull-up)
 *         PA7  → MOSI (alternate function push-pull)
 */
static void GPIO_Init_SPI1(void)
{
    /* ── Enable clocks ─────────────────────────────────────────── */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN   /* GPIOA clock */
                 |  RCC_APB2ENR_AFIOEN;  /* AFIO  clock */

    /* ── PA4 : CS — general-purpose output, push-pull, 10 MHz ─── */
    /* CRL bits [19:16] for PA4 → MODE=01 (out 10MHz), CNF=00 (GP push-pull) */
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);    
    GPIOA->CRL |=  GPIO_CRL_MODE4_0;   /* MODE4 = 01 → 10 MHz output */
    /* CNF4 = 00 → general purpose push-pull (default after mask) */

    /* CS high (deselected) */
    GPIOA->BSRR = GPIO_BSRR_BS4;

    /* ── PA5 : SCK — AF push-pull, 50 MHz ─────────────────────── */
    /* CRL bits [23:20] for PA5 → MODE=11 (out 50MHz), CNF=10 (AF push-pull)*/
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
    GPIOA->CRL |=  GPIO_CRL_CNF5_1     /* CNF5 = 10 → AF push-pull */
               |   GPIO_CRL_MODE5_0    /* MODE5 = 11 → 50 MHz */
               |   GPIO_CRL_MODE5_1;

    /* ── PA6 : MISO — input with pull-up ──────────────────────── */
    /* CRL bits [27:24] for PA6 → MODE=00 (input), CNF=10 (input pull-up/dn)*/
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL |=  GPIO_CRL_CNF6_1;    /* CNF6 = 10 → input with pull */
    GPIOA->ODR |=  GPIO_ODR_ODR6;      /* ODR=1 → pull-up enabled */

    /* ── PA7 : MOSI — AF push-pull, 50 MHz ────────────────────── */
    /* CRL bits [31:28] for PA7 → MODE=11 (out 50MHz), CNF=10 (AF push-pull)*/
    GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |=  GPIO_CRL_CNF7_1     /* CNF7 = 10 → AF push-pull */
               |   GPIO_CRL_MODE7_0    /* MODE7 = 11 → 50 MHz */
               |   GPIO_CRL_MODE7_1;
}

/**
 * @brief  Configure SPI1 peripheral registers directly (no HAL/StdPeriph)
 *         Mode 0 (CPOL=0, CPHA=0), MSB first, 8-bit, prescaler as given
 * @param  baud_div  SPI_CR1 BR[2:0] bits (0=div2 … 7=div256)
 */
static void SPI1_Configure(uint16_t baud_div)
{
    /* ── Enable SPI1 APB2 clock ────────────────────────────────── */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* ── Disable SPI before changing config ────────────────────── */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /*
     * CR1 layout:
     *   BIDIMODE=0, BIDIOE=0   — full duplex
     *   CRCEN=0                — no CRC
     *   CRCNEXT=0
     *   DFF=0                  — 8-bit frame
     *   RXONLY=0               — full duplex
     *   SSM=1                  — software slave management
     *   SSI=1                  — internal NSS high (we drive CS manually)
     *   LSBFIRST=0             — MSB first
     *   SPE=0                  — enable later
     *   BR[2:0]=baud_div       — baud rate
     *   MSTR=1                 — master mode
     *   CPOL=0                 — idle CLK low
     *   CPHA=0                 — sample on rising edge
     */
    SPI1->CR1 = SPI_CR1_SSM         /* Software NSS management */
              | SPI_CR1_SSI         /* Internal NSS = 1 */
              | SPI_CR1_MSTR        /* Master mode */
              | (baud_div & 0x0038) /* BR[2:0] — 3 bits, position [5:3] */
              ;
    /* CR2: keep default (interrupts off, no DMA) */
    SPI1->CR2 = 0;

    /* ── Enable SPI1 ───────────────────────────────────────────── */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ───────────────────────────────────────────────────────────────
 *  Public low-level SPI helpers
 * ─────────────────────────────────────────────────────────────── */

/** Init SPI1 at slow speed for SD card initialisation (≤400 kHz) */
void SPI1_Init_Slow(void)
{
    GPIO_Init_SPI1();
    /* Prescaler 256 → 72 MHz / 256 = 281.25 kHz */
    SPI1_Configure(SPI_CR1_BR);   /* BR[2:0] = 111 = div256 */
}

/** Switch SPI1 to fast speed after SD card init */
void SPI1_SetFast(void)
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    /* Clear BR bits, then set prescaler 4 → 72 MHz / 4 = 18 MHz */
    SPI1->CR1 &= ~SPI_CR1_BR;
    SPI1->CR1 |= SPI_CR1_BR_0;   /* BR[2:0] = 001 = div4 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/** Full-duplex SPI byte exchange */
uint8_t SPI1_Transfer(uint8_t data)
{
    /* Wait until TX buffer empty */
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = data;
    /* Wait until RX buffer not empty */
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI1->DR;
}

void SD_CS_Low(void)
{
    GPIOA->BSRR = GPIO_BSRR_BR4;  /* Reset PA4 → CS low */
}

void SD_CS_High(void)
{
    GPIOA->BSRR = GPIO_BSRR_BS4;  /* Set PA4 → CS high */
    SPI1_Transfer(0xFF);           /* Extra clock after deselect */
}

/* ═══════════════════════════════════════════════════════════════
 *  SD Card Command Layer
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief  Send 6-byte SD SPI command frame
 * @param  cmd   Command byte (0x40 | cmd_index)
 * @param  arg   32-bit argument
 * @param  crc   CRC byte (must be correct for CMD0 & CMD8; others ignored)
 */
static void SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    SPI1_Transfer(cmd);
    SPI1_Transfer((uint8_t)(arg >> 24));
    SPI1_Transfer((uint8_t)(arg >> 16));
    SPI1_Transfer((uint8_t)(arg >>  8));
    SPI1_Transfer((uint8_t)(arg >>  0));
    SPI1_Transfer(crc);
}

/**
 * @brief  Wait for R1 response (MSB=0 means valid)
 * @return R1 byte, or 0xFF on timeout
 */
static uint8_t SD_WaitR1(void)
{
    uint8_t r1;
    uint16_t timeout = SD_TIMEOUT_CMD;
    do {
        r1 = SPI1_Transfer(0xFF);
        if (r1 != 0xFF) return r1;
    } while (--timeout);
    return 0xFF;  /* Timeout */
}

/**
 * @brief  Send command, return R1 response.  CS must already be low.
 */
static uint8_t SD_Command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    /* Per spec: send ≥1 dummy byte before each command */
    SPI1_Transfer(0xFF);
    SD_SendCommand(cmd, arg, crc);
    return SD_WaitR1();
}

/**
 * @brief  Send ACMD (prefix with CMD55 APP_CMD)
 */
static uint8_t SD_ACommand(uint8_t acmd, uint32_t arg)
{
    uint8_t r1;
    r1 = SD_Command(CMD55, 0, 0x65);  /* CRC for CMD55 with arg=0 */
    if (r1 > 1) return r1;            /* Error (not idle is OK here) */
    return SD_Command(acmd, arg, 0x77);
}

/* ═══════════════════════════════════════════════════════════════
 *  SD Card Initialisation
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief  Full SD card SPI initialisation sequence.
 *
 * Sequence:
 *  Step 1 — Power-on: ≥74 dummy clocks with CS HIGH
 *  Step 2 — CMD0:  Force SPI mode (expect R1=0x01)
 *  Step 3 — CMD8:  Check voltage range / detect v2 card
 *  Step 4 — ACMD41: Poll until card leaves idle state
 *  Step 5 — CMD58: Read OCR to determine SDHC/SDXC
 *  Step 6 — CMD16: Set block length 512 (SDSC cards only)
 *  Step 7 — CMD59: Disable CRC (already default; belt+braces)
 *  Step 8 — Switch SPI to fast clock
 *
 * @param  card_info  Pointer to SD_CardInfo struct (filled on success)
 * @return SD_OK on success, error code otherwise
 */
SD_Status SD_Init(SD_CardInfo *card_info)
{
    uint8_t  r1 = 0x0;
    uint8_t  ocr[4];
    uint32_t timeout;
    uint8_t  is_v2_card  = 0;
    uint8_t  is_block_addr = 0;   /* 1 = SDHC/SDXC (byte addressing vs block) */

    /* ─── Step 0: Hardware init ──────────────────────────────────── */
    SPI1_Init_Slow();

    if (card_info) {
        card_info->type = SD_TYPE_UNKNOWN;
    }

    /* ─── Step 1: Power-on delay — 80 dummy clocks, CS HIGH ─────── */
    SD_CS_High();  /* Ensure CS is deasserted */
    delay_ms(1000);  /* Wait for card power stabilisation */

    for (uint8_t i = 0; i < 10; i++) {
        SPI1_Transfer(0xFF);  /* 10 × 8 = 80 clock pulses */
    }

    /* ─── Step 2: CMD0 — GO_IDLE_STATE ──────────────────────────── */
    /*   Correct CRC for CMD0 (arg=0x00000000): 0x95                 */
    SD_CS_Low();
    r1 = SD_Command(CMD0, 0x00000000, 0x95);
    SD_CS_High();

    if (r1 != 0x01) {
        /* Expected 0x01 (idle state). Card not responding. */
        return SD_ERROR;
    }

    /* ─── Step 3: CMD8 — SEND_IF_COND ───────────────────────────── */
    /*   Arg: VHS=0x01 (2.7-3.6V), check pattern=0xAA               */
    /*   Valid CRC for CMD8 (arg=0x000001AA): 0x87                   */
    SD_CS_Low();
    r1 = SD_Command(CMD8, 0x000001AA, 0x87); // 0x48 <20x0> 0001 10101010 

    if (r1 == 0x01) {
        /* R7 response — 4 additional bytes */
        uint8_t cmd8_resp[4];
        for (uint8_t i = 0; i < 4; i++) {
            cmd8_resp[i] = SPI1_Transfer(0xFF);
        }
        SD_CS_High();

        /* Validate echo-back: voltage + check pattern */
        if ((cmd8_resp[2] & 0x0F) == 0x01 && cmd8_resp[3] == 0xAA) {
            is_v2_card = 1;  /* SD v2 (or later) */
        } else {
            return SD_UNSUPPORTED;  /* Voltage mismatch */
        }
    } else if (r1 & 0x04) {
        /* Illegal command → SD v1 or MMC card */
        SD_CS_High();
        is_v2_card = 0;
    } else {
        SD_CS_High();
        return SD_ERROR;
    }

    /* ─── Step 4: ACMD41 — SD_SEND_OP_COND (poll until ready) ───── */
    /*   For v2 cards pass HCS bit (host supports SDHC) in arg       */
    timeout = SD_TIMEOUT_INIT;
    do {
        SD_CS_Low();
        r1 = SD_ACommand(CMD41, is_v2_card ? 0x40000000 : 0x00000000);
        SD_CS_High();
        delay_us(500);
    } while (r1 == 0x01 && --timeout);

    if (r1 != 0x00) {
        /* Card failed to initialise — try CMD1 for MMC */
        timeout = SD_TIMEOUT_INIT;
        do {
            SD_CS_Low();
            r1 = SD_Command(CMD1, 0, 0xF9);
            SD_CS_High();
            delay_us(500);
        } while (r1 == 0x01 && --timeout);

        if (r1 != 0x00) return SD_TIMEOUT;

        if (card_info) card_info->type = SD_TYPE_MMC;
    } else {
        if (card_info) card_info->type = is_v2_card ? SD_TYPE_SD2 : SD_TYPE_SD1;
    }

    /* ─── Step 5: CMD58 — READ_OCR → detect SDHC / SDXC ─────────── */
    SD_CS_Low();
    r1 = SD_Command(CMD58, 0, 0xFD);
    if (r1 == 0x00) {
        for (uint8_t i = 0; i < 4; i++) {
            ocr[i] = SPI1_Transfer(0xFF);
        }
        /* OCR bit 30 (CCS) = 1 → SDHC / SDXC */
        if (ocr[0] & 0x40) {
            is_block_addr = 1;
            if (card_info) card_info->type = SD_TYPE_SDHC;
        }
    }
    SD_CS_High();

    /* ─── Step 6: CMD16 — SET_BLOCKLEN = 512 (SDSC only) ─────────── */
    if (!is_block_addr) {
        SD_CS_Low();
        r1 = SD_Command(CMD16, 512, 0x15);
        SD_CS_High();
        if (r1 != 0x00) return SD_ERROR;
    }

    /* ─── Step 7: CMD59 — CRC_ON_OFF = 0 (disable CRC in SPI mode) ─ */
    SD_CS_Low();
    SD_Command(CMD59, 0, 0x91);
    SD_CS_High();

    /* ─── Step 8: Switch SPI to high speed ───────────────────────── */
    SPI1_SetFast();

    return SD_OK;
}

/* ═══════════════════════════════════════════════════════════════
 *  Block Read / Write (512-byte blocks)
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief  Read a single 512-byte block
 * @param  block_addr  Block number (byte address for SDSC, block for SDHC)
 * @param  buf         Output buffer (must be 512 bytes)
 */
SD_Status SD_ReadBlock(uint32_t block_addr, uint8_t *buf)
{
    uint8_t r1;
    uint32_t timeout;

    SD_CS_Low();
    r1 = SD_Command(CMD17, block_addr, 0xFF);
    if (r1 != 0x00) {
        SD_CS_High();
        return SD_ERROR;
    }

    /* Wait for data token 0xFE */
    timeout = SD_TIMEOUT_DATA;
    uint8_t token;
    do {
        token = SPI1_Transfer(0xFF);
    } while (token == 0xFF && --timeout);

    if (token != 0xFE) {
        SD_CS_High();
        return (timeout == 0) ? SD_TIMEOUT : SD_ERROR;
    }

    /* Read 512 data bytes */
    for (uint16_t i = 0; i < 512; i++) {
        buf[i] = SPI1_Transfer(0xFF);
    }
    /* Discard 2 CRC bytes */
    SPI1_Transfer(0xFF);
    SPI1_Transfer(0xFF);

    SD_CS_High();
    return SD_OK;
}

/**
 * @brief  Write a single 512-byte block
 * @param  block_addr  Block number
 * @param  buf         Input buffer (512 bytes)
 */
SD_Status SD_WriteBlock(uint32_t block_addr, const uint8_t *buf)
{
    uint8_t r1;
    uint32_t timeout;

    SD_CS_Low();
    r1 = SD_Command(CMD24, block_addr, 0xFF);
    if (r1 != 0x00) {
        SD_CS_High();
        return SD_ERROR;
    }

    /* Send data token 0xFE, then 512 bytes, then dummy CRC */
    SPI1_Transfer(0xFE);
    for (uint16_t i = 0; i < 512; i++) {
        SPI1_Transfer(buf[i]);
    }
    SPI1_Transfer(0xFF);  /* Dummy CRC high */
    SPI1_Transfer(0xFF);  /* Dummy CRC low  */

    /* Read data response token */
    uint8_t resp = SPI1_Transfer(0xFF) & 0x1F;
    if (resp != 0x05) {
        SD_CS_High();
        return (resp == 0x0B) ? SD_CRC_ERROR : SD_ERROR;
    }

    /* Wait until card finishes programming (MISO goes HIGH) */
    timeout = SD_TIMEOUT_DATA;
    while (SPI1_Transfer(0xFF) == 0x00 && --timeout) { }

    SD_CS_High();
    return (timeout == 0) ? SD_TIMEOUT : SD_OK;
}

/**
 * @brief  Check SD card presence (basic: poll R1 with CMD0)
 * @return 0x01 = present & idle, 0xFF = not responding
 */
uint8_t SD_GetStatus(void)
{
    uint8_t r1;
    SD_CS_Low();
    r1 = SD_Command(CMD0, 0, 0x95);
    SD_CS_High();
    return r1;
}
