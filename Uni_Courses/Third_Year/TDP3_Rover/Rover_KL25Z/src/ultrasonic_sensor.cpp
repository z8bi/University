#include "ultrasonic_sensor.hpp"

UltrasonicSensor::UltrasonicSensor(PinName trig_pin, PinName echo_pin,
                                   bool trig_active_high)
    : _trig(trig_pin), _echo(echo_pin), _trig_active_high(trig_active_high)
{
    // Ensure trigger idle-low (typical HC-SR04 expects low idle, high pulse)
    _trig = _trig_active_high ? 0 : 1;
}

UltrasonicSensor::Reading UltrasonicSensor::read_cm(float max_range_cm)
{
    // Convert max_range_cm to a pulse-width timeout.
    // HC-SR04: distance_cm = time_us * 0.0343 / 2  => time_us = distance_cm * 2 / 0.0343
    // Add a small buffer.
    if (max_range_cm < 1.0f) max_range_cm = 1.0f;
    const int fall_timeout_us = (int)(max_range_cm * 2.0f / 0.0343f) + 300;

    // Trigger pulse (10us high)
    _trig = _trig_active_high ? 0 : 1;
    wait_us(2);

    _trig = _trig_active_high ? 1 : 0;
    wait_us(10);
    _trig = _trig_active_high ? 0 : 1;

    // Wait for rising edge with timeout
    Timer guard;
    guard.start();
    while (_echo.read() == 0) {
        if ((int)chrono::duration_cast<chrono::microseconds>(guard.elapsed_time()).count() > RISE_TIMEOUT_US) {
            return {-1.0f, false};
        }
    }

    // Measure high pulse width with timeout
    Timer t;
    t.start();
    while (_echo.read() == 1) {
        if ((int)chrono::duration_cast<chrono::microseconds>(t.elapsed_time()).count() > fall_timeout_us) {
            return {-2.0f, false};
        }
    }
    t.stop();

    const int time_us = (int)chrono::duration_cast<chrono::microseconds>(t.elapsed_time()).count();
    if (time_us <= 0) return {-3.0f, false};

    const float cm = (float)time_us * 0.0343f / 2.0f;
    return {cm, true};
}

float UltrasonicSensor::read_cm_value(float max_range_cm)
{
    Reading r = read_cm(max_range_cm);
    return r.valid ? r.cm : -1.0f;
}
