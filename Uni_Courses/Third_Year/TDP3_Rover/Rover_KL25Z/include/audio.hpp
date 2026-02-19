#pragma once
#include "mbed.h"

// Simple 8-bit unsigned PCM player using DAC + Ticker.
// Assumes samples are 0..255, mono.
// Sample rate default: 8000 Hz.

class AudioPlayer {
public:
    AudioPlayer(PinName dac_pin) : _dac(dac_pin) {}

    void init() {
        stop();
        _dac.write_u16(0x8000); // mid-scale
    }

    // Start playing a buffer (non-blocking)
    void play_u8_mono(const uint8_t* data, uint32_t len, uint32_t sample_rate_hz = 8000) {
        if (!data || len == 0 || sample_rate_hz == 0) return;

        core_util_critical_section_enter();
        _data = data;
        _len  = len;
        _idx  = 0;
        _playing = true;
        core_util_critical_section_exit();

        const float dt = 1.0f / (float)sample_rate_hz;
        _tick.detach();
        _tick.attach(callback(this, &AudioPlayer::isr), dt);
    }

    void stop() {
        _tick.detach();
        core_util_critical_section_enter();
        _playing = false;
        _data = nullptr;
        _len = 0;
        _idx = 0;
        core_util_critical_section_exit();
        _dac.write_u16(0x8000); // mid-scale
    }

    bool is_playing() const { return _playing; }

private:
    void isr() {
        // Keep ISR tiny + integer-only
        if (!_playing || !_data) return;

        uint32_t i = _idx;
        if (i >= _len) {
            stop(); // safe enough; ticker detach is ok here on Mbed
            return;
        }

        // 8-bit unsigned -> 16-bit for write_u16 (0..65535)
        const uint16_t v = ((uint16_t)_data[i]) << 8; // 0..65280
        _dac.write_u16(v);

        _idx = i + 1;
    }

    AnalogOut _dac;
    Ticker _tick;

    volatile bool _playing{false};
    const uint8_t* volatile _data{nullptr};
    volatile uint32_t _len{0};
    volatile uint32_t _idx{0};
};
