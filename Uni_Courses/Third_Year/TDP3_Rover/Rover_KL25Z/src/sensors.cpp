#include "sensors.hpp"
#include "pin_definitions.hpp"
#include <cmath>

// Schmitt trigger thresholds
static constexpr float TH_ON  = 0.60f;
static constexpr float TH_OFF = 0.50f;

Sensors read_sensors() {
    HW::sensor_transistor = 1;

    Sensors s{};
    s.a[0] = HW::left_sensor_2.read();
    s.a[1] = HW::left_sensor_1.read();
    s.a[2] = HW::middle_sensor.read();
    s.a[3] = HW::right_sensor_1.read();
    s.a[4] = HW::right_sensor_2.read();

    static bool prev[5] = {false,false,false,false,false};

    for (int i = 0; i < 5; i++) {
        if (!prev[i] && s.a[i] >= TH_ON)  prev[i] = true;
        if ( prev[i] && s.a[i] <= TH_OFF) prev[i] = false;
        s.on[i] = prev[i];
    }

    return s;
}

LineInfo interpret(const Sensors& s) {
    static constexpr int w[5] = {-2, -1, 0, 1, 2};

    static int   last_active_sensor = 2;
    static float last_pos = 0.0f;

    LineInfo li{};

    for (int i = 0; i < 5; i++) {
        if (s.on[i]) {
            li.active_count++;
            li.pos_sum += w[i];
        }
    }

    if (li.active_count > 0) {
        li.pos = static_cast<float>(li.pos_sum) / li.active_count;
        last_pos = li.pos;

        // remember the furthest sensor currently seeing the line
        if (s.on[0])      last_active_sensor = 0;
        else if (s.on[4]) last_active_sensor = 4;
        else if (s.on[1]) last_active_sensor = 1;
        else if (s.on[3]) last_active_sensor = 3;
        else if (s.on[2]) last_active_sensor = 2;
    } else {
        li.pos = last_pos;
    }

    li.middle     = s.on[2];
    li.centered   = (li.active_count == 1 && s.on[2]);

    li.left_most  = s.on[0];
    li.right_most = s.on[4];

    // 90-degree corner: any 2+ sensors on at once
    li.junction = (li.active_count >= 2);

    // choose turn direction for the corner
    li.left_turn_sig  = li.junction && (li.pos < 0.0f);
    li.right_turn_sig = li.junction && (li.pos > 0.0f);

    // all sensors off
    li.fully_lost = (li.active_count == 0);

    // only SEEK if we lost it after it was already at an outermost sensor
    li.lost = li.fully_lost &&
              (last_active_sensor == 0 || last_active_sensor == 4);

    return li;
}