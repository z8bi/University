#include "mbed.h"
#include "types.hpp"
#include "sensors.hpp"
#include "motors.hpp"
#include "ultrasonic_sensor.hpp"
#include "tcs3472.hpp"
#include "debug.hpp"
#include "audio.hpp"
#include "sounds.hpp"

using namespace std::chrono_literals;

//======================================================
//====================== CONFIG ========================
//======================================================

namespace Config {

//Main PWM frequency (for motors)
static constexpr float PWM_FREQ_HZ = 20000.0f;

// Sensor reading timings
static constexpr int CTRL_PERIOD_MS = 10;
static constexpr int DEBUG_PERIOD_MS = 200;
static constexpr int COLOR_PERIOD_MS = 100;
static constexpr int ULTRA_PERIOD_MS  = 80;

// # of ticks of confirmed "found line" before SEEK->ALIGN
static constexpr int FOUND_TICKS = 1;

// Drive tunables
static constexpr float DUTY_FWD   = 0.75f;
static constexpr float DUTY_TURN  = 0.77f;

//Align parameters
static constexpr float ALIGN_DUTY_TURN     = 0.82f;
static constexpr float TURN_BRAKE_STRENGTH = 0.90f;

// Gentler turning in FOLLOW
static constexpr float DUTY_FOLLOW_TURN      = 0.7f;
static constexpr float FOLLOW_BRAKE_STRENGTH = 0.8;

//SEEK BRAKING
static constexpr float BRAKE_STRENGTH = 1.0f;
static constexpr int   BRAKE_MS       = 50;

// FULLY_STOPPED behavior (BT/manual hard stop)
static constexpr int   FULL_STOP_BRAKE_MS       = 200;  // tune: 60–200ms
static constexpr float FULL_STOP_BRAKE_STRENGTH = 1.0f; // tune: 0.7–1.0

//Align and Follow parameters
static constexpr int FOLLOW_LOST_CONFIRM_MS = 30;
static constexpr int FOLLOW_LOST_TICKS      = FOLLOW_LOST_CONFIRM_MS / CTRL_PERIOD_MS;

static constexpr int ALIGN_LOST_CONFIRM_MS  = 30;
static constexpr int ALIGN_LOST_TICKS       = ALIGN_LOST_CONFIRM_MS / CTRL_PERIOD_MS;

//How long does SEEK look for the center of the line
static constexpr int SEEK_CENTER_LOCK_MS = 300;

// Ultrasonic
static constexpr float FRONT_MAX_CM     = 40.0f;
static constexpr float RIGHT_MAX_CM     = 40.0f;
static constexpr int   ULTRA_STAGGER_MS = 8;

//============OBSTACLE AVOID===========
static constexpr float OB_FRONT_TRIG_CM      = 35.0f;
static constexpr float OB_RIGHT_TRIG_CM      = 30.0f;
static constexpr int   OB_CONFIRM_CTRL_TICKS = 1;
static constexpr int   OB_BRAKE_MS           = 150;
static constexpr int   TURN_45_MS     = 800;
static constexpr float DUTY_OB_TURN   = 0.75f;
static constexpr float DUTY_OB_FWD    = 0.40f;
static constexpr float DUTY_OB_RIGHT  = 0.75f;

// Traffic light confirm at controller level
static constexpr int RED_CTRL_CONFIRM_TICKS = 2;
static constexpr int COLOR_INTENSITY = 6000;

// STOPPED behavior for red light: how long to reverse before trying to find red again
static constexpr float DUTY_REV_FIND_RED          = 0.30f;
static constexpr int   LOST_RED_TICKS_BEFORE_REV  = 10;  // 5*20ms = 100ms
static constexpr int   RED_REACQUIRE_TICKS        = 2;
static constexpr int   STOP_BRAKE_MS              = 250;



//=======MANUAL========
// Manual BT (WASD) control
static constexpr float BT_DUTY_FWD  = 0.6f;
static constexpr float BT_DUTY_REV  = 0.6f;
static constexpr float BT_DUTY_TURN = 0.75f;

} // namespace Config

//======================================================
//================== GLOBAL HW OBJECTS =================
//======================================================

// State machine timers
CtrlState state = CtrlState::FOLLOW;
Kernel::Clock::time_point state_until;
static Kernel::Clock::time_point stop_brake_until{};

static int last_pos = 0;
char c{}; //bluetooth input 
static bool manual_mode = false;    

//=========================================
//================== PINS =================
//=========================================

// DAC output (unused in this file, but keep if you use it elsewhere)
AudioPlayer audio(PTE30);

// Line sensors
AnalogIn left_sensor_2(A0);
AnalogIn left_sensor_1(A1);
AnalogIn middle_sensor(A2);
AnalogIn right_sensor_1(A3);
AnalogIn right_sensor_2(A4);
DigitalOut sensor_transistor(PTC11);

//Motors
DigitalOut right_in1(D7);
DigitalOut right_in2(D6);
PwmOut     right_pwm(D5);
DigitalOut left_in2(D4);
DigitalOut left_in1(D3);
PwmOut     left_pwm(D2);

// RGB LEDs (KL25Z are active-low)
DigitalOut LED_R(LED_RED);
DigitalOut LED_G(LED_GREEN);
DigitalOut LED_B(LED_BLUE);

// Color sensor
tcs3472::TCS3472 color(PTE0, PTE1, 100000); // SDA=PTE0, SCL=PTE1 (if wired that way)

// Ultrasonic sensors - DECLARED IN MAIN!
//UltrasonicSensor us_front(D8,  D9);
//UltrasonicSensor us_right(D10, D11);

//Bluetooth state
DigitalIn bt_state(PTE21);
static constexpr PinName BT_TX = PTE22;  // KL25Z TX -> BT RX
static constexpr PinName BT_RX = PTE23;  // KL25Z RX -> BT TX
static constexpr int     BT_BAUD = 115200;

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

// Shared data used by DEBUG for printing to USB and BT

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

bool bt_connected() {
    return bt_state.read() == 1;
}

static inline void leds_set(bool r, bool g, bool b) {
    LED_R = !r;
    LED_G = !g;
    LED_B = !b;
}

// Generic debounce helper - helps avoid issues due to sudden error spikes in readings which last only one reading
static inline bool debounce(bool cond, int& counter, int threshold) {
    counter = cond ? (counter + 1) : 0;
    return counter >= threshold;
}

//helper to read shared data with critical section protection
static inline Shared shared_snapshot()
{
    Shared sh;
    core_util_critical_section_enter();
    sh = g_shared;
    core_util_critical_section_exit();
    return sh;
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

enum class ObPhase {
    TURN_LEFT_45,
    SEARCH_RIGHT_WALL,
    FOLLOW_UNTIL_LOST,
    REACQUIRE
};

static inline void leds_for_ob_phase(ObPhase p)
{
    // pick any mapping you like; these are just clear/unique
    switch (p) {
        case ObPhase::TURN_LEFT_45:      leds_set(true,  false, false); break; // RED
        case ObPhase::SEARCH_RIGHT_WALL: leds_set(false, false, true ); break; // BLUE
        case ObPhase::FOLLOW_UNTIL_LOST: leds_set(false, true,  true ); break; // CYAN
        case ObPhase::REACQUIRE:         leds_set(true,  true,  false); break; // YELLOW
        default:                         leds_set(false, true,  true ); break; // fallback
    }
}

//helps change LED's when we're in manual control
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
            break;
    }
}

//runs to change state and for how long
static void enter_state(CtrlState st, int duration_ms) {
    core_util_critical_section_enter();
    state = st;
    state_until = Kernel::Clock::now() + std::chrono::milliseconds(duration_ms);
    core_util_critical_section_exit();

    leds_for_state(st);
    Debug::update_state(st);
}

//checks for elapsed time 
static inline bool state_time_elapsed() {
    return Kernel::Clock::now() >= state_until;
}

//helper to check if red or green is spotted
static LightState classify_light(const tcs3472::RGBC& v) {
    //IF LIGHT IS TOO DIM THEN CHANGE V.C LOWER
    if (!v.valid || v.c < Config::COLOR_INTENSITY) return LightState::NONE;

    const int rP = (int)((100U * v.r) / v.c);
    const int gP = (int)((100U * v.g) / v.c);

    // Tuned from your measurements
    if ((rP - gP) >= 7 && rP >= 40) return LightState::RED;
    if ((gP - rP) >= 5 && gP >= 38) return LightState::GREEN;

    return LightState::NONE;
}

//is the state correct to ping front or right sensor -- helps avoid unnecessary wait times from the sound travelling if reading is not neccessary
static inline bool should_ping_front(CtrlState st) {
    return (st == CtrlState::FULLY_STOPPED || st == CtrlState::FOLLOW || st == CtrlState::ALIGN || st == CtrlState::OBSTACLE_AVOID || st == CtrlState::SEEK_LEFT || st == CtrlState::SEEK_RIGHT);
}

static inline bool should_ping_right(CtrlState st) {
    return (st == CtrlState::FULLY_STOPPED || st == CtrlState::OBSTACLE_AVOID);
}

//======================================================
//================== CONTROLLER UPDATE =================
//======================================================

static void controller_update() {
    Sensors s = read_sensors();
    LineInfo li = interpret(s);

    static CtrlState prev_state = CtrlState::FOLLOW;

    Debug::update_line(Debug::make_line(s, li, true));

    const Shared sh = shared_snapshot();
    const LightState ls = sh.light;

    if (manual_mode && state != CtrlState::FULLY_STOPPED) return;

    //================================================
    //======= STATIC STATE (declare once) ============
    //================================================
    static bool ob_reset_pending = false;
    static bool ob_active        = false;  

    static int  red_ctrl_cnt   = 0;
    static int  ob_cnt         = 0;
    static int  found_cnt      = 0;
    static int  lost_cnt       = 0;
    static int  align_lost_cnt = 0;

    static bool seek_lock_active = false;
    static int  seek_lock_dir    = 0;
    static Kernel::Clock::time_point seek_lock_until;

    //================================================
    //======= MODE / ENTRY DETECT (always runs) ======
    //================================================
    const bool lock_ob = ob_active;  

    if (state != prev_state) {
        if (state == CtrlState::OBSTACLE_AVOID) ob_reset_pending = true;
        prev_state = state;
    }

    if (ob_active && state != CtrlState::OBSTACLE_AVOID) {
        enter_state(CtrlState::OBSTACLE_AVOID, 0);
    }

    //================================================
    //======= COMPUTED FLAGS (defaults if locked) =====
    //================================================
    bool red_confirmed         = false;
    bool obstacle_confirmed    = false;
    bool found_confirmed       = false;
    bool follow_lost_confirmed = false;
    bool align_lost_confirmed  = false;

    bool front_hit   = false;
    bool front_fresh = false;

    //================================================
    //======= DEBOUNCE + TRANSITIONS (skip if locked) 
    //================================================
    if (!lock_ob) {

        // -------- RED -> STOPPED --------
        red_confirmed = debounce(ls == LightState::RED,
                                 red_ctrl_cnt,
                                 Config::RED_CTRL_CONFIRM_TICKS);

        if (state != CtrlState::FULLY_STOPPED &&
            state != CtrlState::STOPPED &&
            red_confirmed)
        {
            red_ctrl_cnt = 0;
            enter_state(CtrlState::STOPPED, 0);
            stop_brake_until = Kernel::Clock::now() + chrono::milliseconds(Config::STOP_BRAKE_MS);
            motors_brake(Config::BRAKE_STRENGTH);
            return;
        }

        // -------- FRONT OBSTACLE -> OBSTACLE_AVOID --------
        front_fresh = (Kernel::Clock::now() - sh.front_stamp) < 300ms;
        front_hit =
            sh.front_ok && front_fresh &&
            sh.front_cm > 0.0f && sh.front_cm < Config::OB_FRONT_TRIG_CM;

        const bool ob_gate = (state == CtrlState::FOLLOW || state == CtrlState::ALIGN);

        obstacle_confirmed = debounce(ob_gate && front_hit,
                                      ob_cnt,
                                      Config::OB_CONFIRM_CTRL_TICKS);

        if (ob_gate && obstacle_confirmed) {
            
            ob_active = true;                 
            ob_reset_pending = true;          

            red_ctrl_cnt = 0;
            lost_cnt = 0;
            align_lost_cnt = 0;
            found_cnt = 0;
            ob_cnt = 0;

            enter_state(CtrlState::OBSTACLE_AVOID, Config::OB_BRAKE_MS);
            motors_brake(Config::BRAKE_STRENGTH);
            return;
        }

        // -------- LINE / SEEK / ALIGN debounce --------
        const bool in_seek = (state == CtrlState::SEEK_LEFT || state == CtrlState::SEEK_RIGHT);

        found_confirmed = debounce(in_seek && !li.lost,
                                   found_cnt,
                                   Config::FOUND_TICKS);

        // last_pos update (you probably want this even when locked_ob is false only;
        // if you want it always, move it out of this block)
        if (!li.lost) {
            if (li.pos > 0.2f)       last_pos = +1;
            else if (li.pos < -0.2f) last_pos = -1;
        }

        follow_lost_confirmed = debounce(state == CtrlState::FOLLOW && li.lost,
                                         lost_cnt,
                                         Config::FOLLOW_LOST_TICKS);

        align_lost_confirmed  = debounce(state == CtrlState::ALIGN && li.lost,
                                         align_lost_cnt,
                                         Config::ALIGN_LOST_TICKS);

        // SEEK center-lock housekeeping
        if (state != CtrlState::SEEK_LEFT && state != CtrlState::SEEK_RIGHT) {
            seek_lock_active = false;
            seek_lock_dir = 0;
        }
    }

    //============================== FSM ==============================

    /*
        FOLLOW - DRIVES IN A STRAIGHT LINE IF CENTERED, ATTEMPTS SLIGHT SMOOTH TURNS --> FALLS BACK TO ALIGN FOR BIGGER MISALIGNMENTS
        ALIGN - USES SMOOTH TURNING TO MORE AGGRESIVELY ATTEMPT TO CENTER --> FALLS BACK TO SEEK IF LINE IS LOST
        SEEK - USES SKID STEERING TO TURN IN A DIRECTION BASED ON THE LAST KNOWN DIRECTION UNTIL THE LINE IS FOUND AGAIN, ATTEMPTS TO FIND CENTER BUT GOES TO ALIGN IF CENTER NOT FOUND IN TIME LIMIT SET BY CONFIG
        BRAKING - OBVIOUS
        OBSTACLE_AVOID - TRIGGERED WHEN FRONT SENSOR SPOTS OBJECT, TURNS LEFT AND USES THE RIGHT SENSOR TO MANUEVER AROUND THE OBJECT UNTIL THE LINE IS FOUND AGAIN
        STOPPED - TRIGGERED WHEN THE COLOR SENSOR SPOTS THE RED TRAFFIC LIGHT, RELEASED WHEN GREEN IS SPOTTED. IF RED LIGHT IS LOST (ROVER HADN'T BRAKED FOR LONG ENOUGH AND WENT PAST), REVERSE UNTIL RED LIGHT IS SPOTTED AGAIN
        FULLY_STOPPED - DEFAULT STATE AT THE START SO BLUETOOTH CAN BE PAIRED, ALSO TRIGGERED BY MANUAL CONTROL AND CAN ONLY BE RELEASED BY THE USER OVER BLUETOOTH
    */

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

        // Initial brake window 
        if (!state_time_elapsed()) {
            motors_brake(Config::BRAKE_STRENGTH);
            leds_for_state(CtrlState::OBSTACLE_AVOID); 
            break;
        }

        static ObPhase phase = ObPhase::TURN_LEFT_45;
        static Kernel::Clock::time_point phase_until;
        static bool did_follow = false;
        static int line_found_cnt = 0;

        auto set_phase = [&](ObPhase p) {
            if (phase != p) {
                phase = p;
                leds_for_ob_phase(phase);
            }
        };

        // ---- line exit condition ----
        if (!li.lost) line_found_cnt++;
        else          line_found_cnt = 0;

        // reset when re-entering obstacle avoid
        if (ob_reset_pending) {
            ob_reset_pending = false;
            did_follow = false;
            phase = ObPhase::TURN_LEFT_45;
            phase_until = {};
            line_found_cnt = 0;
            leds_for_ob_phase(phase);
        }

        if (did_follow && line_found_cnt >= 2) {
            line_found_cnt = 0;
            ob_active = false; 
            enter_state(CtrlState::ALIGN, 0);
            break;
        }

        const bool right_seen  = sh.right_ok && sh.right_cm < Config::OB_RIGHT_TRIG_CM;

        switch (phase) {

            case ObPhase::TURN_LEFT_45: {
                // ensure LED is correct even if we land here without a transition
                leds_for_ob_phase(phase);

                if (phase_until.time_since_epoch().count() == 0) {
                    phase_until = Kernel::Clock::now() + chrono::milliseconds(Config::TURN_45_MS);
                }

                turn_left_skid_reverse_inner(Config::DUTY_OB_TURN);

                if (Kernel::Clock::now() >= phase_until) {
                    phase_until = {};
                    set_phase(ObPhase::SEARCH_RIGHT_WALL);
                }
                break;
            }

            case ObPhase::SEARCH_RIGHT_WALL: {
                move_forward(Config::DUTY_OB_FWD);

                if (right_seen) {
                    phase = ObPhase::FOLLOW_UNTIL_LOST;
                    leds_for_ob_phase(phase);
                }
                break;
            }

            case ObPhase::FOLLOW_UNTIL_LOST: {
                move_forward(Config::DUTY_OB_FWD);
                did_follow = true;

                if (!right_seen) {
                    set_phase(ObPhase::REACQUIRE);
                }
                break;
            }

            case ObPhase::REACQUIRE: {
                turn_right_skid_reverse_inner(Config::DUTY_OB_RIGHT);

                if (right_seen) {
                    set_phase(ObPhase::FOLLOW_UNTIL_LOST);
                }
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

    sensor_transistor = 1; //Line sensing transistor on -> can be blinked on and off for ambient light removal, works fine the now

    //USB + Bluetooth debugging
    Debug::init(true, true, BT_TX, BT_RX, BT_BAUD);
    manual_mode = false; //default state is not under manual control --> this variable is used to skip the FSM because rover is under manual control

    //Motors PWM start
    motors_init(Config::PWM_FREQ_HZ);

    UltrasonicSensor us_front(D8,  D9);
    UltrasonicSensor us_right(D10, D11);

    //Audio init (for DAC speaker)
    /*
    audio.init(); 
    audio.play_u8_mono(mario_style, mario_style_len, mario_style_rate);
    */

    // Color Sensor init
    const bool tcs_ok = color.init(100.0f, tcs3472::Gain::X16);
    if (!tcs_ok) {
        Debug::log("TCS3472 init failed");
    }

    //Start in fully sopped --> user releases the rover over bluetooth
    enter_state(CtrlState::SEEK_LEFT, 0);
    
    //Init ticks and ISRs
    colorTick.attach(&color_isr, chrono::milliseconds(Config::COLOR_PERIOD_MS));
    controlTick.attach(&control_isr, chrono::milliseconds(Config::CTRL_PERIOD_MS));
    ultraTick.attach(&ultra_isr, chrono::milliseconds(Config::ULTRA_PERIOD_MS));
    debugTick.attach(&debug_isr, chrono::milliseconds(Config::DEBUG_PERIOD_MS));  

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

        //================ CONTROL =================
        if (do_update) {
            controller_update();
        }

        //================ ULTRASONIC =================
        if (do_ultra) {
            CtrlState st;
            core_util_critical_section_enter();
            st = state;
            core_util_critical_section_exit();

            const bool do_front = should_ping_front(st);
            const bool do_right = should_ping_right(st);

            UltrasonicSensor::Reading::State front_state = UltrasonicSensor::Reading::State::NO_ECHO;
            UltrasonicSensor::Reading::State right_state = UltrasonicSensor::Reading::State::NO_ECHO;

            if (do_front) {
                auto rf = us_front.read_cm(Config::FRONT_MAX_CM);
                front_state = rf.state;

                core_util_critical_section_enter();
                g_shared.front_cm    = rf.valid ? rf.cm : -1.0f;
                g_shared.front_ok    = rf.valid;
                g_shared.front_stamp = Kernel::Clock::now();
                core_util_critical_section_exit();
            }
            
            //avoids crosstalk between the two
            if (do_front && do_right) {
                ThisThread::sleep_for(chrono::milliseconds(Config::ULTRA_STAGGER_MS));
            }

            if (do_right) {
                auto rr = us_right.read_cm(Config::RIGHT_MAX_CM);
                right_state = rr.state;

                core_util_critical_section_enter();
                g_shared.right_cm    = rr.valid ? rr.cm : -1.0f;
                g_shared.right_ok    = rr.valid;
                g_shared.right_stamp = Kernel::Clock::now();
                core_util_critical_section_exit();
            }
            
            //Add new data to debug snapshot
            const Shared sh = shared_snapshot();

            //Update debug snapshot with ultrasonic data
            Debug::update_ultra(Debug::make_ultra(
                sh.front_cm, sh.front_ok, front_state,
                sh.right_cm, sh.right_ok, right_state
            ));
        }
        
        
        //================ COLOR ================= (read only if color was initialized)
        if (do_color && tcs_ok) {
            auto v = color.read_raw(true, 1); //1 milisecond maximum block if reading isn't done

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
            const Shared sh = shared_snapshot();

            // Update debug snapshot with color data
            Debug::update_color(Debug::make_color(sh.rgbc, sh.rgbc_valid, sh.light));
        }
        
        //================ DEBUG USB + BLUETOOTH =================
        if (do_debug) {
            Debug::tick(); //prints to USB and BT
        }

        //READS BLUETOOTH COMMANDS
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
                //Debug::log(buf);

                if (c == 'W' || c == 'A' || c == 'S' || c == 'D') {
                    manual_mode = true;
                    //Debug::log("BT: MANUAL MODE\r\n");

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
    }
    
}
