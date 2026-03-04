//======================================================
//====================== CONFIG ========================
//======================================================

namespace Config {

// Main PWM frequency (for motors)
static constexpr float PWM_FREQ_HZ = 20000.0f;

// Periodic task timings
static constexpr int CTRL_PERIOD_MS  = 5;
static constexpr int DEBUG_PERIOD_MS = 100;
static constexpr int COLOR_PERIOD_MS = 50;
static constexpr int ULTRA_PERIOD_MS = 50; //actually 100 per sensor, each function run only updates one of the sensors

// Debounce / confirm
static constexpr int FOUND_TICKS = 1;

// Drive tunables
static constexpr float DUTY_FWD  = 0.75f;
static constexpr float DUTY_TURN = 0.77f;

// Align parameters
static constexpr float ALIGN_DUTY_TURN     = 0.82f;
static constexpr float TURN_BRAKE_STRENGTH = 0.90f;

// Follow gentle turning
static constexpr float DUTY_FOLLOW_TURN      = 0.7f;
static constexpr float FOLLOW_BRAKE_STRENGTH = 0.8f;

// SEEK braking
static constexpr float BRAKE_STRENGTH = 1.0f;
static constexpr int   BRAKE_MS       = 50;

// FULLY_STOPPED behavior
static constexpr int   FULL_STOP_BRAKE_MS       = 200;
static constexpr float FULL_STOP_BRAKE_STRENGTH = 1.0f;

// Lost-line confirm windows
static constexpr int FOLLOW_LOST_TICKS     = 2;
static constexpr int ALIGN_LOST_TICKS      = 2;

// SEEK center-lock
static constexpr int SEEK_CENTER_LOCK_MS = 300;

// Ultrasonic
static constexpr float FRONT_MAX_CM     = 40.0f;
static constexpr float RIGHT_MAX_CM     = 40.0f;

// Obstacle avoid
static constexpr float OB_FRONT_TRIG_CM      = 30.0f;
static constexpr float OB_RIGHT_TRIG_CM      = 45.0f;

static constexpr float OB_FRONT_CLOSE_CM     = 20.0f; 
static constexpr float OB_FRONT_CLOSER_SPEED = 0.30f; 

static constexpr int   OB_PHASE_MIN_MS       = 2000; 
static constexpr int   OB_SEEK_DURATION      = 350;
static constexpr int   OB_CONFIRM_CTRL_TICKS = 1;
static constexpr int   OB_BRAKE_MS           = 700;
static constexpr int   TURN_45_MS            = 1050;
static constexpr float DUTY_OB_TURN          = 0.73f;
static constexpr float DUTY_OB_FWD           = 0.35f;
static constexpr float DUTY_OB_RIGHT         = 0.68f;

// Traffic light
static constexpr int RED_CTRL_CONFIRM_TICKS = 2;
static constexpr int COLOR_INTENSITY        = 1000;

// STOPPED (red light) behavior
static constexpr float DUTY_REV_FIND_RED         = 0.25f;
static constexpr int   LOST_RED_TICKS_BEFORE_REV = 10;
static constexpr int   RED_REACQUIRE_TICKS       = 2;
static constexpr int   STOP_BRAKE_MS             = 350;

// Manual BT
static constexpr float BT_DUTY_FWD  = 0.6f;
static constexpr float BT_DUTY_FWD_SLOW  = 0.4f;
static constexpr float BT_DUTY_REV  = 0.6f;
static constexpr float BT_DUTY_REV_SLOW  = 0.4f;
static constexpr float BT_DUTY_TURN = 0.75f;

} // namespace Config
