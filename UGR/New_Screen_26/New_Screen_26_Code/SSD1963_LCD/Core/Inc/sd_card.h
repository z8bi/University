#ifndef SD_CARD_H
#define SD_CARD_H

#include "main.h"
#include "spi.h"
#include "gpio.h"

uint8_t SD_Init(void);

uint8_t SD_ReadBlock(uint32_t sector, uint8_t *buffer);

void SD_Debug(const char *msg, uint16_t color);

#endif