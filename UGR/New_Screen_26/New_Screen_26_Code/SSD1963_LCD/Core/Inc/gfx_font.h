#ifndef GFX_FONT_H
#define GFX_FONT_H

#include <stdint.h>

typedef struct {
    uint16_t bitmapOffset; // Offset into bitmap array
    uint8_t  width;        // Glyph width in pixels
    uint8_t  height;       // Glyph height in pixels
    uint8_t  xAdvance;     // Cursor advance after drawing
    int8_t   xOffset;      // X offset from cursor position
    int8_t   yOffset;      // Y offset from baseline
} GFXglyph;

typedef struct {
    const uint8_t  *bitmap; // Glyph bitmaps, packed bits
    const GFXglyph *glyph;  // Glyph metadata array
    uint16_t first;         // First supported character
    uint16_t last;          // Last supported character
    uint8_t  yAdvance;      // Line advance
} GFXfont;

#endif