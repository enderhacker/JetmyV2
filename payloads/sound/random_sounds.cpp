#include "sound.h"
#include <cmath>

namespace SoundPayloads {
float RandomChirps(uint32_t t, std::mt19937& random) {
    // Pick a new note every 1/4 second, not a new frequency for every sample.
    static float frequency = 440;
    constexpr uint32_t noteLength = SampleRate / 4;
    uint32_t position = t % noteLength;
    if (position == 0) frequency = std::uniform_real_distribution<float>(180, 1200)(random);
    float local = static_cast<float>(position) / SampleRate;
    float envelope = sinf(3.14159265f * position / noteLength);
    return sinf(6.2831853f * (frequency * local + 350 * local * local)) * envelope;
}
float NoisePercussion(uint32_t t, std::mt19937& random) {
    float local = static_cast<float>(t % (SampleRate / 3)) / SampleRate;
    float noise = std::uniform_real_distribution<float>(-1, 1)(random);
    float kick = sinf(6.2831853f * (90 * local - 75 * local * local));
    return (0.55f * noise + 0.45f * kick) * expf(-24 * local);
}
}
