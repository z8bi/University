#include "ultrasonic_sensor.hpp"

UltrasonicSensor::UltrasonicSensor(PinName trig_pin, PinName echo_pin,
                                   bool trig_active_high)
    : _trig(trig_pin), _echo(echo_pin), _trig_active_high(trig_active_high)
{
    _trig = _trig_active_high ? 0 : 1;
}

UltrasonicSensor::Reading UltrasonicSensor::read_cm(float max_range_cm)
{
    if (max_range_cm < 1.0f) max_range_cm = 1.0f;
    const int fall_timeout_us = (int)(max_range_cm * 2.0f / 0.0343f) + 300;

    // Trigger pulse
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
            return {-1.0f, false, Reading::State::NO_ECHO};
        }
    }

    // Measure high pulse width with timeout
    Timer t;
    t.start();
    while (_echo.read() == 1) {
        if ((int)chrono::duration_cast<chrono::microseconds>(t.elapsed_time()).count() > fall_timeout_us) {
            return {-2.0f, false, Reading::State::MAX_RANGE};
        }
    }
    t.stop();

    const int time_us = (int)chrono::duration_cast<chrono::microseconds>(t.elapsed_time()).count();
    if (time_us <= 0) {
        return {-3.0f, false, Reading::State::TIMING_ERR};
    }

    const float cm = (float)time_us * 0.0343f / 2.0f;
    return {cm, true, Reading::State::VALID};
}

float UltrasonicSensor::read_cm_value(float max_range_cm)
{
    Reading r = read_cm(max_range_cm);
    return r.valid ? r.cm : -1.0f;
}