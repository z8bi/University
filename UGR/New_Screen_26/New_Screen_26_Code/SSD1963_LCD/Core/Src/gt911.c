#include "gt911.h"
#include <string.h>

static I2C_HandleTypeDef *gt911_i2c = NULL;
static uint16_t gt911_addr = GT911_ADDR1;
static uint8_t gt911_ready = 0;

static HAL_StatusTypeDef GT911_Read(uint16_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t regbuf[2];
    regbuf[0] = (uint8_t)((reg >> 8) & 0xFF);
    regbuf[1] = (uint8_t)(reg & 0xFF);

    HAL_StatusTypeDef st;

    st = HAL_I2C_Master_Transmit(gt911_i2c, gt911_addr, regbuf, 2, HAL_MAX_DELAY);
    if (st != HAL_OK) return st;

    return HAL_I2C_Master_Receive(gt911_i2c, gt911_addr, buf, len, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef GT911_Write(uint16_t reg, const uint8_t *buf, uint16_t len)
{
    uint8_t tmp[18];
    if (len > (sizeof(tmp) - 2)) return HAL_ERROR;

    tmp[0] = (uint8_t)((reg >> 8) & 0xFF);
    tmp[1] = (uint8_t)(reg & 0xFF);
    memcpy(&tmp[2], buf, len);

    return HAL_I2C_Master_Transmit(gt911_i2c, gt911_addr, tmp, len + 2, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef GT911_TryAddress(uint16_t addr)
{
    uint8_t pid[4];

    gt911_addr = addr;
    if (GT911_Read(GT911_REG_PRODUCTID, pid, 4) != HAL_OK)
        return HAL_ERROR;

    if ((pid[0] >= '0' && pid[0] <= '9') ||
        (pid[1] >= '0' && pid[1] <= '9') ||
        (pid[2] >= '0' && pid[2] <= '9'))
    {
        return HAL_OK;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef GT911_Init(I2C_HandleTypeDef *hi2c)
{
    gt911_i2c = hi2c;
    gt911_ready = 0;

    HAL_Delay(50);

    if (GT911_TryAddress(GT911_ADDR1) == HAL_OK)
    {
        gt911_ready = 1;
        return HAL_OK;
    }

    if (GT911_TryAddress(GT911_ADDR2) == HAL_OK)
    {
        gt911_ready = 1;
        return HAL_OK;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef GT911_ReadState(GT911_State *state)
{
    uint8_t status;
    uint8_t point[7];
    uint8_t clear = 0;

    if (!gt911_ready || state == NULL)
        return HAL_ERROR;

    memset(state, 0, sizeof(*state));

    if (GT911_Read(GT911_REG_STATUS, &status, 1) != HAL_OK)
        return HAL_ERROR;

    if ((status & 0x80U) == 0U)
        return HAL_OK;

    state->points = status & 0x0FU;

    if (state->points == 0)
    {
        if (GT911_Write(GT911_REG_STATUS, &clear, 1) != HAL_OK)
            return HAL_ERROR;
        return HAL_OK;
    }

    if (GT911_Read(GT911_REG_POINT1, point, 7) != HAL_OK)
        return HAL_ERROR;

    state->touched  = 1;
    state->track_id = point[0];
    state->x        = (uint16_t)point[1] | ((uint16_t)point[2] << 8);
    state->y        = (uint16_t)point[3] | ((uint16_t)point[4] << 8);
    state->size     = (uint16_t)point[5] | ((uint16_t)point[6] << 8);

    if (GT911_Write(GT911_REG_STATUS, &clear, 1) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}