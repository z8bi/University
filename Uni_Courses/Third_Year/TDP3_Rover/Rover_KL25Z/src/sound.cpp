/*#include "mbed.h"

//========================SOUND=========================

// Playback config (most common voice setting)
static constexpr int SAMPLE_RATE = 8000;

// Playback state
static volatile uint32_t sample_i = 0;
static volatile bool playing = false;

// Read one int16 little-endian sample from the byte array
static inline int16_t read_i16le(const unsigned char* p) {
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

void audio_isr() {
    if (!playing) {
        dac.write(0.5f);
        return;
    }

    uint32_t byte_i = sample_i * 2;
    if (byte_i + 1 >= lib_Sounds_car_x_pcm_len) {
        playing = false;
        sample_i = 0;
        dac.write(0.5f);
        return;
    }

    int16_t s = read_i16le(&lib_Sounds_car_x_pcm[byte_i]);
    sample_i++;

    // Map -32768..+32767 -> 0.0..1.0
    float v = 0.5f + (float)s / 65536.0f;
    dac.write(v);
}

// Call this to play the clip once
void play_car_x() {
    sample_i = 0;
    playing = true;
}

// Optional
bool is_playing() { return playing; }

*/
