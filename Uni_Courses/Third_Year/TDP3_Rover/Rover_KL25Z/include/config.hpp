#pragma once

enum class SpeedMode {
    Slow = 0,
    Fast = 1
};

// Defined in one .cpp file
extern SpeedMode speed_mode;

namespace Config {

// Main PWM frequency
static constexpr float PWM_FREQ_HZ = 20000.0f;

// Periodic task timings
static constexpr int CTRL_PERIOD_MS  = 5;
static constexpr int DEBUG_PERIOD_MS = 100;
static constexpr int COLOR_PERIOD_MS = 50;
static constexpr int ULTRA_PERIOD_MS = 50;

// Debounce / confirm
static constexpr int FOUND_TICKS = 1;

// ---------- Speed-dependent values ----------
inline float DUTY_FWD() {
    return (speed_mode == SpeedMode::Fast) ? 0.85f : 0.60f;
}

inline float DUTY_TURN() {
    return (speed_mode == SpeedMode::Fast) ? 0.85f : 0.75f;
}

inline float ALIGN_DUTY_TURN() {
    return 0.82f;
}

inline float TURN_BRAKE_STRENGTH() {
    return 0.90f;
}

inline float BRAKE_STRENGTH() {
    return 1.0f;
}

inline int BRAKE_MS_FOLLOW() {
    return (speed_mode == SpeedMode::Fast) ? 200 : 200;
}

inline int BRAKE_MS_ALIGN() {
    return (speed_mode == SpeedMode::Fast) ? 200 : 200;
}
// -------------------------------------------

// FULLY_STOPPED behavior
static constexpr int   FULL_STOP_BRAKE_MS       = 300;
static constexpr float FULL_STOP_BRAKE_STRENGTH = 1.0f;

// Lost-line confirm windows
static constexpr int FOLLOW_LOST_TICKS = 2;
static constexpr int ALIGN_LOST_TICKS  = 2;

// SEEK center-lock
static constexpr int SEEK_CENTER_LOCK_MS = 300;

// Ultrasonic
static constexpr float FRONT_MAX_CM = 40.0f;
static constexpr float RIGHT_MAX_CM = 40.0f;

// Obstacle avoid
static constexpr float OB_FRONT_TRIG_CM      = 35.0f;
static constexpr float OB_RIGHT_TRIG_CM      = 45.0f;
static constexpr float OB_FRONT_CLOSE_CM     = 25.0f;
static constexpr float OB_FRONT_CLOSER_SPEED = 0.30f;

static constexpr int   OB_PHASE_MIN_MS       = 2000;
static constexpr int   OB_SEEK_DURATION      = 350;
static constexpr int   OB_CONFIRM_CTRL_TICKS = 1;
static constexpr int   OB_BRAKE_MS           = 500;
static constexpr int   TURN_45_MS            = 700;
static constexpr float DUTY_OB_TURN          = 0.73f;
static constexpr float DUTY_OB_FWD           = 0.35f;
static constexpr float DUTY_OB_RIGHT         = 0.68f;

// Traffic light
static constexpr int RED_CTRL_CONFIRM_TICKS = 1;
static constexpr int COLOR_INTENSITY        = 1000;

// STOPPED (red light) behavior
static constexpr float DUTY_REV_FIND_RED         = 0.30f;
static constexpr int   LOST_RED_TICKS_BEFORE_REV = 10;
static constexpr int   RED_REACQUIRE_TICKS       = 2;
static constexpr int   STOP_BRAKE_MS             = 350;

// Manual BT
static constexpr float BT_DUTY_FWD      = 0.6f;
static constexpr float BT_DUTY_FWD_SLOW = 0.4f;
static constexpr float BT_DUTY_REV      = 0.6f;
static constexpr float BT_DUTY_REV_SLOW = 0.4f;
static constexpr float BT_DUTY_TURN     = 0.75f;

} // namespace Config