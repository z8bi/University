#include "sd_card.h"
#include "fatfs.h"
#include "ff.h"
#include "ssd1963.h"
#include "logos/ugr_logo.h"

#define SD_CS_PORT SD_Card_CS_GPIO_Port
#define SD_CS_PIN  SD_Card_CS_Pin

//buffer
static uint8_t sd_dummy_tx[512];

static uint8_t sd_is_sdhc = 0;

static void SD_CS_LOW(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
}

static void SD_CS_HIGH(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
}

static uint8_t SPI_TxRx(uint8_t data)
{
    uint8_t rx = 0xFF;
    HAL_SPI_TransmitReceive(&hspi3, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static void SD_SendClockTrain(void)
{
    SD_CS_HIGH();
    for (int i = 0; i < 10; i++)
    {
        SPI_TxRx(0xFF);
    }
}

static uint8_t SD_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (SPI_TxRx(0xFF) == 0xFF)
        {
            return 1;
        }
    }
    return 0;
}

static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t response = 0xFF;

    SD_CS_HIGH();
    SPI_TxRx(0xFF);

    SD_CS_LOW();
    SPI_TxRx(0xFF);

    SPI_TxRx(cmd | 0x40);
    SPI_TxRx((uint8_t)(arg >> 24));
    SPI_TxRx((uint8_t)(arg >> 16));
    SPI_TxRx((uint8_t)(arg >> 8));
    SPI_TxRx((uint8_t)(arg));
    SPI_TxRx(crc);

    for (int i = 0; i < 10; i++)
    {
        response = SPI_TxRx(0xFF);
        if ((response & 0x80) == 0)
        {
            break;
        }
    }

    return response;
}

static uint8_t SD_SendAppCommand(uint8_t acmd, uint32_t arg, uint8_t *r1)
{
    *r1 = SD_SendCommand(55, 0, 0x01);   // CMD55
    SD_CS_HIGH();
    SPI_TxRx(0xFF);

    if (*r1 > 0x01)
    {
        return 0;
    }

    *r1 = SD_SendCommand(acmd, arg, 0x01);  // ACMDxx
    SD_CS_HIGH();
    SPI_TxRx(0xFF);

    return 1;
}

uint8_t SD_Init(void)
{
    uint8_t r1;
    uint8_t r7[4];
    uint8_t ocr[4];

    for (int i = 0; i < 512; i++)
    {
        sd_dummy_tx[i] = 0xFF;
    }

    sd_is_sdhc = 0;

    SD_CS_HIGH();
    SPI_TxRx(0xFF);
    SD_SendClockTrain();

    // CMD0: enter idle state
    for (int i = 0; i < 10; i++)
    {
        r1 = SD_SendCommand(0, 0, 0x95);
        SD_CS_HIGH();
        SPI_TxRx(0xFF);

        if (r1 == 0x01)
        {
            break;
        }

        if (i == 9)
        {
            return 0;
        }
    }

    // CMD8: check SD v2
    r1 = SD_SendCommand(8, 0x000001AA, 0x87);
    if (r1 == 0x01)
    {
        for (int i = 0; i < 4; i++)
        {
            r7[i] = SPI_TxRx(0xFF);
        }
        SD_CS_HIGH();
        SPI_TxRx(0xFF);

        // Expect voltage accepted + check pattern 0x1AA
        if (r7[2] != 0x01 || r7[3] != 0xAA)
        {
            return 0;
        }

        // ACMD41 with HCS
        uint32_t start = HAL_GetTick();
        do
        {
            if (!SD_SendAppCommand(41, 0x40000000, &r1))
            {
                return 0;
            }

            if (r1 == 0x00)
            {
                break;
            }
        } while ((HAL_GetTick() - start) < 1000);

        if (r1 != 0x00)
        {
            return 0;
        }

        // CMD58: read OCR
        r1 = SD_SendCommand(58, 0, 0x01);
        if (r1 != 0x00)
        {
            SD_CS_HIGH();
            SPI_TxRx(0xFF);
            return 0;
        }

        for (int i = 0; i < 4; i++)
        {
            ocr[i] = SPI_TxRx(0xFF);
        }
        SD_CS_HIGH();
        SPI_TxRx(0xFF);

        // CCS bit indicates SDHC/SDXC block addressing
        if (ocr[0] & 0x40)
        {
            sd_is_sdhc = 1;
        }

        return 1;
    }
    else if (r1 == 0x05)
    {
        // Older SDSC card path
        SD_CS_HIGH();
        SPI_TxRx(0xFF);

        uint32_t start = HAL_GetTick();
        do
        {
            if (!SD_SendAppCommand(41, 0x00000000, &r1))
            {
                return 0;
            }

            if (r1 == 0x00)
            {
                break;
            }
        } while ((HAL_GetTick() - start) < 1000);

        if (r1 != 0x00)
        {
            return 0;
        }

        return 1;
    }
    else
    {
        SD_CS_HIGH();
        SPI_TxRx(0xFF);
        return 0;
    }
}

uint8_t SD_ReadBlock(uint32_t sector, uint8_t *buffer)
{
    uint8_t token = 0xFF;
    uint32_t addr = sd_is_sdhc ? sector : (sector * 512);

    uint8_t r1 = SD_SendCommand(17, addr, 0x01);   // CMD17
    if (r1 != 0x00)
    {
        SD_CS_HIGH();
        SPI_TxRx(0xFF);
        return 0;
    }

    uint32_t start = HAL_GetTick();
    do
    {
        token = SPI_TxRx(0xFF);
        if (token == 0xFE)
        {
            break;
        }
    } while ((HAL_GetTick() - start) < 100);

    if (token != 0xFE)
    {
        SD_CS_HIGH();
        SPI_TxRx(0xFF);
        return 0;
    }

    if (HAL_SPI_TransmitReceive(&hspi3, sd_dummy_tx, buffer, 512, HAL_MAX_DELAY) != HAL_OK)
    {
        SD_CS_HIGH();
        SPI_TxRx(0xFF);
        return 0;
    }

    // discard CRC
    SPI_TxRx(0xFF);
    SPI_TxRx(0xFF);

    SD_CS_HIGH();
    SPI_TxRx(0xFF);

    return 1;
}

uint8_t SD_ReadBlocks(uint32_t sector, uint8_t *buffer, uint32_t count)
{
    uint8_t token = 0xFF;
    uint32_t addr = sd_is_sdhc ? sector : (sector * 512);

    if (count == 0)
    {
        return 0;
    }

    // Single block can still use CMD17
    if (count == 1)
    {
        return SD_ReadBlock(sector, buffer);
    }

    // CMD18 = READ_MULTIPLE_BLOCK
    if (SD_SendCommand(18, addr, 0x01) != 0x00)
    {
        SD_CS_HIGH();
        SPI_TxRx(0xFF);
        return 0;
    }

    for (uint32_t blk = 0; blk < count; blk++)
    {
        uint32_t start = HAL_GetTick();

        do
        {
            token = SPI_TxRx(0xFF);
            if (token == 0xFE)
            {
                break;
            }
        } while ((HAL_GetTick() - start) < 100);

        if (token != 0xFE)
        {
            SD_CS_HIGH();
            SPI_TxRx(0xFF);
            return 0;
        }

        if (HAL_SPI_TransmitReceive(&hspi3,
                                    sd_dummy_tx,
                                    buffer + (blk * 512),
                                    512,
                                    HAL_MAX_DELAY) != HAL_OK)
        {
            SD_CS_HIGH();
            SPI_TxRx(0xFF);
            return 0;
        }

        // discard CRC
        SPI_TxRx(0xFF);
        SPI_TxRx(0xFF);
    }

    // Stop transmission: CMD12
    SD_CS_HIGH();
    SPI_TxRx(0xFF);

    SD_CS_LOW();
    SPI_TxRx(0xFF);
    SPI_TxRx(0x40 | 12);
    SPI_TxRx(0x00);
    SPI_TxRx(0x00);
    SPI_TxRx(0x00);
    SPI_TxRx(0x00);
    SPI_TxRx(0x01);

    // CMD12 has a stuff byte / delayed response behavior
    for (int i = 0; i < 10; i++)
    {
        token = SPI_TxRx(0xFF);
        if ((token & 0x80) == 0)
        {
            break;
        }
    }

    SD_CS_HIGH();
    SPI_TxRx(0xFF);

    return 1;
}

