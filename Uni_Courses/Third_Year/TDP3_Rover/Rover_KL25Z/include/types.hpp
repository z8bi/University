#pragma once

enum class CtrlState {
    FOLLOW,
    SEEK_LEFT,
    SEEK_RIGHT,
    ALIGN,
    BRAKING,
    OBSTACLE_AVOID,
    STOPPED,         // traffic-light stop logic
    FULLY_STOPPED,    // absolute stop (BT stop / startup hold)
    MANUAL_BT       // manual BT control (WASD)
};


enum class LightState {
    NONE,
    RED,
    GREEN
};
