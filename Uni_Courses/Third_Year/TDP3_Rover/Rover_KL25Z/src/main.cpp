
#include "mbed.h"
#include <cmath>
#include <cstdint>

/*Custom Includes
#include "wav_player.h"
#include "car_x_pcm.h"
*/
//===============State Machine==================

enum class CtrlState {
    FOLLOW,
    SEEK_LEFT,
    SEEK_RIGHT,
    NUDGE_FORWARD,
    TURN_90_RIGHT,
    TURN_90_LEFT,
    BRAKING,
    STOPPED
};

CtrlState state = CtrlState::FOLLOW;
Kernel::Clock::time_point state_until;
bool pending_left_turn = false;

static int last_pos = 0;
CtrlState after_brake_state = CtrlState::FOLLOW;

//==================Pin assignments============================

// DAC output
AnalogOut dac(PTE30);

//Light sensors
AnalogIn left_sensor_2(A4);
AnalogIn left_sensor_1(A3);
AnalogIn middle_sensor(A2);
AnalogIn right_sensor_2(A1);
AnalogIn right_sensor_1(A0);
DigitalOut sensor_transistor(PTC11);

// Left motor
DigitalOut left_in2(D7);
DigitalOut left_in1(D6);
PwmOut     left_pwm(D5);

// Right motor
DigitalOut right_in2(D4);
DigitalOut right_in1(D3);
PwmOut     right_pwm(D2);

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
        case CtrlState::FOLLOW:        leds_set(false, true,  false); break; // Green
        case CtrlState::SEEK_LEFT:     leds_set(true,  false, false); break; // Red
        case CtrlState::SEEK_RIGHT:    leds_set(true,  false, false); break; // Red
        case CtrlState::NUDGE_FORWARD: leds_set(true,  true,  false); break; // Yellow
        case CtrlState::TURN_90_LEFT:  leds_set(true,  false, true ); break; // Magenta
        case CtrlState::TURN_90_RIGHT: leds_set(false, true,  true ); break; // Cyan
        case CtrlState::BRAKING:       leds_set(false, false, true ); break; // Blue
        case CtrlState::STOPPED:       leds_set(false, false, false); break; // Off
        default:                       leds_set(false, false, false); break;
    }
}  

//=================LIGHT SENSING CONTROL TICKER===================

Ticker controlTick;
volatile bool control_due = false;
void control_isr() { control_due = true; }

//=================ROVER MOVE PROTORYPES===============

void motors_all_off();
void motors_brake(float strength);
void move_forward(float duty);
void turn_left_coast_inner(float duty_outer);
void turn_right_coast_inner(float duty_outer);
void turn_left_skid_reverse_inner(float duty_outer);
void turn_right_skid_reverse_inner(float duty_outer);

void motors_release();
void enter_state(CtrlState st, int duration_ms);
bool state_time_elapsed();

void enter_state(CtrlState st, int duration_ms) {
    state = st;
    state_until = Kernel::Clock::now() + chrono::milliseconds(duration_ms);
    leds_for_state(state);
}

bool state_time_elapsed() {
    return Kernel::Clock::now() >= state_until;
}

void motors_release() {
    motors_all_off();
}

//======================================================
//====================TUNABLE===========================

static constexpr float DUTY_FWD = 0.25f;
static constexpr float DUTY_TURN = 0.75f;
static constexpr float BRAKE_STRENGTH = 0.40f;

static constexpr int NUDGE_MS  = 300; //90 degree calibration
static constexpr int TURN90_MS = 1600; // right 90
static constexpr int TURN90L_MS = 1600; // left 90 (tune separately if needed)
static constexpr int BRAKE_MS  = 120;

static constexpr float POS_DEADBAND_F = 0.5f; // tune 0.20–0.40

//======================================================
//======================================================

//=============================================================
//======================LINE SENSING STATE-MACHINE=============
//=============================================================

static constexpr float TH_ON  = 0.60f; // tune these if line sensing doesn't work
static constexpr float TH_OFF = 0.50f;

struct Sensors {
    float a[5];     // raw analog 0..1
    bool  on[5];    // debounced digital
};

Sensors read_sensors() {
    Sensors s{};
    s.a[0] = left_sensor_2.read();
    s.a[1] = left_sensor_1.read();
    s.a[2] = middle_sensor.read();
    s.a[3] = right_sensor_1.read();
    s.a[4] = right_sensor_2.read();

    // static keeps previous state for hysteresis
    static bool prev[5] = {false,false,false,false,false};

    for (int i=0;i<5;i++) {
        if (!prev[i] && s.a[i] >= TH_ON)  prev[i] = true;
        if ( prev[i] && s.a[i] <= TH_OFF) prev[i] = false;
        s.on[i] = prev[i];
    }
    return s;
}

struct LineInfo {
    int   active_count = 0;
    int   pos_sum      = 0;     // sum of weights for active sensors
    float pos          = 0.0f;  // normalized position estimate (-2..+2 approx)
    bool centered      = false;
    bool lost          = false;
    bool junction      = false;
    bool right_turn_sig = false;
    bool left_turn_sig  = false;
};

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

    // Normalized "where is the line" estimate:
    // -2 far left ... +2 far right (roughly), stable even when 2-3 sensors are on.
    li.pos = (!li.lost) ? (static_cast<float>(li.pos_sum) / li.active_count) : 0.0f;

    // Strictly centered only if middle is on and immediate neighbors aren't
    li.centered = s.on[2] && !(s.on[1] || s.on[3]);

    // Turn signatures (tight): outer pair must be ON
    li.right_turn_sig = (s.on[3] && s.on[4]);
    li.left_turn_sig  = (s.on[0] && s.on[1]);

    // Junction-ish detection (stricter than before):
    // Require >=3 sensors active OR center + an outer-pair signature.
    li.junction = (li.active_count >= 3) || (s.on[2] && (li.right_turn_sig || li.left_turn_sig));

    return li;
}

void controller_update() {
    Sensors s = read_sensors();
    LineInfo li = interpret(s);

    // Keep last seen direction for SEEK
    if (!li.lost) {
        last_pos = (li.pos >= 0.0f) ? +1 : -1;   // store direction only
    }

    switch (state) {

    case CtrlState::FOLLOW: {

        // --- 90-degree turn detection ---
        if (li.junction) {
            if (li.right_turn_sig && !li.left_turn_sig) {
                pending_left_turn = false;
                enter_state(CtrlState::NUDGE_FORWARD, NUDGE_MS);
                move_forward(DUTY_FWD);
                break;
            }
            if (li.left_turn_sig && !li.right_turn_sig) {
                pending_left_turn = true;
                enter_state(CtrlState::NUDGE_FORWARD, NUDGE_MS);
                move_forward(DUTY_FWD);
                break;
            }
            // ambiguous -> fall through to normal following
        }

        // --- Normal line following ---
        if (li.centered) {
            move_forward(DUTY_FWD);
            break;
        }

        if (!li.lost) {
            if (li.pos < -POS_DEADBAND_F) {
                turn_left_coast_inner(DUTY_TURN);
            } else if (li.pos > POS_DEADBAND_F) {
                turn_right_coast_inner(DUTY_TURN);
            } else {
                move_forward(DUTY_FWD);
            }
            break;
        }

        // --- Lost line completely: brake briefly then seek ---
        enter_state(CtrlState::BRAKING, BRAKE_MS);
        motors_brake(BRAKE_STRENGTH);

        // decide where to seek after braking finishes
        // (we'll pick this in BRAKING-> transition below)
        break;
    }

    case CtrlState::SEEK_LEFT: {
        if (!li.lost) {
            enter_state(CtrlState::BRAKING, BRAKE_MS);
            motors_brake(BRAKE_STRENGTH);
            // after brake, we go FOLLOW (see BRAKING)
        } else {
            turn_left_skid_reverse_inner(DUTY_TURN);
        }
        break;
    }

    case CtrlState::SEEK_RIGHT: {
        if (!li.lost) {
            enter_state(CtrlState::BRAKING, BRAKE_MS);
            motors_brake(BRAKE_STRENGTH);
        } else {
            turn_right_skid_reverse_inner(DUTY_TURN);
        }
        break;
    }

    case CtrlState::BRAKING: {
        if (state_time_elapsed()) {
            motors_release();

            // If we were braking to start a planned turn, go do it now
            if (after_brake_state == CtrlState::TURN_90_LEFT) {
                enter_state(CtrlState::TURN_90_LEFT, TURN90L_MS);
                turn_left_skid_reverse_inner(DUTY_TURN);
            }
            else if (after_brake_state == CtrlState::TURN_90_RIGHT) {
                enter_state(CtrlState::TURN_90_RIGHT, TURN90_MS);
                turn_right_skid_reverse_inner(DUTY_TURN);
            }
            else {
                if (li.lost) {
                    enter_state((last_pos >= 0) ? CtrlState::SEEK_RIGHT : CtrlState::SEEK_LEFT, 0);
                } else {
                    enter_state(CtrlState::FOLLOW, 0);
                }
            }

            // Always reset
            after_brake_state = CtrlState::FOLLOW;

        } else {
            motors_brake(BRAKE_STRENGTH);
        }
        break;
    }


    case CtrlState::NUDGE_FORWARD: {
        if (state_time_elapsed()) {
            // Decide which 90° turn we want AFTER braking
            after_brake_state = pending_left_turn ? CtrlState::TURN_90_LEFT : CtrlState::TURN_90_RIGHT;

            // Brake briefly before the turn
            enter_state(CtrlState::BRAKING, BRAKE_MS);
            motors_brake(BRAKE_STRENGTH);
        } else {
            move_forward(DUTY_FWD);
        }
        break;
    }


    case CtrlState::TURN_90_RIGHT: {
        if (state_time_elapsed()) {
            enter_state(CtrlState::BRAKING, BRAKE_MS);
            motors_brake(BRAKE_STRENGTH);
        } else {
            turn_right_skid_reverse_inner(0.7);
        }
        break;
    }

    case CtrlState::TURN_90_LEFT: {
        if (state_time_elapsed()) {
            enter_state(CtrlState::BRAKING, BRAKE_MS);
            motors_brake(BRAKE_STRENGTH);
        } else {
            turn_left_skid_reverse_inner(0.65);
        }
        break;
    }

    case CtrlState::STOPPED:
    default:
        motors_all_off();
        break;
    }
}


/*========================SOUND=========================

Ticker audioTick;

// Playback config (most common voice setting)
static constexpr int SAMPLE_RATE = 8000;

// Playback state
static volatile uint32_t sample_i = 0;
static volatile bool playing = false;

// Read one int16 little-endian sample from the byte array
static inline int16_t read_i16le(const unsigned char* p) {
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

void audio_isr() {
    if (!playing) {
        dac.write(0.5f);
        return;
    }

    uint32_t byte_i = sample_i * 2;
    if (byte_i + 1 >= lib_Sounds_car_x_pcm_len) {
        playing = false;
        sample_i = 0;
        dac.write(0.5f);
        return;
    }

    int16_t s = read_i16le(&lib_Sounds_car_x_pcm[byte_i]);
    sample_i++;

    // Map -32768..+32767 -> 0.0..1.0
    float v = 0.5f + (float)s / 65536.0f;
    dac.write(v);
}

// Call this to play the clip once
void play_car_x() {
    sample_i = 0;
    playing = true;
}

// Optional
bool is_playing() { return playing; }

//=================================================
*/

// === PWM helper ===
void motors_set_duty_sync(float left_duty, float right_duty) {
    left_pwm.write(left_duty);
    right_pwm.write(right_duty);
}

// === Safe shutdown ===
void motors_all_off() {
    motors_set_duty_sync(0.0f, 0.0f);

    // Coast both motors
    left_in1 = 0; left_in2 = 0;
    right_in1 = 0; right_in2 = 0;
}

// === Stop modes ===

// Soft stop: let motors freewheel
void motors_coast() {
    motors_all_off();
}

// Hard stop: short both motor terminals to brake
void motors_brake(float strength) {
    // Both inputs HIGH -> motor terminals shorted (active braking)
    left_in1 = 1; left_in2 = 1;
    right_in1 = 1; right_in2 = 1;

    // Apply PWM to control braking torque
    left_pwm.write(strength);
    right_pwm.write(strength);
}

// === Motion routines ===
void move_forward(float duty) {

    left_in1 = 1; left_in2 = 0; // Left forward
    right_in1 = 1; right_in2 = 0; // Right forward

    motors_set_duty_sync(duty, duty);
}

void move_forward_different(float dut_R, float dut_L) {

    left_in1 = 1; left_in2 = 0; // Left forward
    right_in1 = 1; right_in2 = 0; // Right forward

    motors_set_duty_sync(dut_L, dut_R);
}

void move_backward(float duty) {

    left_in1 = 0; left_in2 = 1; // Left backward
    right_in1 = 0; right_in2 = 1; // Right backward

    motors_set_duty_sync(duty, duty);
}

void turn_left_skid_reverse_inner(float duty_outer) {

    left_in1 = 0; left_in2 = 1; // Left reverse
    right_in1 = 1; right_in2 = 0; // Right forward

    motors_set_duty_sync(duty_outer, duty_outer);
}

void turn_left_break_inner(float break_inner, float duty_outer) {

    left_in1 = 1; left_in2 = 1; // Left break
    right_in1 = 1; right_in2 = 0; // Right forward

    left_pwm.write(break_inner);
    right_pwm.write(duty_outer);
}

void turn_right_skid_reverse_inner(float duty_outer) {

    left_in1 = 1; left_in2 = 0; // Left forward
    right_in1 = 0; right_in2 = 1; // Right reverse

    motors_set_duty_sync(duty_outer, duty_outer);
}

void turn_right_break_inner(float break_inner, float duty_outer) {

    left_in1 = 1; left_in2 = 0; // Left go
    right_in1 = 1; right_in2 = 1; // Right break

    left_pwm.write(duty_outer);
    right_pwm.write(break_inner);
}

void turn_right_coast_inner(float duty_outer) {

    left_in1 = 1; left_in2 = 0; // Left go
    right_in1 = 0; right_in2 = 0; // Right coast

    left_pwm.write(duty_outer);
    right_pwm.write(0.0f);
}

void turn_left_coast_inner(float duty_outer) {

    left_in1 = 0; left_in2 = 0; // Left coast
    right_in1 = 1; right_in2 = 0; // Right go

    right_pwm.write(duty_outer);
    left_pwm.write(0.0f);
}

int main() {

    // Set PWM frequency
    left_pwm.period(1.0f / PWM_FREQ_HZ);
    right_pwm.period(1.0f / PWM_FREQ_HZ);

    //Start the control ticker
    controlTick.attach(&control_isr, 20ms); // 20 ms = 50 Hz control loop

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

