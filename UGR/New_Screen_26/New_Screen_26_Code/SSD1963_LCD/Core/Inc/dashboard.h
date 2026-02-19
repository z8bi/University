#pragma once
#include <stdint.h>

typedef struct {
    int16_t speed;            // km/h
    int16_t battery_charge;   // %
    int16_t cell_temperature; // °C
    int16_t water_temperature;// °C
} Dashboard;

void dash_init(void);
void dash_update(const Dashboard *d);
