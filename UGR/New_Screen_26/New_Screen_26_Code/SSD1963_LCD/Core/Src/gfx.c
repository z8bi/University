#include "gfx.h"
#include "ssd1963.h"

static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;

    SSD1963_FillRect((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h, color);
}

void gfx_blit565(int x, int y, int w, int h, const uint16_t *src)
{
    if (!src || w <= 0 || h <= 0) return;

    // Clip to screen (800x480 fixed in your driver)
    int x0 = x, y0 = y, x1 = x + w - 1, y1 = y + h - 1;
    if (x1 < 0 || y1 < 0) return;
    if (x0 > 799 || y0 > 479) return;

    int cx0 = clampi(x0, 0, 799);
    int cy0 = clampi(y0, 0, 479);
    int cx1 = clampi(x1, 0, 799);
    int cy1 = clampi(y1, 0, 479);

    int cw = cx1 - cx0 + 1;
    int ch = cy1 - cy0 + 1;

    // Offset into src if we clipped on left/top
    int sx = cx0 - x0;
    int sy = cy0 - y0;

    // Stream row-by-row into LCD window
    for (int row = 0; row < ch; row++) {
        const uint16_t *s = src + (sy + row) * w + sx;
        SSD1963_SetWindow((uint16_t)cx0, (uint16_t)(cy0 + row), (uint16_t)(cx0 + cw - 1), (uint16_t)(cy0 + row));
        SSD1963_WritePixels(s, (uint32_t)cw);
    }
}

// Simple color-key transparency: draws per pixel (fine for 24–48px icons)
void gfx_blit565_key(int x, int y, int w, int h, const uint16_t *src, uint16_t key)
{
    if (!src || w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < 0 || yy > 479) continue;

        for (int i = 0; i < w; i++) {
            int xx = x + i;
            if (xx < 0 || xx > 799) continue;

            uint16_t p = src[j*w + i];
            if (p != key) {
                SSD1963_DrawPixel((uint16_t)xx, (uint16_t)yy, p);
            }
        }
    }
}

// Bresenham line
void gfx_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        if ((unsigned)x0 < 800u && (unsigned)y0 < 480u)
            SSD1963_DrawPixel((uint16_t)x0, (uint16_t)y0, color);

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

//better transparency function with cropping (for blitting large icons in pieces)
void gfx_blit565_key_crop(int dst_x, int dst_y,
                                 int src_w, int src_h,
                                 const uint16_t *src,
                                 int crop_x, int crop_y, int crop_w, int crop_h,
                                 uint16_t key)
{
    if (!src || crop_w <= 0 || crop_h <= 0) return;

    for (int j = 0; j < crop_h; j++) {
        int sy = crop_y + j;
        int dy = dst_y + j;

        if (sy < 0 || sy >= src_h) continue;
        if (dy < 0 || dy >= 480) continue;

        for (int i = 0; i < crop_w; i++) {
            int sx = crop_x + i;
            int dx = dst_x + i;

            if (sx < 0 || sx >= src_w) continue;
            if (dx < 0 || dx >= 800) continue;

            uint16_t p = src[sy * src_w + sx];
            if (p != key) {
                SSD1963_DrawPixel((uint16_t)dx, (uint16_t)dy, p);
            }
        }
    }
}
