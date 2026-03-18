#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <stdint.h>

typedef struct {
    uint8_t battery_charge;      // 0–100
    uint8_t cell_temperature;    // 0–150
    uint8_t water_temperature;   // 0–150
    uint16_t speed;              // or whatever range
} Dashboard;

typedef struct {
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
} UI_Area;

typedef enum {
    DASH_AREA_SMALL_LOGO = 0,
    DASH_AREA_BIG_LOGO
} DashAreaId;

void dash_init(double initial_battery_charge,
               double initial_cell_temperature,
               double initial_water_temperature,
               int initial_speed);

void dash_update(const Dashboard *d);

void draw_UGR_logo(void);
void draw_big_UGR_logo(void);

UI_Area dash_get_area(DashAreaId id);

#endif