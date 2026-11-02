#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads {
float RandomChirps(uint32_t t, Voice& voice) {
    // Sparse, smooth ascending/descending whistles with silence between calls.
    constexpr uint32_t length = SampleRate / 2;
    uint32_t position = t % length;
    if (position == 0) {
        voice.frequency = std::uniform_real_distribution<float>(300, 1100)(voice.random);
        voice.sweep = std::uniform_real_distribution<float>(-450, 900)(voice.random);
        voice.phase = 0;
    }
    float age = float(position) / length;
    if (age >= 0.72f) return 0;
    float frequency = std::clamp(voice.frequency + voice.sweep * age, 120.0f, 2000.0f);
    voice.phase += frequency / SampleRate;
    voice.phase -= floorf(voice.phase);
    float envelope = sinf(3.14159265f * age / 0.72f);
    return sinf(6.2831853f * voice.phase) * envelope * envelope * 0.85f;
}

float NoisePercussion(uint32_t t, Voice& voice) {
    // A repeatable 16-step drum pattern, not continuous white noise.
    constexpr uint32_t stepLength = SampleRate / 8;
    uint32_t position = t % stepLength, step = t / stepLength % 16;
    float seconds = float(position) / SampleRate;
    float noise = voice.Noise();
    voice.filteredNoise += (noise - voice.filteredNoise) * 0.18f;
    float hat = (noise - voice.filteredNoise) * expf(-100 * seconds) * (step % 2 ? 0.12f : 0.25f);
    float kick = 0, snare = 0;
    if (step == 0 || step == 6 || step == 8 || step == 14) {
        float frequency = 45 + 100 * expf(-40 * seconds);
        if (position == 0) voice.phase = 0;
        voice.phase += frequency / SampleRate;
        voice.phase -= floorf(voice.phase);
        kick = sinf(6.2831853f * voice.phase) * expf(-23 * seconds) * 0.9f;
    }
    if (step == 4 || step == 12)
        snare = (voice.filteredNoise * 0.6f + noise * 0.4f) * expf(-30 * seconds) * 0.8f;
    return kick + snare + hat;
}
}
