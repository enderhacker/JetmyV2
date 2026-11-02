#include "music.h"

namespace SoundPayloads {
float BytebeatArcadeQuest(uint32_t t, Voice& voice) {
    using namespace Music;
    constexpr int roots[] = {48, 43, 45, 41};
    constexpr int melody[] = {72,76,79,84,83,79,76,-1, 71,74,79,83,81,79,74,-1,
        72,76,81,84,88,84,81,76, 77,81,84,86,84,81,79,72};
    auto clock = Sequence(t, 64);
    uint32_t step = clock.step % 64, bar = step / 16;
    float eighth = (float(step % 2) + clock.phase) * 0.5f;
    int note = melody[step / 2];
    float lead = Tone(voice, 0, note, Wave::Pulse, step / 8 % 2 ? 64 : 128) * Gate(eighth);
    float harmony = Tone(voice, 1, note < 0 ? -1 : note - 12, Wave::Pulse, 96) * Gate(eighth, 0.72f);
    int bassNote = roots[bar] - 12 + (step % 8 >= 4 ? 7 : 0);
    float bass = Tone(voice, 2, bassNote, Wave::Triangle) * Gate((float(step % 4) + clock.phase) / 4);
    float noise = voice.Noise();
    // A small XOR counter rhythm supplies the classic bytebeat percussion texture.
    float counter = float(((t >> 3) ^ (t >> 6)) & 31) / 15.5f - 1;
    float drums = Hat(noise * 0.7f + counter * 0.3f, clock.phase) * 0.055f;
    if (step % 8 == 0) drums += Kick(clock.phase) * 0.22f;
    if (step % 8 == 4) drums += Snare(noise, clock.phase) * 0.15f;
    return lead * 0.32f + harmony * 0.13f + bass * 0.25f + drums;
}
}
