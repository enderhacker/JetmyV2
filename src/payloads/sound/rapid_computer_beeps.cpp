#include "chaos.h"

namespace SoundPayloads {
float PCSpeakerBeepStorm(uint32_t t, Voice& voice) {
    using namespace Chaos;
    constexpr float tones[] = {440, 880, 1174, 1760, 2349};
    float sum = 0;
    for (size_t i = 0; i < 4; ++i) {
        if (t == 0 || voice.timers[i] == 0) {
            voice.frequencies[i] = tones[voice.random() % 5u];
            voice.timers[i] = SampleRate / 100u + voice.random() % (SampleRate * 11u / 100u + 1u);
        }
        float beep = Square(Advance(voice.phases[i], voice.frequencies[i]));
        // Each independent timer ends with a 5 ms silence gap; parity flips the beeps.
        if (voice.timers[i] > SampleRate / 200u)
            sum += beep * (((t >> (i + 3)) & 1u) ? 1.0f : -1.0f);
        --voice.timers[i];
    }
    return Clip(sum * 0.35f);
}
} // namespace SoundPayloads
