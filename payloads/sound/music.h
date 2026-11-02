#pragma once
#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads::Music {
inline constexpr float Tau = 6.28318530718f;

struct Clock { uint32_t step; float phase; };
// Exact rational sequencing: each complete arrangement occupies ten seconds.
inline Clock Sequence(uint32_t sample, uint32_t steps) {
    constexpr uint32_t length = SampleRate * 10;
    uint64_t position = static_cast<uint64_t>(sample) * steps;
    return {static_cast<uint32_t>(position / length), float(position % length) / length};
}
inline const std::array<float, 128>& Frequencies() {
    static const auto notes = [] {
        std::array<float, 128> result{};
        for (size_t i = 0; i < result.size(); ++i) result[i] = 440 * std::exp2((float(i) - 69) / 12);
        return result;
    }();
    return notes;
}
enum class Wave { Triangle, Pulse, Saw, Sine };
inline float Tone(Voice& voice, size_t channel, int note, Wave wave, uint32_t duty = 128) {
    if (note < 0) return 0; // Explicit rests; no random out-of-scale notes.
    note = std::clamp(note, 12, 95); // Keep fundamentals below Nyquist at 8 kHz.
    if (voice.songNote[channel] != note) {
        voice.songNote[channel] = note;
        voice.songIncrement[channel] = static_cast<uint32_t>(
            double(Frequencies()[note]) * (4294967296.0 / SampleRate));
    }
    uint32_t phase = (voice.songPhase[channel] += voice.songIncrement[channel]);
    uint32_t byte = phase >> 24;
    switch (wave) {
    case Wave::Triangle: {
        uint32_t ramp = phase >> 23;
        uint32_t folded = (ramp & 256) ? 511 - ramp : ramp;
        return (float(folded) - 127.5f) / 127.5f;
    }
    case Wave::Pulse: return byte < duty ? 0.65f : -0.65f;
    case Wave::Saw: return (float(byte) - 127.5f) / 127.5f;
    case Wave::Sine: return sinf(float(phase >> 8) * (Tau / 16777216.0f));
    }
    return 0;
}
inline float Gate(float phase, float hold = 0.86f) {
    return std::clamp(phase * 40, 0.0f, 1.0f) * std::clamp((hold - phase) * 20, 0.0f, 1.0f);
}
inline float Pluck(float phase) {
    float tail = 1 - phase;
    return std::min(1.0f, phase * 48) * tail * tail;
}
inline float Kick(float phase) {
    return sinf(Tau * (phase * 7 + 0.6f * (1 - expf(-phase * 22)))) * expf(-phase * 9)
        * std::min(1.0f, phase * 80);
}
inline float Hat(float noise, float phase) { return noise * expf(-phase * 38); }
inline float Snare(float noise, float phase) {
    return (noise * 0.75f + sinf(phase * Tau * 28) * 0.25f) * expf(-phase * 17);
}
} // namespace SoundPayloads::Music
