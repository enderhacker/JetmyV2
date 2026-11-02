#include "music.h"

namespace SoundPayloads {
float BytebeatMidnightKeys(uint32_t t, Voice& voice) {
    using namespace Music;
    // Soft electric-piano voicings: Dm9 / G13 / Cmaj9 / Am9.
    constexpr int chords[4][4] = {{62,65,69,76}, {59,64,65,69}, {60,64,67,74}, {60,64,67,71}};
    constexpr int roots[] = {38,43,36,33};
    constexpr int melody[] = {77,-1,76,72, 74,76,69,-1, 71,74,76,79, 76,72,71,69};
    auto clock = Sequence(t, 64);
    uint32_t step = clock.step % 64, bar = step / 16;
    float quarter = (float(step % 4) + clock.phase) * 0.25f;
    float half = (float(step % 8) + clock.phase) * 0.125f;
    float keys = 0;
    for (size_t i = 0; i < 4; ++i) {
        // Quantized triangle fundamentals and sine overtones give a mellow tine sound.
        keys += Tone(voice, i, chords[bar][i], Wave::Triangle) * Pluck(half) * 0.065f;
    }
    float bass = Tone(voice, 4, roots[bar], Wave::Sine) * Gate(half, 0.96f) * 0.19f;
    int note = melody[step / 4];
    float lead = Tone(voice, 5, note, Wave::Sine) * Pluck(quarter) * 0.25f;
    float tine = Tone(voice, 6, note < 0 ? -1 : note + 12, Wave::Sine) * Pluck(quarter)
        * expf(-quarter * 6) * 0.055f;
    float brush = Hat(voice.Noise(), clock.phase) * (step % 8 == 4 ? 0.045f : 0.008f);
    return keys + bass + lead + tine + brush;
}
}
