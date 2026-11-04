#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectMeltingScreen(float time, float) {
    struct Flow { float delay, acceleration, bulge; };
    constexpr size_t KnotCount = 33;
    static std::array<Flow, KnotCount> knots{};
    static std::vector<float> offsets;
    static int lastCycle = -1;
    int cycle = int(time / 14.0f);
    float phase = fmodf(time, 14.0f);
    if (time == 0 || cycle != lastCycle) {
        lastCycle = cycle;
        for (auto& knot : knots)
            knot = {FastRandFloat(0.1f, 2.0f), FastRandFloat(0.012f, 0.035f), FastRandFloat(0.65f, 1.3f)};
    }
    offsets.resize(g_ScreenWidth);
    float fade = Smooth((phase - 11.0f) / 2.8f);
    for (int x = 0; x < g_ScreenWidth; ++x) {
        float position = float(x) * (KnotCount - 1) / std::max(1, g_ScreenWidth - 1);
        size_t index = std::min(KnotCount - 2, static_cast<size_t>(position));
        float mix = Smooth(position - float(index));
        const auto& a = knots[index]; const auto& b = knots[index + 1];
        float delay = a.delay + (b.delay - a.delay) * mix;
        float acceleration = a.acceleration + (b.acceleration - a.acceleration) * mix;
        float bulge = a.bulge + (b.bulge - a.bulge) * mix;
        float age = std::max(0.0f, phase - delay);
        // Neighboring columns flow together, but differently delayed lobes accelerate
        // under gravity. There is no sideways sine wave or per-frame random jitter.
        offsets[x] = acceleration * age * age * bulge * g_ScreenHeight;
    }
    int step = RasterStep();
    struct Column { float displacement, stretch, slope, poolDepth, shade; };
    static std::vector<Column> columns;
    columns.resize(g_ScreenWidth);
    for (int x = 0; x < g_ScreenWidth; x += step) {
        float displacement = offsets[x];
        float front = displacement;
        float stretch = 1 + displacement / g_ScreenHeight * 0.85f;
        float slope = (offsets[std::min(g_ScreenWidth - 1, x + step)] - offsets[std::max(0, x - step)]) / (2 * step);
        float poolDepth = std::min(g_ScreenHeight * 0.12f, displacement * 0.09f) * (1 - Smooth((displacement / g_ScreenHeight - 0.7f) / 0.3f));
        float shade = std::min(0.25f, fabsf(slope) * 0.07f) * Smooth(displacement / 35);
        columns[x] = {front, stretch, slope, poolDepth, shade};
    }
    // Contiguous row writes rather than striding an entire framebuffer per column.
    Runtime::ParallelFor((g_ScreenHeight + step - 1) / step, [&](int row) {
        int y = row * step;
        for (int x = 0; x < g_ScreenWidth; x += step) {
            const auto& column = columns[x];
            float front = column.displacement, stretch = column.stretch;
            float slope = column.slope, poolDepth = column.poolDepth;
            float displacement = front;
            float distance = y - front;
            uint32_t color = 0x000000;
            if (distance >= 0) {
                float sourceY = distance / stretch;
                color = Sample(x, int(sourceY));
                // Viscous folds have a shaded side and a narrow wet leading lip.
                float shade = column.shade;
                color = BlendColor(color, slope > 0 ? 0x030507 : 0xFFF1D7, shade);
                float lip = (1 - Smooth(distance / std::max(2.0f, g_ScreenHeight * 0.006f))) * Smooth(displacement / 25);
                color = BlendColor(color, 0xFFF5E1, lip * 0.38f);
                float poolStart = g_ScreenHeight - poolDepth;
                if (poolDepth > 1 && y > poolStart) {
                    float depth = (y - poolStart) / poolDepth;
                    // The displaced lower image collects in a compressed bottom pool.
                    uint32_t pooled = Sample(x, int(sourceY + (g_ScreenHeight - 1 - sourceY) * depth));
                    color = BlendColor(color, pooled, Smooth(depth));
                    color = BlendColor(color, 0x080A0D, depth * 0.22f);
                }
            }
            Tile(x, y, step, BlendColor(color, Sample(x, y), fade));
        }
    });
}
} // namespace VisualPayloads
