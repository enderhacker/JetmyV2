#include "chaos.h"

namespace SoundPayloads {
float SubharmonicMeltdown(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float sub = Square(Advance(voice.phase, 32));
    float unstable = 320 + 32 * sinf(Seconds(t) * 3.7f) + voice.Noise() * 9;
    float carrier = Square(Advance(voice.secondPhase, unstable));
    float neighbor = Square(Advance(voice.phases[0], unstable + 32 + 7 * sinf(Seconds(t) * 0.9f)));
    return tanhf(sub * 0.7f + carrier * neighbor * 1.2f + sub * carrier * 0.4f);
}
} // namespace SoundPayloads
