#include "sensors.hpp"
#include <cmath>

// --- DEFINE the pin objects exactly once here ---
AnalogIn left_sensor_2(A4);
AnalogIn left_sensor_1(A3);
AnalogIn middle_sensor(A2);
AnalogIn right_sensor_2(A1);
AnalogIn right_sensor_1(A0);
DigitalOut sensor_transistor(PTC11);

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


LostHint classify_lost_hint(const Sensors& s) {
    // Adjacent pair "evidence" (sum and max)
    auto pair_score = [&](int i, int j) {
        float sum = s.a[i] + s.a[j];
        float mx  = (s.a[i] > s.a[j]) ? s.a[i] : s.a[j];
        // require both "somewhat high", not just one spike
        if (sum > GAP_SUM_TH && mx > GAP_MAX_TH) return sum;
        return 0.0f;
    };

    float sc01 = pair_score(0,1);
    float sc12 = pair_score(1,2);
    float sc23 = pair_score(2,3);
    float sc34 = pair_score(3,4);

    // pick strongest adjacent evidence
    float best = 0.0f;
    LostHint hint = LostHint::TRUE_LOST;

    if (sc01 > best) { best = sc01; hint = LostHint::GAP_L2_L1; }
    if (sc12 > best) { best = sc12; hint = LostHint::GAP_L1_M;  }
    if (sc23 > best) { best = sc23; hint = LostHint::GAP_M_R1;  }
    if (sc34 > best) { best = sc34; hint = LostHint::GAP_R1_R2; }

    return hint;
}

