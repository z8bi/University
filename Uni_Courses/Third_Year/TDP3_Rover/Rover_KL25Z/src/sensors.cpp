#include "sensors.hpp"
#include <cmath>

// --- DEFINE the pin objects exactly once here ---
extern AnalogIn left_sensor_2;
extern AnalogIn left_sensor_1;
extern AnalogIn middle_sensor;
extern AnalogIn right_sensor_2;
extern AnalogIn right_sensor_1;
extern DigitalOut sensor_transistor;

static constexpr float TH_ON  = 0.60f;
static constexpr float TH_OFF = 0.50f;

//Analogue thresholds for "gap" detection (classifying lost hints)
static constexpr float GAP_SUM_TH = 1.05f; // tune (depends on your surface)
static constexpr float GAP_MAX_TH = 0.55f; // tune


Sensors read_sensors() {
    Sensors s{};
    s.a[0] = left_sensor_2.read();
    s.a[1] = left_sensor_1.read();
    s.a[2] = middle_sensor.read();
    s.a[3] = right_sensor_1.read();
    s.a[4] = right_sensor_2.read();

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

    li.lost = (li.active_count == 0);
    li.pos  = (!li.lost) ? (static_cast<float>(li.pos_sum) / li.active_count) : 0.0f;

    li.centered = s.on[2] && !(s.on[1] || s.on[3]);

    li.right_turn_sig = (s.on[3] && s.on[4]) || (s.on[2] && s.on[4]);
    li.left_turn_sig  = (s.on[0] && s.on[1]) || (s.on[0] && s.on[2]);
    li.junction        = (s.on[2] && (li.right_turn_sig || li.left_turn_sig));

    return li;
}
