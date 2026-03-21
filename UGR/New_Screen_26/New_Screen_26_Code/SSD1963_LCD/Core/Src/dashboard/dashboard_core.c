#include "dashboard_core.h"
#include "dashboard_fonts.h"

#include "ssd1963.h"
#include "gfx_text.h"
#include "logos/ugr_logo.h"

// REAL FONT DEFINITIONS: include in exactly one .c file
#include "FreeSansBold18pt7b.h"
#include "FreeSansBold24pt7b.h"
#include "fonts/F1Bold18pt7b.h"
#include "fonts/F1Bold24pt7b.h"
#include "fonts/F1Bold40pt7b.h"

// SD CARD
#include "sd_card.h"
#include "fatfs.h"
#include "ff.h"

#include <string.h>
#include <stdio.h>

// ======================================================
// INTERNAL CONSTANTS
// ======================================================

#define LOGO_CHUNK_ROWS 1

//GRAPH RAM ALLOCATION
#define GRAPH_FB_W  700
#define GRAPH_FB_H  130

//Scrolling speed
#define GRAPH_VISIBLE_SAMPLES 200   // 5 seconds at 50 Hz

static uint16_t graph_fb[GRAPH_FB_W * GRAPH_FB_H];

// ======================================================
// INTERNAL STORAGE
// ======================================================

static uint16_t logo_chunk[BIG_UGR_LOGO_W * LOGO_CHUNK_ROWS];

// ======================================================
// INTERNAL HELPER PROTOTYPES
// ======================================================

static const GFXfont *numeric_block_value_font(const NumericBlockWidget *w);
static Rect get_numeric_block_value_rect(const NumericBlockWidget *w, int value);
static Rect get_lap_value_rect(const NumericBlockWidget *w, int current_lap, int total_laps);

static Rect get_vertical_bar_inner_rect(const VerticalBarWidget *w);
static Rect get_vertical_bar_value_rect(const VerticalBarWidget *w, int value);
static void draw_vertical_bar_guides(const VerticalBarWidget *w);
static void redraw_vertical_bar_guides_in_range(const VerticalBarWidget *w, int y0, int y1);
static void draw_vertical_bar_value_text(const VerticalBarWidget *w, int value, uint16_t fill_col);

static int map_value_to_fill_height(int value, int min_value, int max_value, int pixel_height);

static int graph_oldest_index(const DualTraceGraphWidget *g);
static int graph_buffer_index(const DualTraceGraphWidget *g, int visible_pos);
static int graph_value_to_y(const DualTraceGraphWidget *g, int value);

// ======================================================
// GENERIC HELPERS
// ======================================================

static Rect get_lap_counter_styled_rect(const NumericBlockWidget *w,
                                        int current_lap, int total_laps)
{
    char main_buf[12];
    char small_buf[12];

    int main_w, main_min_y, main_max_y;
    int small_w, small_min_y, small_max_y;

    int main_baseline_y;
    int main_x;
    int small_x;
    int small_baseline_y;

    int total_w;

    Rect r_main;
    Rect r_small;

    const GFXfont *main_font = &F1Bold40pt7b;
    const GFXfont *small_font = &F1Bold24pt7b;

    if (!w) {
        return make_rect(0, 0, 0, 0);
    }

    snprintf(main_buf, sizeof(main_buf), "%d", current_lap);
    snprintf(small_buf, sizeof(small_buf), "/%d", total_laps);

    font_text_bounds(main_buf, main_font, &main_w, &main_min_y, &main_max_y);
    font_text_bounds(small_buf, small_font, &small_w, &small_min_y, &small_max_y);

    main_baseline_y = (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset;

    total_w = main_w + 6 + small_w;

    main_x = w->area.x + (w->area.w - total_w) / 2;
    small_x = main_x + main_w + 6;

    r_main = make_rect(
        main_x - 6,
        main_baseline_y + main_min_y - 6,
        main_w + 12,
        (main_max_y - main_min_y + 1) + 12
    );

    small_baseline_y = main_baseline_y - 12;

    r_small = make_rect(
        small_x - 4,
        small_baseline_y + small_min_y - 4,
        small_w + 8,
        (small_max_y - small_min_y + 1) + 8
    );

    return union_rects(r_main, r_small);
}

static void draw_hline_thick(int x0, int x1, int y, uint16_t col, int thickness)
{
    int i;
    int half = thickness / 2;

    for (i = -half; i <= half; i++) {
        gfx_draw_line(x0, y + i, x1, y + i, col);
    }
}

static void graph_fb_clear(uint16_t color, int w, int h)
{
    int n = w * h;
    int i;
    for (i = 0; i < n; i++) {
        graph_fb[i] = color;
    }
}

static void graph_fb_put_pixel(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return;
    }
    graph_fb[y * w + x] = color;
}

static void graph_fb_draw_line(int x0, int y0, int x1, int y1,
                               int w, int h, uint16_t color)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        graph_fb_put_pixel(x0, y0, w, h, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        {
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

static void graph_fb_draw_thick_line(int x0, int y0, int x1, int y1,
                                     int w, int h,
                                     uint16_t color, int thickness)
{
    int i;
    int half = thickness / 2;

    for (i = -half; i <= half; i++) {
        graph_fb_draw_line(x0, y0 + i, x1, y1 + i, w, h, color);
    }
}

static void graph_fb_blit_to_lcd(int lcd_x, int lcd_y, int w, int h)
{
    int row;

    for (row = 0; row < h; row++) {
        SSD1963_SetWindow(lcd_x, lcd_y + row,
                          lcd_x + w - 1, lcd_y + row);
        SSD1963_WritePixels(&graph_fb[row * w], w);
    }
}

static void graph_draw_thick_line(int x0, int y0,
                                  int x1, int y1,
                                  uint16_t color,
                                  int thickness)
{
    int i;
    int half;

    if (thickness <= 1) {
        gfx_draw_line(x0, y0, x1, y1, color);
        return;
    }

    half = thickness / 2;

    for (i = -half; i <= half; i++) {
        gfx_draw_line(x0, y0 + i, x1, y1 + i, color);
    }
}

Rect make_rect(int x, int y, int w, int h)
{
    Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

Rect union_rects(Rect a, Rect b)
{
    int x1 = (a.x < b.x) ? a.x : b.x;
    int y1 = (a.y < b.y) ? a.y : b.y;
    int x2 = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
    int y2 = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

    return make_rect(x1, y1, x2 - x1, y2 - y1);
}

int clampi(int v, int lo, int hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void swap_int(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int font_text_width(const char *text, const GFXfont *font)
{
    int w = 0;

    if (!text || !font) {
        return 0;
    }

    while (*text) {
        unsigned char c = (unsigned char)*text++;
        if (c < font->first || c > font->last) {
            continue;
        }
        w += font->glyph[c - font->first].xAdvance;
    }

    return w;
}

void font_text_bounds(const char *text, const GFXfont *font,
                      int *out_w, int *out_min_y, int *out_max_y)
{
    int x = 0;
    int min_y = 32767;
    int max_y = -32768;

    if (!out_w || !out_min_y || !out_max_y) {
        return;
    }

    if (!text || !font) {
        *out_w = 0;
        *out_min_y = 0;
        *out_max_y = 0;
        return;
    }

    while (*text) {
        unsigned char c = (unsigned char)*text++;
        if (c < font->first || c > font->last) {
            continue;
        }

        {
            const GFXglyph *g = &font->glyph[c - font->first];
            int gy0 = g->yOffset;
            int gy1 = g->yOffset + g->height - 1;

            if (g->height > 0) {
                if (gy0 < min_y) {
                    min_y = gy0;
                }
                if (gy1 > max_y) {
                    max_y = gy1;
                }
            }

            x += g->xAdvance;
        }
    }

    if (min_y == 32767) {
        min_y = 0;
        max_y = 0;
    }

    *out_w = x;
    *out_min_y = min_y;
    *out_max_y = max_y;
}

void draw_rect_outline(int x, int y, int w, int h, uint16_t col)
{
    gfx_draw_line(x,         y,         x + w - 1, y,         col);
    gfx_draw_line(x,         y + h - 1, x + w - 1, y + h - 1, col);
    gfx_draw_line(x,         y,         x,         y + h - 1, col);
    gfx_draw_line(x + w - 1, y,         x + w - 1, y + h - 1, col);
}

void draw_text_centered_in_rect(const Rect *r, int y, const char *text,
                                const GFXfont *font, uint16_t color)
{
    int tw;
    int x;

    if (!r) {
        return;
    }

    tw = font_text_width(text, font);
    x = r->x + (r->w - tw) / 2;
    gfx_draw_text_font(x, y, text, font, color);
}

void draw_text_right_aligned(int right_x, int y, const char *text,
                             const GFXfont *font, uint16_t color)
{
    int tw = font_text_width(text, font);
    gfx_draw_text_font(right_x - tw, y, text, font, color);
}

void draw_text_right_aligned_centered_y(int right_x, int center_y,
                                        const char *text,
                                        const GFXfont *font,
                                        uint16_t color)
{
    int tw;
    int min_y;
    int max_y;
    int baseline_y;

    font_text_bounds(text, font, &tw, &min_y, &max_y);
    baseline_y = center_y - (min_y + max_y) / 2;

    gfx_draw_text_font(right_x - tw, baseline_y, text, font, color);
}

// ======================================================
// COLOR HELPERS
// ======================================================

uint16_t battery_color_for_percent(int percent)
{
    percent = clampi(percent, 0, 100);

    if (percent <= BATT_RED_MAX) {
        return COL_WARN;
    }
    if (percent <= BATT_ORANGE_MAX) {
        return RGB565(255, 165, 0);
    }
    return COL_GOOD;
}

uint16_t temp_color_for_value(int temp)
{
    temp = clampi(temp, 0, 150);

    if (temp <= TEMP_GREEN_MAX) {
        return COL_GOOD;
    }
    if (temp <= TEMP_ORANGE_MAX) {
        return RGB565(255, 165, 0);
    }
    return COL_WARN;
}

// ======================================================
// UNIT GLYPHS
// ======================================================

void draw_percent(int x, int y, int s, uint16_t col)
{
    int r = s;
    gfx_fill_rect(x,         y,         r, r, col);
    gfx_fill_rect(x + 4 * s, y + 4 * s, r, r, col);
    gfx_draw_line(x, y + 5 * s, x + 5 * s, y, col);
}

void draw_C(int x, int y, int s, uint16_t col)
{
    gfx_fill_rect(x,         y,         4 * s, s,     col);
    gfx_fill_rect(x,         y,         s,     6 * s, col);
    gfx_fill_rect(x,         y + 5 * s, 4 * s, s,     col);
}

// ======================================================
// NUMERIC BLOCK INTERNAL HELPERS
// ======================================================

static const GFXfont *numeric_block_value_font(const NumericBlockWidget *w)
{
    if (w && w->value_font) {
        return w->value_font;
    }
    return &FreeSansBold24pt7b;
}

static Rect get_numeric_block_value_rect(const NumericBlockWidget *w, int value)
{
    char buf[16];
    int tw;
    int min_y;
    int max_y;
    int baseline_y;
    int text_h;
    int x;
    int y;
    const GFXfont *font = numeric_block_value_font(w);

    snprintf(buf, sizeof(buf), "%d", value);
    font_text_bounds(buf, font, &tw, &min_y, &max_y);

    baseline_y = (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset;
    text_h = max_y - min_y + 1;

    x = w->area.x + (w->area.w - tw) / 2 - 6;
    y = baseline_y + min_y - 6;

    return make_rect(x, y, tw + 12, text_h + 12);
}

static Rect get_lap_value_rect(const NumericBlockWidget *w, int current_lap, int total_laps)
{
    char buf[20];
    int tw;
    int min_y;
    int max_y;
    int baseline_y;
    int text_h;
    int x;
    int y;
    const GFXfont *font = numeric_block_value_font(w);

    snprintf(buf, sizeof(buf), "%d", current_lap, total_laps);
    font_text_bounds(buf, font, &tw, &min_y, &max_y);

    baseline_y = (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset;
    text_h = max_y - min_y + 1;

    x = w->area.x + (w->area.w - tw) / 2 - 6;
    y = baseline_y + min_y - 6;

    return make_rect(x, y, tw + 12, text_h + 12);
}

static void draw_numeric_block_value_text(const NumericBlockWidget *w, int value, uint16_t color)
{
    char buf[16];
    Rect value_rect = get_numeric_block_value_rect(w, value);
    const GFXfont *font = numeric_block_value_font(w);

    gfx_fill_rect(value_rect.x, value_rect.y, value_rect.w, value_rect.h, w->bg_color);

    snprintf(buf, sizeof(buf), "%d", value);

    draw_text_centered_in_rect(
        &w->area,
        (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset,
        buf,
        font,
        color
    );
}

// ======================================================
// VERTICAL BAR INTERNAL HELPERS
// ======================================================

static Rect get_vertical_bar_inner_rect(const VerticalBarWidget *w)
{
    return make_rect(
        w->bar_rect.x + 2,
        w->bar_rect.y + 2,
        w->bar_rect.w - 4,
        w->bar_rect.h - 4
    );
}

static Rect get_vertical_bar_value_rect(const VerticalBarWidget *w, int value)
{
    char buf[12];
    int text_w;
    int min_y;
    int max_y;
    int unit_w = 0;
    int unit_h = 12;
    int gap = 0;
    int total_w;
    int total_h;
    int x;
    int y;
    int cx;
    int text_h;

    snprintf(buf, sizeof(buf), "%d", value);
    font_text_bounds(buf, &F1Bold24pt7b, &text_w, &min_y, &max_y);

    if (w->unit_draw_fn) {
        unit_w = 12;
        gap = 10;
    }

    text_h = max_y - min_y + 1;
    total_w = text_w + gap + unit_w;
    total_h = (text_h > unit_h) ? text_h : unit_h;

    cx = w->bar_rect.x + w->bar_rect.w / 2;
    x = cx - total_w / 2 - 8;
    y = w->bar_rect.y + w->bar_rect.h + 4;

    return make_rect(x, y, total_w + 16, total_h + 16);
}

static void redraw_vertical_bar_guides_in_range(const VerticalBarWidget *w, int y0, int y1)
{
    const Rect *b = &w->bar_rect;
    int inner_x = b->x + 2;
    int inner_y = b->y + 2;
    int inner_w = b->w - 4;
    int inner_h = b->h - 4;
    int i;

    if (y0 > y1) {
        swap_int(&y0, &y1);
    }

    for (i = 0; i <= 4; i++) {
        int gy = inner_y + (i * inner_h) / 4;
        if (gy >= y0 && gy <= y1) {
            draw_hline_thick(inner_x, inner_x + inner_w - 1, gy, COL_GRID, 3);
        }
    }
}

static void draw_vertical_bar_value_text(const VerticalBarWidget *w, int value, uint16_t fill_col)
{
    Rect value_rect = get_vertical_bar_value_rect(w, value);
    char buf[12];
    int text_w;
    int min_y;
    int max_y;
    int text_x;
    int baseline_y;
    int text_h;
    int unit_y;

    gfx_fill_rect(value_rect.x, value_rect.y, value_rect.w, value_rect.h, w->bg_color);

    snprintf(buf, sizeof(buf), "%d", value);
    font_text_bounds(buf, &F1Bold24pt7b, &text_w, &min_y, &max_y);

    text_h = max_y - min_y + 1;
    text_x = value_rect.x + 8;
    baseline_y = value_rect.y + 8 - min_y;

    gfx_draw_text_font(text_x, baseline_y, buf, &F1Bold24pt7b, fill_col);

    if (w->unit_draw_fn) {
        unit_y = value_rect.y + 8 + (text_h - 12) / 2;
        w->unit_draw_fn(text_x + text_w + 10, unit_y, 2, fill_col);
    }
}

static void draw_vertical_bar_guides(const VerticalBarWidget *w)
{
    const Rect *b = &w->bar_rect;
    int inner_x = b->x + 2;
    int inner_y = b->y + 2;
    int inner_w = b->w - 4;
    int inner_h = b->h - 4;
    int i;
    char buf[12];

    for (i = 0; i <= 4; i++) {
        int gy = inner_y + (i * inner_h) / 4;
        int value = w->max_value - ((w->max_value - w->min_value) * i) / 4;

        int tw;
        int min_y;
        int max_y;
        int th;
        int label_cy;

        snprintf(buf, sizeof(buf), "%d", value);
        font_text_bounds(buf, &FreeSansBold18pt7b, &tw, &min_y, &max_y);
        th = max_y - min_y + 1;

        label_cy = clampi(gy,
                          inner_y + th / 2,
                          inner_y + inner_h - th / 2);

        draw_hline_thick(inner_x, inner_x + inner_w - 1, gy, COL_GRID, 3);

        draw_text_right_aligned_centered_y(
            b->x + b->w + 55,
            label_cy,
            buf,
            &FreeSansBold18pt7b,
            w->text_color
        );
    }
}

static int map_value_to_fill_height(int value, int min_value, int max_value, int pixel_height)
{
    value = clampi(value, min_value, max_value);

    if (max_value <= min_value) {
        return 0;
    }

    return ((value - min_value) * pixel_height) / (max_value - min_value);
}

// ======================================================
// VERTICAL BAR WIDGET
// ======================================================

void draw_vertical_bar_static(const VerticalBarWidget *w)
{
    const Rect *a;
    const Rect *b;

    if (!w) {
        return;
    }

    a = &w->area;
    b = &w->bar_rect;

    gfx_fill_rect(a->x, a->y, a->w, a->h, w->bg_color);

    draw_text_centered_in_rect(
        a,
        a->y + 22,
        w->title,
        &FreeSansBold24pt7b,
        w->text_color
    );

    draw_rect_outline(b->x, b->y, b->w, b->h, w->frame_color);
    gfx_fill_rect(b->x + 2, b->y + 2, b->w - 4, b->h - 4, COL_BAR_BG);

    draw_vertical_bar_guides(w);
}

void draw_vertical_bar_value_full(const VerticalBarWidget *w, int value)
{
    Rect inner;
    int clamped;
    int shown_value;
    int fill_h;
    uint16_t fill_col;

    if (!w) {
        return;
    }

    inner = get_vertical_bar_inner_rect(w);
    clamped = clampi(value, w->min_value, w->max_value);
    shown_value = w->clamp_value_text ? clamped : value;
    fill_h = map_value_to_fill_height(clamped, w->min_value, w->max_value, inner.h);
    fill_col = w->color_fn ? w->color_fn(value) : COL_ACC;

    gfx_fill_rect(inner.x, inner.y, inner.w, inner.h, COL_BAR_BG);

    if (fill_h > 0) {
        int fy = inner.y + inner.h - fill_h;
        gfx_fill_rect(inner.x, fy, inner.w, fill_h, fill_col);
    }

    draw_vertical_bar_guides(w);
    draw_vertical_bar_value_text(w, shown_value, fill_col);
}

void draw_vertical_bar_value_delta(const VerticalBarWidget *w, int old_value, int new_value)
{
    Rect inner;
    int old_clamped;
    int new_clamped;
    int old_shown;
    int new_shown;
    int old_fill_h;
    int new_fill_h;
    uint16_t old_col;
    uint16_t new_col;
    int old_top;
    int new_top;

    if (!w) {
        return;
    }

    inner = get_vertical_bar_inner_rect(w);

    old_clamped = clampi(old_value, w->min_value, w->max_value);
    new_clamped = clampi(new_value, w->min_value, w->max_value);

    old_shown = w->clamp_value_text ? old_clamped : old_value;
    new_shown = w->clamp_value_text ? new_clamped : new_value;

    old_fill_h = map_value_to_fill_height(old_clamped, w->min_value, w->max_value, inner.h);
    new_fill_h = map_value_to_fill_height(new_clamped, w->min_value, w->max_value, inner.h);

    old_col = w->color_fn ? w->color_fn(old_value) : COL_ACC;
    new_col = w->color_fn ? w->color_fn(new_value) : COL_ACC;

    old_top = inner.y + inner.h - old_fill_h;
    new_top = inner.y + inner.h - new_fill_h;

    if (old_col != new_col) {
        if (new_fill_h > 0) {
            gfx_fill_rect(inner.x, new_top, inner.w, new_fill_h, new_col);
            redraw_vertical_bar_guides_in_range(w, new_top, inner.y + inner.h - 1);
        }

        if (new_top > inner.y) {
            gfx_fill_rect(inner.x, inner.y, inner.w, new_top - inner.y, COL_BAR_BG);
            redraw_vertical_bar_guides_in_range(w, inner.y, new_top - 1);
        }
    } else {
        if (new_fill_h > old_fill_h) {
            int grow_y = new_top;
            int grow_h = old_top - new_top;

            if (grow_h > 0) {
                gfx_fill_rect(inner.x, grow_y, inner.w, grow_h, new_col);
                redraw_vertical_bar_guides_in_range(w, grow_y, grow_y + grow_h - 1);
            }
        } else if (new_fill_h < old_fill_h) {
            int erase_y = old_top;
            int erase_h = new_top - old_top;

            if (erase_h > 0) {
                gfx_fill_rect(inner.x, erase_y, inner.w, erase_h, COL_BAR_BG);
                redraw_vertical_bar_guides_in_range(w, erase_y, erase_y + erase_h - 1);
            }
        }
    }

    if (old_shown != new_shown || old_col != new_col) {
        Rect old_r = get_vertical_bar_value_rect(w, old_shown);
        Rect new_r = get_vertical_bar_value_rect(w, new_shown);
        Rect dirty = union_rects(old_r, new_r);

        gfx_fill_rect(dirty.x, dirty.y, dirty.w, dirty.h, w->bg_color);
        draw_vertical_bar_value_text(w, new_shown, new_col);
    }
}

// ======================================================
// NUMERIC BLOCK WIDGET
// ======================================================

void draw_numeric_block_static(const NumericBlockWidget *w)
{
    const GFXfont *label_font;

    if (!w) {
        return;
    }

    label_font = (w->label_font) ? w->label_font : &FreeSansBold18pt7b;

    gfx_fill_rect(w->area.x, w->area.y, w->area.w, w->area.h, w->bg_color);

    draw_text_centered_in_rect(
        &w->area,
        w->area.y + 18 + w->label_y_offset,
        w->label,
        label_font,
        w->fg_color
    );
}

void draw_numeric_block_value_full(const NumericBlockWidget *w, int value, uint16_t color)
{
    int label_baseline;
    const GFXfont *label_font;

    if (!w) {
        return;
    }

    label_baseline = w->area.y + 18 + w->label_y_offset;
    label_font = (w->label_font) ? w->label_font : &FreeSansBold18pt7b;

    gfx_fill_rect(w->area.x, w->area.y, w->area.w, w->area.h, w->bg_color);

    draw_text_centered_in_rect(
        &w->area,
        label_baseline,
        w->label,
        label_font,
        w->fg_color
    );

    draw_numeric_block_value_text(w, value, color);
}

void draw_numeric_block_value_delta(const NumericBlockWidget *w,
                                    int old_value, int new_value,
                                    uint16_t color)
{
    Rect old_r;
    Rect new_r;
    Rect dirty;

    if (!w || old_value == new_value) {
        return;
    }

    old_r = get_numeric_block_value_rect(w, old_value);
    new_r = get_numeric_block_value_rect(w, new_value);
    dirty = union_rects(old_r, new_r);

    gfx_fill_rect(dirty.x, dirty.y, dirty.w, dirty.h, w->bg_color);
    draw_numeric_block_value_text(w, new_value, color);
}

static void draw_lap_counter_styled(const NumericBlockWidget *w,
                                    int current_lap, int total_laps,
                                    uint16_t color)
{
    char main_buf[12];
    char small_buf[12];

    int main_w, main_min_y, main_max_y;
    int small_w, small_min_y, small_max_y;

    int main_baseline_y;
    int main_x;
    int small_x;
    int small_baseline_y;

    const GFXfont *main_font = &F1Bold40pt7b;
    const GFXfont *small_font = &F1Bold18pt7b;

    if (!w) {
        return;
    }

    snprintf(main_buf, sizeof(main_buf), "%d", current_lap);
    snprintf(small_buf, sizeof(small_buf), "/%d", total_laps);

    font_text_bounds(main_buf, main_font, &main_w, &main_min_y, &main_max_y);
    font_text_bounds(small_buf, small_font, &small_w, &small_min_y, &small_max_y);

    main_baseline_y = (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset;

    /* center BOTH as a group */
    int total_w = main_w + 6 + small_w;

    main_x = w->area.x + (w->area.w - total_w) / 2;
    small_x = main_x + main_w + 6;

    /* draw big number */
    gfx_draw_text_font(main_x, main_baseline_y, main_buf, main_font, color);

    /* draw small /22 slightly higher */
    small_baseline_y = main_baseline_y - 12;  // <-- vertical lift (tune this)

    gfx_draw_text_font(small_x, small_baseline_y, small_buf, small_font, color);
}

void draw_lap_counter_full(const NumericBlockWidget *w,
                           int current_lap, int total_laps,
                           uint16_t color,
                           int previous_lap_for_dirty_rect)
{
    Rect value_rect;

    (void)previous_lap_for_dirty_rect;

    if (!w) {
        return;
    }

    value_rect = get_lap_counter_styled_rect(w, current_lap, total_laps);

    gfx_fill_rect(value_rect.x, value_rect.y, value_rect.w, value_rect.h, w->bg_color);

    draw_lap_counter_styled(w, current_lap, total_laps, color);
}

void draw_lap_counter_delta(const NumericBlockWidget *w,
                            int old_lap, int new_lap,
                            int total_laps, uint16_t color)
{
    Rect old_r;
    Rect new_r;
    Rect dirty;

    if (!w || old_lap == new_lap) {
        return;
    }

    old_r = get_lap_counter_styled_rect(w, old_lap, total_laps);
    new_r = get_lap_counter_styled_rect(w, new_lap, total_laps);
    dirty = union_rects(old_r, new_r);

    gfx_fill_rect(dirty.x, dirty.y, dirty.w, dirty.h, w->bg_color);

    draw_lap_counter_styled(w, new_lap, total_laps, color);
}

// ======================================================
// GRAPH INTERNAL HELPERS
// ======================================================

static int graph_oldest_index(const DualTraceGraphWidget *g)
{
    if (g->count < g->capacity) {
        return 0;
    }
    return g->head;
}

static int graph_buffer_index(const DualTraceGraphWidget *g, int visible_pos)
{
    int oldest = graph_oldest_index(g);
    return (oldest + visible_pos) % g->capacity;
}

static int graph_value_to_y(const DualTraceGraphWidget *g, int value)
{
    int inner_top = g->area.y + 2;
    int inner_h = g->area.h - 4;

    value = clampi(value, 0, 100);

    return inner_top + ((100 - value) * (inner_h - 1)) / 100;
}

// ======================================================
// GRAPH WIDGET
// ======================================================

void graph_init(DualTraceGraphWidget *g, Rect area)
{
    if (!g) {
        return;
    }

    memset(g, 0, sizeof(*g));

    g->area = area;
    g->capacity = GRAPH_MAX_SAMPLES;

    if (g->capacity < 1) {
        g->capacity = 1;
    }

    g->bg_color = COL_BG;
    g->grid_color = COL_GRID;
    g->throttle_color = RGB565(0, 255, 0);
    g->brake_color = RGB565(255, 0, 0);
    g->frame_color = COL_TEXT;
    g->line_thickness = 8;
}

void graph_clear(DualTraceGraphWidget *g)
{
    if (!g) {
        return;
    }

    g->head = 0;
    g->count = 0;
    memset(g->throttle, 0, sizeof(g->throttle));
    memset(g->brake, 0, sizeof(g->brake));
}

void graph_push_sample(DualTraceGraphWidget *g, int throttle, int brake)
{
    int prev_idx;
    int prev_throttle;
    int prev_brake;

    if (!g || g->capacity <= 0) {
        return;
    }

    throttle = clampi(throttle, 0, 100);
    brake = clampi(brake, 0, 100);

    if (g->count > 0) {
        prev_idx = (g->head - 1 + g->capacity) % g->capacity;
        prev_throttle = g->throttle[prev_idx];
        prev_brake = g->brake[prev_idx];

        //SMOOTHING
        /*
        throttle = (prev_throttle + throttle) / 2;
        brake = (prev_brake + brake) / 2;
        */

    }

    g->throttle[g->head] = (uint8_t)throttle;
    g->brake[g->head] = (uint8_t)brake;

    g->head = (g->head + 1) % g->capacity;

    if (g->count < g->capacity) {
        g->count++;
    }
}

void draw_graph_static(const DualTraceGraphWidget *g)
{
    int i;
    int inner_x;
    int inner_y;
    int inner_w;
    int inner_h;

    if (!g) {
        return;
    }

    inner_x = g->area.x + 1;
    inner_y = g->area.y + 1;
    inner_w = g->area.w - 2;
    inner_h = g->area.h - 2;

    gfx_fill_rect(g->area.x, g->area.y, g->area.w, g->area.h, g->bg_color);
    draw_rect_outline(g->area.x, g->area.y, g->area.w, g->area.h, g->frame_color);

    for (i = 0; i <= 4; i++) {
        int gy = inner_y + (i * inner_h) / 4;
        gfx_draw_line(inner_x, gy, inner_x + inner_w - 1, gy, g->grid_color);
    }
}

void draw_graph_full(const DualTraceGraphWidget *g)
{
    int inner_x;
    int inner_y;
    int inner_w;
    int inner_h;
    int visible;
    int i;

    if (!g) {
        return;
    }

    inner_x = g->area.x + 1;
    inner_y = g->area.y + 1;
    inner_w = g->area.w - 2;
    inner_h = g->area.h - 2;

    /* Fixed scrolling window from startup */
    visible = GRAPH_VISIBLE_SAMPLES;

    if (visible < 2) {
        visible = 2;
    }
    if (visible > g->capacity) {
        visible = g->capacity;
    }

    graph_fb_clear(g->bg_color, inner_w, inner_h);

    for (i = 0; i <= 4; i++) {
        int gy = (i * (inner_h - 1)) / 4;
        graph_fb_draw_line(0, gy, inner_w - 1, gy,
                           inner_w, inner_h, g->grid_color);
    }

    /* Window always spans [g->count - visible, ..., g->count - 1]
       Missing samples before startup are drawn as 0 */
    for (i = 1; i < visible; i++) {
        int sample_pos0 = g->count - visible + (i - 1);
        int sample_pos1 = g->count - visible + i;

        int t0 = 0, t1 = 0;
        int b0 = 0, b1 = 0;

        if (sample_pos0 >= 0) {
            int idx0 = graph_buffer_index(g, sample_pos0);
            t0 = g->throttle[idx0];
            b0 = g->brake[idx0];
        }

        if (sample_pos1 >= 0) {
            int idx1 = graph_buffer_index(g, sample_pos1);
            t1 = g->throttle[idx1];
            b1 = g->brake[idx1];
        }

        {
            int x0 = ((i - 1) * (inner_w - 1)) / (visible - 1);
            int x1 = (i * (inner_w - 1)) / (visible - 1);

            int y0_throttle = ((100 - t0) * (inner_h - 1)) / 100;
            int y1_throttle = ((100 - t1) * (inner_h - 1)) / 100;

            int y0_brake = ((100 - b0) * (inner_h - 1)) / 100;
            int y1_brake = ((100 - b1) * (inner_h - 1)) / 100;

            graph_fb_draw_thick_line(x0, y0_brake, x1, y1_brake,
                                     inner_w, inner_h,
                                     g->brake_color, g->line_thickness);

            graph_fb_draw_thick_line(x0, y0_throttle, x1, y1_throttle,
                                     inner_w, inner_h,
                                     g->throttle_color, g->line_thickness);
        }
    }

    draw_rect_outline(g->area.x, g->area.y, g->area.w, g->area.h, g->frame_color);
    graph_fb_blit_to_lcd(inner_x, inner_y, inner_w, inner_h);
}

// ======================================================
// LOGOS / TOUCH AREAS
// ======================================================

void draw_UGR_logo(void)
{
    gfx_blit565_key(
        SMALL_LOGO_X,
        SMALL_LOGO_Y,
        UGR_LOGO_W,
        UGR_LOGO_H,
        (const uint16_t *)ugr_logo,
        RGB565(0, 0, 0)
    );
}

void draw_big_UGR_logo(void)
{
    draw_raw565_from_sd_chunked(
        "0:/logo.bin",
        BIG_LOGO_X,
        BIG_LOGO_Y,
        BIG_UGR_LOGO_W,
        BIG_UGR_LOGO_H
    );
}

UI_Area dash_get_area(DashAreaId id)
{
    UI_Area a = {0, 0, 0, 0};

    switch (id) {
        case DASH_AREA_SMALL_LOGO:
            a.x1 = SMALL_LOGO_X;
            a.y1 = SMALL_LOGO_Y;
            a.x2 = SMALL_LOGO_X + UGR_LOGO_W - 1;
            a.y2 = SMALL_LOGO_Y + UGR_LOGO_H - 1;
            break;

        case DASH_AREA_BIG_LOGO:
            a.x1 = BIG_LOGO_X;
            a.y1 = BIG_LOGO_Y;
            a.x2 = BIG_LOGO_X + BIG_UGR_LOGO_W - 1;
            a.y2 = BIG_LOGO_Y + BIG_UGR_LOGO_H - 1;
            break;

        default:
            break;
    }

    return a;
}

// ======================================================
// SD CARD IMAGE DRAWING
// ======================================================

uint8_t draw_raw565_from_sd_chunked(const char *path,
                                    uint16_t x,
                                    uint16_t y,
                                    uint16_t w,
                                    uint16_t h)
{
    UINT br;

    if (w == 0 || h == 0 || w > DASH_W) {
        SD_Debug("BAD SIZE", RGB565(255, 0, 0));
        return 0;
    }

    {
        uint32_t total_start = HAL_GetTick();
        uint32_t read_time = 0;
        uint32_t lcd_time = 0;
        uint16_t row = 0;

        if (f_open(&USERFile, path, FA_READ) != FR_OK) {
            SD_Debug("OPEN FAIL", RGB565(255, 0, 0));
            return 0;
        }

        while (row < h) {
            uint16_t rows_this_chunk = LOGO_CHUNK_ROWS;

            if ((h - row) < rows_this_chunk) {
                rows_this_chunk = (uint16_t)(h - row);
            }

            {
                UINT bytes_to_read = (UINT)(w * rows_this_chunk * 2);
                uint32_t t0 = HAL_GetTick();
                FRESULT fr = f_read(&USERFile, logo_chunk, bytes_to_read, &br);
                uint32_t t1 = HAL_GetTick();

                read_time += (t1 - t0);

                if (fr != FR_OK || br != bytes_to_read) {
                    f_close(&USERFile);
                    SD_Debug("READ FAIL", RGB565(255, 0, 0));
                    return 0;
                }
            }

            {
                uint32_t t0 = HAL_GetTick();

                SSD1963_SetWindow(
                    x,
                    y + row,
                    x + w - 1,
                    y + row + rows_this_chunk - 1
                );

                SSD1963_WritePixels(logo_chunk, (uint32_t)w * rows_this_chunk);

                lcd_time += (HAL_GetTick() - t0);
            }

            row += rows_this_chunk;
        }

        f_close(&USERFile);

        (void)total_start;
        (void)read_time;
        (void)lcd_time;
    }

    return 1;
}

void SD_Debug(const char *msg, uint16_t color)
{
    SSD1963_Fill(RGB565(0, 0, 0));
    gfx_draw_text_font(20, 40, msg, &FreeSansBold24pt7b, color);
}