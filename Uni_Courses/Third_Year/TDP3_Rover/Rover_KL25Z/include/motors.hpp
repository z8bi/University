#pragma once
#include "mbed.h"

// Motor initialisation
void motors_init(float pwm_freq_hz);

// Motor control API
void motors_set_duty_sync(float left_duty, float right_duty);
void motors_all_off();
void motors_coast();
void motors_brake(float strength);

void move_forward(float duty);
void move_forward_different(float dut_R, float dut_L);
void move_backward(float duty);

void turn_left_skid_reverse_inner(float duty_outer);
void turn_right_skid_reverse_inner(float duty_outer);

void turn_left_break_inner(float break_inner, float duty_outer);
void turn_right_break_inner(float break_inner, float duty_outer);

void turn_left_coast_inner(float duty_outer);
void turn_right_coast_inner(float duty_outer);
