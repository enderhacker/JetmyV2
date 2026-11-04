#include "chaos.h"

namespace SoundPayloads {
float ReverseShrapnel(uint32_t t, Voice& voice) {
    using namespace Chaos;
    uint32_t part = t % 2800u;
    float rise = float(part) / 2800;
    float carrier = sinf(Tau * Advance(voice.phase, 2800 - 2400 * rise * rise));
    float ring = sinf(Tau * Advance(voice.secondPhase, 137 + 900 * rise));
    float noise = voice.Noise();
    float high = noise - voice.memory[0];
    voice.memory[0] = noise;
    float envelope = rise * rise * rise;
    return tanhf((carrier * ring + high * 0.35f) * envelope * 4);
}
}
