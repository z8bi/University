#pragma once
#include "mbed.h"
#include "tcs3472.hpp"

namespace HW {
// Line sensors
extern AnalogIn   left_sensor_2;
extern AnalogIn   left_sensor_1;
extern AnalogIn   middle_sensor;
extern AnalogIn   right_sensor_1;
extern AnalogIn   right_sensor_2;
extern DigitalOut sensor_transistor;

// Motors
extern DigitalOut right_in1;
extern DigitalOut right_in2;
extern PwmOut     right_pwm;
extern DigitalOut left_in2;
extern DigitalOut left_in1;
extern PwmOut     left_pwm;

// RGB LEDs (KL25Z active-low)
extern DigitalOut LED_R;
extern DigitalOut LED_G;
extern DigitalOut LED_B;

// Color sensor
extern tcs3472::TCS3472 color;

// Bluetooth state
extern DigitalIn bt_state;

// These can stay header-only
extern const PinName BT_TX;
extern const PinName BT_RX;
extern const int     BT_BAUD;

//Starting Switch
extern DigitalIn start_switch;
} // namespace HW