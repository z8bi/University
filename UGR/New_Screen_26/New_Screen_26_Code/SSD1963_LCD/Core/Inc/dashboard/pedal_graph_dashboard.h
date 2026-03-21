#ifndef PEDAL_GRAPH_DASHBOARD_H
#define PEDAL_GRAPH_DASHBOARD_H

#include "dashboard.h"

void pedal_graph_dash_init(double initial_battery_charge,
                           double initial_cell_temperature,
                           int initial_lap,
                           int initial_speed);

void pedal_graph_dash_update(const Dashboard *d);

#endif