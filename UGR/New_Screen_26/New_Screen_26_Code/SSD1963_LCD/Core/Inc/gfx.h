#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint16_t w, h;
    const uint16_t *px;   // RGB565, row-major
} Bitmap565;

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color);

// Fast bitmap draw (no transparency)
void gfx_blit565(int x, int y, int w, int h, const uint16_t *src);
static inline void gfx_draw_bitmap(int x, int y, const Bitmap565 *bm)
{
    gfx_blit565(x, y, bm->w, bm->h, bm->px);
}

// Color-key transparency (simple version – good for small icons)
void gfx_blit565_key(int x, int y, int w, int h, const uint16_t *src, uint16_t key);

// Needle primitive
void gfx_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
