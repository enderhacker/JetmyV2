#include "chaos.h"

namespace SoundPayloads {
float SquareWaveStrobe(uint32_t t, Voice& voice) {
    float tone = Chaos::Square(Chaos::Advance(voice.phase, 800));
    return t % (SampleRate / 25u) < SampleRate / 50u ? tone : 0;
}
} // namespace SoundPayloads
