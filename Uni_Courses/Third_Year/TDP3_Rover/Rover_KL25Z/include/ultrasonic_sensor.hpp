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

        enum class State : uint8_t {
            VALID,       // measurement OK
            NO_ECHO,     // never saw rising edge (timeout waiting for echo HIGH)
            MAX_RANGE,   // echo stayed HIGH too long (beyond max_range_cm)
            TIMING_ERR   // pulse width <= 0 (unexpected)
        };

        float cm;     // distance in cm (only meaningful when state == VALID)
        bool  valid;  // preserved for backwards compatibility
        State state;  // NEW: reason codes
    };

    UltrasonicSensor(PinName trig_pin, PinName echo_pin,
                     bool trig_active_high = true);

    Reading read_cm(float max_range_cm = 40.0f);
    float read_cm_value(float max_range_cm = 40.0f);

private:
    DigitalOut _trig;
    DigitalIn  _echo;
    bool       _trig_active_high;

    static constexpr int RISE_TIMEOUT_US = 3000;
};


#endif // ULTRASONIC_SENSOR_HPP
