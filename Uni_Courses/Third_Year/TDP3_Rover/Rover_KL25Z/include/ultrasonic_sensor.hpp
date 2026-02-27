#ifndef ULTRASONIC_SENSOR_HPP
#define ULTRASONIC_SENSOR_HPP

#include "mbed.h"

// Simple bounded (timeout-safe) ultrasonic ranger (HC-SR04 style).
// Notes:
//  - Echo pin on HC-SR04 is 5V: use a divider/level shifter for KL25Z (3.3V GPIO).
//  - read_cm() is blocking but bounded by timeouts; rate-limit calls (e.g. every 50–100ms).

class UltrasonicSensor {
public:
    struct Reading {
        float cm;     // distance in cm
        bool  valid;  // true if measurement is valid
    };

    UltrasonicSensor(PinName trig_pin, PinName echo_pin,
                     bool trig_active_high = true);

    // Returns {cm, valid}. If invalid, cm is -1.
    // max_range_cm clamps the pulse width timeout.
    Reading read_cm(float max_range_cm = 40.0f);

    // Convenience: returns cm or -1 if invalid
    float read_cm_value(float max_range_cm = 40.0f);

private:
    DigitalOut _trig;
    DigitalIn  _echo;
    bool       _trig_active_high;

    // Tunable timeouts (microseconds)
    static constexpr int RISE_TIMEOUT_US = 3000;   // wait for echo rising edge - if doesn't come then leave funciton and return -1 
};

#endif // ULTRASONIC_SENSOR_HPP
