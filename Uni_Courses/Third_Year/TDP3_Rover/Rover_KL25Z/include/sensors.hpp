#pragma once
#include "mbed.h"
#include <cstdint>

struct Sensors {
    float a[5];  // raw analog 0..1
    bool  on[5]; // debounced digital
};

enum class LostHint {
    NONE,
    LEFT,
    RIGHT,
    TRUE_LOST
};

struct LineInfo {
    int   active_count = 0;
    int   pos_sum      = 0;
    float pos          = 0.0f;
    bool  centered     = false;
    bool  middle       = false;

    bool fully_lost = false; // no sensors active now
    bool lost       = false; // no sensors active now, and last active was outermost
    bool  searching = false;   // no sensors active, but we still know side

    bool junction       = false;
    bool right_turn_sig = false;
    bool left_turn_sig  = false;

    bool left_inner  = false;
    bool right_inner = false;
    bool right_most = false;
    bool left_most  = false;

    LostHint lost_hint = LostHint::NONE;
};

// --- function declarations ---
Sensors read_sensors();
LineInfo interpret(const Sensors& s);