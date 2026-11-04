#include "chaos.h"

namespace SoundPayloads {
float RingModTritoneClash(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float frequency = 550 + 170 * sinf(Seconds(t) * 0.8f);
    float a = Tau * Advance(voice.phase, frequency);
    float b = Tau * Advance(voice.secondPhase, frequency * 1.41421356237f);
    float first = Clip(3 * sinf(a + 1.7f * voice.memory[1]));
    float second = Clip(3 * sinf(b + 1.7f * voice.memory[0]));
    voice.memory[0] = first; voice.memory[1] = second;
    return Clip(first * second * 1.4f + 0.15f * (first + second));
}
} // namespace SoundPayloads
