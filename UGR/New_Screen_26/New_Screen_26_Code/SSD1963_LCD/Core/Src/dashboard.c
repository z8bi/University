#include "dashboard.h"
#include "gfx.h"
#include "ssd1963.h"
#include "logos/ugr_logo.h"
#include "logos/icons.h"
#include "dashboard_font_20.h"
#include "font_draw.h"

#include <string.h>
#include <math.h>

// If you use math (sinf/cosf), ensure your link flags include -lm in CMake.

#define W 800
#define H 480

//=== Touch areas for toggling screens ===

#define SMALL_LOGO_X   380
#define SMALL_LOGO_Y   5

#define BIG_LOGO_X     50
#define BIG_LOGO_Y     85

#define LOGO_BACK_BTN_X   300
#define LOGO_BACK_BTN_Y   410
#define LOGO_BACK_BTN_W   200
#define LOGO_BACK_BTN_H   50

// Battery color thresholds
#define BATT_RED_MAX        20
#define BATT_ORANGE_MAX     50

// Temperature color thresholds
#define TEMP_GREEN_MAX      60
#define TEMP_ORANGE_MAX     90
// above TEMP_ORANGE_MAX => red

#define SPEED_MAX 350
#define COL_NEEDLE      RGB565(255,0,0)       // bright red
#define COL_NEEDLE_EDGE RGB565(255,255,255)   // white outline
#define COL_HUB         RGB565(255,255,255)

// Colors (RGB565 macro comes from your ssd1963.h)
#define COL_BG     RGB565(0,0,0)
#define COL_PANEL  RGB565(18,18,18)
#define COL_TEXT   RGB565(230,230,230)
#define COL_DIM    RGB565(120,120,120)
#define COL_ACC    RGB565(0,200,255)
#define COL_WARN   RGB565(255,80,0)
#define COL_GOOD   RGB565(0,220,0)

// Layout
static const int DASH_Y_OFFSET = -30;   // adjust this to move the whole dashboard

static const int GAUGE_CX = 230;
static const int GAUGE_CY = 270 + DASH_Y_OFFSET;
static const int GAUGE_R  = 165;

// Speed number box 
static const int SPEED_BOX_W = 130; 
static const int SPEED_BOX_H = 110; 
static const int SPEED_BOX_X = GAUGE_CX - SPEED_BOX_W / 2; 
static const int SPEED_BOX_Y = 340 + DASH_Y_OFFSET;

// Right-side panels
static const int PX = 480;
static const int PW = 320;
static const int P1Y = 115 + DASH_Y_OFFSET;
static const int P2Y = 240 + DASH_Y_OFFSET;
static const int P3Y = 365 + DASH_Y_OFFSET;

// Speed number box
static const int NEEDLE_LEN = GAUGE_R - 28;

static const float START_ANGLE_DEG = 140.0f;
static const float TOP_ANGLE_DEG   = 270.0f;
static const float END_ANGLE_DEG   = 40.0f;   // same as 400°, just wrapped

static const int SPEED_AT_START = 0;
static const int SPEED_AT_TOP   = 140;
static const int SPEED_AT_END   = 350;

// Value text anchor in panels
static const int ICON_X = PX + 16;
static const int ICON_W = 56;
static const int ICON_H = 40;

static const int WATER_BMP_W = 48;
static const int WATER_BMP_H = 80;

static const int THERMO_BMP_W = 33;
static const int THERMO_BMP_H = 80;

// Manual icon offsets after centering
static const int ICON_Y_FINE = 15;   // negative = move icons up, positive = down
static const int BATTERY_ICON_Y_FINE = 0;
static const int THERMO_ICON_Y_FINE  = 0;
static const int WATER_ICON_Y_FINE   = 0;
static const int VAL_X  = PX + 95;

// Digit geometry (scale = 3)
static const int DIGIT_SCALE = 3;

static const int DIGIT_W = (6*DIGIT_SCALE) + (2*DIGIT_SCALE);
static const int DIGIT_H = (2*(10*DIGIT_SCALE)) + (3*DIGIT_SCALE);

// 3 digits + spacing
static const int DIGITS_W = (3 * DIGIT_W) + (2 * DIGIT_SCALE);

// unit symbol size
static const int UNIT_W = 12;
static const int UNIT_H = 12;

// spacing between digits and unit
static const int VALUE_GAP = 12;

// value box padding
static const int VALUE_PAD_X = 10;
static const int VALUE_PAD_Y = 10;

// black outlined value box
static const int VALUE_BOX_X = VAL_X;          // align with label text
static const int VALUE_BOX_Y_OFF = 36;
static const int VALUE_BOX_W = DIGITS_W + VALUE_GAP + UNIT_W + VALUE_PAD_X*2;
static const int VALUE_BOX_H = DIGIT_H + VALUE_PAD_Y*2;

// make gray panel extend down to the bottom of the black box, plus a little margin
static const int PANEL_BOTTOM_PAD = 8;
static const int PH = VALUE_BOX_Y_OFF + VALUE_BOX_H + PANEL_BOTTOM_PAD;

//prototypes
static void draw_hline_clipped(int x0, int x1, int y, uint16_t col);
static void swap_int(int *a, int *b);
static uint16_t battery_color_for_percent(int percent);
static uint16_t temp_color_for_value(int temp);
static void draw_value_box(int panel_y);
static void get_value_layout(int panel_y, int *digits_x, int *digits_y, int *unit_x, int *unit_y);
static void icon_battery(int x, int y, int w, int h, int percent, int on);
static void draw_int7(int x, int y, int s, int value, int digits, uint16_t fg, uint16_t bg);
static void draw_percent(int x, int y, int s, uint16_t col);
static void draw_C(int x, int y, int s, uint16_t col);
static void redraw_battery_panel_value_delta(int old_percent, int new_percent);
static void redraw_celltemp_panel_value_delta(int old_temp, int new_temp);
static void redraw_watertemp_panel_value_delta(int old_temp, int new_temp);
static void redraw_speed_number_delta(int old_speed, int new_speed);
static void draw_digit7(int x, int y, int s, int digit, uint16_t fg, uint16_t bg);

// Internal state
static Dashboard prev;
static int inited = 0;

typedef struct {
    int x;
    int y;
} Pt;

// ---------- NEEDLE HELPERS ----------

typedef struct {
    Pt a, b, c;
} Tri;

typedef struct {
    int x0, y0, x1, y1;
} Rect;

static Tri needle_triangle_ex(Pt tip, float base_half_width, float base_back)
{
    Tri t;

    float dx = (float)(tip.x - GAUGE_CX);
    float dy = (float)(tip.y - GAUGE_CY);
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) len = 1.0f;

    // unit direction from center to tip
    float ux = dx / len;
    float uy = dy / len;

    // unit perpendicular
    float px = -uy;
    float py =  ux;

    float bx = GAUGE_CX - ux * base_back;
    float by = GAUGE_CY - uy * base_back;

    t.a.x = tip.x;
    t.a.y = tip.y;

    t.b.x = (int)lroundf(bx + px * base_half_width);
    t.b.y = (int)lroundf(by + py * base_half_width);

    t.c.x = (int)lroundf(bx - px * base_half_width);
    t.c.y = (int)lroundf(by - py * base_half_width);

    return t;
}

static void draw_needle_hub(void)
{
    gfx_fill_rect(GAUGE_CX - 5, GAUGE_CY - 5, 10, 10, COL_HUB);
    gfx_fill_rect(GAUGE_CX - 2, GAUGE_CY - 2, 4, 4, COL_NEEDLE);
}

static void restore_gauge_hline(int x0, int x1, int y)
{
    int gauge_x0 = GAUGE_CX - SPEEDOMETER_ICON_W / 2;
    int gauge_y0 = GAUGE_CY - SPEEDOMETER_ICON_H / 2;

    if (y < gauge_y0 || y >= gauge_y0 + SPEEDOMETER_ICON_H) return;
    if (x0 > x1) swap_int(&x0, &x1);

    int gx0 = x0;
    int gx1 = x1;

    if (gx0 < gauge_x0) gx0 = gauge_x0;
    if (gx1 >= gauge_x0 + SPEEDOMETER_ICON_W) gx1 = gauge_x0 + SPEEDOMETER_ICON_W - 1;
    if (gx0 > gx1) return;

    int src_y = y - gauge_y0;
    int src_x = gx0 - gauge_x0;
    int len   = gx1 - gx0 + 1;

    const uint16_t *src = (const uint16_t*)speedometer_icon + src_y * SPEEDOMETER_ICON_W + src_x;

    int run_start = gx0;
    uint16_t run_col = (*src == RGB565(0,0,0)) ? COL_BG : *src;

    for (int i = 1; i < len; i++) {
        uint16_t col = (src[i] == RGB565(0,0,0)) ? COL_BG : src[i];

        if (col != run_col) {
            gfx_fill_rect(run_start, y, gx0 + i - run_start, 1, run_col);
            run_start = gx0 + i;
            run_col = col;
        }
    }

    gfx_fill_rect(run_start, y, gx0 + len - run_start, 1, run_col);
}

static void fill_flat_triangle(Pt v0, Pt v1, Pt v2, uint16_t col)
{
    float invslope1 = (v1.y != v0.y) ? (float)(v1.x - v0.x) / (float)(v1.y - v0.y) : 0.0f;
    float invslope2 = (v2.y != v0.y) ? (float)(v2.x - v0.x) / (float)(v2.y - v0.y) : 0.0f;

    float curx1 = (float)v0.x;
    float curx2 = (float)v0.x;

    for (int y = v0.y; y <= v1.y; y++) {
        draw_hline_clipped((int)curx1, (int)curx2, y, col);
        curx1 += invslope1;
        curx2 += invslope2;
    }
}

static void fill_flat_triangle_up(Pt v0, Pt v1, Pt v2, uint16_t col)
{
    float invslope1 = (v2.y != v0.y) ? (float)(v2.x - v0.x) / (float)(v2.y - v0.y) : 0.0f;
    float invslope2 = (v2.y != v1.y) ? (float)(v2.x - v1.x) / (float)(v2.y - v1.y) : 0.0f;

    float curx1 = (float)v2.x;
    float curx2 = (float)v2.x;

    for (int y = v2.y; y >= v0.y; y--) {
        draw_hline_clipped((int)curx1, (int)curx2, y, col);
        curx1 -= invslope1;
        curx2 -= invslope2;
    }
}

static void restore_flat_triangle(Pt v0, Pt v1, Pt v2)
{
    float invslope1 = (v1.y != v0.y) ? (float)(v1.x - v0.x) / (float)(v1.y - v0.y) : 0.0f;
    float invslope2 = (v2.y != v0.y) ? (float)(v2.x - v0.x) / (float)(v2.y - v0.y) : 0.0f;

    float curx1 = (float)v0.x;
    float curx2 = (float)v0.x;

    for (int y = v0.y; y <= v1.y; y++) {
        restore_gauge_hline((int)curx1, (int)curx2, y);
        curx1 += invslope1;
        curx2 += invslope2;
    }
}

static void restore_flat_triangle_up(Pt v0, Pt v1, Pt v2)
{
    float invslope1 = (v2.y != v0.y) ? (float)(v2.x - v0.x) / (float)(v2.y - v0.y) : 0.0f;
    float invslope2 = (v2.y != v1.y) ? (float)(v2.x - v1.x) / (float)(v2.y - v1.y) : 0.0f;

    float curx1 = (float)v2.x;
    float curx2 = (float)v2.x;

    for (int y = v2.y; y >= v0.y; y--) {
        restore_gauge_hline((int)curx1, (int)curx2, y);
        curx1 -= invslope1;
        curx2 -= invslope2;
    }
}

static void restore_triangle(Pt v0, Pt v1, Pt v2)
{
    if (v1.y < v0.y) { Pt t = v0; v0 = v1; v1 = t; }
    if (v2.y < v0.y) { Pt t = v0; v0 = v2; v2 = t; }
    if (v2.y < v1.y) { Pt t = v1; v1 = v2; v2 = t; }

    if (v1.y == v2.y) {
        restore_flat_triangle(v0, v1, v2);
    } else if (v0.y == v1.y) {
        restore_flat_triangle_up(v0, v1, v2);
    } else {
        Pt v3;
        v3.y = v1.y;
        v3.x = v0.x + (int)((float)(v1.y - v0.y) / (float)(v2.y - v0.y) * (float)(v2.x - v0.x));

        restore_flat_triangle(v0, v1, v3);
        restore_flat_triangle_up(v1, v3, v2);
    }
}

static void fill_triangle(Pt v0, Pt v1, Pt v2, uint16_t col)
{
    if (v1.y < v0.y) { Pt t = v0; v0 = v1; v1 = t; }
    if (v2.y < v0.y) { Pt t = v0; v0 = v2; v2 = t; }
    if (v2.y < v1.y) { Pt t = v1; v1 = v2; v2 = t; }

    if (v1.y == v2.y) {
        fill_flat_triangle(v0, v1, v2, col);
    } else if (v0.y == v1.y) {
        fill_flat_triangle_up(v0, v1, v2, col);
    } else {
        Pt v3;
        v3.y = v1.y;
        v3.x = v0.x + (int)((float)(v1.y - v0.y) / (float)(v2.y - v0.y) * (float)(v2.x - v0.x));

        fill_flat_triangle(v0, v1, v3, col);
        fill_flat_triangle_up(v1, v3, v2, col);
    }
}

static void get_speed_layout(int *digits_x, int *digits_y) { 
    *digits_x = SPEED_BOX_X + (SPEED_BOX_W - 104) / 2; 
    *digits_y = SPEED_BOX_Y + (SPEED_BOX_H - 92) / 2; 
}

static void swap_int(int *a, int *b)
{
    int t = *a; *a = *b; *b = t;
}

static void draw_hline_clipped(int x0, int x1, int y, uint16_t col)
{
    if (y < 0 || y >= H) return;
    if (x0 > x1) swap_int(&x0, &x1);
    if (x1 < 0 || x0 >= W) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= W) x1 = W - 1;
    gfx_fill_rect(x0, y, x1 - x0 + 1, 1, col);
}

static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static int centered_icon_y(int panel_y)
{
    return panel_y + (PH - ICON_H) / 2 + ICON_Y_FINE;
}

static int battery_icon_y(void)
{
    return centered_icon_y(P1Y) + BATTERY_ICON_Y_FINE;
}

static int thermo_icon_y(void)
{
    return P2Y + (PH - THERMO_BMP_H) / 2 + ICON_Y_FINE + THERMO_ICON_Y_FINE;
}

static int water_icon_y(void)
{
    return P3Y + (PH - WATER_BMP_H) / 2 + ICON_Y_FINE + WATER_ICON_Y_FINE;
}

static float speed_to_angle_deg(int speed)
{
    speed = clampi(speed, SPEED_AT_START, SPEED_AT_END);

    if (speed <= SPEED_AT_TOP) {
        float t = (float)(speed - SPEED_AT_START) /
                  (float)(SPEED_AT_TOP - SPEED_AT_START);

        return START_ANGLE_DEG + t * (TOP_ANGLE_DEG - START_ANGLE_DEG);
    } else {
        float t = (float)(speed - SPEED_AT_TOP) /
                  (float)(SPEED_AT_END - SPEED_AT_TOP);

        return TOP_ANGLE_DEG + t * ((END_ANGLE_DEG + 360.0f) - TOP_ANGLE_DEG);
    }
}

static Pt needle_lut[SPEED_MAX + 1];

static void init_needle_lut(void)
{
    for (int s = 0; s <= SPEED_MAX; s++) {
        float a_deg = speed_to_angle_deg(s);

        if (a_deg >= 360.0f)
            a_deg -= 360.0f;

        float a = a_deg * (3.1415926f / 180.0f);

        needle_lut[s].x = (int)lroundf(GAUGE_CX + NEEDLE_LEN * cosf(a));
        needle_lut[s].y = (int)lroundf(GAUGE_CY + NEEDLE_LEN * sinf(a));
    }
}

static Pt needle_tip_from_speed(int speed)
{
    return needle_lut[clampi(speed, 0, SPEED_MAX)];
}

static Pt prev_needle_tip = { -1, -1 };

static void draw_needle(Pt tip)
{
    // Outer white outline
    Tri outer = needle_triangle_ex(tip, 7.0f, 13.0f);
    fill_triangle(outer.a, outer.b, outer.c, COL_NEEDLE_EDGE);

    // Inner red needle
    Tri inner = needle_triangle_ex(tip, 4.0f, 11.0f);
    fill_triangle(inner.a, inner.b, inner.c, COL_NEEDLE);
}

static void update_needle(int speed)
{
    Pt new_tip = needle_tip_from_speed(speed);

    if (prev_needle_tip.x >= 0 && prev_needle_tip.y >= 0) {
        Tri old_tri = needle_triangle_ex(prev_needle_tip, 7.0f, 13.0f);
        restore_triangle(old_tri.a, old_tri.b, old_tri.c);

        for (int y = GAUGE_CY - 6; y <= GAUGE_CY + 6; y++) {
            restore_gauge_hline(GAUGE_CX - 6, GAUGE_CX + 6, y);
        }
    }

    draw_needle(new_tip);
    draw_needle_hub();

    prev_needle_tip = new_tip;
}

// ---------- 7-seg digits (no font assets needed) ----------
/*
Segments:  a
         f   b
          g
         e   c
          d
*/
static void seg_h(int x, int y, int len, int t, uint16_t col) { gfx_fill_rect(x, y, len, t, col); }
static void seg_v(int x, int y, int len, int t, uint16_t col) { gfx_fill_rect(x, y, t, len, col); }

static void split_3digits(int value, int out[3])
{
    value = clampi(value, 0, 999);
    out[0] = (value / 100) % 10;
    out[1] = (value / 10) % 10;
    out[2] = value % 10;
}

static void redraw_changed_digits_3(
    int x, int y, int s,
    int old_value, int new_value,
    uint16_t fg, uint16_t bg)
{
    int oldd[3], newd[3];
    split_3digits(old_value, oldd);
    split_3digits(new_value, newd);

    const int t = s + 1;
    const int L = 6*s;
    const int w = L + 2*t;

    for (int i = 0; i < 3; i++) {
        if (oldd[i] != newd[i]) {
            int dx = x + i*(w + s);
            draw_digit7(dx, y, s, newd[i], fg, bg);
        }
    }
}

static void redraw_digits_3(
    int x, int y, int s,
    int value,
    uint16_t fg, uint16_t bg)
{
    int d[3];
    split_3digits(value, d);

    const int t = s + 1;
    const int L = 6*s;
    const int w = L + 2*t;

    for (int i = 0; i < 3; i++) {
        int dx = x + i*(w + s);
        draw_digit7(dx, y, s, d[i], fg, bg);
    }
}

static void draw_digit7(int x, int y, int s, int digit, uint16_t fg, uint16_t bg)
{
    // digit box size
    const int t = s + 1;          // thickness
    const int L = 6*s;        // segment length
    const int Hh = 10*s;      // vertical span per side (top/bottom)
    const int w = L + 2*t;
    const int h = (2*Hh) + 3*t;

    // clear digit area
    gfx_fill_rect(x, y, w, h, bg);

    // which segments are on for each digit
    // bits: a b c d e f g (LSB=a)
    static const uint8_t map[10] = {
        0b0111111, //0 a b c d e f
        0b0000110, //1 b c
        0b1011011, //2 a b d e g
        0b1001111, //3 a b c d g
        0b1100110, //4 b c f g
        0b1101101, //5 a c d f g
        0b1111101, //6 a c d e f g
        0b0000111, //7 a b c
        0b1111111, //8 all
        0b1101111  //9 a b c d f g
    };

    uint8_t m = (digit >= 0 && digit <= 9) ? map[digit] : 0;

    // Segment positions
    // a: top
    if (m & (1<<0)) seg_h(x + t, y + 0,     L, t, fg);
    // b: upper-right
    if (m & (1<<1)) seg_v(x + t + L, y + t, Hh, t, fg);
    // c: lower-right
    if (m & (1<<2)) seg_v(x + t + L, y + 2*t + Hh, Hh, t, fg);
    // d: bottom
    if (m & (1<<3)) seg_h(x + t, y + 2*t + 2*Hh, L, t, fg);
    // e: lower-left
    if (m & (1<<4)) seg_v(x + 0,     y + 2*t + Hh, Hh, t, fg);
    // f: upper-left
    if (m & (1<<5)) seg_v(x + 0,     y + t, Hh, t, fg);
    // g: middle
    if (m & (1<<6)) seg_h(x + t, y + t + Hh, L, t, fg);
}

// ---------- tiny label text ----------
// ---------- scalable label text ----------

static void draw_label(int x, int y, const char *txt, int s, uint16_t col)
{
    // s = scale (2 or 3 recommended)

    const int cw = 6*s;

    for (int i = 0; txt[i]; i++) {

        int px = x + i*cw;

        switch (txt[i]) {

        case 'A':
            gfx_fill_rect(px, y, 4*s, s, col);
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px+3*s, y, s, 6*s, col);
            gfx_fill_rect(px, y+3*s, 4*s, s, col);
            break;

        case 'B':
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px, y, 3*s, s, col);
            gfx_fill_rect(px, y+3*s, 3*s, s, col);
            gfx_fill_rect(px, y+5*s, 3*s, s, col);
            gfx_fill_rect(px+3*s, y+s, s, 2*s, col);
            gfx_fill_rect(px+3*s, y+4*s, s, s, col);
            break;

        case 'C':
            gfx_fill_rect(px, y, 4*s, s, col);
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px, y+5*s, 4*s, s, col);
            break;

        case 'E':
            gfx_fill_rect(px, y, 4*s, s, col);
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px, y+3*s, 3*s, s, col);
            gfx_fill_rect(px, y+5*s, 4*s, s, col);
            break;

        case 'L':
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px, y+5*s, 4*s, s, col);
            break;

        case 'M':
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px+4*s, y, s, 6*s, col);
            gfx_fill_rect(px+2*s, y+2*s, s, 2*s, col);
            break;

        case 'P':
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px, y, 4*s, s, col);
            gfx_fill_rect(px+4*s, y+s, s, 2*s, col);
            gfx_fill_rect(px, y+3*s, 4*s, s, col);
            break;

        case 'R':
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px, y, 4*s, s, col);
            gfx_fill_rect(px+4*s, y+s, s, 2*s, col);
            gfx_fill_rect(px, y+3*s, 4*s, s, col);
            gfx_fill_rect(px+3*s, y+3*s, s, 3*s, col);
            break;

        case 'T':
            gfx_fill_rect(px, y, 5*s, s, col);
            gfx_fill_rect(px+2*s, y, s, 6*s, col);
            break;

        case 'W':
            gfx_fill_rect(px, y, s, 6*s, col);
            gfx_fill_rect(px+4*s, y, s, 6*s, col);
            gfx_fill_rect(px+2*s, y+3*s, s, 3*s, col);
            break;

        case 'Y':
            gfx_fill_rect(px, y, s, 3*s, col);
            gfx_fill_rect(px+4*s, y, s, 3*s, col);
            gfx_fill_rect(px+2*s, y+3*s, s, 3*s, col);
            break;

        case ' ':
            break;
        }
    }
}

static void draw_battery_panel_value_full(int percent)
{
    int digits_x, digits_y, unit_x, unit_y;
    uint16_t col = battery_color_for_percent(percent);

    percent = clampi(percent, 0, 100);

    icon_battery(ICON_X, battery_icon_y(), ICON_W, ICON_H, percent, 1);
    get_value_layout(P1Y, &digits_x, &digits_y, &unit_x, &unit_y);

    redraw_digits_3(digits_x, digits_y, 3, percent, col, COL_BG);
    gfx_fill_rect(unit_x, unit_y, UNIT_W, UNIT_H + 4, COL_BG);
    draw_percent(unit_x, unit_y + 2, 2, col);
}

static void draw_celltemp_panel_value_full(int temp)
{
    int digits_x, digits_y, unit_x, unit_y;
    uint16_t col = temp_color_for_value(temp);

    temp = clampi(temp, 0, 150);

    get_value_layout(P2Y, &digits_x, &digits_y, &unit_x, &unit_y);

    redraw_digits_3(digits_x, digits_y, 3, temp, col, COL_BG);
    gfx_fill_rect(unit_x, unit_y, UNIT_W, UNIT_H + 4, COL_BG);
    draw_C(unit_x, unit_y + 1, 2, col);
}

static void draw_watertemp_panel_value_full(int temp)
{
    int digits_x, digits_y, unit_x, unit_y;
    uint16_t col = temp_color_for_value(temp);

    temp = clampi(temp, 0, 150);

    get_value_layout(P3Y, &digits_x, &digits_y, &unit_x, &unit_y);

    redraw_digits_3(digits_x, digits_y, 3, temp, col, COL_BG);
    gfx_fill_rect(unit_x, unit_y, UNIT_W, UNIT_H + 4, COL_BG);
    draw_C(unit_x, unit_y + 1, 2, col);
}

static void redraw_celltemp_panel_value_delta(int old_temp, int new_temp)
{
    int digits_x, digits_y, unit_x, unit_y;

    old_temp = clampi(old_temp, 0, 150);
    new_temp = clampi(new_temp, 0, 150);

    uint16_t old_col = temp_color_for_value(old_temp);
    uint16_t new_col = temp_color_for_value(new_temp);

    get_value_layout(P2Y, &digits_x, &digits_y, &unit_x, &unit_y);

    if (old_col != new_col) {
        redraw_digits_3(digits_x, digits_y, 3, new_temp, new_col, COL_BG);

        // clear and redraw unit area
        gfx_fill_rect(unit_x, unit_y, UNIT_W, UNIT_H + 4, COL_BG);
        draw_C(unit_x, unit_y + 1, 2, new_col);
    } else {
        redraw_changed_digits_3(digits_x, digits_y, 3, old_temp, new_temp, new_col, COL_BG);
    }
}

static void redraw_watertemp_panel_value_delta(int old_temp, int new_temp)
{
    int digits_x, digits_y, unit_x, unit_y;

    old_temp = clampi(old_temp, 0, 150);
    new_temp = clampi(new_temp, 0, 150);

    uint16_t old_col = temp_color_for_value(old_temp);
    uint16_t new_col = temp_color_for_value(new_temp);

    get_value_layout(P3Y, &digits_x, &digits_y, &unit_x, &unit_y);

    if (old_col != new_col) {
        redraw_digits_3(digits_x, digits_y, 3, new_temp, new_col, COL_BG);
        gfx_fill_rect(unit_x, unit_y, UNIT_W, UNIT_H + 4, COL_BG);
        draw_C(unit_x, unit_y + 1, 2, new_col);
    } else {
        redraw_changed_digits_3(digits_x, digits_y, 3, old_temp, new_temp, new_col, COL_BG);
    }
}

static void redraw_battery_panel_value_delta(int old_percent, int new_percent)
{
    int digits_x, digits_y, unit_x, unit_y;

    old_percent = clampi(old_percent, 0, 100);
    new_percent = clampi(new_percent, 0, 100);

    uint16_t old_col = battery_color_for_percent(old_percent);
    uint16_t new_col = battery_color_for_percent(new_percent);

    // icon changes whenever value changes
    icon_battery(ICON_X, battery_icon_y(), ICON_W, ICON_H, new_percent, 1);

    get_value_layout(P1Y, &digits_x, &digits_y, &unit_x, &unit_y);

    if (old_col != new_col) {
        redraw_digits_3(digits_x, digits_y, 3, new_percent, new_col, COL_BG);
        gfx_fill_rect(unit_x, unit_y, UNIT_W, UNIT_H + 4, COL_BG);
        draw_percent(unit_x, unit_y + 2, 2, new_col);
    } else {
        redraw_changed_digits_3(digits_x, digits_y, 3, old_percent, new_percent, new_col, COL_BG);
    }
}

static void redraw_speed_number_delta(int old_speed, int new_speed)
{
    int digits_x, digits_y;

    old_speed = clampi(old_speed, 0, SPEED_MAX);
    new_speed = clampi(new_speed, 0, SPEED_MAX);

    get_speed_layout(&digits_x, &digits_y);

    // Speed color is fixed, so we only redraw changed digit cells
    redraw_changed_digits_3(digits_x, digits_y, 4, old_speed, new_speed, COL_TEXT, COL_BG);
}

static void draw_int7(int x, int y, int s, int value, int digits, uint16_t fg, uint16_t bg)
{
    int v = value;
    if (v < 0) v = 0;

    const int t = s + 1;
    const int L = 6*s;
    const int w = L + 2*t;

    for (int i = digits-1; i >= 0; i--) {
        int d = v % 10;
        v /= 10;
        draw_digit7(x + i*(w + s), y, s, d, fg, bg);
    }
}

// small “°C” and “%” markers drawn procedurally
static void draw_percent(int x, int y, int s, uint16_t col)
{
    // two dots + slash
    int r = s;
    gfx_fill_rect(x, y, r, r, col);
    gfx_fill_rect(x + 4*s, y + 4*s, r, r, col);
    gfx_draw_line(x, y + 5*s, x + 5*s, y, col);
}
static void draw_C(int x, int y, int s, uint16_t col)
{
    // crude C with 3 bars
    gfx_fill_rect(x, y, 4*s, s, col);
    gfx_fill_rect(x, y, s, 6*s, col);
    gfx_fill_rect(x, y + 5*s, 4*s, s, col);
}

static void draw_value_box(int panel_y)
{
    int x = VALUE_BOX_X;
    int y = panel_y + VALUE_BOX_Y_OFF;

    gfx_fill_rect(x, y, VALUE_BOX_W, VALUE_BOX_H, COL_BG);

    gfx_draw_line(x, y, x + VALUE_BOX_W - 1, y, COL_TEXT);
    gfx_draw_line(x, y + VALUE_BOX_H - 1, x + VALUE_BOX_W - 1, y + VALUE_BOX_H - 1, COL_TEXT);
    gfx_draw_line(x, y, x, y + VALUE_BOX_H - 1, COL_TEXT);
    gfx_draw_line(x + VALUE_BOX_W - 1, y, x + VALUE_BOX_W - 1, y + VALUE_BOX_H - 1, COL_TEXT);
}

static void get_value_layout(int panel_y, int *digits_x, int *digits_y, int *unit_x, int *unit_y)
{
    int box_x = VALUE_BOX_X;
    int box_y = panel_y + VALUE_BOX_Y_OFF;

    int content_w = DIGITS_W + VALUE_GAP + UNIT_W;
    int content_h = DIGIT_H;

    int start_x = box_x + (VALUE_BOX_W - content_w) / 2;
    int start_y = box_y + (VALUE_BOX_H - content_h) / 2;

    *digits_x = start_x;
    *digits_y = start_y;
    *unit_x   = start_x + DIGITS_W + VALUE_GAP;
    *unit_y   = start_y + (DIGIT_H - UNIT_H) / 2;
}

// ---------- icons (procedural placeholders; swap to bitmaps later) ----------
static void icon_battery(int x, int y, int w, int h, int percent, int on)
{
    uint16_t stroke = on ? COL_TEXT : COL_DIM;
    uint16_t fill   = on ? battery_color_for_percent(percent) : COL_DIM;

    // outline
    gfx_fill_rect(x, y, w, h, COL_PANEL);
    gfx_draw_line(x, y, x+w-1, y, stroke);
    gfx_draw_line(x, y+h-1, x+w-1, y+h-1, stroke);
    gfx_draw_line(x, y, x, y+h-1, stroke);
    gfx_draw_line(x+w-1, y, x+w-1, y+h-1, stroke);

    // terminal nub
    gfx_fill_rect(x+w, y + h/3, w/8, h/3, stroke);

    // full interior redraw every time
    int inner = w - 4;
    int level = clampi(percent, 0, 100) * inner / 100;

    gfx_fill_rect(x+2, y+2, inner, h-4, RGB565(8,8,8));

    if (level > 0) {
        gfx_fill_rect(x+2, y+2, level, h-4, fill);
    }
}

// ---------- gauge drawing ----------
static void draw_gauge_static(void)
{
    gfx_blit565(
        GAUGE_CX - SPEEDOMETER_ICON_W / 2,
        GAUGE_CY - SPEEDOMETER_ICON_H / 2,
        SPEEDOMETER_ICON_W,
        SPEEDOMETER_ICON_H,
        (const uint16_t*)speedometer_icon
    );

    gfx_fill_rect(GAUGE_CX - 5, GAUGE_CY - 5, 10, 10, COL_TEXT);
}


// ---------- panels ----------

static uint16_t battery_color_for_percent(int percent)
{
    percent = clampi(percent, 0, 100);

    if (percent <= BATT_RED_MAX) {
        return COL_WARN;                 // red
    } else if (percent <= BATT_ORANGE_MAX) {
        return RGB565(255,165,0);        // orange
    } else {
        return COL_GOOD;                 // green
    }
}

static uint16_t temp_color_for_value(int temp)
{
    temp = clampi(temp, 0, 150);

    if (temp <= TEMP_GREEN_MAX) {
        return COL_GOOD;                 // green
    } else if (temp <= TEMP_ORANGE_MAX) {
        return RGB565(255,165,0);        // orange
    } else {
        return COL_WARN;                 // red
    }
}

static void draw_speed_number(int speed)
{
    int digits_x, digits_y;
    get_speed_layout(&digits_x, &digits_y);

    draw_int7(
        digits_x,
        digits_y,
        4,
        clampi(speed, 0, SPEED_MAX),
        3,
        COL_TEXT,
        COL_BG
    );
}

static void draw_battery_panel_static(void)
{
    gfx_fill_rect(PX, P1Y, PW, PH, COL_PANEL);
    draw_label(VAL_X, P1Y + 8, "BATTERY", 3, COL_DIM);

    icon_battery(ICON_X, battery_icon_y(), ICON_W, ICON_H, 0, 1);

    draw_value_box(P1Y);
}

static void draw_celltemp_panel_static(void)
{
    gfx_fill_rect(PX, P2Y, PW, PH, COL_PANEL);
    draw_label(VAL_X, P2Y + 8, "CELL TEMP", 3, COL_DIM);

    gfx_blit565(
        ICON_X + 12,
        thermo_icon_y(),
        THERMO_BMP_W,
        THERMO_BMP_H,
        (const uint16_t*)thermometer_icon
    );

    draw_value_box(P2Y);
}

static void draw_watertemp_panel_static(void)
{
    gfx_fill_rect(PX, P3Y, PW, PH, COL_PANEL);
    draw_label(VAL_X, P3Y + 8, "WATER TEMP", 3, COL_DIM);

    gfx_blit565(
        ICON_X+4,
        water_icon_y(),
        WATER_BMP_W,
        WATER_BMP_H,
        (const uint16_t*)water_icon
    );

    draw_value_box(P3Y);
}

// ---------- public API ----------
void dash_init(double initial_battery_charge, double initial_cell_temperature, double initial_water_temperature, int initial_speed)
{
    init_needle_lut();
    SSD1963_Fill(COL_BG);

    prev.speed = initial_speed;
    prev.battery_charge = initial_battery_charge;
    prev.cell_temperature = initial_cell_temperature;
    prev.water_temperature = initial_water_temperature;
    prev_needle_tip = (Pt){ -1, -1 };

    draw_gauge_static();
    update_needle(clampi(initial_speed, 0, SPEED_MAX));
    draw_speed_number(clampi(initial_speed, 0, SPEED_MAX));

    draw_battery_panel_static();
    draw_celltemp_panel_static();
    draw_watertemp_panel_static();

    // draw real initial values, not zeros
    draw_battery_panel_value_full((int)initial_battery_charge);
    draw_celltemp_panel_value_full((int)initial_cell_temperature);
    draw_watertemp_panel_value_full((int)initial_water_temperature);

    draw_UGR_logo();

    inited = 1;
}

void dash_update(const Dashboard *d)
{
    if (!inited) dash_init(0, 0, 0, 0);
    if (!d) return;

    if (d->battery_charge != prev.battery_charge) {
        redraw_battery_panel_value_delta(prev.battery_charge, d->battery_charge);
        prev.battery_charge = d->battery_charge;
    }

    if (d->cell_temperature != prev.cell_temperature) {
        redraw_celltemp_panel_value_delta(prev.cell_temperature, d->cell_temperature);
        prev.cell_temperature = d->cell_temperature;
    }

    if (d->water_temperature != prev.water_temperature) {
        redraw_watertemp_panel_value_delta(prev.water_temperature, d->water_temperature);
        prev.water_temperature = d->water_temperature;
    }

    if (d->speed != prev.speed) {
        update_needle(d->speed);
        redraw_speed_number_delta(prev.speed, d->speed);
        prev.speed = d->speed;
    }
}

void draw_UGR_logo(void)
{
    gfx_blit565_key(
        SMALL_LOGO_X, SMALL_LOGO_Y,
        UGR_LOGO_W, UGR_LOGO_H,
        (const uint16_t*)ugr_logo,
        RGB565(0,0,0)
    );
}

void draw_big_UGR_logo(void)
{
    gfx_blit565(
        BIG_LOGO_X, BIG_LOGO_Y,
        BIG_UGR_LOGO_W, BIG_UGR_LOGO_H,
        (const uint16_t*)big_ugr_logo
    );
}

//TOUCH AREA CALCULATIONS

UI_Area dash_get_area(DashAreaId id)
{
    UI_Area a = {0, 0, 0, 0};

    switch (id)
    {
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