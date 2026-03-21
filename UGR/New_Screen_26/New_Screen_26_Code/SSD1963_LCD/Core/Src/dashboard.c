#include "dashboard.h"

#include "gfx.h"
#include "ssd1963.h"
#include "logos/ugr_logo.h"
#include "logos/icons.h"
#include "gfx_text.h"

// SD CARD
#include "sd_card.h"
#include "fatfs.h"
#include "ff.h"

// Fonts
#include "FreeSansBold9pt7b.h"
#include "FreeSansBold12pt7b.h"
#include "FreeSansBold18pt7b.h"
#include "FreeSansBold24pt7b.h"

#include "fonts/F1Bold24pt7b.h"
#include "fonts/F1Bold28pt7b.h"
#include "fonts/F1Bold32pt7b.h"
#include "fonts/F1Bold36pt7b.h"
#include "fonts/F1Bold40pt7b.h"

#include <string.h>
#include <math.h>
#include <stdio.h>

// ======================================================
// SCREEN SIZE
// ======================================================

#define W 800
#define H 480

// ======================================================
// LOGO CHUNK BUFFER
// ======================================================

#define LOGO_CHUNK_ROWS 1
static uint16_t logo_chunk[BIG_UGR_LOGO_W * LOGO_CHUNK_ROWS];

// ======================================================
// TOUCH AREAS (LOGO BUTTONS)
// ======================================================

#define SMALL_LOGO_X      729
#define SMALL_LOGO_Y      10

#define BIG_LOGO_X        50
#define BIG_LOGO_Y        85

#define LOGO_BACK_BTN_X   300
#define LOGO_BACK_BTN_Y   410
#define LOGO_BACK_BTN_W   200
#define LOGO_BACK_BTN_H   50

// ======================================================
// COLOR THRESHOLDS
// ======================================================

#define BATT_RED_MAX      20
#define BATT_ORANGE_MAX   50

#define TEMP_GREEN_MAX    40
#define TEMP_ORANGE_MAX   50

// ======================================================
// UI COLOR PALETTE
// ======================================================

#define COL_BG     RGB565(0, 0, 0)
#define COL_PANEL  RGB565(18, 18, 18)
#define COL_TEXT   RGB565(230, 230, 230)
#define COL_DIM    RGB565(120, 120, 120)
#define COL_ACC    RGB565(0, 200, 255)
#define COL_WARN   RGB565(255, 80, 0)
#define COL_GOOD   RGB565(0, 220, 0)
#define COL_GRID   RGB565(70, 70, 70)
#define COL_BAR_BG RGB565(8, 8, 8)

// ======================================================
// LAP DISPLAY DEFAULTS
// ======================================================

#define LAP_TOTAL_DEFAULT         22
#define RIGHT_VALUE_BASELINE_GAP  58

// ======================================================
// INTERNAL GEOMETRY TYPES
// ======================================================

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rect;

typedef struct {
    Rect area;
    Rect bar_rect;
    const char *title;

    int min_value;
    int max_value;

    uint16_t bg_color;
    uint16_t frame_color;
    uint16_t text_color;

    uint16_t (*color_fn)(int value);
    void (*unit_draw_fn)(int x, int y, int s, uint16_t col);

    int clamp_value_text;   // 1 = clamp displayed text, 0 = show raw value
} VerticalBarWidget;

typedef struct {
    Rect area;
    const char *label;

    int digits;
    int seg_scale;
    int digit_gap;
    int digit_outline;

    int label_y_offset;
    int value_y_offset;

    uint16_t fg_color;
    uint16_t bg_color;

    const GFXfont *label_font;
    const GFXfont *value_font;
} NumericBlockWidget;

typedef struct {
    int screen_w;
    int screen_h;

    int top_margin;
    int section_w;
    int section_h;
    int section_gap;
    int side_margin;

    int bar_w;
    int bar_h;
    int bar_top_offset;

    int right_block_top_gap;
    int right_block_h;
    int right_block_gap;
} DashboardTheme;

typedef struct {
    int screen_w;
    int screen_h;

    Rect left_section;
    Rect mid_section;
    Rect right_section;

    VerticalBarWidget soc_bar;
    VerticalBarWidget invtemp_bar;

    NumericBlockWidget lap_block;
    NumericBlockWidget temp_block;
    NumericBlockWidget speed_block;
} DashboardLayout;

// ======================================================
// INTERNAL STATE
// ======================================================

static Dashboard prev;
static int inited = 0;

static DashboardTheme g_theme = {
    .screen_w = W,
    .screen_h = H,

    .top_margin = 12,
    .section_w = 200,
    .section_h = 456,
    .section_gap = 50,
    .side_margin = 50,

    .bar_w = 150,
    .bar_h = 370,
    .bar_top_offset = 42,

    .right_block_top_gap = 40,
    .right_block_h = 144,
    .right_block_gap = 20
};

static DashboardLayout g_layout;

// ======================================================
// INTERNAL FUNCTION PROTOTYPES
// ======================================================

// Generic helpers
static void u32_to_str(uint32_t val, char *buf);
static void swap_int(int *a, int *b);
static inline int clampi(int v, int lo, int hi);
static Rect make_rect(int x, int y, int w, int h);
static Rect union_rects(Rect a, Rect b);
static void draw_rect_outline(int x, int y, int w, int h, uint16_t col);

static int font_text_width(const char *text, const GFXfont *font);
static void font_text_bounds(const char *text, const GFXfont *font,
                             int *out_w, int *out_min_y, int *out_max_y);

static void draw_text_centered_in_rect(const Rect *r, int y, const char *text,
                                       const GFXfont *font, uint16_t color);
static void draw_text_right_aligned(int right_x, int y, const char *text,
                                    const GFXfont *font, uint16_t color);
static void draw_text_right_aligned_centered_y(int right_x, int center_y,
                                               const char *text,
                                               const GFXfont *font,
                                               uint16_t color);

// Layout
static void dash_build_layout(DashboardLayout *L, const DashboardTheme *T);

// Colors
static uint16_t battery_color_for_percent(int percent);
static uint16_t temp_color_for_value(int temp);

// Unit glyphs
static void draw_percent(int x, int y, int s, uint16_t col);
static void draw_C(int x, int y, int s, uint16_t col);

// 7-segment helpers
static void seg_h(int x, int y, int len, int t, uint16_t col);
static void seg_v(int x, int y, int len, int t, uint16_t col);
static void draw_digit7(int x, int y, int s, int digit,
                        uint16_t fg, uint16_t bg, int outline);
static void draw_int7(int x, int y, int s, int gap, int value, int digits,
                      uint16_t fg, uint16_t bg, int outline);
static int digit7_total_width(int digits, int s, int gap);
static int digit7_total_height(int s);
static int digit7_thickness(int s);
static int digit7_digit_width(int s);
static int digit7_digit_height(int s);

// Numeric/text geometry
static const GFXfont *numeric_block_value_font(const NumericBlockWidget *w);
static Rect get_numeric_block_value_rect(const NumericBlockWidget *w, int value);
static Rect get_lap_value_rect(const NumericBlockWidget *w,
                               int current_lap, int total_laps);
static void draw_numeric_block_value_text(const NumericBlockWidget *w,
                                          int value, uint16_t color);
static void draw_lap_counter_delta(const NumericBlockWidget *w,
                                   int current_lap, int total_laps,
                                   uint16_t color);
static void draw_lap_counter_full(const NumericBlockWidget *w,
                                  int current_lap, int total_laps,
                                  uint16_t color);

// Vertical bar helpers
static Rect get_vertical_bar_inner_rect(const VerticalBarWidget *w);
static Rect get_vertical_bar_value_rect(const VerticalBarWidget *w, int value);
static void draw_vertical_bar_guides(const VerticalBarWidget *w);
static void redraw_vertical_bar_guides_in_range(const VerticalBarWidget *w,
                                                int y0, int y1);
static void draw_vertical_bar_value_text(const VerticalBarWidget *w,
                                         int value, uint16_t fill_col);

// Widgets
static int map_value_to_fill_height(int value, int min_value,
                                    int max_value, int pixel_height);
static void draw_vertical_bar_static(const VerticalBarWidget *w);
static void draw_vertical_bar_value_full(const VerticalBarWidget *w, int value);
static void draw_vertical_bar_value_delta(const VerticalBarWidget *w,
                                          int old_value, int new_value);

static void draw_numeric_block_static(const NumericBlockWidget *w);
static void draw_numeric_block_value_full(const NumericBlockWidget *w,
                                          int value, uint16_t color);
static void draw_numeric_block_value_delta(const NumericBlockWidget *w,
                                           int old_value, int new_value,
                                           uint16_t color);

// Dashboard draw
static void dash_draw_static(const DashboardLayout *L);
static void dash_draw_values_full(const DashboardLayout *L,
                                  int battery_charge,
                                  int cell_temperature,
                                  int speed,
                                  int lap);

// SD image drawing
uint8_t draw_raw565_from_sd_chunked(const char *path,
                                    uint16_t x,
                                    uint16_t y,
                                    uint16_t w,
                                    uint16_t h);

// ======================================================
// GENERIC HELPERS
// ======================================================

static Rect make_rect(int x, int y, int w, int h)
{
    Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

static Rect union_rects(Rect a, Rect b)
{
    int x1 = (a.x < b.x) ? a.x : b.x;
    int y1 = (a.y < b.y) ? a.y : b.y;
    int x2 = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
    int y2 = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

    return make_rect(x1, y1, x2 - x1, y2 - y1);
}

static inline int clampi(int v, int lo, int hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void swap_int(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

static void u32_to_str(uint32_t val, char *buf)
{
    char tmp[12];
    int i = 0;
    int j = 0;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        buf[j++] = tmp[--i];
    }

    buf[j] = '\0';
}

static void draw_rect_outline(int x, int y, int w, int h, uint16_t col)
{
    gfx_draw_line(x,         y,         x + w - 1, y,         col);
    gfx_draw_line(x,         y + h - 1, x + w - 1, y + h - 1, col);
    gfx_draw_line(x,         y,         x,         y + h - 1, col);
    gfx_draw_line(x + w - 1, y,         x + w - 1, y + h - 1, col);
}

static int font_text_width(const char *text, const GFXfont *font)
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

static void font_text_bounds(const char *text, const GFXfont *font,
                             int *out_w, int *out_min_y, int *out_max_y)
{
    int x = 0;
    int min_y = 32767;
    int max_y = -32768;

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

    if (min_y == 32767) {
        min_y = 0;
        max_y = 0;
    }

    *out_w = x;
    *out_min_y = min_y;
    *out_max_y = max_y;
}

static void draw_text_centered_in_rect(const Rect *r, int y, const char *text,
                                       const GFXfont *font, uint16_t color)
{
    int tw = font_text_width(text, font);
    int x = r->x + (r->w - tw) / 2;
    gfx_draw_text_font(x, y, text, font, color);
}

static void draw_text_right_aligned(int right_x, int y, const char *text,
                                    const GFXfont *font, uint16_t color)
{
    int tw = font_text_width(text, font);
    gfx_draw_text_font(right_x - tw, y, text, font, color);
}

static void draw_text_right_aligned_centered_y(int right_x, int center_y,
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
// NUMERIC/TEXT GEOMETRY HELPERS
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

static Rect get_lap_value_rect(const NumericBlockWidget *w,
                               int current_lap, int total_laps)
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

static void draw_numeric_block_value_text(const NumericBlockWidget *w,
                                          int value, uint16_t color)
{
    char buf[16];
    Rect value_rect = get_numeric_block_value_rect(w, value);
    const GFXfont *font = numeric_block_value_font(w);

    gfx_fill_rect(value_rect.x, value_rect.y, value_rect.w, value_rect.h,
                  w->bg_color);

    snprintf(buf, sizeof(buf), "%d", value);

    draw_text_centered_in_rect(
        &w->area,
        (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset,
        buf,
        font,
        color
    );
}

static void draw_lap_counter_delta(const NumericBlockWidget *w,
                                   int current_lap, int total_laps,
                                   uint16_t color)
{
    char old_buf[20];
    char new_buf[20];
    Rect old_r;
    Rect new_r;
    Rect dirty;
    const GFXfont *font = numeric_block_value_font(w);

    snprintf(old_buf, sizeof(old_buf), "%d", prev.lap, total_laps);
    snprintf(new_buf, sizeof(new_buf), "%d", current_lap, total_laps);

    old_r = get_lap_value_rect(w, prev.lap, total_laps);
    new_r = get_lap_value_rect(w, current_lap, total_laps);
    dirty = union_rects(old_r, new_r);

    gfx_fill_rect(dirty.x, dirty.y, dirty.w, dirty.h, w->bg_color);

    draw_text_centered_in_rect(
        &w->area,
        (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset,
        new_buf,
        font,
        color
    );
}

// ======================================================
// VERTICAL BAR GEOMETRY/TEXT HELPERS
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

static void redraw_vertical_bar_guides_in_range(const VerticalBarWidget *w,
                                                int y0, int y1)
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
            gfx_draw_line(inner_x, gy, inner_x + inner_w - 1, gy, COL_GRID);
        }
    }
}

static void draw_vertical_bar_value_text(const VerticalBarWidget *w,
                                         int value, uint16_t fill_col)
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

    gfx_fill_rect(value_rect.x, value_rect.y, value_rect.w, value_rect.h,
                  w->bg_color);

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

        gfx_draw_line(inner_x, gy, inner_x + inner_w - 1, gy, COL_GRID);

        draw_text_right_aligned_centered_y(
            b->x + b->w + 55,
            label_cy,
            buf,
            &FreeSansBold18pt7b,
            w->text_color
        );
    }
}

// ======================================================
// LAYOUT
// ======================================================

static void dash_build_layout(DashboardLayout *L, const DashboardTheme *T)
{
    int x0 = T->side_margin;
    int x1 = x0 + T->section_w + T->section_gap;
    int x2 = x1 + T->section_w + T->section_gap;

    int right_blocks_y0 = T->top_margin + T->right_block_top_gap;
    int right_block_h =
        (T->section_h - T->right_block_top_gap - 2 * T->right_block_gap) / 3;

    L->screen_w = T->screen_w;
    L->screen_h = T->screen_h;

    L->left_section  = make_rect(x0, T->top_margin, T->section_w, T->section_h);
    L->mid_section   = make_rect(x1, T->top_margin, T->section_w, T->section_h);
    L->right_section = make_rect(x2, T->top_margin, T->section_w, T->section_h);

    // Left: SoC
    L->soc_bar.area = L->left_section;
    L->soc_bar.bar_rect = make_rect(
        L->left_section.x + (L->left_section.w - T->bar_w) / 2,
        L->left_section.y + T->bar_top_offset,
        T->bar_w,
        T->bar_h
    );
    L->soc_bar.title = "SoC";
    L->soc_bar.min_value = 0;
    L->soc_bar.max_value = 100;
    L->soc_bar.clamp_value_text = 1;
    L->soc_bar.bg_color = COL_BG;
    L->soc_bar.frame_color = COL_TEXT;
    L->soc_bar.text_color = COL_TEXT;
    L->soc_bar.color_fn = battery_color_for_percent;
    L->soc_bar.unit_draw_fn = draw_percent;

    // Middle: inverter temp (stored in cell_temperature)
    L->invtemp_bar.area = L->mid_section;
    L->invtemp_bar.bar_rect = make_rect(
        L->mid_section.x + (L->mid_section.w - T->bar_w) / 2,
        L->mid_section.y + T->bar_top_offset,
        T->bar_w,
        T->bar_h
    );
    L->invtemp_bar.title = "TEMP";
    L->invtemp_bar.min_value = 20;
    L->invtemp_bar.max_value = 60;
    L->invtemp_bar.clamp_value_text = 0;
    L->invtemp_bar.bg_color = COL_BG;
    L->invtemp_bar.frame_color = COL_TEXT;
    L->invtemp_bar.text_color = COL_TEXT;
    L->invtemp_bar.color_fn = temp_color_for_value;
    L->invtemp_bar.unit_draw_fn = draw_C;

    // Right blocks
    L->lap_block.area = make_rect(
        L->right_section.x,
        right_blocks_y0,
        L->right_section.w,
        right_block_h
    );
    L->lap_block.label = "LAP";
    L->lap_block.digits = 2;
    L->lap_block.seg_scale = 2;
    L->lap_block.label_y_offset = 2;
    L->lap_block.value_y_offset = 10;
    L->lap_block.digit_gap = 0;
    L->lap_block.digit_outline = 0;
    L->lap_block.fg_color = COL_TEXT;
    L->lap_block.bg_color = COL_BG;
    L->lap_block.label_font = &FreeSansBold18pt7b;
    L->lap_block.value_font = &F1Bold40pt7b;

    L->temp_block.area = make_rect(
        L->right_section.x,
        right_blocks_y0 + right_block_h + T->right_block_gap,
        L->right_section.w,
        right_block_h
    );
    L->temp_block.label = "TEMP";
    L->temp_block.digits = 3;
    L->temp_block.seg_scale = 4;
    L->temp_block.label_y_offset = -4;
    L->temp_block.value_y_offset = 10;
    L->temp_block.digit_gap = 10;
    L->temp_block.digit_outline = 0;
    L->temp_block.fg_color = COL_TEXT;
    L->temp_block.bg_color = COL_BG;
    L->temp_block.label_font = &FreeSansBold18pt7b;
    L->temp_block.value_font = &F1Bold40pt7b;

    L->speed_block.area = make_rect(
        L->right_section.x,
        right_blocks_y0 + 2 * (right_block_h + T->right_block_gap),
        L->right_section.w,
        right_block_h
    );
    L->speed_block.label = "SPEED";
    L->speed_block.digits = 3;
    L->speed_block.seg_scale = 4;
    L->speed_block.label_y_offset = 0;
    L->speed_block.value_y_offset = 10;
    L->speed_block.digit_gap = 10;
    L->speed_block.digit_outline = 0;
    L->speed_block.fg_color = COL_TEXT;
    L->speed_block.bg_color = COL_BG;
    L->speed_block.label_font = &FreeSansBold18pt7b;
    L->speed_block.value_font = &F1Bold40pt7b;
}

// ======================================================
// COLOR HELPERS
// ======================================================

static uint16_t battery_color_for_percent(int percent)
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

static uint16_t temp_color_for_value(int temp)
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

static void draw_percent(int x, int y, int s, uint16_t col)
{
    int r = s;
    gfx_fill_rect(x,         y,         r, r, col);
    gfx_fill_rect(x + 4 * s, y + 4 * s, r, r, col);
    gfx_draw_line(x, y + 5 * s, x + 5 * s, y, col);
}

static void draw_C(int x, int y, int s, uint16_t col)
{
    gfx_fill_rect(x,         y,         4 * s, s,     col);
    gfx_fill_rect(x,         y,         s,     6 * s, col);
    gfx_fill_rect(x,         y + 5 * s, 4 * s, s,     col);
}

// ======================================================
// 7-SEGMENT DRAWING HELPERS
// ======================================================

static void seg_h(int x, int y, int len, int t, uint16_t col)
{
    gfx_fill_rect(x, y, len, t, col);
}

static void seg_v(int x, int y, int len, int t, uint16_t col)
{
    gfx_fill_rect(x, y, t, len, col);
}

static int digit7_thickness(int s)
{
    return s + 2;
}

static int digit7_digit_width(int s)
{
    const int t = digit7_thickness(s);
    const int L = 6 * s;
    return L + 2 * t;
}

static int digit7_digit_height(int s)
{
    const int t = digit7_thickness(s);
    const int Hh = 10 * s;
    return (2 * Hh) + 3 * t;
}

static void draw_digit7(int x, int y, int s, int digit,
                        uint16_t fg, uint16_t bg, int outline)
{
    const int t = digit7_thickness(s);
    const int L = 6 * s;
    const int Hh = 10 * s;
    const int w = digit7_digit_width(s);
    const int h = digit7_digit_height(s);

    static const uint8_t map[10] = {
        0b0111111,
        0b0000110,
        0b1011011,
        0b1001111,
        0b1100110,
        0b1101101,
        0b1111101,
        0b0000111,
        0b1111111,
        0b1101111
    };

    uint8_t m = (digit >= 0 && digit <= 9) ? map[digit] : 0;

    gfx_fill_rect(x, y, w, h, bg);

    if (outline) {
        draw_rect_outline(x, y, w, h, COL_TEXT);
    }

    if (m & (1 << 0)) seg_h(x + t,     y,               L,  t, fg);
    if (m & (1 << 1)) seg_v(x + t + L, y + t,           Hh, t, fg);
    if (m & (1 << 2)) seg_v(x + t + L, y + 2 * t + Hh,  Hh, t, fg);
    if (m & (1 << 3)) seg_h(x + t,     y + 2 * t + 2 * Hh, L, t, fg);
    if (m & (1 << 4)) seg_v(x,         y + 2 * t + Hh,  Hh, t, fg);
    if (m & (1 << 5)) seg_v(x,         y + t,           Hh, t, fg);
    if (m & (1 << 6)) seg_h(x + t,     y + t + Hh,      L,  t, fg);
}

static void draw_int7(int x, int y, int s, int gap, int value, int digits,
                      uint16_t fg, uint16_t bg, int outline)
{
    int v = value;
    int i;

    if (v < 0) {
        v = 0;
    }

    {
        int maxv = 1;
        for (i = 0; i < digits; i++) {
            maxv *= 10;
        }
        if (v >= maxv) {
            v = maxv - 1;
        }
    }

    {
        const int w = digit7_digit_width(s);

        for (i = digits - 1; i >= 0; i--) {
            int d = v % 10;
            v /= 10;
            draw_digit7(x + i * (w + gap), y, s, d, fg, bg, outline);
        }
    }
}

static int digit7_total_width(int digits, int s, int gap)
{
    const int w = digit7_digit_width(s);
    return digits * w + (digits - 1) * gap;
}

static int digit7_total_height(int s)
{
    return digit7_digit_height(s);
}

// ======================================================
// VALUE/BAR HELPERS
// ======================================================

static int map_value_to_fill_height(int value, int min_value,
                                    int max_value, int pixel_height)
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

static void draw_vertical_bar_static(const VerticalBarWidget *w)
{
    const Rect *a = &w->area;
    const Rect *b = &w->bar_rect;

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

static void draw_vertical_bar_value_full(const VerticalBarWidget *w, int value)
{
    Rect inner = get_vertical_bar_inner_rect(w);
    int clamped = clampi(value, w->min_value, w->max_value);
    int shown_value = w->clamp_value_text ? clamped : value;
    int fill_h = map_value_to_fill_height(clamped, w->min_value,
                                          w->max_value, inner.h);
    uint16_t fill_col = w->color_fn ? w->color_fn(value) : COL_ACC;

    gfx_fill_rect(inner.x, inner.y, inner.w, inner.h, COL_BAR_BG);

    if (fill_h > 0) {
        int fy = inner.y + inner.h - fill_h;
        gfx_fill_rect(inner.x, fy, inner.w, fill_h, fill_col);
    }

    draw_vertical_bar_guides(w);
    draw_vertical_bar_value_text(w, shown_value, fill_col);
}

static void draw_vertical_bar_value_delta(const VerticalBarWidget *w,
                                          int old_value, int new_value)
{
    Rect inner = get_vertical_bar_inner_rect(w);

    int old_clamped = clampi(old_value, w->min_value, w->max_value);
    int new_clamped = clampi(new_value, w->min_value, w->max_value);

    int old_shown = w->clamp_value_text ? old_clamped : old_value;
    int new_shown = w->clamp_value_text ? new_clamped : new_value;

    int old_fill_h = map_value_to_fill_height(old_clamped, w->min_value,
                                              w->max_value, inner.h);
    int new_fill_h = map_value_to_fill_height(new_clamped, w->min_value,
                                              w->max_value, inner.h);

    uint16_t old_col = w->color_fn ? w->color_fn(old_value) : COL_ACC;
    uint16_t new_col = w->color_fn ? w->color_fn(new_value) : COL_ACC;

    int old_top = inner.y + inner.h - old_fill_h;
    int new_top = inner.y + inner.h - new_fill_h;

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

static void draw_numeric_block_static(const NumericBlockWidget *w)
{
    const GFXfont *label_font =
        (w->label_font) ? w->label_font : &FreeSansBold18pt7b;

    gfx_fill_rect(w->area.x, w->area.y, w->area.w, w->area.h, w->bg_color);

    draw_text_centered_in_rect(
        &w->area,
        w->area.y + 18 + w->label_y_offset,
        w->label,
        label_font,
        w->fg_color
    );
}

static void draw_numeric_block_value_full(const NumericBlockWidget *w,
                                          int value, uint16_t color)
{
    int label_baseline = w->area.y + 18 + w->label_y_offset;
    const GFXfont *label_font =
        (w->label_font) ? w->label_font : &FreeSansBold18pt7b;

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

static void draw_numeric_block_value_delta(const NumericBlockWidget *w,
                                           int old_value, int new_value,
                                           uint16_t color)
{
    Rect old_r;
    Rect new_r;
    Rect dirty;

    if (old_value == new_value) {
        return;
    }

    old_r = get_numeric_block_value_rect(w, old_value);
    new_r = get_numeric_block_value_rect(w, new_value);
    dirty = union_rects(old_r, new_r);

    gfx_fill_rect(dirty.x, dirty.y, dirty.w, dirty.h, w->bg_color);
    draw_numeric_block_value_text(w, new_value, color);
}

// ======================================================
// LAP DISPLAY
// ======================================================

static void draw_lap_counter_full(const NumericBlockWidget *w,
                                  int current_lap, int total_laps,
                                  uint16_t color)
{
    char buf[20];
    Rect value_rect = get_lap_value_rect(w, current_lap, total_laps);
    const GFXfont *font = numeric_block_value_font(w);

    gfx_fill_rect(value_rect.x, value_rect.y, value_rect.w, value_rect.h,
                  w->bg_color);

    snprintf(buf, sizeof(buf), "%d", current_lap, total_laps);

    draw_text_centered_in_rect(
        &w->area,
        (w->area.y + 30) + RIGHT_VALUE_BASELINE_GAP + w->value_y_offset,
        buf,
        font,
        color
    );
}

// ======================================================
// DASHBOARD DRAW HELPERS
// ======================================================

static void dash_draw_static(const DashboardLayout *L)
{
    SSD1963_Fill(COL_BG);

    draw_vertical_bar_static(&L->soc_bar);
    draw_vertical_bar_static(&L->invtemp_bar);

    draw_numeric_block_static(&L->lap_block);
    draw_numeric_block_static(&L->temp_block);
    draw_numeric_block_static(&L->speed_block);

    // draw_UGR_logo();
}

static void dash_draw_values_full(const DashboardLayout *L,
                                  int battery_charge,
                                  int cell_temperature,
                                  int speed,
                                  int lap)
{
    draw_vertical_bar_value_full(&L->soc_bar, battery_charge);
    draw_vertical_bar_value_full(&L->invtemp_bar, cell_temperature);

    draw_lap_counter_full(
        &L->lap_block,
        lap,
        LAP_TOTAL_DEFAULT,
        COL_ACC
    );

    draw_numeric_block_value_full(
        &L->temp_block,
        cell_temperature,
        temp_color_for_value(cell_temperature)
    );

    draw_numeric_block_value_full(
        &L->speed_block,
        speed,
        COL_TEXT
    );
}

// ======================================================
// DASHBOARD PUBLIC API
// ======================================================

void dash_init(double initial_battery_charge,
               double initial_cell_temperature,
               double initial_water_temperature,
               int initial_speed)
{
    (void)initial_water_temperature;

    dash_build_layout(&g_layout, &g_theme);

    memset(&prev, 0, sizeof(prev));
    prev.battery_charge = (uint8_t)clampi((int)initial_battery_charge, 0, 100);
    prev.cell_temperature = (uint8_t)clampi((int)initial_cell_temperature, 0, 150);
    prev.water_temperature = 0;
    prev.speed = (uint16_t)clampi(initial_speed, 0, 999);
    prev.lap = 0;

    dash_draw_static(&g_layout);
    dash_draw_values_full(
        &g_layout,
        prev.battery_charge,
        prev.cell_temperature,
        prev.speed,
        prev.lap
    );

    inited = 1;
}

void dash_update(const Dashboard *d)
{
    if (!d) {
        return;
    }

    if (!inited) {
        dash_init(d->battery_charge,
                  d->cell_temperature,
                  d->water_temperature,
                  d->speed);

        prev.lap = d->lap;
        draw_lap_counter_full(&g_layout.lap_block, prev.lap,
                              LAP_TOTAL_DEFAULT, COL_ACC);
        return;
    }

    if ((int)d->battery_charge != (int)prev.battery_charge) {
        draw_vertical_bar_value_delta(
            &g_layout.soc_bar,
            (int)prev.battery_charge,
            (int)d->battery_charge
        );
        prev.battery_charge = d->battery_charge;
    }

    if ((int)d->cell_temperature != (int)prev.cell_temperature) {
        int old_temp = (int)prev.cell_temperature;
        int new_temp = (int)d->cell_temperature;
        uint16_t old_temp_col = temp_color_for_value(old_temp);
        uint16_t new_temp_col = temp_color_for_value(new_temp);

        if (new_temp != old_temp) {
            draw_vertical_bar_value_delta(
                &g_layout.invtemp_bar,
                old_temp,
                new_temp
            );

            if (old_temp_col != new_temp_col) {
                draw_numeric_block_value_full(
                    &g_layout.temp_block,
                    new_temp,
                    new_temp_col
                );
            } else {
                draw_numeric_block_value_delta(
                    &g_layout.temp_block,
                    old_temp,
                    new_temp,
                    new_temp_col
                );
            }

            prev.cell_temperature = d->cell_temperature;
        }
    }

    if ((int)d->speed != (int)prev.speed) {
        draw_numeric_block_value_delta(
            &g_layout.speed_block,
            (int)prev.speed,
            (int)d->speed,
            COL_TEXT
        );
        prev.speed = d->speed;
    }

    if ((int)d->lap != (int)prev.lap) {
        draw_lap_counter_delta(
            &g_layout.lap_block,
            d->lap,
            LAP_TOTAL_DEFAULT,
            COL_ACC
        );
        prev.lap = d->lap;
    }

    // water_temperature intentionally ignored
}

// ======================================================
// UGR LOGOS
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

// ======================================================
// TOUCH AREA CALCULATIONS
// ======================================================

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

    if (w == 0 || h == 0 || w > 800) {
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

            t0 = HAL_GetTick();

            SSD1963_SetWindow(
                x,
                y + row,
                x + w - 1,
                y + row + rows_this_chunk - 1
            );

            SSD1963_WritePixels(logo_chunk, (uint32_t)w * rows_this_chunk);

            t1 = HAL_GetTick();
            lcd_time += (t1 - t0);

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

//GRAPHING

typedef struct {
    Rect area;

    uint8_t throttle[300];
    uint8_t brake[300];

    int capacity;   // e.g. 300
    int head;       // next write position
    int count;      // number of valid samples, <= capacity

    uint16_t bg_color;
    uint16_t grid_color;
    uint16_t throttle_color;
    uint16_t brake_color;
    uint16_t frame_color;
} DualTraceGraphWidget;

static void graph_push_sample(DualTraceGraphWidget *g, int throttle, int brake)
{
    if (!g || g->capacity <= 0) {
        return;
    }

    throttle = clampi(throttle, 0, 100);
    brake = clampi(brake, 0, 100);

    g->throttle[g->head] = (uint8_t)throttle;
    g->brake[g->head] = (uint8_t)brake;

    g->head = (g->head + 1) % g->capacity;

    if (g->count < g->capacity) {
        g->count++;
    }
}

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

static void draw_graph_static(const DualTraceGraphWidget *g)
{
    int i;
    int inner_x = g->area.x + 1;
    int inner_y = g->area.y + 1;
    int inner_w = g->area.w - 2;
    int inner_h = g->area.h - 2;

    gfx_fill_rect(g->area.x, g->area.y, g->area.w, g->area.h, g->bg_color);
    draw_rect_outline(g->area.x, g->area.y, g->area.w, g->area.h, g->frame_color);

    for (i = 0; i <= 4; i++) {
        int gy = inner_y + (i * inner_h) / 4;
        gfx_draw_line(inner_x, gy, inner_x + inner_w - 1, gy, g->grid_color);
    }
}

static void draw_graph_full(const DualTraceGraphWidget *g)
{
    int inner_x = g->area.x + 1;
    int inner_y = g->area.y + 1;
    int inner_w = g->area.w - 2;
    int inner_h = g->area.h - 2;
    int visible = (g->count < inner_w) ? g->count : inner_w;
    int start_x = inner_x + inner_w - visible;
    int i;

    gfx_fill_rect(inner_x, inner_y, inner_w, inner_h, g->bg_color);

    for (i = 0; i <= 4; i++) {
        int gy = inner_y + (i * inner_h) / 4;
        gfx_draw_line(inner_x, gy, inner_x + inner_w - 1, gy, g->grid_color);
    }

    if (visible < 2) {
        return;
    }

    for (i = 1; i < visible; i++) {
        int idx0 = graph_buffer_index(g, g->count - visible + (i - 1));
        int idx1 = graph_buffer_index(g, g->count - visible + i);

        int x0 = start_x + (i - 1);
        int x1 = start_x + i;

        int y0_throttle = graph_value_to_y(g, g->throttle[idx0]);
        int y1_throttle = graph_value_to_y(g, g->throttle[idx1]);

        int y0_brake = graph_value_to_y(g, g->brake[idx0]);
        int y1_brake = graph_value_to_y(g, g->brake[idx1]);

        gfx_draw_line(x0, y0_throttle, x1, y1_throttle, g->throttle_color);
        gfx_draw_line(x0, y0_brake, x1, y1_brake, g->brake_color);
    }
}