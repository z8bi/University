#include "gfx_text.h"
#include "gfx.h"

void gfx_draw_char_font(int16_t x, int16_t y, char c,
                        const GFXfont *font, uint16_t color)
{
    if (!font) return;
    if ((uint8_t)c < font->first || (uint8_t)c > font->last) return;

    const GFXglyph *glyph = &font->glyph[(uint8_t)c - font->first];
    const uint8_t *bitmap = font->bitmap + glyph->bitmapOffset;

    uint8_t bits = 0, bitmask = 0;

    for (int yy = 0; yy < glyph->height; yy++) {
        for (int xx = 0; xx < glyph->width; xx++) {
            if (bitmask == 0) {
                bits = *bitmap++;
                bitmask = 0x80;
            }

            if (bits & bitmask) {
                gfx_draw_pixel(x + glyph->xOffset + xx,
                               y + glyph->yOffset + yy,
                               color);
            }

            bitmask >>= 1;
        }
    }
}

void gfx_draw_text_font(int16_t x, int16_t y, const char *s,
                        const GFXfont *font, uint16_t color)
{
    if (!font || !s) return;

    int16_t cursor_x = x;
    int16_t cursor_y = y; // baseline

    while (*s) {
        if (*s == '\n') {
            cursor_x = x;
            cursor_y += font->yAdvance;
            s++;
            continue;
        }

        uint8_t ch = (uint8_t)*s;
        if (ch >= font->first && ch <= font->last) {
            const GFXglyph *glyph = &font->glyph[ch - font->first];
            gfx_draw_char_font(cursor_x, cursor_y, *s, font, color);
            cursor_x += glyph->xAdvance;
        }

        s++;
    }
}

int16_t gfx_text_width_font(const char *s, const GFXfont *font)
{
    if (!font || !s) return 0;

    int16_t w = 0;
    while (*s && *s != '\n') {
        uint8_t ch = (uint8_t)*s;
        if (ch >= font->first && ch <= font->last) {
            w += font->glyph[ch - font->first].xAdvance;
        }
        s++;
    }
    return w;
}