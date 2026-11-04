#include "chaos.h"

namespace SoundPayloads {
float ModemCarrierHell(uint32_t t, Voice& voice) {
    using namespace Chaos;
    if (t % 13u == 0) voice.memory[0] = voice.Noise() > 0 ? 1.0f : -1.0f;
    float pilot = sinf(Tau * Advance(voice.phase, 1800));
    float quadrature = sinf(Tau * Advance(voice.secondPhase, 2400 + 300 * voice.memory[0]));
    float noise = voice.Noise();
    float hash = noise - voice.memory[1];
    voice.memory[1] = noise;
    // Stylized handshake texture, not a V.34 protocol implementation.
    return tanhf(2.6f * (pilot * 0.4f + quadrature * voice.memory[0] * 0.5f + hash * 0.35f));
}
} // namespace SoundPayloads
