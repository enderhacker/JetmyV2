#include "music.h"

namespace SoundPayloads {
float SonarSoundscape(uint32_t t, Voice& voice) {
    using namespace Music;
    constexpr uint32_t period = SampleRate * 2;
    float seconds = float(t % period) / SampleRate;
    float attack = std::min(1.0f, seconds * 80);
    float ping = Tone(voice, 0, 81, Wave::Sine) * expf(-seconds * 5) * attack;
    float echo = Tone(voice, 1, 81, Wave::Sine) * (seconds > 0.42f ?
        expf(-(seconds - 0.42f) * 6) * std::min(1.0f, (seconds - 0.42f) * 80) : 0);
    voice.memory[0] += (voice.Noise() - voice.memory[0]) * 0.015f;
    float drone = Tone(voice, 2, 33, Wave::Sine);
    return ping * 0.43f + echo * 0.17f + voice.memory[0] * 0.13f + drone * 0.07f;
}

float GlassBells(uint32_t t, Voice& voice) {
    using namespace Music;
    constexpr int notes[] = {72,79,76,83,74,81,79,88};
    auto clock = Sequence(t, 16);
    int note = notes[clock.step % 8];
    float phase = clock.phase;
    float fundamental = Tone(voice, 0, note, Wave::Sine);
    float overtone = Tone(voice, 1, note + 12, Wave::Sine);
    float high = Tone(voice, 2, note + 19, Wave::Sine);
    float attack = std::min(1.0f, phase * 90);
    return attack * (fundamental * expf(-phase * 4) * 0.37f
        + overtone * expf(-phase * 7) * 0.15f + high * expf(-phase * 13) * 0.08f);
}

float RainOnMetal(uint32_t t, Voice& voice) {
    using namespace Music;
    // Low rain bed plus randomized, damped droplets; no per-sample distributions.
    constexpr uint32_t period = SampleRate / 12;
    uint32_t position = t % period;
    if (position == 0) {
        voice.songNote[0] = 0;
        voice.frequency = float(std::uniform_int_distribution<int>(72, 91)(voice.random));
        voice.sweep = std::uniform_real_distribution<float>(0.15f, 0.8f)(voice.random);
    }
    float noise = voice.Noise();
    voice.memory[0] += (noise - voice.memory[0]) * 0.07f;
    float age = float(position) / period;
    float drop = Tone(voice, 0, int(voice.frequency), Wave::Sine) * expf(-age * 20)
        * std::min(1.0f, age * 100) * voice.sweep;
    return voice.memory[0] * 0.42f + (noise - voice.memory[0]) * 0.025f + drop * 0.38f;
}

float CosmicDrone(uint32_t t, Voice& voice) {
    using namespace Music;
    float seconds = float(t) / SampleRate;
    float swell = 0.65f + 0.35f * sinf(seconds * Tau / 7);
    float root = Tone(voice, 0, 45, Wave::Sine), fifth = Tone(voice, 1, 52, Wave::Sine);
    float ninth = Tone(voice, 2, 71, Wave::Triangle), octave = Tone(voice, 3, 57, Wave::Sine);
    voice.memory[0] += (voice.Noise() - voice.memory[0]) * 0.003f;
    float breath = voice.memory[0] * (0.3f + 0.2f * sinf(seconds * 0.8f));
    return (root * 0.22f + fifth * 0.14f + ninth * 0.07f + octave * 0.10f + breath) * swell;
}
}
