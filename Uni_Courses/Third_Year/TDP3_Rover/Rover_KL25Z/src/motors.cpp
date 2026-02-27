#include "motors.hpp"

// ===== Hardware objects =====
extern DigitalOut left_in2;
extern DigitalOut left_in1;
extern DigitalOut right_in1;
extern DigitalOut right_in2;
extern PwmOut     left_pwm;
extern PwmOut     right_pwm;

// ===== Initialisation =====
void motors_init(float pwm_freq_hz) {
    left_pwm.period(1.0f / pwm_freq_hz);
    right_pwm.period(1.0f / pwm_freq_hz);

    motors_all_off();   // safe startup
}

// ================= Helpers =================

void motors_set_duty_sync(float left_duty, float right_duty) {
    left_pwm.write(left_duty);
    right_pwm.write(right_duty);
}

// ================= Stop modes =================

void motors_all_off() {
    motors_set_duty_sync(0.0f, 0.0f);

    // Coast both motors
    left_in1 = 0; left_in2 = 0;
    right_in1 = 0; right_in2 = 0;
}

void motors_coast() {
    motors_all_off();
}

void motors_brake(float strength) {
    // Active braking (short motor terminals)
    left_in1 = 1; left_in2 = 1;
    right_in1 = 1; right_in2 = 1;

    left_pwm.write(strength);
    right_pwm.write(strength);
}

// ================= Motion routines =================

void move_forward(float duty) {
    left_in1 = 1; left_in2 = 0;
    right_in1 = 1; right_in2 = 0;

    motors_set_duty_sync(duty, duty);
}

void move_forward_different(float dut_R, float dut_L) {
    left_in1 = 1; left_in2 = 0;
    right_in1 = 1; right_in2 = 0;

    motors_set_duty_sync(dut_L, dut_R);
}

void move_backward(float duty) {
    left_in1 = 0; left_in2 = 1;
    right_in1 = 0; right_in2 = 1;

    motors_set_duty_sync(duty, duty);
}

void turn_left_skid_reverse_inner(float duty_outer) {
    left_in1 = 0; left_in2 = 1;
    right_in1 = 1; right_in2 = 0;

    motors_set_duty_sync(duty_outer, duty_outer);
}

void turn_right_skid_reverse_inner(float duty_outer) {
    left_in1 = 1; left_in2 = 0;
    right_in1 = 0; right_in2 = 1;

    motors_set_duty_sync(duty_outer, duty_outer);
}

void turn_left_break_inner(float break_inner, float duty_outer) {
    left_in1 = 1; left_in2 = 1;
    right_in1 = 1; right_in2 = 0;

    left_pwm.write(break_inner);
    right_pwm.write(duty_outer);
}

void turn_right_break_inner(float break_inner, float duty_outer) {
    left_in1 = 1; left_in2 = 0;
    right_in1 = 1; right_in2 = 1;

    left_pwm.write(duty_outer);
    right_pwm.write(break_inner);
}

void turn_right_coast_inner(float duty_outer) {
    left_in1 = 1; left_in2 = 0;
    right_in1 = 0; right_in2 = 0;

    left_pwm.write(duty_outer);
    right_pwm.write(0.0f);
}

void turn_left_coast_inner(float duty_outer) {
    left_in1 = 0; left_in2 = 0;
    right_in1 = 1; right_in2 = 0;

    right_pwm.write(duty_outer);
    left_pwm.write(0.0f);
}
