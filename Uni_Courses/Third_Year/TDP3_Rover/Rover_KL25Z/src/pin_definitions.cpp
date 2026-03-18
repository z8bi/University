#include "pin_definitions.hpp"

namespace HW {
// Line sensors
AnalogIn   left_sensor_2(A0);
AnalogIn   left_sensor_1(A1);
AnalogIn   middle_sensor(A2);
AnalogIn   right_sensor_1(A3);
AnalogIn   right_sensor_2(A4);
DigitalOut sensor_transistor(PTC11);

// Motors
DigitalOut right_in1(D7);
DigitalOut right_in2(D6);
DigitalOut left_in2(D5);
DigitalOut left_in1(D4);
PwmOut     right_pwm(D3);
PwmOut     left_pwm(D2);

// RGB LEDs (KL25Z active-low)
DigitalOut LED_R(LED_RED);
DigitalOut LED_G(LED_GREEN);
DigitalOut LED_B(LED_BLUE);

// Color sensor
tcs3472::TCS3472 color(PTE0, PTE1, 100000);

// Bluetooth state
DigitalIn bt_state(PTE21);
const PinName BT_TX = PTE22;
const PinName BT_RX = PTE23;
const int     BT_BAUD = 115200;

DigitalIn start_switch(PTE30, PullUp);
} // namespace HW