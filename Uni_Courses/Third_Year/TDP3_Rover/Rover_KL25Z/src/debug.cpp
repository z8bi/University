#include "debug.hpp"
#include "types.hpp"

#include <cstdio>   // snprintf
#include <cstring>  // strlen

namespace Debug {

//======================================================
//================== INTERNAL STATE ====================
//======================================================

static bool inited = false;

static bool usb_on = true;
static bool bt_on  = false;

// USB serial
static BufferedSerial usb(USBTX, USBRX, 115200);

// BT serial (constructed lazily to allow default pins)
static BufferedSerial* bt = nullptr;

// Default BT pins for FRDM-KL25Z UART2:
static constexpr PinName DEFAULT_BT_TX   = PTE22; // UART2_TX
static constexpr PinName DEFAULT_BT_RX   = PTE23; // UART2_RX
static constexpr int     DEFAULT_BT_BAUD = 9600;

// 1 Hz print ticker
static Ticker printTick;
static volatile bool print_due = false;
static void print_isr() { print_due = true; }

// Latest snapshots (written by update_* from main thread)
static LineSnap  line_snap{};
static UltraSnap ultra_snap{};
static ColorSnap color_snap{};
static CtrlState state_snap{};

//======================================================
//================== INTERNAL HELPERS ==================
//======================================================

static void write_all(const char* s, int n) {
    if (n <= 0) return;
    if (usb_on) usb.write(s, n);
    if (bt_on && bt) bt->write(s, n);
}

static void write_str(const char* s) {
    write_all(s, (int)std::strlen(s));
}

static void ensure_init() {
    if (!inited) {
        // HARD DEFAULT: USB on, BT on, PTE22/PTE23 @ 9600
        init(true, true, DEFAULT_BT_TX, DEFAULT_BT_RX, DEFAULT_BT_BAUD);
    }
}

static const char* light_str(LightState ls) {
    switch (ls) {
        case LightState::RED:   return "RED";
        case LightState::GREEN: return "GREEN";
        default:                return "NONE";
    }
}

static const char* state_str(CtrlState st) {
    switch (st) {
        case CtrlState::FOLLOW:         return "FOLLOW";
        case CtrlState::SEEK_LEFT:      return "SEEK_LEFT";
        case CtrlState::SEEK_RIGHT:     return "SEEK_RIGHT";
        case CtrlState::ALIGN:          return "ALIGN";
        case CtrlState::BRAKING:        return "BRAKING";
        case CtrlState::OBSTACLE_AVOID: return "OBSTACLE_AVOID";
        case CtrlState::STOPPED:        return "STOPPED";
        default:                        return "UNK";
    }
}

//======================================================
//================== PUBLIC API ========================
//======================================================

void init(bool enable_usb, bool enable_bt, PinName bt_tx, PinName bt_rx, int bt_baud) {
    usb_on = enable_usb;
    bt_on  = enable_bt;

    usb.set_blocking(false);

    if (bt_on) {
        if (bt_tx == NC) bt_tx = DEFAULT_BT_TX;
        if (bt_rx == NC) bt_rx = DEFAULT_BT_RX;
        if (bt_baud <= 0) bt_baud = DEFAULT_BT_BAUD;

        static BufferedSerial bt_static(bt_tx, bt_rx, bt_baud);
        bt_static.set_blocking(false);
        bt = &bt_static;
    }

    printTick.attach(&print_isr, 1s);

    write_str("\r\nDEBUG INIT\r\n");
    inited = true;
}

// Lightweight printf-style logging (non-blocking serial)
void log(const char* fmt, ...) {
    ensure_init();

    char buf[256];

    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Ensure line ends nicely if caller didn't include it (optional)
    // write_all(buf, n);
    if (n > 0) write_all(buf, n);
}

LineSnap make_line(const Sensors& s, const LineInfo& li, bool valid) {
    LineSnap o; o.s = s; o.li = li; o.valid = valid; return o;
}

UltraSnap make_ultra(float front_cm, bool front_ok, float right_cm, bool right_ok) {
    UltraSnap o;
    o.front_cm = front_cm; o.front_ok = front_ok;
    o.right_cm = right_cm; o.right_ok = right_ok;
    return o;
}

ColorSnap make_color(const tcs3472::RGBC& v, bool valid, LightState ls) {
    ColorSnap o; o.rgbc = v; o.valid = valid; o.ls = ls; return o;
}

void update_line(const LineSnap& l)   { line_snap  = l; }
void update_ultra(const UltraSnap& u) { ultra_snap = u; }
void update_color(const ColorSnap& c) { color_snap = c; }
void update_state(CtrlState st)       { state_snap = st; }

// Call this frequently (your main loop), prints at 1 Hz
void tick() {
    ensure_init();
    if (!print_due) return;
    print_due = false;

    // Take a local snapshot once (cleaner + avoids mixing old/new fields)
    const LineSnap  lsnap = line_snap;
    const UltraSnap usnap = ultra_snap;
    const ColorSnap csnap = color_snap;
    const CtrlState st    = state_snap;

    char buf[280];

    // Which sensors see the line: s.on[0..4]
    const bool* on = lsnap.s.on;

    if (csnap.valid && csnap.rgbc.c > 0) {
        const auto& v = csnap.rgbc;
        const unsigned rP = (100U * v.r) / v.c;
        const unsigned gP = (100U * v.g) / v.c;
        const unsigned bP = (100U * v.b) / v.c;

        const int n = snprintf(buf, sizeof(buf),
            "ON=[%d%d%d%d%d] | %s | F=%.1fcm(%d) R=%.1fcm(%d) | %s | "
            "R=%u G=%u B=%u C=%u | r=%u%% g=%u%% b=%u%%\r\n",
            on[0], on[1], on[2], on[3], on[4],
            state_str(st),
            usnap.front_cm, (int)usnap.front_ok,
            usnap.right_cm, (int)usnap.right_ok,
            light_str(csnap.ls),
            v.r, v.g, v.b, v.c,
            rP, gP, bP
        );
        write_all(buf, n);
    } else {
        const int n = snprintf(buf, sizeof(buf),
            "ON=[%d%d%d%d%d] | %s | F=%.1fcm(%d) R=%.1fcm(%d) | %s | No valid color\r\n",
            on[0], on[1], on[2], on[3], on[4],
            state_str(st),
            usnap.front_cm, (int)usnap.front_ok,
            usnap.right_cm, (int)usnap.right_ok,
            light_str(csnap.ls)
        );
        write_all(buf, n);
    }
}

} // namespace Debug
