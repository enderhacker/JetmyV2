#include "music.h"

namespace SoundPayloads {
float BytebeatNeonDrive(uint32_t t, Voice& voice) {
    using namespace Music;
    // Original four-bar A-minor / F / C / G synthwave arrangement.
    constexpr int roots[] = {45, 41, 48, 43};
    constexpr int melody[] = {69,-1,72,76,74,-1,72,67, 69,72,77,-1,76,72,69,-1,
        72,-1,76,79,76,74,72,-1, 71,74,79,77,74,71,67,69};
    auto clock = Sequence(t, 64);
    uint32_t step = clock.step % 64, bar = step / 16;
    float phase = clock.phase;
    float eighth = (float(step % 2) + phase) * 0.5f;
    int bassNote = roots[bar] - 12 + ((step % 8 == 6) ? 12 : 0);
    float bass = Tone(voice, 0, bassNote, Wave::Saw) * Gate(eighth);
    voice.memory[0] += (bass - voice.memory[0]) * 0.1f;
    int leadNote = melody[step / 2];
    float lead = Tone(voice, 1, leadNote, Wave::Pulse, 80) * Gate(eighth, 0.92f);
    int chord = roots[bar] + 12 + (step % 4 == 0 ? 0 : step % 4 == 1 ? (bar == 0 ? 3 : 4) : step % 4 == 2 ? 7 : 12);
    float shimmer = Tone(voice, 2, chord, Wave::Triangle) * Pluck(phase);
    float noise = voice.Noise();
    float drums = (step % 4 == 0 ? Kick(phase) * 0.23f : 0) + Hat(noise, phase) * 0.045f;
    if (step % 8 == 4) drums += Snare(noise, phase) * 0.16f;
    return voice.memory[0] * 0.23f + lead * 0.30f + shimmer * 0.14f + drums;
}
}
