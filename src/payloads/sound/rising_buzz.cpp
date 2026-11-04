#include "chaos.h"

namespace SoundPayloads {
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
} // namespace SoundPayloads
