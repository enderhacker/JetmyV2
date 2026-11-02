#include "music.h"

namespace SoundPayloads {
float BytebeatPocketGroove(uint32_t t, Voice& voice) {
    using namespace Music;
    constexpr int bassline[] = {40,-1,40,52,-1,43,45,-1, 47,-1,45,43,40,-1,38,40};
    constexpr int hookNotes[] = {64,-1,67,69,71,-1,69,67, 64,62,-1,64,67,64,62,-1};
    auto clock = Sequence(t, 64);
    uint32_t step = clock.step % 64;
    // Swing pairs without accumulating timing drift: long first sixteenth, short second.
    float pair = (float(step % 2) + clock.phase) * 0.5f;
    uint32_t swung = step / 2 * 2 + (pair >= 0.62f ? 1u : 0u);
    float phase = pair < 0.62f ? pair / 0.62f : (pair - 0.62f) / 0.38f;
    float bass = Tone(voice, 0, bassline[swung % 16], Wave::Triangle) * Pluck(phase);
    float hookPhase = (float(swung % 4) + phase) * 0.25f;
    float hook = Tone(voice, 1, hookNotes[(swung / 4) % 16], Wave::Sine) * Pluck(hookPhase);
    int third = swung / 16 == 2 ? 3 : 4;
    float stab = (Tone(voice, 2, 64, Wave::Pulse, 100) + Tone(voice, 3, 64 + third, Wave::Triangle)
        + Tone(voice, 4, 71, Wave::Triangle)) * (swung % 8 == 3 || swung % 8 == 6 ? Pluck(phase) : 0);
    float noise = voice.Noise();
    float drums = Hat(noise, phase) * (swung % 2 ? 0.035f : 0.07f);
    if (swung % 16 == 0 || swung % 16 == 7 || swung % 16 == 10) drums += Kick(phase) * 0.25f;
    if (swung % 8 == 4) drums += Snare(noise, phase) * 0.13f;
    return bass * 0.35f + hook * 0.22f + stab * 0.09f + drums;
}
}
