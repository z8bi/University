#pragma once

#include "mbed.h"
#include "types.hpp"
#include "tcs3472.hpp"
#include "sensors.hpp"

namespace Debug {

struct LineSnap {
    Sensors  s{};
    LineInfo li{};
    bool     valid{false};
};

struct UltraSnap {
    float front_cm{-1.0f};
    bool  front_ok{false};
    float right_cm{-1.0f};
    bool  right_ok{false};
};

struct ColorSnap {
    tcs3472::RGBC rgbc{};
    bool          valid{false};
    LightState    ls{LightState::NONE};
};

void init(bool enable_usb = true,
          bool enable_bt  = false,
          PinName bt_tx   = NC,
          PinName bt_rx   = NC,
          int bt_baud     = 9600);

// ADD THIS:
void log(const char* fmt, ...);

LineSnap  make_line(const Sensors& s, const LineInfo& li, bool valid);
UltraSnap make_ultra(float front_cm, bool front_ok, float right_cm, bool right_ok);
ColorSnap make_color(const tcs3472::RGBC& v, bool valid, LightState ls);

void update_line(const LineSnap& l);
void update_ultra(const UltraSnap& u);
void update_color(const ColorSnap& c);
void update_state(CtrlState st);

void tick(); // call often; it prints at 1 Hz internally

} // namespace Debug
