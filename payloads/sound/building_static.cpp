#include "chaos.h"

namespace SoundPayloads {
float RadiationAvalanche(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float progress = float(t % (SampleRate * 10u)) / (SampleRate * 10u);
    float rate = 2 * expf(progress * 9); // Poisson arrivals: sparse clicks -> overlapping static.
    float probability = 1 - expf(-rate / SampleRate);
    float uniform = (voice.Noise() + 1) * 0.5f;
    voice.memory[0] *= 0.78f;
    if (uniform < probability) voice.memory[0] += voice.Noise() * 2;
    return tanhf(voice.memory[0] * (1 + progress * 6));
}
} // namespace SoundPayloads
