#ifndef SPEEDOMETER_H
#define SPEEDOMETER_H

#include <stdint.h>

// ======================================================
// SPEEDOMETER CONFIGURATION
// ======================================================

#define SPEED_MAX 350

// Needle drawing colours
#define COL_NEEDLE      RGB565(255,0,0)
#define COL_NEEDLE_EDGE RGB565(255,255,255)
#define COL_HUB         RGB565(255,255,255)

// ======================================================
// SPEEDOMETER POSITION
// ======================================================

extern const int GAUGE_CX;
extern const int GAUGE_CY;
extern const int GAUGE_R;

// ======================================================
// SPEED DISPLAY BOX
// ======================================================

extern const int SPEED_BOX_W;
extern const int SPEED_BOX_H;
extern const int SPEED_BOX_X;
extern const int SPEED_BOX_Y;

// ======================================================
// NEEDLE LENGTH
// ======================================================

extern const int NEEDLE_LEN;

// ======================================================
// SPEED → ANGLE MAPPING
// ======================================================

extern const float START_ANGLE_DEG;
extern const float TOP_ANGLE_DEG;
extern const float END_ANGLE_DEG;

extern const int SPEED_AT_START;
extern const int SPEED_AT_TOP;
extern const int SPEED_AT_END;

// ======================================================
// TYPES
// ======================================================

typedef struct {
    int x;
    int y;
} Pt;

typedef struct {
    Pt a, b, c;
} Tri;

// ======================================================
// PUBLIC API
// ======================================================

void speedometer_init(void);
void speedometer_draw_static(void);
void speedometer_draw_number(int speed);
void speedometer_redraw_number_delta(int old_speed, int new_speed);
void speedometer_update_needle(int speed);

// Optional helpers if you want them visible outside
Pt speedometer_needle_tip_from_speed(int speed);

#endif