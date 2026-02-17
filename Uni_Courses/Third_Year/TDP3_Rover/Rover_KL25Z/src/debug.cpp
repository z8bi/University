#include "debug.hpp"
#include <cstdio>
#include <cstring>

namespace Debug {

static BufferedSerial* usb = nullptr;
static BufferedSerial* bt  = nullptr;

static bool usb_enabled = false;
static bool bt_enabled  = false;

// Latest snapshots
static CtrlState   st_latest = CtrlState::FOLLOW;
static LinePacket  line_latest{};
static UltraPacket ultra_latest{};
static ColorPacket color_latest{};

static void write_all(const char* s, int n)
{
    if (n <= 0) return;
    if (usb_enabled && usb) usb->write(s, n);
    if (bt_enabled  && bt)  bt->write(s, n);
}

static const char* state_str(CtrlState st)
{
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

static const char* light_str(LightState ls)
{
    switch (ls) {
        case LightState::RED:   return "RED";
        case LightState::GREEN: return "GREEN";
        case LightState::NONE:  return "NONE";
        default:                return "UNK";
    }
}

//==================== BUILDERS ====================

LinePacket make_line(const Sensors& s, const LineInfo& li, bool valid)
{
    LinePacket p;
    p.s = s;
    p.li = li;
    p.valid = valid;
    return p;
}

UltraPacket make_ultra(float front_cm, bool front_valid, float right_cm, bool right_valid)
{
    UltraPacket p;
    p.front_cm = front_cm;
    p.front_valid = front_valid;
    p.right_cm = right_cm;
    p.right_valid = right_valid;
    return p;
}

ColorPacket make_color(const tcs3472::RGBC& v, bool valid, LightState light)
{
    ColorPacket p;
    p.rgbc = v;
    p.valid = valid;
    p.light = light;
    return p;
}

//==================== INIT / LOG ====================

void init(bool enable_usb, bool enable_bt, PinName bt_tx, PinName bt_rx, int bt_baud)
{
    usb_enabled = enable_usb;
    bt_enabled  = enable_bt;

    if (usb_enabled) {
        static BufferedSerial usb_ser(USBTX, USBRX, 115200);
        usb_ser.set_blocking(false);
        usb = &usb_ser;
    }

    if (bt_enabled) {
        static BufferedSerial bt_ser(bt_tx, bt_rx, bt_baud);
        bt_ser.set_blocking(false);
        bt = &bt_ser;
    }

    log("\r\nDEBUG INIT OK\r\n");
}

void log(const char* msg)
{
    if (!msg) return;
    write_all(msg, (int)strlen(msg));
}

//==================== UPDATE ====================

void update_state(CtrlState st)
{
    core_util_critical_section_enter();
    st_latest = st;
    core_util_critical_section_exit();
}

void update_line(const LinePacket& p)
{
    core_util_critical_section_enter();
    line_latest = p;
    core_util_critical_section_exit();
}

void update_ultra(const UltraPacket& p)
{
    core_util_critical_section_enter();
    ultra_latest = p;
    core_util_critical_section_exit();
}

void update_color(const ColorPacket& p)
{
    core_util_critical_section_enter();
    color_latest = p;
    core_util_critical_section_exit();
}

//==================== PRINT ====================

void tick()
{
    CtrlState st;
    LinePacket lp;
    UltraPacket up;
    ColorPacket cp;

    core_util_critical_section_enter();
    st = st_latest;
    lp = line_latest;
    up = ultra_latest;
    cp = color_latest;
    core_util_critical_section_exit();

    int s0 = lp.s.on[0] ? 1 : 0;
    int s1 = lp.s.on[1] ? 1 : 0;
    int s2 = lp.s.on[2] ? 1 : 0;
    int s3 = lp.s.on[3] ? 1 : 0;
    int s4 = lp.s.on[4] ? 1 : 0;

    // Fixed-point ultrasonic (avoid %f)
    int front_dcm = (int)(up.front_cm * 10.0f);
    int right_dcm = (int)(up.right_cm * 10.0f);

    char buf[260];

    int n = snprintf(buf, sizeof(buf),
        "SENS:[%d %d %d %d %d] | "
        "STATE:%s | "
        "FRONT:%d.%dcm(%d) RIGHT:%d.%dcm(%d) | "
        "LIGHT:%s | "
        "RGB:C R=%u G=%u B=%u C=%u\r\n",
        s0, s1, s2, s3, s4,
        state_str(st),
        front_dcm/10, abs(front_dcm%10), up.front_valid ? 1 : 0,
        right_dcm/10, abs(right_dcm%10), up.right_valid ? 1 : 0,
        light_str(cp.light),
        cp.rgbc.r, cp.rgbc.g, cp.rgbc.b, cp.rgbc.c
    );

    if (n < 0) return;
    if (n >= (int)sizeof(buf)) n = sizeof(buf) - 1;
    write_all(buf, n);
}

} // namespace Debug
