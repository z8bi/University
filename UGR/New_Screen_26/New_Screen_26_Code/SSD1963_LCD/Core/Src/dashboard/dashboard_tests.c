#include "dashboard_tests.h"

#include <string.h>

enum {
    SIM_COAST = 0,
    SIM_ACCEL,
    SIM_BRAKE
};

static int sim_clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int sim_approach(int current, int target, int up_step, int down_step)
{
    if (current < target) {
        current += up_step;
        if (current > target) {
            current = target;
        }
    } else if (current > target) {
        current -= down_step;
        if (current < target) {
            current = target;
        }
    }

    return current;
}

static uint32_t sim_rand_u32(void)
{
    static uint32_t s = 0x12345678u;
    s = s * 1664525u + 1013904223u;
    return s;
}

static int sim_rand_range(int min_v, int max_v)
{
    uint32_t r;

    if (max_v <= min_v) {
        return min_v;
    }

    r = sim_rand_u32();
    return min_v + (int)(r % (uint32_t)(max_v - min_v + 1));
}

static void pick_next_drive_state(DashboardTestSim *sim, uint32_t now_ms)
{
    int pick = sim_rand_range(0, 99);

    if (pick < 45) {
        sim->drive_state = SIM_ACCEL;
        sim->throttle_target = sim_rand_range(25, 100);
        sim->brake_target = 0;
        sim->state_end_ms = now_ms + (uint32_t)sim_rand_range(500, 1800);
    }
    else if (pick < 75) {
        sim->drive_state = SIM_COAST;
        sim->throttle_target = sim_rand_range(0, 8);
        sim->brake_target = 0;
        sim->state_end_ms = now_ms + (uint32_t)sim_rand_range(400, 1400);
    }
    else {
        sim->drive_state = SIM_BRAKE;
        sim->throttle_target = 0;
        sim->brake_target = sim_rand_range(20, 95);
        sim->state_end_ms = now_ms + (uint32_t)sim_rand_range(250, 800);
    }
}

static void sim_pedals(DashboardTestSim *sim, Dashboard *d, uint32_t now_ms)
{
    int throttle_up = 0;
    int throttle_down = 0;
    int brake_up = 0;
    int brake_down = 0;

    if (now_ms >= sim->state_end_ms) {
        pick_next_drive_state(sim, now_ms);
    }

    switch (sim->drive_state) {
        case SIM_ACCEL:
            throttle_up = 4;
            throttle_down = 2;
            brake_up = 1;
            brake_down = 5;
            break;

        case SIM_COAST:
            throttle_up = 1;
            throttle_down = 3;
            brake_up = 1;
            brake_down = 4;
            break;

        case SIM_BRAKE:
            throttle_up = 1;
            throttle_down = 7;
            brake_up = 5;
            brake_down = 3;
            break;

        default:
            break;
    }

    d->throttle_percent = sim_approach(d->throttle_percent,
                                       sim->throttle_target,
                                       throttle_up,
                                       throttle_down);

    d->brake_percent = sim_approach(d->brake_percent,
                                    sim->brake_target,
                                    brake_up,
                                    brake_down);

    /* avoid unrealistic simultaneous throttle + brake */
    if (d->brake_percent > 10) {
        d->throttle_percent = sim_clamp(d->throttle_percent - 3, 0, 100);
    }

    if (d->throttle_percent > 20) {
        d->brake_percent = sim_clamp(d->brake_percent - 2, 0, 100);
    }

    /* tiny jitter */
    d->throttle_percent = sim_clamp(d->throttle_percent + sim_rand_range(-1, 1), 0, 100);
    d->brake_percent    = sim_clamp(d->brake_percent + sim_rand_range(-1, 1), 0, 100);
}

static void sim_speed(Dashboard *d)
{
    int speed = (int)d->speed;
    int accel_term = d->throttle_percent / 7;
    int brake_term = d->brake_percent / 4;
    int drag_term = speed / 90;

    speed += accel_term;
    speed -= brake_term;
    speed -= drag_term;

    speed = sim_clamp(speed, 0, 350);
    d->speed = (uint16_t)speed;
}

static void sim_soc(DashboardTestSim *sim, Dashboard *d)
{
    sim->soc_tick++;

    /* very slow discharge */
    if (sim->soc_tick >= 200) {
        sim->soc_tick = 0;

        if (d->battery_charge > 0) {
            d->battery_charge--;
        }
    }
}

static void sim_cell_temp(Dashboard *d)
{
    int temp = d->cell_temperature;
    int rise = d->throttle_percent / 30;
    int cool = 0;

    if (d->speed > 40) {
        cool = 1;
    }
    if (d->speed > 100) {
        cool = 2;
    }

    temp += rise;
    temp -= cool;
    temp += sim_rand_range(-1, 1);

    temp = sim_clamp(temp, 20, 95);
    d->cell_temperature = temp;
}

static void sim_water_temp(Dashboard *d)
{
    int temp = d->water_temperature;
    int rise = d->throttle_percent / 40;
    int cool = 0;

    if (d->speed > 30) {
        cool = 1;
    }
    if (d->speed > 120) {
        cool = 2;
    }

    temp += rise;
    temp -= cool;
    temp += sim_rand_range(-1, 1);

    temp = sim_clamp(temp, 20, 110);
    d->water_temperature = temp;
}

static void sim_lap(DashboardTestSim *sim, Dashboard *d)
{
    sim->lap_tick++;

    /* ~12 seconds per lap at 50 Hz */
    if (sim->lap_tick >= 600) {
        sim->lap_tick = 0;
        d->lap = (d->lap + 1) % 23;
    }
}

void dashboard_test_sim_init(DashboardTestSim *sim, Dashboard *d, uint32_t now_ms)
{
    if (!sim || !d) {
        return;
    }

    memset(sim, 0, sizeof(*sim));

    d->lap = 0;
    d->battery_charge = 100;
    d->cell_temperature = 25;
    d->water_temperature = 20;
    d->speed = 0;
    d->throttle_percent = 0;
    d->brake_percent = 0;

    sim->initialized = 1;
    sim->last_update_ms = now_ms;

    pick_next_drive_state(sim, now_ms);
}

void dashboard_test_sim_step(DashboardTestSim *sim, Dashboard *d, uint32_t now_ms)
{
    if (!sim || !d) {
        return;
    }

    if (!sim->initialized) {
        dashboard_test_sim_init(sim, d, now_ms);
    }

    sim_pedals(sim, d, now_ms);
    sim_speed(d);
    sim_soc(sim, d);
    sim_cell_temp(d);
    sim_water_temp(d);
    sim_lap(sim, d);

    sim->last_update_ms = now_ms;
}