#include "mbed.h"
#include "types.hpp"
#include "sensors.hpp"
#include "motors.hpp"
#include "ultrasonic_sensor.hpp"
#include "tcs3472.hpp"
#include "debug.hpp"
#include <cctype>

using namespace std::chrono_literals;

//======================================================
//====================== CONFIG ========================
//======================================================

namespace Config {

static constexpr int FOUND_TICKS = 1;

// Drive tunables
static constexpr float DUTY_FWD   = 0.35f;
static constexpr float DUTY_TURN  = 0.74f;

static constexpr float ALIGN_DUTY_TURN     = 0.80f;
static constexpr float TURN_BRAKE_STRENGTH = 0.90f;

// Gentler turning in FOLLOW
static constexpr float DUTY_FOLLOW_TURN      = 0.60f;
static constexpr float FOLLOW_BRAKE_STRENGTH = 0.70f;

//SEEK BRAKING
static constexpr float BRAKE_STRENGTH = 1.0f;
static constexpr int   BRAKE_MS       = 50;

// FULLY_STOPPED behavior (BT/manual hard stop)
static constexpr int   FULL_STOP_BRAKE_MS       = 120;  // tune: 60–200ms
static constexpr float FULL_STOP_BRAKE_STRENGTH = 1.0f; // tune: 0.7–1.0

// Controller timing
static constexpr int CTRL_PERIOD_MS         = 20;
static constexpr int FOLLOW_LOST_CONFIRM_MS = 40;
static constexpr int FOLLOW_LOST_TICKS      = FOLLOW_LOST_CONFIRM_MS / CTRL_PERIOD_MS;

static constexpr int ALIGN_LOST_CONFIRM_MS  = 100;
static constexpr int ALIGN_LOST_TICKS       = ALIGN_LOST_CONFIRM_MS / CTRL_PERIOD_MS;

static constexpr int SEEK_CENTER_LOCK_MS = 300;

// Ultrasonic
static constexpr int   ULTRA_PERIOD_MS  = 80;
static constexpr float FRONT_MAX_CM     = 200.0f;
static constexpr float RIGHT_MAX_CM     = 80.0f;
static constexpr int   ULTRA_STAGGER_MS = 8;

// Obstacle detection
static constexpr float OB_FRONT_TRIG_CM      = 15.0f;
static constexpr int   OB_CONFIRM_CTRL_TICKS = 2;

// Traffic light confirm at controller level
static constexpr int RED_CTRL_CONFIRM_TICKS = 2;

// STOPPED behavior
static constexpr float DUTY_REV_FIND_RED          = 0.25f;
static constexpr int   LOST_RED_TICKS_BEFORE_REV  = 5;  // 5*20ms = 100ms
static constexpr int   RED_REACQUIRE_TICKS        = 2;
static constexpr int   STOP_BRAKE_MS              = 500;

static constexpr float PWM_FREQ_HZ = 20000.0f;

//=======MANUAL========
// Manual BT (WASD) control
static constexpr float BT_DUTY_FWD  = 0.4f;
static constexpr float BT_DUTY_REV  = 0.3f;
static constexpr float BT_DUTY_TURN = 0.72f;

} // namespace Config

//======================================================
//================== GLOBAL HW OBJECTS =================
//======================================================

// State machine
CtrlState state = CtrlState::FOLLOW;
Kernel::Clock::time_point state_until;

static int last_pos = 0;
char c{}; //bluetooth input
static bool manual_mode = true;    

DigitalIn bt_state(PTE20); // pick your pin

bool bt_connected() {
    return bt_state.read() == 1;
}

// DAC output (unused in this file, but keep if you use it elsewhere)
AnalogOut dac(PTE30);

// RGB LEDs (KL25Z are active-low)
DigitalOut LED_R(LED_RED);
DigitalOut LED_G(LED_GREEN);
DigitalOut LED_B(LED_BLUE);

// Color sensor
tcs3472::TCS3472 color(PTE0, PTE1, 100000); // SDA=PTE0, SCL=PTE1 (if wired that way)

// Ultrasonic sensors
UltrasonicSensor us_front(D8,  D9);
UltrasonicSensor us_right(D10, D11);

//======================================================
//==================== ISR FLAGS =======================
//======================================================

Ticker debugTick;
Ticker controlTick;
Ticker ultraTick;
Ticker colorTick;

volatile bool control_due = false;
volatile bool ultra_due   = false;
volatile bool color_due   = false;
volatile bool debug_due = false;

static void control_isr() { control_due = true; }
static void ultra_isr()   { ultra_due   = true; }
static void color_isr()   { color_due   = true; }
static void debug_isr() { debug_due = true; }

//======================================================
//==================== SHARED DATA =====================
//======================================================

struct Shared {
    // Color
    tcs3472::RGBC rgbc{};
    bool rgbc_valid = false;
    LightState light = LightState::NONE;

    // Ultrasonic
    float front_cm = -1.0f;
    bool  front_ok = false;
    Kernel::Clock::time_point front_stamp{};

    float right_cm = -1.0f;
    bool  right_ok = false;
    Kernel::Clock::time_point right_stamp{};
};

static Shared g_shared;

//======================================================
//==================== HELPERS =========================
//======================================================

static inline void leds_set(bool r, bool g, bool b) {
    LED_R = !r;
    LED_G = !g;
    LED_B = !b;
}

// Generic debounce helper
static inline bool debounce(bool cond, int& counter, int threshold) {
    counter = cond ? (counter + 1) : 0;
    return counter >= threshold;
}

static void leds_for_state(CtrlState st) {
    switch (st) {
        case CtrlState::FOLLOW:         leds_set(false, true,  false); break;
        case CtrlState::SEEK_LEFT:      leds_set(true,  false, false); break;
        case CtrlState::SEEK_RIGHT:     leds_set(true,  false, false); break;
        case CtrlState::ALIGN:          leds_set(true,  true,  false); break;
        case CtrlState::BRAKING:        leds_set(false, false, true ); break;
        case CtrlState::OBSTACLE_AVOID: leds_set(false, true,  true ); break;
        case CtrlState::STOPPED:        leds_set(true,  false, true ); break;
        case CtrlState::FULLY_STOPPED:  leds_set(true, true, true);    break;
        default:                        leds_set(false, false, false); break;
    }
}

static void leds_for_manual_cmd(char cmd)
{
    // cmd is already uppercased in your BT handler
    switch (cmd) {
        case 'W': // forward
            leds_set(false, true,  false); // GREEN
            break;
        case 'S': // backward
            leds_set(true,  false, true ); // MAGENTA (R+B)
            break;
        case 'A': // turning
        case 'D':
            leds_set(true,  false, false); // RED
            break;
        default:
            // leave as-is
            break;
    }
}


static void enter_state(CtrlState st, int duration_ms) {
    core_util_critical_section_enter();
    state = st;
    state_until = Kernel::Clock::now() + std::chrono::milliseconds(duration_ms);
    core_util_critical_section_exit();

    leds_for_state(st);
    Debug::update_state(st);
}

static inline bool state_time_elapsed() {
    return Kernel::Clock::now() >= state_until;
}

static LightState classify_light(const tcs3472::RGBC& v) {
    if (!v.valid || v.c < 5000) return LightState::NONE;

    const int rP = (int)((100U * v.r) / v.c);
    const int gP = (int)((100U * v.g) / v.c);

    // Tuned from your measurements
    if ((rP - gP) >= 7 && rP >= 40) return LightState::RED;
    if ((gP - rP) >= 5 && gP >= 38) return LightState::GREEN;

    return LightState::NONE;
}

static inline bool should_ping_front(CtrlState st) {
    return (st == CtrlState::FOLLOW || st == CtrlState::ALIGN);
}

static inline bool should_ping_right(CtrlState st) {
    return (st != CtrlState::SEEK_LEFT && st != CtrlState::SEEK_RIGHT);
}

//======================================================
//================== CONTROLLER UPDATE =================
//======================================================

static Kernel::Clock::time_point stop_brake_until{};

static void controller_update() {
    Sensors s = read_sensors();
    LineInfo li = interpret(s);

    static CtrlState prev_state = CtrlState::FOLLOW;

    Debug::update_line(Debug::make_line(s, li, true));

    // ---- snapshot shared data (one critical section) ----
    Shared sh;
    core_util_critical_section_enter();
    sh = g_shared;
    core_util_critical_section_exit();

    const LightState ls = sh.light;

    //====SKIP STATE MACHINE IF USER IS IN CONTROL OVER BLUETOOTH (MANUAL MODE)====  

    if (manual_mode && state != CtrlState::FULLY_STOPPED) {
        return;
    }

    ///================ COLOR -> STOPPED (RED) ================
    static int red_ctrl_cnt = 0;
    if (ls == LightState::RED) red_ctrl_cnt++;
    else                       red_ctrl_cnt = 0;

    // Don't let traffic-light logic kick in if we're FULLY_STOPPED (BT/manual hold)
    if (state != CtrlState::FULLY_STOPPED &&
        state != CtrlState::STOPPED &&
        red_ctrl_cnt >= Config::RED_CTRL_CONFIRM_TICKS)
    {
        red_ctrl_cnt = 0;
        enter_state(CtrlState::STOPPED, 0);
        stop_brake_until = Kernel::Clock::now() + chrono::milliseconds(Config::STOP_BRAKE_MS);
        motors_brake(Config::BRAKE_STRENGTH);
        return;
    }


    //================ ULTRASONIC (front obstacle) ============
    const bool front_fresh = (Kernel::Clock::now() - sh.front_stamp) < 300ms;
    const bool front_hit =
        sh.front_ok && front_fresh && sh.front_cm > 0.0f && sh.front_cm < Config::OB_FRONT_TRIG_CM;

    static int ob_cnt = 0;
    if (state == CtrlState::FOLLOW || state == CtrlState::ALIGN) {
        if (front_hit) ob_cnt++;
        else           ob_cnt = 0;
    } else {
        ob_cnt = 0;
    }

    const bool obstacle_confirmed = (ob_cnt >= Config::OB_CONFIRM_CTRL_TICKS);

    if ((state == CtrlState::FOLLOW || state == CtrlState::ALIGN) && obstacle_confirmed) {
        enter_state(CtrlState::OBSTACLE_AVOID, 0);
        motors_brake(Config::BRAKE_STRENGTH);
        return;
    }

    //=========================== line debouncing ===========================

    static int found_cnt = 0;
    if (state == CtrlState::SEEK_LEFT || state == CtrlState::SEEK_RIGHT) {
        if (!li.lost) found_cnt++;
        else          found_cnt = 0;
    } else {
        found_cnt = 0;
    }
    const bool found_confirmed = (found_cnt >= Config::FOUND_TICKS);

    if (!li.lost) {
        if (li.pos > 0.2f)       last_pos = +1;
        else if (li.pos < -0.2f) last_pos = -1;
    }

    static int lost_cnt = 0;
    if (state == CtrlState::FOLLOW) {
        if (li.lost) lost_cnt++;
        else         lost_cnt = 0;
    } else {
        lost_cnt = 0;
    }
    const bool follow_lost_confirmed = (lost_cnt >= Config::FOLLOW_LOST_TICKS);

    static int align_lost_cnt = 0;
    if (state == CtrlState::ALIGN) {
        if (li.lost) align_lost_cnt++;
        else         align_lost_cnt = 0;
    } else {
        align_lost_cnt = 0;
    }
    const bool align_lost_confirmed = (align_lost_cnt >= Config::ALIGN_LOST_TICKS);

    // SEEK center-lock
    static bool seek_lock_active = false;
    static int  seek_lock_dir    = 0;
    static Kernel::Clock::time_point seek_lock_until;

    if (state != CtrlState::SEEK_LEFT && state != CtrlState::SEEK_RIGHT) {
        seek_lock_active = false;
        seek_lock_dir = 0;
    }

    // OBSTACLE reset flag
    static bool ob_reset_pending = false;
    if (state != prev_state) {
        if (state == CtrlState::OBSTACLE_AVOID) ob_reset_pending = true;
        prev_state = state;
    }

    //============================== FSM ==============================
    switch (state) {

    case CtrlState::FOLLOW: {
        if (follow_lost_confirmed) {
            enter_state(CtrlState::ALIGN, 0);
            break;
        }

        if (li.centered) {
            move_forward(Config::DUTY_FWD);
        } else {
            if (li.pos > 0.0f) turn_right_break_inner(Config::FOLLOW_BRAKE_STRENGTH, Config::DUTY_FOLLOW_TURN);
            else               turn_left_break_inner (Config::FOLLOW_BRAKE_STRENGTH, Config::DUTY_FOLLOW_TURN);
        }
        break;
    }

    case CtrlState::SEEK_LEFT: {
        if (seek_lock_active && seek_lock_dir != -1) { seek_lock_active = false; seek_lock_dir = 0; }

        if (!seek_lock_active && s.on[0]) {
            seek_lock_active = true;
            seek_lock_dir = -1;
            seek_lock_until = Kernel::Clock::now() + chrono::milliseconds(Config::SEEK_CENTER_LOCK_MS);
        }

        if (seek_lock_active && seek_lock_dir == -1) {
            if (li.centered) {
                seek_lock_active = false;
                enter_state(CtrlState::ALIGN, 0);
                break;
            }
            if (Kernel::Clock::now() < seek_lock_until) {
                turn_left_skid_reverse_inner(Config::DUTY_TURN);
                break;
            }
            seek_lock_active = false;
        }

        if (found_confirmed) {
            enter_state(CtrlState::ALIGN, 0);
            break;
        }
        turn_left_skid_reverse_inner(Config::DUTY_TURN);
        break;
    }

    case CtrlState::SEEK_RIGHT: {
        if (seek_lock_active && seek_lock_dir != +1) { seek_lock_active = false; seek_lock_dir = 0; }

        if (!seek_lock_active && s.on[4]) {
            seek_lock_active = true;
            seek_lock_dir = +1;
            seek_lock_until = Kernel::Clock::now() + chrono::milliseconds(Config::SEEK_CENTER_LOCK_MS);
        }

        if (seek_lock_active && seek_lock_dir == +1) {
            if (li.centered) {
                seek_lock_active = false;
                enter_state(CtrlState::ALIGN, 0);
                break;
            }
            if (Kernel::Clock::now() < seek_lock_until) {
                turn_right_skid_reverse_inner(Config::DUTY_TURN);
                break;
            }
            seek_lock_active = false;
        }

        if (found_confirmed) {
            enter_state(CtrlState::ALIGN, 0);
            break;
        }
        turn_right_skid_reverse_inner(Config::DUTY_TURN);
        break;
    }

    case CtrlState::ALIGN: {
        if (li.centered) {
            enter_state(CtrlState::FOLLOW, 0);
            move_forward(Config::DUTY_FWD);
            break;
        }

        if (align_lost_confirmed) {
            enter_state(CtrlState::BRAKING, Config::BRAKE_MS);
            motors_brake(Config::BRAKE_STRENGTH);
            break;
        }

        if (li.pos > 0.0f) turn_right_break_inner(Config::TURN_BRAKE_STRENGTH, Config::ALIGN_DUTY_TURN);
        else               turn_left_break_inner (Config::TURN_BRAKE_STRENGTH, Config::ALIGN_DUTY_TURN);
        break;
    }

    case CtrlState::BRAKING: {
        if (state_time_elapsed()) {
            motors_all_off();

            if (li.lost) {
                enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT : CtrlState::SEEK_LEFT, 0);
            } else {
                enter_state(CtrlState::ALIGN, 0);
            }
        } else {
            motors_brake(Config::BRAKE_STRENGTH);
        }
        break;
    }

    case CtrlState::OBSTACLE_AVOID: {
        enum class Phase { TURN_LEFT_45, GO_AROUND };
        static Phase phase = Phase::TURN_LEFT_45;
        static bool doing_forward = true;

        static constexpr int   TURN_45_MS     = 220;
        static constexpr float DUTY_OB_TURN   = 0.70f;
        static constexpr float DUTY_OB_FWD    = 0.40f;
        static constexpr float DUTY_OB_RIGHT  = 0.55f;
        static constexpr int   FWD_BURST_MS   = 180;
        static constexpr int   RIGHT_BURST_MS = 70;

        static Kernel::Clock::time_point phase_until;
        static int line_found_cnt = 0;

        if (!li.lost) line_found_cnt++;
        else          line_found_cnt = 0;

        if (ob_reset_pending) {
            ob_reset_pending = false;
            phase = Phase::TURN_LEFT_45;
            doing_forward = true;
            phase_until = {};
            line_found_cnt = 0;
        }

        if (line_found_cnt >= 2) {
            line_found_cnt = 0;
            enter_state(CtrlState::ALIGN, 0);
            break;
        }

        switch (phase) {
            case Phase::TURN_LEFT_45: {
                if (phase_until.time_since_epoch().count() == 0) {
                    phase_until = Kernel::Clock::now() + chrono::milliseconds(TURN_45_MS);
                }

                turn_left_skid_reverse_inner(DUTY_OB_TURN);

                if (Kernel::Clock::now() >= phase_until) {
                    phase_until = {};
                    phase = Phase::GO_AROUND;
                    doing_forward = true;
                }
                break;
            }

            case Phase::GO_AROUND:
            default: {
                if (phase_until.time_since_epoch().count() == 0) {
                    phase_until = Kernel::Clock::now() + chrono::milliseconds(FWD_BURST_MS);
                    doing_forward = true;
                }

                if (Kernel::Clock::now() >= phase_until) {
                    doing_forward = !doing_forward;
                    phase_until = Kernel::Clock::now() + chrono::milliseconds(
                        doing_forward ? FWD_BURST_MS : RIGHT_BURST_MS
                    );
                }

                if (doing_forward) move_forward(DUTY_OB_FWD);
                else               turn_right_skid_reverse_inner(DUTY_OB_RIGHT);
                break;
            }
        }

        break;
    }

    case CtrlState::STOPPED: {
        if (Kernel::Clock::now() < stop_brake_until) {
            motors_brake(Config::BRAKE_STRENGTH);
            break;
        }

        enum class StopPhase { HOLD_ON_RED, BACKUP_FIND_RED };
        static StopPhase sp = StopPhase::HOLD_ON_RED;

        static int no_red_cnt     = 0;
        static int red_reacq_cnt  = 0;
        static int green_cnt      = 0;

        if (ls == LightState::GREEN) green_cnt++;
        else                         green_cnt = 0;

        if (green_cnt >= 2) {
            green_cnt = 0;
            no_red_cnt = 0;
            red_reacq_cnt = 0;
            sp = StopPhase::HOLD_ON_RED;
            enter_state(CtrlState::FOLLOW, 0);
            break;
        }

        const bool red_now = (ls == LightState::RED);

        if (sp == StopPhase::HOLD_ON_RED) {
            motors_all_off();

            if (red_now) {
                no_red_cnt = 0;
            } else {
                no_red_cnt++;
                if (no_red_cnt >= Config::LOST_RED_TICKS_BEFORE_REV) {
                    no_red_cnt = 0;
                    red_reacq_cnt = 0;
                    sp = StopPhase::BACKUP_FIND_RED;
                }
            }
        } else {
            move_backward(Config::DUTY_REV_FIND_RED);

            if (red_now) red_reacq_cnt++;
            else         red_reacq_cnt = 0;

            if (red_reacq_cnt >= Config::RED_REACQUIRE_TICKS) {
                red_reacq_cnt = 0;
                sp = StopPhase::HOLD_ON_RED;
                motors_all_off();
            }
        }

        break;
    }
    case CtrlState::FULLY_STOPPED: {
        if (!state_time_elapsed()) motors_brake(Config::FULL_STOP_BRAKE_STRENGTH);
        else                       motors_all_off();
        break;
    }

    case CtrlState::MANUAL_BT: {
        break;
    }


    } // switch
}

//======================================================
//========================== MAIN ======================
//======================================================

int main() {
    sensor_transistor = 1; //Line sensing transistor on

    Debug::init(true, true, PTE22, PTE23, 9600); //USB + Bluetooth debugging

    motors_init(Config::PWM_FREQ_HZ);
    
    // Color Sensor init
    const bool tcs_ok = color.init(100.0f, tcs3472::Gain::X16);
    if (!tcs_ok) {
        // Optional: indicate sensor failure; keep running anyway
        Debug::log("TCS3472 init failed");
    }

    enter_state(CtrlState::FULLY_STOPPED, 0);
    
    //Init ticks and ISRs
    colorTick.attach(&color_isr, 100ms);
    controlTick.attach(&control_isr, chrono::milliseconds(Config::CTRL_PERIOD_MS));
    ultraTick.attach(&ultra_isr, chrono::milliseconds(Config::ULTRA_PERIOD_MS));
    debugTick.attach(&debug_isr, 1s);   // 1 Hz

    //Main loop: process ISRs
    while (true) {
        bool do_update = false;
        bool do_ultra  = false;
        bool do_color  = false;
        bool do_debug = false;

        core_util_critical_section_enter();
        if (control_due) { control_due = false; do_update = true; }
        if (ultra_due)   { ultra_due   = false; do_ultra  = true; }
        if (color_due)   { color_due   = false; do_color  = true; }
        if (debug_due)   { debug_due   = false; do_debug  = true; }
        core_util_critical_section_exit();

        //================ ULTRASONIC =================
        if (do_ultra) {
            CtrlState st;
            core_util_critical_section_enter();
            st = state;
            core_util_critical_section_exit();

            const bool do_front = should_ping_front(st);
            const bool do_right = should_ping_right(st);

            if (do_front) {
                auto rf = us_front.read_cm(Config::FRONT_MAX_CM);
                core_util_critical_section_enter();
                g_shared.front_cm    = rf.valid ? rf.cm : -1.0f;
                g_shared.front_ok    = rf.valid;
                g_shared.front_stamp = Kernel::Clock::now();
                core_util_critical_section_exit();
            }

            if (do_front && do_right) {
                ThisThread::sleep_for(chrono::milliseconds(Config::ULTRA_STAGGER_MS));
            }

            if (do_right) {
                auto rr = us_right.read_cm(Config::RIGHT_MAX_CM);
                core_util_critical_section_enter();
                g_shared.right_cm    = rr.valid ? rr.cm : -1.0f;
                g_shared.right_ok    = rr.valid;
                g_shared.right_stamp = Kernel::Clock::now();
                core_util_critical_section_exit();
            }

            Shared sh;
            core_util_critical_section_enter();
            sh = g_shared;
            core_util_critical_section_exit();

            Debug::update_ultra(Debug::make_ultra(sh.front_cm, sh.front_ok, sh.right_cm, sh.right_ok));
        }

        //================ COLOR =================
        if (do_color) {
            auto v = color.read_raw(true, 120);

            static LightState cand = LightState::NONE;
            static int cand_cnt = 0;

            if (v.valid) {
                const LightState s = classify_light(v);

                if (s == cand) cand_cnt++;
                else { cand = s; cand_cnt = 1; }

                core_util_critical_section_enter();
                g_shared.rgbc = v;
                g_shared.rgbc_valid = true;
                g_shared.light = cand;
                core_util_critical_section_exit();
            } else {
                core_util_critical_section_enter();
                g_shared.rgbc_valid = false;
                core_util_critical_section_exit();
            }

            // Debug snapshot
            Shared sh;
            core_util_critical_section_enter();
            sh = g_shared;
            core_util_critical_section_exit();

            Debug::update_color(Debug::make_color(sh.rgbc, sh.rgbc_valid, sh.light));
        }

        //================ CONTROL =================
        if (do_update) {
            controller_update();
        }

        if (do_debug) {
            Debug::tick();
        }

        //Do bluetooth stuff
        //================ BT: enter manual mode on drive keys ================
        //================ BT: manual control =================
        if (bt_connected() && Debug::bt_read_char(c)) {

            // ignore CR/LF
            if (c == '\r' || c == '\n' || c < 32 || c > 126) {
                // do nothing
            } else {

                // make lowercase work
                c = (char)toupper((unsigned char)c);

                // USB-visible ACK (so you can confirm chars are arriving)
                char buf[32];
                snprintf(buf, sizeof(buf), "BT RX: %c\r\n", c);
                Debug::log(buf);

                // If your Debug library has a BT print, you can also echo on BT:
                // Debug::bt_write(buf);  // <-- uncomment ONLY if this exists

                if (c == 'W' || c == 'A' || c == 'S' || c == 'D') {
                    manual_mode = true;
                    Debug::log("BT: MANUAL MODE\r\n");

                    enter_state(CtrlState::MANUAL_BT, 0);

                    leds_for_manual_cmd(c);   // <-- add this

                    if (c == 'W') move_forward(Config::BT_DUTY_FWD);
                    if (c == 'S') move_backward(Config::BT_DUTY_REV);
                    if (c == 'A') turn_left_skid_reverse_inner(Config::BT_DUTY_TURN);
                    if (c == 'D') turn_right_skid_reverse_inner(Config::BT_DUTY_TURN);
                }
                else if (c == 'Q') {
                    // Hard stop
                    manual_mode = true; // keep manual locked
                    enter_state(CtrlState::FULLY_STOPPED, Config::FULL_STOP_BRAKE_MS);
                    motors_brake(Config::FULL_STOP_BRAKE_STRENGTH); // immediate
                    Debug::log("BT: FULLY_STOPPED\r\n");
                }
                else if (c == 'X') {
                    // Exit manual + re-seek
                    manual_mode = false;
                    enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT : CtrlState::SEEK_LEFT, 0);
                    Debug::log("BT: EXIT MANUAL -> SEEK\r\n");
                }
                else if (c == 'H') {
                    Debug::log("BT: WASD=manual, Q=hard stop, X=exit manual+seek\r\n");
                }
                else {
                    Debug::log("BT: unknown cmd\r\n");
                }
            }
        }


        ThisThread::sleep_for(1ms);
    }
    
}
