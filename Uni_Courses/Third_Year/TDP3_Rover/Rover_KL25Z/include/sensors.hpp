#pragma once
#include "mbed.h"
#include <cstdint>

struct Sensors {
    float a[5];  // raw analog 0..1
    bool  on[5]; // debounced digital
};

struct LineInfo {
    int   active_count = 0;
    int   pos_sum      = 0;
    float pos          = 0.0f;
    bool  centered     = false;
    bool  middle = false;
    bool  lost         = false;

    // you can delete these later if truly unused
    bool junction       = false;
    bool right_turn_sig = false;
    bool left_turn_sig  = false;
};

enum class LostHint {
    TRUE_LOST,
    GAP_L1_M,   // between left1 and middle
    GAP_M_R1,   // between middle and right1
    GAP_R1_R2,  // between right1 and right2
    GAP_L2_L1,  // between left2 and left1
};

// --- function declarations ---
Sensors read_sensors();
LineInfo interpret(const Sensors& s);
LostHint classify_lost_hint(const Sensors& s);
