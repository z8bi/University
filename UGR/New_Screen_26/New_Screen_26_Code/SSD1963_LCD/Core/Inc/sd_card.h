#ifndef SD_CARD_H
#define SD_CARD_H

#include "main.h"
#include "spi.h"
#include "gpio.h"

uint8_t SD_Init(void);

uint8_t SD_ReadBlock(uint32_t sector, uint8_t *buffer);

void SD_Debug(const char *msg, uint16_t color);

uint8_t SD_ReadBlocks(uint32_t sector, uint8_t *buffer, uint32_t count);

uint8_t draw_raw565_from_sd(const char *path,
                            uint16_t x,
                            uint16_t y,
                            uint16_t w,
                            uint16_t h);

#endif