#include "chaos.h"

namespace SoundPayloads {
float BufferFreezeStutter(uint32_t t, Voice& voice) {
    using namespace Chaos;
    if (t == 0) {
        for (size_t i = 0; i < 120; ++i)
            voice.buffer[i] = 0.6f * sinf(Tau * float(i) * 730 / SampleRate) + 0.4f * voice.Noise();
    }
    if (t % 500u == 0) {
        voice.loopSize = 80u + voice.random() % 40u;
        voice.memory[0] = 0.92f + float(voice.random() % 17u) * 0.01f;
        voice.position = fmodf(voice.position + float(voice.random() % 11u), float(voice.loopSize));
    }
    float sample = voice.buffer[static_cast<size_t>(voice.position)];
    voice.position = fmodf(voice.position + voice.memory[0], float(voice.loopSize));
    return Clip(sample * 2.5f);
}

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
