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
} // namespace SoundPayloads
