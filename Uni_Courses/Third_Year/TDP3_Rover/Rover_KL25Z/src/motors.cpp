#include "motors.hpp"

#include "pin_definitions.hpp"

// ===== Initialisation =====
void motors_init(float pwm_freq_hz) {
    HW::left_pwm.period(1.0f / pwm_freq_hz);
    HW::right_pwm.period(1.0f / pwm_freq_hz);

    motors_all_off();   // safe startup
}

// ================= Helpers =================

void motors_set_duty_sync(float left_duty, float right_duty) {
    HW::left_pwm.write(left_duty);
    HW::right_pwm.write(right_duty);
}

// ================= Stop modes =================

void motors_all_off() {
    motors_set_duty_sync(0.0f, 0.0f);

    // Coast both motors
    HW::left_in1 = 0; HW::left_in2 = 0;
    HW::right_in1 = 0; HW::right_in2 = 0;
}

void motors_coast() {
    motors_all_off();
}

void motors_brake(float strength) {
    // Active braking (short motor terminals)
    HW::left_in1 = 1; HW::left_in2 = 1;
    HW::right_in1 = 1; HW::right_in2 = 1;

    HW::left_pwm.write(strength);
    HW::right_pwm.write(strength);
}

// ================= Motion routines =================

void move_forward(float duty) {
    HW::left_in1 = 1; HW::left_in2 = 0;
    HW::right_in1 = 1; HW::right_in2 = 0;

    motors_set_duty_sync(duty, duty);
}

void move_forward_different(float dut_R, float dut_L) {
    HW::left_in1 = 1; HW::left_in2 = 0;
    HW::right_in1 = 1; HW::right_in2 = 0;

    motors_set_duty_sync(dut_L, dut_R);
}

void move_backward(float duty) {
    HW::left_in1 = 0; HW::left_in2 = 1;
    HW::right_in1 = 0; HW::right_in2 = 1;

    motors_set_duty_sync(duty, duty);
}

void turn_left_skid_reverse_inner(float duty_outer) {
    HW::left_in1 = 0; HW::left_in2 = 1;
    HW::right_in1 = 1; HW::right_in2 = 0;

    motors_set_duty_sync(duty_outer, duty_outer);
}

void turn_right_skid_reverse_inner(float duty_outer) {
    HW::left_in1 = 1; HW::left_in2 = 0;
    HW::right_in1 = 0; HW::right_in2 = 1;

    motors_set_duty_sync(duty_outer, duty_outer);
}

void turn_left_break_inner(float break_inner, float duty_outer) {
    HW::left_in1 = 1; HW::left_in2 = 1;
    HW::right_in1 = 1; HW::right_in2 = 0;

    HW::left_pwm.write(break_inner);
    HW::right_pwm.write(duty_outer);
}

void turn_right_break_inner(float break_inner, float duty_outer) {
    HW::left_in1 = 1; HW::left_in2 = 0;
    HW::right_in1 = 1; HW::right_in2 = 1;

    HW::left_pwm.write(duty_outer);
    HW::right_pwm.write(break_inner);
}

void turn_right_coast_inner(float duty_outer) {
    HW::left_in1 = 1; HW::left_in2 = 0;
    HW::right_in1 = 0; HW::right_in2 = 0;

    HW::left_pwm.write(duty_outer);
    HW::right_pwm.write(0.0f);
}

void turn_left_coast_inner(float duty_outer) {
    HW::left_in1 = 0; HW::left_in2 = 0;
    HW::right_in1 = 1; HW::right_in2 = 0;

    HW::right_pwm.write(duty_outer);
    HW::left_pwm.write(0.0f);
}
