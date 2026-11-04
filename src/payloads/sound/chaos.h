#pragma once
#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads::Chaos {
inline constexpr float Tau = 6.28318530718f;
inline float Advance(float& phase, float frequency) {
    phase += frequency / SampleRate;
    phase -= floorf(phase);
    return phase;
}
inline float Square(float phase) { return phase < 0.5f ? 1.0f : -1.0f; }
inline float Byte(uint32_t value) { return float(value & 255u) / 127.5f - 1.0f; }
inline float Clip(float value) { return std::clamp(value, -1.0f, 1.0f); }
inline float Seconds(uint32_t sample) { return float(sample) / SampleRate; }
} // namespace SoundPayloads::Chaos
