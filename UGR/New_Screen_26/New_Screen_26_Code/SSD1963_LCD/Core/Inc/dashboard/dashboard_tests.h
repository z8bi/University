#ifndef DASHBOARD_TESTS_H
#define DASHBOARD_TESTS_H

#include <stdint.h>
#include "dashboard_core.h"

typedef struct {
    uint8_t initialized;
    uint32_t last_update_ms;

    uint32_t state_end_ms;
    uint8_t drive_state;

    int throttle_target;
    int brake_target;

    uint32_t soc_tick;
    uint32_t lap_tick;
} DashboardTestSim;

void dashboard_test_sim_init(DashboardTestSim *sim, Dashboard *d, uint32_t now_ms);
void dashboard_test_sim_step(DashboardTestSim *sim, Dashboard *d, uint32_t now_ms);

#endif