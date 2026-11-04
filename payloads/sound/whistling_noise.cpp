#include "chaos.h"

namespace SoundPayloads {
float CombFilterScreamer(uint32_t t, Voice& voice) {
    using namespace Chaos;
    // Fractional 2.2--8 sample delay sweeps the fundamental through ~1--3.6 kHz.
    // The 8 kHz stream cannot reproduce upper-human-hearing frequencies.
    float delay = 2.2f + 5.8f * (0.5f + 0.5f * sinf(Seconds(t) * 2.3f));
    float read = fmodf(float(voice.cursor) + float(voice.buffer.size()) - delay,
                       float(voice.buffer.size()));
    size_t first = static_cast<size_t>(read), next = (first + 1) % voice.buffer.size();
    float fraction = read - float(first);
    float echo = voice.buffer[first] * (1 - fraction) + voice.buffer[next] * fraction;
    float value = tanhf(voice.Noise() * 0.2f + echo * 0.97f);
    voice.buffer[voice.cursor] = value;
    voice.cursor = (voice.cursor + 1u) % static_cast<uint32_t>(voice.buffer.size());
    return tanhf(value * 3);
}
} // namespace SoundPayloads
