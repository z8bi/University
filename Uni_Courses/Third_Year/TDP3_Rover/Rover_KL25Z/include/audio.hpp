#pragma once
#include "mbed.h"

// Simple 8-bit unsigned PCM player using DAC + Ticker.
// ISR-safe: the ISR never calls detach/stop or anything that may lock.
// Call service() periodically from thread context (e.g., main loop).

class AudioPlayer {
public:
    explicit AudioPlayer(PinName dac_pin) : _dac(dac_pin) {}

    void init() {
        stop();
        _dac.write_u16(0x8000); // mid-scale
    }

    // Start playing a buffer (non-blocking)
    void play_u8_mono(const uint8_t* data, uint32_t len, uint32_t sample_rate_hz = 8000) {
        if (!data || len == 0 || sample_rate_hz == 0) return;

        // Stop any current playback (thread context)
        stop();

        core_util_critical_section_enter();
        _data = data;
        _len  = len;
        _idx  = 0;
        _done = false;
        _playing = true;
        core_util_critical_section_exit();

        const float dt = 1.0f / (float)sample_rate_hz;
        _tick.attach(callback(this, &AudioPlayer::isr), dt);
    }

    // Must be called from thread context (main loop).
    // Finishes a stop requested by the ISR (end of buffer).
    void service() {
        if (_done) {
            stop();
        }
    }

    // Thread-context stop (safe to call from main)
    void stop() {
        _tick.detach();

        core_util_critical_section_enter();
        _playing = false;
        _done = false;
        _data = nullptr;
        _len = 0;
        _idx = 0;
        core_util_critical_section_exit();

        _dac.write_u16(0x8000); // mid-scale
    }

    bool is_playing() const { return _playing; }

private:
    void isr() {
        // ISR must be tiny + lock-free
        if (!_playing || !_data) return;

        const uint32_t i = _idx;
        if (i >= _len) {
            // DO NOT call stop() or detach() here.
            _playing = false;
            _done = true;
            return;
        }

        const uint16_t v = ((uint16_t)_data[i]) << 8; // 0..65280
        _dac.write_u16(v);

        _idx = i + 1;
    }

    AnalogOut _dac;
    Ticker _tick;

    volatile bool _playing{false};
    volatile bool _done{false};                // set by ISR when finished
    const uint8_t* volatile _data{nullptr};
    volatile uint32_t _len{0};
    volatile uint32_t _idx{0};
};
