#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic RGB565 helpers
#define RGB565(r,g,b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

void SSD1963_Init(void);
void SSD1963_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void SSD1963_Fill(uint16_t rgb565);

// -----------------------------
// Tiny "gfx" helpers (RGB565)
// -----------------------------
void SSD1963_DrawPixel(uint16_t x, uint16_t y, uint16_t rgb565);
void SSD1963_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565);
void SSD1963_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t rgb565);
void SSD1963_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t rgb565);
void SSD1963_WritePixels(const uint16_t *px, uint32_t count);

// Quick confidence tests
void SSD1963_TestColorBars(void);
void SSD1963_TestCheckerboard(uint16_t tile);

#ifdef __cplusplus
}
#endif
