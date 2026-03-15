#ifndef GT911_H
#define GT911_H

#include "main.h"
#include "i2c.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GT911_ADDR1         (0x5D << 1)   // HAL uses 8-bit address
#define GT911_ADDR2         (0x14 << 1)

#define GT911_REG_STATUS    0x814E
#define GT911_REG_POINT1    0x814F
#define GT911_REG_PRODUCTID 0x8140

typedef struct
{
    uint8_t touched;
    uint8_t points;
    uint16_t x;
    uint16_t y;
    uint16_t size;
    uint8_t track_id;
} GT911_State;

/* Optional helper type for rectangular touch areas */
typedef struct
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
} GT911_Area;

HAL_StatusTypeDef GT911_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef GT911_ReadState(GT911_State *state);
uint8_t GT911_IsReady(void);

#ifdef __cplusplus
}
#endif

#endif