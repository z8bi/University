#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "mbed.h"
#include "types.hpp"        // CtrlState, LightState
#include "sensors.hpp"      // Sensors, LineInfo
#include "tcs3472.hpp"      // tcs3472::RGBC

namespace Debug {

//==================== PACKETS ====================

struct LinePacket {
    Sensors  s{};
    LineInfo li{};
    bool     valid = false;
};

struct UltraPacket {
    float front_cm = -1.0f;
    bool  front_valid = false;
    float right_cm = -1.0f;
    bool  right_valid = false;
};

struct ColorPacket {
    tcs3472::RGBC rgbc{};
    bool          valid = false;
    LightState    light = LightState::NONE;
};

//==================== BUILDERS ====================

LinePacket  make_line(const Sensors& s, const LineInfo& li, bool valid);
UltraPacket make_ultra(float front_cm, bool front_valid, float right_cm, bool right_valid);
ColorPacket make_color(const tcs3472::RGBC& v, bool valid, LightState light);

//==================== INIT / LOG ====================

// Defaults: USB on, BT off.
// Default BT pins: TX=PTE22, RX=PTE23, 9600 baud.
void init(bool enable_usb = true,
          bool enable_bt  = false,
          PinName bt_tx   = PTE22,
          PinName bt_rx   = PTE23,
          int bt_baud     = 9600);

void log(const char* msg);

//==================== UPDATE ====================

void update_state(CtrlState st);
void update_line(const LinePacket& p);
void update_ultra(const UltraPacket& p);
void update_color(const ColorPacket& p);

//==================== PRINT ====================

// Call at 1 Hz (or whenever). Prints: sensors array, state, distances, light, RGB+C and %.
void tick();

} // namespace Debug

#endif
