#ifndef DASHBOARD_CORE_H
#define DASHBOARD_CORE_H

#include <stdint.h>
#include "dashboard.h"
#include "gfx.h"
#include "gfx_font.h"

// ======================================================
// SCREEN SIZE
// ======================================================

#define DASH_W 800
#define DASH_H 480

// ======================================================
// TOUCH AREAS
// ======================================================

#define SMALL_LOGO_X      729
#define SMALL_LOGO_Y      10

#define BIG_LOGO_X        50
#define BIG_LOGO_Y        85

// ======================================================
// COLORS
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

#define BATT_RED_MAX      20
#define BATT_ORANGE_MAX   50
#define TEMP_GREEN_MAX    40
#define TEMP_ORANGE_MAX   50

#define LAP_TOTAL_DEFAULT         22
#define RIGHT_VALUE_BASELINE_GAP  58

#define GRAPH_MAX_SAMPLES 512

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

    int clamp_value_text;
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
    Rect area;

    uint8_t throttle[GRAPH_MAX_SAMPLES];
    uint8_t brake[GRAPH_MAX_SAMPLES];

    int capacity;
    int head;
    int count;

    uint16_t bg_color;
    uint16_t grid_color;
    uint16_t throttle_color;
    uint16_t brake_color;
    uint16_t frame_color;

    int line_thickness;  
} DualTraceGraphWidget;

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

// ======================================================
// GENERIC HELPERS
// ======================================================

Rect make_rect(int x, int y, int w, int h);
Rect union_rects(Rect a, Rect b);
int clampi(int v, int lo, int hi);
void swap_int(int *a, int *b);

int font_text_width(const char *text, const GFXfont *font);
void font_text_bounds(const char *text, const GFXfont *font,
                      int *out_w, int *out_min_y, int *out_max_y);

void draw_rect_outline(int x, int y, int w, int h, uint16_t col);
void draw_text_centered_in_rect(const Rect *r, int y, const char *text,
                                const GFXfont *font, uint16_t color);
void draw_text_right_aligned(int right_x, int y, const char *text,
                             const GFXfont *font, uint16_t color);
void draw_text_right_aligned_centered_y(int right_x, int center_y,
                                        const char *text,
                                        const GFXfont *font,
                                        uint16_t color);

// ======================================================
// COLOR HELPERS
// ======================================================

uint16_t battery_color_for_percent(int percent);
uint16_t temp_color_for_value(int temp);

// ======================================================
// UNIT GLYPHS
// ======================================================

void draw_percent(int x, int y, int s, uint16_t col);
void draw_C(int x, int y, int s, uint16_t col);

// ======================================================
// VERTICAL BAR WIDGET
// ======================================================

void draw_vertical_bar_static(const VerticalBarWidget *w);
void draw_vertical_bar_value_full(const VerticalBarWidget *w, int value);
void draw_vertical_bar_value_delta(const VerticalBarWidget *w,
                                   int old_value, int new_value);

// ======================================================
// NUMERIC BLOCK WIDGET
// ======================================================

void draw_numeric_block_static(const NumericBlockWidget *w);
void draw_numeric_block_value_full(const NumericBlockWidget *w,
                                   int value, uint16_t color);
void draw_numeric_block_value_delta(const NumericBlockWidget *w,
                                    int old_value, int new_value,
                                    uint16_t color);
void draw_lap_counter_full(const NumericBlockWidget *w,
                           int current_lap, int total_laps,
                           uint16_t color,
                           int previous_lap_for_dirty_rect);
void draw_lap_counter_delta(const NumericBlockWidget *w,
                            int old_lap, int new_lap,
                            int total_laps, uint16_t color);

// ======================================================
// GRAPH WIDGET
// ======================================================

void graph_init(DualTraceGraphWidget *g, Rect area);
void graph_clear(DualTraceGraphWidget *g);
void graph_push_sample(DualTraceGraphWidget *g, int throttle, int brake);
void draw_graph_static(const DualTraceGraphWidget *g);
void draw_graph_full(const DualTraceGraphWidget *g);

// ======================================================
// LOGOS / SD
// ======================================================

void draw_UGR_logo(void);
void draw_big_UGR_logo(void);
UI_Area dash_get_area(DashAreaId id);

uint8_t draw_raw565_from_sd_chunked(const char *path,
                                    uint16_t x,
                                    uint16_t y,
                                    uint16_t w,
                                    uint16_t h);

void SD_Debug(const char *msg, uint16_t color);

#endif