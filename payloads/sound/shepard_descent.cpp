#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads {
float ShepardRissetDescent(uint32_t sample, Voice& voice) {
    constexpr float tau = 6.28318530718f;
    const float time = float(sample) / SampleRate;
    // One octave per ten-second loop; each voice is replaced by its octave neighbor.
    const float descent = fmodf(time / 10.0f, 10.0f);
    float sum = 0, weightSum = 0;
    for (size_t i = 0; i < voice.descentPhase.size(); ++i) {
        float octave = fmodf(float(i) - descent + 10.0f, 10.0f);
        float frequency = 4.0f * exp2f(octave);
        float distance = log2f(frequency / 1200.0f);
        float weight = expf(-0.5f * distance * distance / (0.8f * 0.8f));
        // Fade before the 4 kHz Nyquist limit of the existing 8 kHz stream.
        weight *= std::clamp((3500.0f - frequency) / 700.0f, 0.0f, 1.0f);
        auto& phase = voice.descentPhase[i];
        phase += frequency / SampleRate;
        phase -= floorf(phase);
        sum += sinf(tau * phase) * weight;
        weightSum += weight;
    }
    voice.phase += 50.0f / SampleRate;
    voice.phase -= floorf(voice.phase);
    float hum = sinf(tau * voice.phase) + 0.3f * sinf(tau * voice.phase * 3);
    return 0.67f * sum / std::max(1.0f, weightSum) + 0.14f * hum;
}
} // namespace SoundPayloads
