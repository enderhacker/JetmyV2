#include "visual.h"

namespace VisualPayloads {
void EffectKaleidoscopicReality(float time, float) {
    // Cache the six-sector fold in polar coordinates. Only a single rotation
    // and source-center shift change per frame; no per-frame atan2 or sqrt.
    struct Fold { float x, y; };
    static std::vector<Fold> map;
    static int cachedW = 0, cachedH = 0;
    int step = RasterStep();
    int columns = (g_ScreenWidth + step - 1) / step, rows = (g_ScreenHeight + step - 1) / step;
    if (cachedW != g_ScreenWidth || cachedH != g_ScreenHeight) {
        map.resize(static_cast<size_t>(columns) * rows);
        Runtime::ParallelFor(rows, [&](int row) {
            for (int col = 0; col < columns; ++col) {
                float dx = col * step - g_ScreenWidth * 0.5f, dy = row * step - g_ScreenHeight * 0.5f;
                float radius = sqrtf(dx * dx + dy * dy);
                float angle = fmodf(atan2f(dy, dx) + Tau, Pi / 3);
                angle = std::min(angle, Pi / 3 - angle);
                map[static_cast<size_t>(row) * columns + col] = {cosf(angle) * radius, sinf(angle) * radius};
            }
        });
        cachedW = g_ScreenWidth; cachedH = g_ScreenHeight;
    }
    float c = cosf(time * 0.14f), s = sinf(time * 0.14f);
    float cx = g_ScreenWidth * (0.37f + 0.12f * sinf(time * 0.11f));
    float cy = g_ScreenHeight * (0.38f + 0.12f * cosf(time * 0.09f));
    auto mirror = [](int value, int size) {
        if (size <= 1) return 0;
        int period = 2 * (size - 1);
        value %= period;
        if (value < 0) value += period;
        return value < size ? value : period - value;
    };
    Runtime::ParallelFor(rows, [&](int row) {
        for (int col = 0; col < columns; ++col) {
            const auto& p = map[static_cast<size_t>(row) * columns + col];
            int sx = mirror(int(cx + p.x * c - p.y * s), g_ScreenWidth);
            int sy = mirror(int(cy + p.x * s + p.y * c), g_ScreenHeight);
            Tile(col * step, row * step, step, Sample(sx, sy));
        }
    });
}
}
