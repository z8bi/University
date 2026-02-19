#include "dashboard.h"
#include "gfx.h"
#include "ssd1963.h"
#include "logos/ugr_logo.h"
#include <string.h>
#include <math.h>

// If you use math (sinf/cosf), ensure your link flags include -lm in CMake.

#define W 800
#define H 480

// Colors (RGB565 macro comes from your ssd1963.h)
#define COL_BG     RGB565(0,0,0)
#define COL_PANEL  RGB565(18,18,18)
#define COL_TEXT   RGB565(230,230,230)
#define COL_DIM    RGB565(120,120,120)
#define COL_ACC    RGB565(0,200,255)
#define COL_WARN   RGB565(255,80,0)
#define COL_GOOD   RGB565(0,220,0)

// Layout
static const int GAUGE_CX = 230;
static const int GAUGE_CY = 270;
static const int GAUGE_R  = 165;

// Right-side panels
static const int PX = 450;
static const int PW = 320;
static const int PH = 95;
static const int P1Y = 70;
static const int P2Y = 190;
static const int P3Y = 310;

// Speed number box
static const int SPEED_BOX_X = 110;
static const int SPEED_BOX_Y = 350;
static const int SPEED_BOX_W = 240;
static const int SPEED_BOX_H = 95;

// Needle redraw box (keep it tight)
static const int NEEDLE_BOX_X = GAUGE_CX - GAUGE_R - 6;
static const int NEEDLE_BOX_Y = GAUGE_CY - GAUGE_R - 6;
static const int NEEDLE_BOX_W = (GAUGE_R*2) + 12;
static const int NEEDLE_BOX_H = (GAUGE_R*2) + 12;

// Value text boxes in panels
static const int ICON_X = PX + 16;
static const int VAL_X  = PX + 95;
static const int VAL_W  = PW - (VAL_X - PX) - 16;

// Internal state
static Dashboard prev;
static int inited = 0;

// ---------- small helpers ----------
static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

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

static void draw_digit7(int x, int y, int s, int digit, uint16_t fg, uint16_t bg)
{
    // digit box size
    const int t = s;          // thickness
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

static void draw_int7(int x, int y, int s, int value, int digits, uint16_t fg, uint16_t bg)
{
    // Render right-aligned
    int v = value;
    if (v < 0) v = 0;

    const int t = s;
    const int L = 6*s;
    const int Hh = 10*s;
    const int w = L + 2*t;
    const int h = (2*Hh) + 3*t;

    // Clear total area
    gfx_fill_rect(x, y, digits*w + (digits-1)*s, h, bg);

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

// ---------- icons (procedural placeholders; swap to bitmaps later) ----------
static void icon_battery(int x, int y, int w, int h, int percent, int on)
{
    uint16_t stroke = on ? COL_TEXT : COL_DIM;
    uint16_t fill   = (percent < 20) ? COL_WARN : COL_GOOD;

    // outline
    gfx_fill_rect(x, y, w, h, COL_PANEL);
    gfx_draw_line(x, y, x+w-1, y, stroke);
    gfx_draw_line(x, y+h-1, x+w-1, y+h-1, stroke);
    gfx_draw_line(x, y, x, y+h-1, stroke);
    gfx_draw_line(x+w-1, y, x+w-1, y+h-1, stroke);

    // terminal nub
    gfx_fill_rect(x+w, y + h/3, w/8, h/3, stroke);

    // fill level
    int inner = w - 4;
    int level = clampi(percent, 0, 100) * inner / 100;
    gfx_fill_rect(x+2, y+2, inner, h-4, RGB565(8,8,8));
    gfx_fill_rect(x+2, y+2, level, h-4, on ? fill : COL_DIM);
}

static void icon_thermo(int x, int y, int w, int h, int on)
{
    uint16_t stroke = on ? COL_TEXT : COL_DIM;
    gfx_fill_rect(x, y, w, h, COL_PANEL);

    // bulb
    int cx = x + w/2;
    int by = y + h - 10;
    gfx_fill_rect(cx-5, by-5, 10, 10, stroke);
    // stem
    gfx_fill_rect(cx-2, y+6, 4, h-20, stroke);
}

static void icon_water(int x, int y, int w, int h, int on)
{
    uint16_t stroke = on ? COL_TEXT : COL_DIM;
    gfx_fill_rect(x, y, w, h, COL_PANEL);

    // droplet-ish: triangle + base
    int cx = x + w/2;
    gfx_draw_line(cx, y+4, x+4, y+h-4, stroke);
    gfx_draw_line(cx, y+4, x+w-4, y+h-4, stroke);
    gfx_draw_line(x+4, y+h-4, x+w-4, y+h-4, stroke);
}

// ---------- gauge drawing ----------
static void draw_gauge_static(void)
{
    // background already cleared by caller
    // tick marks
    for (int i = 0; i <= 12; i++) {
        float t = (float)i / 12.0f;
        float a = (-140.0f + 280.0f * t) * (3.1415926f / 180.0f);
        int x0 = (int)(GAUGE_CX + (GAUGE_R - 16) * cosf(a));
        int y0 = (int)(GAUGE_CY + (GAUGE_R - 16) * sinf(a));
        int x1 = (int)(GAUGE_CX + (GAUGE_R) * cosf(a));
        int y1 = (int)(GAUGE_CY + (GAUGE_R) * sinf(a));
        gfx_draw_line(x0, y0, x1, y1, COL_TEXT);
    }

    // hub
    gfx_fill_rect(GAUGE_CX-5, GAUGE_CY-5, 10, 10, COL_TEXT);
}

static void redraw_needle_region(void)
{
    // Clear needle box and redraw static gauge elements (ticks + hub)
    gfx_fill_rect(NEEDLE_BOX_X, NEEDLE_BOX_Y, NEEDLE_BOX_W, NEEDLE_BOX_H, COL_BG);
    draw_gauge_static();
}

static void draw_needle_for_speed(int speed)
{
    int s = clampi(speed, 0, 240);
    float t = (float)s / 240.0f;
    float a = (-140.0f + 280.0f * t) * (3.1415926f / 180.0f);

    int x1 = (int)(GAUGE_CX + (GAUGE_R - 28) * cosf(a));
    int y1 = (int)(GAUGE_CY + (GAUGE_R - 28) * sinf(a));

    // needle (slightly thick)
    gfx_draw_line(GAUGE_CX, GAUGE_CY, x1, y1, COL_ACC);
    gfx_draw_line(GAUGE_CX+1, GAUGE_CY, x1+1, y1, COL_ACC);
}

// ---------- panels ----------
static void draw_panels_static(void)
{
    gfx_fill_rect(PX, P1Y, PW, PH, COL_PANEL);
    gfx_fill_rect(PX, P2Y, PW, PH, COL_PANEL);
    gfx_fill_rect(PX, P3Y, PW, PH, COL_PANEL);
}

static void draw_speed_number(int speed)
{
    // Clear box
    gfx_fill_rect(SPEED_BOX_X, SPEED_BOX_Y, SPEED_BOX_W, SPEED_BOX_H, COL_BG);

    // 3 digits, scale 4 looks good on 800x480
    draw_int7(SPEED_BOX_X + 10, SPEED_BOX_Y + 10, 4, clampi(speed, 0, 240), 3, COL_TEXT, COL_BG);

    // optional "km/h" label omitted for now (no font). Add later if you want.
}

static void draw_battery_panel(int percent)
{
    // clear value area
    gfx_fill_rect(VAL_X, P1Y + 10, VAL_W, PH - 20, COL_PANEL);

    icon_battery(ICON_X, P1Y + 18, 56, 40, percent, 1);

    // number + %
    draw_int7(VAL_X, P1Y + 24, 3, clampi(percent, 0, 100), 3, COL_TEXT, COL_PANEL);
    draw_percent(VAL_X + 3*( (6*3)+(2*3) ) + 3*3 + 10, P1Y + 34, 2, COL_TEXT);
}

static void draw_celltemp_panel(int tempC)
{
    gfx_fill_rect(VAL_X, P2Y + 10, VAL_W, PH - 20, COL_PANEL);

    icon_thermo(ICON_X, P2Y + 18, 56, 40, 1);

    int v = clampi(tempC, -40, 150);
    // For negative, just clamp to 0 for now (easy improvement later)
    if (v < 0) v = 0;

    draw_int7(VAL_X, P2Y + 24, 3, v, 3, COL_TEXT, COL_PANEL);
    draw_C(VAL_X + 3*( (6*3)+(2*3) ) + 3*3 + 12, P2Y + 34, 2, COL_TEXT);
}

static void draw_watertemp_panel(int tempC)
{
    gfx_fill_rect(VAL_X, P3Y + 10, VAL_W, PH - 20, COL_PANEL);

    icon_water(ICON_X, P3Y + 18, 56, 40, 1);

    int v = clampi(tempC, -40, 150);
    if (v < 0) v = 0;

    draw_int7(VAL_X, P3Y + 24, 3, v, 3, COL_TEXT, COL_PANEL);
    draw_C(VAL_X + 3*( (6*3)+(2*3) ) + 3*3 + 12, P3Y + 34, 2, COL_TEXT);
}

// ---------- public API ----------
void dash_init(void)
{
    memset(&prev, 0x7F, sizeof(prev)); // force first update of all fields

    SSD1963_Fill(COL_BG);
    draw_panels_static();
    draw_gauge_static();
    draw_speed_number(0);

    inited = 1;
}

void dash_update(const Dashboard *d)
{
    if (!inited) dash_init();
    if (!d) return;

    // Speed: needle + number
    if (d->speed != prev.speed) {
        redraw_needle_region();
        draw_needle_for_speed(d->speed);
        draw_speed_number(d->speed);
        prev.speed = d->speed;
    }

    // Battery panel
    if (d->battery_charge != prev.battery_charge) {
        draw_battery_panel(d->battery_charge);
        prev.battery_charge = d->battery_charge;
    }

    // Cell temp panel
    if (d->cell_temperature != prev.cell_temperature) {
        draw_celltemp_panel(d->cell_temperature);
        prev.cell_temperature = d->cell_temperature;
    }

    // Water temp panel
    if (d->water_temperature != prev.water_temperature) {
        draw_watertemp_panel(d->water_temperature);
        prev.water_temperature = d->water_temperature;
    }
}

void draw_UGR_logo() {
    gfx_draw_rgb565_image(
        20, 20,
        UGR_LOGO_W, UGR_LOGO_H,
        (const uint16_t*)ugr_logo
    );
}
