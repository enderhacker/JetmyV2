#include "chaos.h"

namespace SoundPayloads {
float BitRotQuantizer(uint32_t t, Voice& voice) {
    using namespace Chaos;
    unsigned bits = 8u - (t / (SampleRate / 2u)) % 8u;
    // Progressively longer sample holds deliberately introduce digital aliasing.
    unsigned hold = 1u + (8u - bits) * 2u;
    float phase = Advance(voice.phase, 173 + 1100 * fabsf(sinf(Seconds(t) * 1.7f)));
    if (t % hold == 0) {
        float source = 0.7f * (2 * phase - 1) + 0.3f * voice.Noise();
        float offset = ((t / 997u) & 1u) ? 0.22f : -0.22f;
        if (bits == 1) {
            float duty = 0.15f + 0.7f * (0.5f + 0.5f * sinf(Seconds(t) * 5));
            voice.memory[0] = phase < duty ? 1.0f : -1.0f;
        } else {
            float levels = float((1u << bits) - 1u);
            voice.memory[0] = roundf((Clip(source + offset) + 1) * 0.5f * levels) * 2 / levels - 1;
        }
    }
    return voice.memory[0];
}
} // namespace SoundPayloads
