#include "music.h"

namespace SoundPayloads {
float BytebeatCrystalWaltz(uint32_t t, Voice& voice) {
    using namespace Music;
    // Six bars in 3/4 at 108 BPM; music-box melody over alternating bass/chords.
    constexpr int roots[] = {48, 45, 41, 43, 48, 43};
    constexpr int melody[] = {76,79,84,83,79,76, 76,81,84,81,79,76,
        77,81,84,81,77,76, 74,79,83,81,79,74, 76,79,84,88,86,84, 83,79,77,74,71,72};
    auto clock = Sequence(t, 36);
    uint32_t step = clock.step % 36, bar = step / 6;
    float phase = clock.phase, beat = (float(step % 2) + phase) * 0.5f;
    int note = melody[step];
    float fundamental = Tone(voice, 0, note, Wave::Triangle);
    float overtone = Tone(voice, 1, note + 12, Wave::Sine);
    float bell = fundamental * Pluck(phase) * 0.31f + overtone * expf(-phase * 9) * Gate(phase) * 0.08f;
    int root = roots[bar];
    float bass = Tone(voice, 2, root - 12, Wave::Sine) * (step % 6 < 2 ? Pluck(beat) : 0);
    float chord = Tone(voice, 3, root + (bar == 1 ? 3 : 4), Wave::Triangle)
        + Tone(voice, 4, root + 7, Wave::Triangle);
    return bell + bass * 0.22f + chord * (step % 6 >= 2 ? Pluck(beat) * 0.09f : 0);
}
}
