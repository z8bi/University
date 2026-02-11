#include "mbed.h"
#include "sensors.hpp"
#include "motors.hpp"

/*Custom Includes
#include "wav_player.h"
#include "car_x_pcm.h"
*/
//===============State Machine==================

enum class CtrlState {
    FOLLOW,
    SEEK_LEFT,
    SEEK_RIGHT,
    ALIGN,
    BRAKING,
    STOPPED
};

CtrlState state = CtrlState::FOLLOW;
Kernel::Clock::time_point state_until;

static int last_pos = 0;

//==================Pin assignments============================

// DAC output
AnalogOut dac(PTE30);

// RGB LEDs (onboard KL25Z)
DigitalOut LED_R(LED_RED);
DigitalOut LED_G(LED_GREEN);
DigitalOut LED_B(LED_BLUE);

// === Constants ===
#define PWM_FREQ_HZ 20000.0f

// === LED helper ===
void leds_set(bool r, bool g, bool b) {
    LED_R = !r; // KL25Z LEDs are active-low
    LED_G = !g;
    LED_B = !b;
}

void leds_for_state(CtrlState st) {
    switch (st) {
        case CtrlState::FOLLOW:     leds_set(false, true,  false); break; // Green
        case CtrlState::SEEK_LEFT:  leds_set(true,  false, false); break; // Red
        case CtrlState::SEEK_RIGHT: leds_set(true,  false, false); break; // Red
        case CtrlState::ALIGN:      leds_set(true,  true,  false); break; // Yellow
        case CtrlState::BRAKING:    leds_set(false, false, true ); break; // Blue
        case CtrlState::STOPPED:    leds_set(false, false, false); break; // Off
        default:                    leds_set(false, false, false); break;
    }
}

//=================LIGHT SENSING CONTROL TICKER===================

Ticker controlTick;
//Ticker audioTick;
volatile bool control_due = false;
void control_isr() { control_due = true; }

void enter_state(CtrlState st, int duration_ms) {
    state = st;
    state_until = Kernel::Clock::now() + chrono::milliseconds(duration_ms);
    leds_for_state(state);
}

bool state_time_elapsed() {
    return Kernel::Clock::now() >= state_until;
}

//======================================================
//====================TUNABLE===========================
//======================================================

static constexpr float DUTY_FWD = 0.45f;
static constexpr float DUTY_TURN = 0.75f;

static constexpr float ALIGN_DUTY_TURN = 0.7f;
static constexpr float TURN_BRAKE_STRENGTH = 0.9f;
static constexpr float DUTY_GRACE = 0.25f;  

//Gentler turning in FOLLOW
static constexpr float DUTY_FOLLOW_TURN = 0.7f;      // less than 0.75
static constexpr float FOLLOW_BRAKE_STRENGTH = 0.7f; // less than 0.75

static constexpr float BRAKE_STRENGTH = 1.0f;
static constexpr int BRAKE_MS  = 50;

static constexpr int LOST_CONFIRM_MS = 80;           // how long line must be gone before recovery
static constexpr int CTRL_PERIOD_MS  = 20;            // your ticker period
static constexpr int LOST_TICKS = LOST_CONFIRM_MS / CTRL_PERIOD_MS; 
static constexpr int ALIGN_LOST_CONFIRM_MS = 80; // e.g. 60–120ms
static constexpr int ALIGN_LOST_TICKS = ALIGN_LOST_CONFIRM_MS / CTRL_PERIOD_MS;

static constexpr int SEEK_CENTER_LOCK_MS = 160;

//=============================================================
//======================LINE SENSING STATE-MACHINE=============
//=============================================================

void controller_update() {
    Sensors s = read_sensors();
    LineInfo li = interpret(s);

    // --- FOUND debouncing: require a couple of consecutive "not lost" samples ---
    static int found_cnt = 0;

    if (state == CtrlState::SEEK_LEFT || state == CtrlState::SEEK_RIGHT) {
        if (!li.lost) found_cnt++;
        else          found_cnt = 0;
    } else {
        found_cnt = 0;
    }

    static constexpr int FOUND_TICKS = 1;   // 2*20ms = 40ms
    bool found_confirmed = (found_cnt >= FOUND_TICKS);

    if (!li.lost) {
        if (li.pos > 0.2f)      last_pos = +1;
        else if (li.pos < -0.2f) last_pos = -1;
    }

    // --- LOST debouncing: only track "lost" while in FOLLOW ---
    static int lost_cnt = 0;

    if (state == CtrlState::FOLLOW) {
        if (li.lost) lost_cnt++;
        else         lost_cnt = 0;
    } else {
        lost_cnt = 0; // don't carry "lost" history through turns/braking/seek
    }

    bool lost_confirmed = (lost_cnt >= LOST_TICKS);

    // --- LOST debouncing while in ALIGN ---
    static int align_lost_cnt = 0;

    if (state == CtrlState::ALIGN) {
        if (li.lost) align_lost_cnt++;
        else         align_lost_cnt = 0;
    } else {
        align_lost_cnt = 0;
    }

    bool align_lost_confirmed = (align_lost_cnt >= ALIGN_LOST_TICKS);

    // --- SEEK center-lock window, triggered by outermost sensor (no new thresholds) ---
    static bool seek_lock_active = false;
    static int  seek_lock_dir    = 0; // -1 = left lock, +1 = right lock
    static Kernel::Clock::time_point seek_lock_until;

    // Reset if we leave SEEK
    if (state != CtrlState::SEEK_LEFT && state != CtrlState::SEEK_RIGHT) {
        seek_lock_active = false;
        seek_lock_dir = 0;
    }

    switch (state) {

    case CtrlState::FOLLOW: {

        // ---- if lost, only act if it stays lost for LOST_TICKS ----
        if (li.lost) {
            // not lost long enough yet -> don't panic
            if (!lost_confirmed) {
                // Option A (recommended): keep moving gently forward
                move_forward(DUTY_GRACE);
                break;
            }

            // now it's a real loss -> brake & recover
            // line loss confirmed in FOLLOW -> try ALIGN first (micro-reacquire)
            enter_state(CtrlState::ALIGN, 0);
            break;
        }

        // ---- normal follow logic (unchanged) ----
        if (li.centered) {
            move_forward(DUTY_FWD);
            break;
        }

        else {
            if (li.pos > 0.0f) turn_right_break_inner(FOLLOW_BRAKE_STRENGTH, DUTY_FOLLOW_TURN);
            else               turn_left_break_inner (FOLLOW_BRAKE_STRENGTH, DUTY_FOLLOW_TURN);
            break;
        }

        break;
    }

    case CtrlState::SEEK_LEFT: {

        // If we swapped SEEK direction, cancel the other lock
        if (seek_lock_active && seek_lock_dir != -1) { seek_lock_active = false; seek_lock_dir = 0; }

        // Trigger lock when LEFT2 is spotted
        if (!seek_lock_active && s.on[0]) {
            seek_lock_active = true;
            seek_lock_dir = -1;
            seek_lock_until = Kernel::Clock::now() + chrono::milliseconds(SEEK_CENTER_LOCK_MS);
        }

        // While locked (LEFT), keep turning LEFT until centered or timeout
        if (seek_lock_active && seek_lock_dir == -1) {

            if (li.centered) {
                seek_lock_active = false;
                enter_state(CtrlState::ALIGN, 0);
                break;
            }

            if (Kernel::Clock::now() >= seek_lock_until) {
                seek_lock_active = false; // timeout -> normal SEEK
            } else {
                turn_left_skid_reverse_inner(DUTY_TURN);
                break;
            }
        }

        // ---- Normal SEEK behavior ----
        if (found_confirmed) {
            enter_state(CtrlState::ALIGN, 0);
            break;
        }
        turn_left_skid_reverse_inner(DUTY_TURN);
        break;
    }

    case CtrlState::SEEK_RIGHT: {

        // If we swapped SEEK direction, cancel the other lock
        if (seek_lock_active && seek_lock_dir != +1) { seek_lock_active = false; seek_lock_dir = 0; }

        // Trigger lock when RIGHT2 is spotted
        if (!seek_lock_active && s.on[4]) {
            seek_lock_active = true;
            seek_lock_dir = +1;
            seek_lock_until = Kernel::Clock::now() + chrono::milliseconds(SEEK_CENTER_LOCK_MS);
        }

        // While locked (RIGHT), keep turning RIGHT until centered or timeout
        if (seek_lock_active && seek_lock_dir == +1) {

            if (li.centered) {
                seek_lock_active = false;
                enter_state(CtrlState::ALIGN, 0);
                break;
            }

            if (Kernel::Clock::now() >= seek_lock_until) {
                seek_lock_active = false;
            } else {
                turn_right_skid_reverse_inner(DUTY_TURN);
                break;
            }
        }

        // ---- Normal SEEK behavior ----
        if (found_confirmed) {
            enter_state(CtrlState::ALIGN, 0);
            break;
        }
        turn_right_skid_reverse_inner(DUTY_TURN);
        break;
    }


    case CtrlState::ALIGN: {

        if (li.centered) {
            enter_state(CtrlState::FOLLOW, 0);
            move_forward(DUTY_FWD);   
            break;
        }



        // If loss persists, now it's a real loss -> brake then seek
        if (align_lost_confirmed) {
            enter_state(CtrlState::BRAKING, BRAKE_MS);
            motors_brake(BRAKE_STRENGTH);
            break;
        }

        // steer toward line
        if (li.pos > 0.0f) turn_right_break_inner(TURN_BRAKE_STRENGTH, ALIGN_DUTY_TURN);
        else               turn_left_break_inner(TURN_BRAKE_STRENGTH, ALIGN_DUTY_TURN);
        break;
    }

    case CtrlState::BRAKING: {
        if (state_time_elapsed()) {
            motors_all_off();

            if (li.lost) {
                enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT
                                            : CtrlState::SEEK_LEFT, 0);
            } else {
                // regained line -> go ALIGN to settle, then FOLLOW
                enter_state(CtrlState::ALIGN, 0);
            }
        } else {
            motors_brake(BRAKE_STRENGTH);
        }
        break;
    }


    case CtrlState::STOPPED:
    default:
        motors_all_off();
        break;
    }
}

int main() {

    motors_init(PWM_FREQ_HZ);

    //Start the control ticker
    controlTick.attach(&control_isr, chrono::milliseconds(CTRL_PERIOD_MS));

    sensor_transistor = 1;

    motors_all_off(); // Ensure safe startup

    enter_state(CtrlState::FOLLOW, 0); // sets LED + state_until

    while (true) {
        bool do_update = false;

        core_util_critical_section_enter();
        if (control_due) {
            control_due = false;
            do_update = true;
        }
        core_util_critical_section_exit();

        if (do_update) {
            controller_update();
        }

        ThisThread::sleep_for(1ms);
    }
    
}

