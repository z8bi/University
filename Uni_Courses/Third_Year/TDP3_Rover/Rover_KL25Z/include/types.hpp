#pragma once

enum class CtrlState {
    FOLLOW,
    SEEK_LEFT,
    SEEK_RIGHT,
    ALIGN,
    BRAKING,
    OBSTACLE_AVOID,
    STOPPED
};

enum class LightState {
    NONE,
    RED,
    GREEN
};
