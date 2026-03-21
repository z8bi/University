#include "speedometer.h"
#include "gfx.h"
#include "logos/icons.h"
#include <math.h>

// You also need access to shared dashboard colours/screen size.
// If those stay in dashboard.c, either:
// 1) move them to dashboard.h, or
// 2) duplicate only what speedometer needs here.

#define W 800
#define H 480

#define COL_BG   RGB565(0,0,0)
#define COL_TEXT RGB565(230,230,230)

static const int DASH_Y_OFFSET = -30;

// ======================================================
// SPEEDOMETER POSITION
// ======================================================

const int GAUGE_CX = 230;
const int GAUGE_CY = 270 + DASH_Y_OFFSET;
const int GAUGE_R  = 165;

// ======================================================
// SPEED DISPLAY BOX
// ======================================================

const int SPEED_BOX_W = 130;
const int SPEED_BOX_H = 110;
const int SPEED_BOX_X = 230 - 130 / 2;
const int SPEED_BOX_Y = 340 + DASH_Y_OFFSET;

// ======================================================
// NEEDLE LENGTH
// ======================================================

const int NEEDLE_LEN = 165 - 28;

// ======================================================
// SPEED → ANGLE MAPPING
// ======================================================

const float START_ANGLE_DEG = 140.0f;
const float TOP_ANGLE_DEG   = 270.0f;
const float END_ANGLE_DEG   = 40.0f;

const int SPEED_AT_START = 0;
const int SPEED_AT_TOP   = 140;
const int SPEED_AT_END   = 350;

// ======================================================
// INTERNAL STATE
// ======================================================

static Pt needle_lut[SPEED_MAX + 1];
static Pt prev_needle_tip = { -1, -1 };

// ======================================================
// INTERNAL HELPERS
// ======================================================

static void swap_int(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

static inline int clampi(int v, int lo, int hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
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

Pt speedometer_needle_tip_from_speed(int speed)
{
    return needle_lut[clampi(speed, 0, SPEED_MAX)];
}

static Tri needle_triangle_ex(Pt tip, float base_half_width, float base_back)
{
    Tri t;

    float dx = (float)(tip.x - GAUGE_CX);
    float dy = (float)(tip.y - GAUGE_CY);
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) len = 1.0f;

    float ux = dx / len;
    float uy = dy / len;

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

static void draw_needle(Pt tip)
{
    Tri outer = needle_triangle_ex(tip, 7.0f, 13.0f);
    fill_triangle(outer.a, outer.b, outer.c, COL_NEEDLE_EDGE);

    Tri inner = needle_triangle_ex(tip, 4.0f, 11.0f);
    fill_triangle(inner.a, inner.b, inner.c, COL_NEEDLE);
}

// You already have these in dashboard.c.
// Either move them too, or call shared digit functions from elsewhere.
extern void draw_int7(int x, int y, int s, int value, int digits, uint16_t fg, uint16_t bg);
extern void redraw_changed_digits_3(int x, int y, int s, int old_value, int new_value, uint16_t fg, uint16_t bg);

static void get_speed_layout(int *digits_x, int *digits_y)
{
    *digits_x = SPEED_BOX_X + (SPEED_BOX_W - 104) / 2;
    *digits_y = SPEED_BOX_Y + (SPEED_BOX_H - 92) / 2;
}

// ======================================================
// PUBLIC API
// ======================================================

void speedometer_init(void)
{
    init_needle_lut();
    prev_needle_tip = (Pt){ -1, -1 };
}

void speedometer_draw_static(void)
{
    gfx_blit565(
        GAUGE_CX - SPEEDOMETER_ICON_W / 2,
        GAUGE_CY - SPEEDOMETER_ICON_H / 2,
        SPEEDOMETER_ICON_W,
        SPEEDOMETER_ICON_H,
        (const uint16_t*)speedometer_icon
    );

    draw_needle_hub();
}

void speedometer_draw_number(int speed)
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

void speedometer_redraw_number_delta(int old_speed, int new_speed)
{
    int digits_x, digits_y;

    old_speed = clampi(old_speed, 0, SPEED_MAX);
    new_speed = clampi(new_speed, 0, SPEED_MAX);

    get_speed_layout(&digits_x, &digits_y);

    redraw_changed_digits_3(digits_x, digits_y, 4, old_speed, new_speed, COL_TEXT, COL_BG);
}

void speedometer_update_needle(int speed)
{
    Pt new_tip = speedometer_needle_tip_from_speed(speed);

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