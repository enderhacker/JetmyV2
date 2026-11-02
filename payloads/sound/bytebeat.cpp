#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads {
namespace {
float Byte(uint32_t value) { return (float(value & 255) - 127.5f) / 127.5f; }
float Advance(float& phase, float frequency) {
    phase += frequency / SampleRate;
    phase -= floorf(phase);
    return phase;
}
}

float BytebeatPulse(uint32_t t, Voice& voice) {
    // Low, syncopated bit-crushed bass; intentionally no melody or random noise.
    constexpr uint32_t beat = SampleRate / 4;
    uint32_t step = t / beat, position = t % beat;
    if (position == 0) voice.frequency = (step & 3) == 3 ? 82.41f : 55.0f;
    float phase = Advance(voice.phase, voice.frequency);
    float pulse = phase < ((step & 1) ? 0.32f : 0.48f) ? 0.65f : -0.65f;
    float crushed = Byte(t * ((t >> 9) | (t >> 12)));
    float age = float(position) / beat;
    float attack = std::min(1.0f, position / 40.0f);
    float envelope = attack * (1 - age) * (1 - age);
    return (pulse * 0.72f + crushed * 0.28f) * envelope;
}

float BytebeatMelody(uint32_t t, Voice& voice) {
    // Warm plucked pentatonic notes with a quiet bytebeat-derived saw overtone.
    constexpr float notes[] = {261.63f, 293.66f, 329.63f, 392.0f, 440.0f, 523.25f};
    constexpr uint32_t noteLength = SampleRate * 3 / 8;
    uint32_t position = t % noteLength, step = t / noteLength;
    if (position == 0) voice.frequency = notes[((step * 5) ^ (step >> 2)) % 6];
    float phase = Advance(voice.phase, voice.frequency);
    float triangle = 1 - 4 * fabsf(phase - 0.5f);
    float age = float(position) / noteLength;
    float envelope = std::min(1.0f, position / 64.0f) * expf(-4.8f * age);
    float texture = Byte(t * (((t >> 12) | (t >> 8)) & 31 & (t >> 4)));
    return (triangle * 0.83f + texture * 0.17f) * envelope;
}

float BytebeatArpeggio(uint32_t t, Voice& voice) {
    // Fast, bright two-oscillator chip arpeggio, an octave above the plucked voice.
    constexpr float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 659.25f, 587.33f, 880.0f};
    constexpr uint32_t noteLength = SampleRate / 8;
    uint32_t position = t % noteLength, step = t / noteLength;
    if (position == 0) voice.frequency = notes[step % 8];
    float phase = Advance(voice.phase, voice.frequency);
    float second = Advance(voice.secondPhase, voice.frequency * 1.005f);
    float square = (phase < 0.25f ? 0.5f : -0.5f) + (second < 0.5f ? 0.25f : -0.25f);
    float byte = Byte((t * ((t >> 10 & 7) + 1)) ^ (t >> 4));
    float release = std::min(1.0f, float(noteLength - 1 - position) / 160);
    float attack = std::min(1.0f, position / 24.0f);
    return (square * 0.85f + byte * 0.15f) * attack * release;
}
}
