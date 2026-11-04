#include "chaos.h"

namespace SoundPayloads {
float RatchetCrash(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float cycle = fmodf(Seconds(t), 2.7f) / 2.7f;
    float tooth = Advance(voice.phase, 12 + cycle * cycle * 170);
    float impact = expf(-tooth * 22);
    float metal = sinf(Tau * Advance(voice.secondPhase, 1100 + cycle * 1900));
    voice.memory[0] = voice.memory[0] * 0.91f + voice.Noise() * 0.09f;
    return tanhf((metal * impact + (voice.Noise() - voice.memory[0]) * impact * 0.7f) * 4);
}
}
