#include "music.h"

namespace SoundPayloads {
float BytebeatDuskBreaks(uint32_t t, Voice& voice) {
    using namespace Music;
    constexpr int bassline[] = {33,33,-1,45, 36,-1,33,40, 33,-1,43,45, 40,36,-1,31};
    constexpr int arp[] = {69,72,76,79,76,72,67,64};
    constexpr uint16_t kickMask = 0x4585; // Steps 0,2,7,8,10,14.
    auto clock = Sequence(t, 64);
    uint32_t step = clock.step % 64;
    float phase = clock.phase;
    int transpose = step / 16 == 2 ? 5 : step / 16 == 3 ? 7 : 0;
    int bassNote = bassline[step % 16];
    float acid = Tone(voice, 0, bassNote < 0 ? -1 : bassNote + transpose, Wave::Saw) * Gate(phase);
    float cutoff = 0.045f + 0.35f * (1 - phase) * (1 - phase);
    voice.memory[0] += cutoff * (acid - voice.memory[0]);
    voice.memory[1] += cutoff * (voice.memory[0] - voice.memory[1]);
    float sparkle = Tone(voice, 1, arp[step % 8] + transpose, Wave::Triangle) * Pluck(phase);
    float noise = voice.Noise();
    float drums = Hat(noise, phase) * (step % 2 ? 0.065f : 0.035f);
    if ((kickMask >> (step % 16)) & 1) drums += Kick(phase) * 0.28f;
    if (step % 8 == 4 || step % 16 == 15) drums += Snare(noise, phase) * (step % 16 == 15 ? 0.10f : 0.22f);
    return voice.memory[1] * 0.38f + sparkle * 0.14f + drums;
}
}
