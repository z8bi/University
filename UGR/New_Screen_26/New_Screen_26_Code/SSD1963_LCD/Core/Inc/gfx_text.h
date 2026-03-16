#ifndef GFX_TEXT_H
#define GFX_TEXT_H

#include <stdint.h>
#include "gfx_font.h"

void gfx_draw_char_font(int16_t x, int16_t y, char c,
                        const GFXfont *font, uint16_t color);

void gfx_draw_text_font(int16_t x, int16_t y, const char *s,
                        const GFXfont *font, uint16_t color);

int16_t gfx_text_width_font(const char *s, const GFXfont *font);

#endif