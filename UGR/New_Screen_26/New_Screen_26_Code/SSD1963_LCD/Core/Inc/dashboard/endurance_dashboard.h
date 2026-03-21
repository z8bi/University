#ifndef ENDURANCE_DASHBOARD_H
#define ENDURANCE_DASHBOARD_H

#include "dashboard.h"

void endurance_dash_init(double initial_battery_charge,
                         double initial_cell_temperature,
                         int initial_lap,
                         int initial_speed);

void endurance_dash_update(const Dashboard *d);

#endif