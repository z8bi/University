#pragma once
#include "mbed.h"

#if defined(TARGET_KL25Z)
  #include "MKL25Z4.h"
#endif

class AudioPlayer {
public:
    explicit AudioPlayer(PinName dac_pin) : _dac_pin(dac_pin) {}

    void init() {
#if defined(TARGET_KL25Z)
        SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;

        DAC0->C0 = 0;
        DAC0->C1 = 0;
        DAC0->C2 = 0;

        // Enable DAC, VDDA reference
        DAC0->C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;

        write_dac12(2048);
#else
        _ao = new AnalogOut(_dac_pin);
        _ao->write_u16(0x8000);
#endif
        // reset internal state (no detach needed yet)
        core_util_critical_section_enter();
        _playing = false;
        _data = nullptr;
        _len = 0;
        _idx = 0;
        core_util_critical_section_exit();
    }

    void play_u8_mono(const uint8_t* data, uint32_t len, uint32_t sample_rate_hz = 8000) {
        if (!data || len == 0 || sample_rate_hz == 0) return;

        stop();

        core_util_critical_section_enter();
        _data = data;
        _len  = len;
        _idx  = 0;
        _playing = true;
        core_util_critical_section_exit();

        const uint32_t period_us = 1000000UL / sample_rate_hz;
        _tick.attach_us(callback(this, &AudioPlayer::isr), period_us);
    }

    void stop() {
        _tick.detach();

        core_util_critical_section_enter();
        _playing = false;
        _data = nullptr;
        _len = 0;
        _idx = 0;
        core_util_critical_section_exit();

#if defined(TARGET_KL25Z)
        write_dac12(2048);
#else
        if (_ao) _ao->write_u16(0x8000);
#endif
    }

    bool is_playing() const { return _playing; }

private:
    inline void isr() {
        if (!_playing || !_data) return;

        const uint32_t i = _idx;
        if (i >= _len) {
            // stop producing samples; detach in thread if you want,
            // but leaving ticker running is OK since this returns fast.
            _playing = false;
            return;
        }

#if defined(TARGET_KL25Z)
        const uint16_t v12 = ((uint16_t)_data[i]) << 4;
        write_dac12(v12);
#else
        const uint16_t v16 = ((uint16_t)_data[i]) << 8;
        if (_ao) _ao->write_u16(v16);
#endif

        _idx = i + 1;
    }

#if defined(TARGET_KL25Z)
    static inline void write_dac12(uint16_t v) {
        v &= 0x0FFF;
        DAC0->DAT[0].DATL = (uint8_t)(v & 0xFF);
        DAC0->DAT[0].DATH = (uint8_t)((v >> 8) & 0x0F);
    }
#endif

    PinName _dac_pin;
    Ticker _tick;

    volatile bool _playing{false};
    const uint8_t* volatile _data{nullptr};
    volatile uint32_t _len{0};
    volatile uint32_t _idx{0};

#if !defined(TARGET_KL25Z)
    AnalogOut* _ao{nullptr};
#endif
};