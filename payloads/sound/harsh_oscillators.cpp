#include "chaos.h"

namespace SoundPayloads {
float DualToneSirenWarp(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float ramp = Seconds(t) * Tau * 6;
    float a = Square(Advance(voice.phase, 450 + 3350 * fabsf(sinf(ramp))));
    float b = Square(Advance(voice.secondPhase, 700 + 2500 * fabsf(cosf(ramp))));
    return Clip((a + b) * 0.65f);
}

float SubharmonicMeltdown(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float sub = Square(Advance(voice.phase, 32));
    float unstable = 320 + 32 * sinf(Seconds(t) * 3.7f) + voice.Noise() * 9;
    float carrier = Square(Advance(voice.secondPhase, unstable));
    float neighbor = Square(Advance(voice.phases[0], unstable + 32 + 7 * sinf(Seconds(t) * 0.9f)));
    return tanhf(sub * 0.7f + carrier * neighbor * 1.2f + sub * carrier * 0.4f);
}

float ModemCarrierHell(uint32_t t, Voice& voice) {
    using namespace Chaos;
    if (t % 13u == 0) voice.memory[0] = voice.Noise() > 0 ? 1.0f : -1.0f;
    float pilot = sinf(Tau * Advance(voice.phase, 1800));
    float quadrature = sinf(Tau * Advance(voice.secondPhase, 2400 + 300 * voice.memory[0]));
    float noise = voice.Noise();
    float hash = noise - voice.memory[1];
    voice.memory[1] = noise;
    // Stylized handshake texture, not a V.34 protocol implementation.
    return tanhf(2.6f * (pilot * 0.4f + quadrature * voice.memory[0] * 0.5f + hash * 0.35f));
}

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

float SquareWaveStrobe(uint32_t t, Voice& voice) {
    float tone = Chaos::Square(Chaos::Advance(voice.phase, 800));
    return t % (SampleRate / 25u) < SampleRate / 50u ? tone : 0;
}

float FMFeedbackHowl(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float movement = 0.5f + 0.5f * sinf(Seconds(t) * 1.9f);
    float carrier = Tau * Advance(voice.phase, 45 + 2650 * movement * movement);
    float modulator = Tau * Advance(voice.secondPhase, 19 + 1180 * movement);
    float output = sinf(carrier + 4 * sinf(modulator + 1.2f * voice.memory[0]));
    voice.memory[0] = output;
    return output;
}

float ShepardPanicAscent(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float seconds = Seconds(t), sum = 0, weights = 0;
    // Recycle inaudible voices at octave boundaries; fade out before Nyquist.
    for (size_t i = 0; i < voice.phases.size(); ++i) {
        float octave = fmodf(float(i) + seconds * 0.35f, 10.0f);
        float frequency = 4 * exp2f(octave);
        float distance = log2f(frequency / 1000);
        float weight = expf(-distance * distance * 0.7f) * std::clamp((3500 - frequency) / 600, 0.0f, 1.0f);
        float phase = Advance(voice.phases[i], frequency);
        // This engine uses the first ten delay-buffer slots as clip-local partner phases.
        float tritone = Advance(voice.buffer[i], frequency * 1.41421356237f);
        float partnerWeight = std::clamp((3500 - frequency * 1.41421356237f) / 600, 0.0f, 1.0f);
        sum += weight * ((2 * phase - 1) + 0.35f * partnerWeight * (2 * tritone - 1));
        weights += weight * (1 + 0.35f * partnerWeight);
    }
    float tempo = 3 + fmodf(seconds, 10.0f) * 1.8f;
    float pulse = Square(Advance(voice.phase, tempo)) > 0 ? 1.0f : 0.3f;
    return tanhf(2.5f * sum / std::max(1.0f, weights)) * pulse;
}

float RingModTritoneClash(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float frequency = 550 + 170 * sinf(Seconds(t) * 0.8f);
    float a = Tau * Advance(voice.phase, frequency);
    float b = Tau * Advance(voice.secondPhase, frequency * 1.41421356237f);
    float first = Clip(3 * sinf(a + 1.7f * voice.memory[1]));
    float second = Clip(3 * sinf(b + 1.7f * voice.memory[0]));
    voice.memory[0] = first; voice.memory[1] = second;
    return Clip(first * second * 1.4f + 0.15f * (first + second));
}
} // namespace SoundPayloads
