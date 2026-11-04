#include "chaos.h"

namespace SoundPayloads {
float DualToneSirenWarp(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float ramp = Seconds(t) * Tau * 6;
    float a = Square(Advance(voice.phase, 450 + 3350 * fabsf(sinf(ramp))));
    float b = Square(Advance(voice.secondPhase, 700 + 2500 * fabsf(cosf(ramp))));
    return Clip((a + b) * 0.65f);
}
} // namespace SoundPayloads
