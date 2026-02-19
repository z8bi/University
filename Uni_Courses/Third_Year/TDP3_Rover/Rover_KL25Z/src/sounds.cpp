#include "sounds.hpp"

const uint32_t mario_style_rate = 8000;

const uint8_t mario_style[] = {
    128,176,217,245,255,245,217,176,128, 80, 39, 11,  1, 11, 39, 80,
    128,176,217,245,255,245,217,176,128, 80, 39, 11,  1, 11, 39, 80,
    128,176,217,245,255,245,217,176,128, 80, 39, 11,  1, 11, 39, 80,
    128,128,128,128,128,128,128,128,
    128,200,240,255,240,200,128, 56, 16,  1, 16, 56,
    128,200,240,255,240,200,128, 56, 16,  1, 16, 56,
    128,176,217,245,255,245,217,176,128
};

const uint32_t mario_style_len = sizeof(mario_style);

// (Optional) also define s_beep here if you want:
const uint32_t s_beep_rate = 8000;
const uint8_t s_beep[] = { 128,176,217,245,255,245,217,176,128,80,39,11,1,11,39,80,128 };
const uint32_t s_beep_len = sizeof(s_beep);
