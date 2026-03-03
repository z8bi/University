#include "sensors.hpp"

#include "pin_definitions.hpp"
#include <cmath>


//Shmidtt trigger thresholds
static constexpr float TH_ON  = 0.60f;
static constexpr float TH_OFF = 0.50f;

Sensors read_sensors() {
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
    LineInfo li{};

    for (int i = 0; i < 5; i++) {
        if (s.on[i]) {
            li.active_count++;
            li.pos_sum += w[i];
        }
    }

    //lost if no sensors are active
    li.lost = (li.active_count == 0);

    //tracks the direction of the last known position
    li.pos  = (!li.lost) ? (static_cast<float>(li.pos_sum) / li.active_count) : 0.0f;

    //centered if the middle sensor is on the line and the two adjacent aren't
    li.centered = s.on[2] && !(s.on[1] || s.on[3]);

    // used to check for right degree turns
    li.right_turn_sig = (s.on[3] && s.on[4]) || (s.on[2] && s.on[4]);
    li.left_turn_sig  = (s.on[0] && s.on[1]) || (s.on[0] && s.on[2]);
    li.junction        = (s.on[2] && (li.right_turn_sig || li.left_turn_sig));
    //
    

    return li;
}
