#include "chaos.h"

namespace SoundPayloads {
float BrokenVowel(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float seconds = Seconds(t);
    float fundamental = 73 + 27 * sinf(seconds * 3.1f);
    float glottis = Advance(voice.phase, fundamental);
    float envelope = expf(-glottis * 8);
    float a = sinf(Tau * Advance(voice.phases[0], 620 + 310 * sinf(seconds * 1.7f)));
    float b = sinf(Tau * Advance(voice.phases[1], 1700 + 500 * sinf(seconds * 2.3f)));
    return tanhf((a + b * 0.65f + voice.Noise() * 0.15f) * envelope * 5);
}
}
