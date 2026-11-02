#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectSynesthesiaStorm(float time, float dt) {
    // Radial pseudo-spectrum, deliberately not another rectangular equalizer.
    // This is procedural animation, not a claim of microphone/audio analysis.
    static std::array<float, 64> levels{}, peaks{};
    float cx = g_ScreenWidth * 0.5f, cy = g_ScreenHeight * 0.5f;
    float unit = float(std::min(g_ScreenWidth, g_ScreenHeight));
    float baseRadius = unit * 0.19f;
    float response = 1 - expf(-dt * 12);
    for (size_t i = 0; i < levels.size(); ++i) {
        float signal = sinf(time * 2.7f + i * 0.32f) * sinf(time * 1.3f - i * 0.17f);
        float target = 0.08f + std::max(0.0f, signal) * 0.18f;
        levels[i] += (target - levels[i]) * response;
        peaks[i] = std::max(levels[i], peaks[i] - dt * 0.055f);
        float a = float(i) * Tau / levels.size() - Pi / 2;
        float c = cosf(a), s = sinf(a), radius = baseRadius + levels[i] * unit;
        uint32_t color = BlendColor(0x5DE1DE, 0xFC83BE, (1 + sinf(a)) * 0.5f);
        Line(int(cx + c * baseRadius), int(cy + s * baseRadius), int(cx + c * radius), int(cy + s * radius), color);
        float peakRadius = baseRadius + peaks[i] * unit;
        Fill(int(cx + c * peakRadius) - 1, int(cy + s * peakRadius) - 1, 2, 2, 0xFFF2CD);
    }
    // Two phase-offset oscilloscope traces inside the ring.
    for (int channel = 0; channel < 2; ++channel) {
        int previousX = 0, previousY = 0;
        for (int i = 0; i <= 96; ++i) {
            float x = -1 + i / 48.0f;
            int px = int(cx + x * baseRadius * 0.78f);
            int py = int(cy + sinf(x * 9 + time * 3 + channel) * baseRadius * 0.23f * (1 - x * x));
            if (i) Line(previousX, previousY, px, py, channel ? 0xFC83BE : 0x91F3E5);
            previousX = px; previousY = py;
        }
    }
}
}
