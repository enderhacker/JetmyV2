#include "visual.h"

namespace VisualPayloads {
void EffectHellShader(float time, float) {
    int step = std::max(2, RasterStep());
    struct Coordinate { float radius, angle; };
    static std::vector<Coordinate> map;
    static int cachedW = 0, cachedH = 0, cachedStep = 0;
    int columns = (g_ScreenWidth + step - 1) / step;
    int rows = (g_ScreenHeight + step - 1) / step;
    float t = fmodf(time, 1000.0f);
    float flash = sinf(t * 17.0f) > 0 ? 1.0f : -1.0f;
    float cx = g_ScreenWidth * 0.5f, cy = g_ScreenHeight * 0.5f;
    if (cachedW != g_ScreenWidth || cachedH != g_ScreenHeight || cachedStep != step) {
        map.resize(static_cast<size_t>(columns) * rows);
        for (int row = 0; row < rows; ++row) for (int col = 0; col < columns; ++col) {
            float dx = (col * step - cx) / std::max(1.0f, cx);
            float dy = (row * step - cy) / std::max(1.0f, cy);
            map[static_cast<size_t>(row) * columns + col] = {sqrtf(dx * dx + dy * dy), atan2f(dy, dx)};
        }
        cachedW = g_ScreenWidth; cachedH = g_ScreenHeight; cachedStep = step;
    }
    static const auto sine = [] {
        std::vector<float> table(4096);
        for (size_t i = 0; i < table.size(); ++i) table[i] = sinf(float(i) * Tau / table.size());
        return table;
    }();
    auto fastSin = [&](float value) { return sine[static_cast<unsigned>(static_cast<int>(value * (4096.0f / Tau))) & 4095u]; };
    Runtime::ParallelFor((g_ScreenHeight + step - 1) / step, [&](int row) {
        int y = row * step;
        float band = sinf(y * 0.055f + t * 9.0f);
        for (int x = 0; x < g_ScreenWidth; x += step) {
            const auto& point = map[static_cast<size_t>(y / step) * columns + x / step];
            float vortex = fastSin(point.angle * 7.0f + point.radius * 24.0f - t * 8.0f);
            float ripple = fastSin(point.radius * 42.0f - t * 11.0f + band * 2.0f);
            int sx = x + static_cast<int>(vortex * 35 + band * 20);
            int sy = y + static_cast<int>(ripple * 28);
            uint32_t source = Sample(sx, sy);
            uint32_t r = static_cast<uint32_t>((fastSin(vortex * 4 + t * 13) + 1) * 127.5f);
            uint32_t g = static_cast<uint32_t>((fastSin(ripple * 5 - t * 11 + 2) + 1) * 127.5f);
            uint32_t b = static_cast<uint32_t>((fastSin((vortex + ripple) * 3 + t * 15 + flash) + 1) * 127.5f);
            uint32_t pattern = (r << 16) | (g << 8) | b;
            if (flash > 0) pattern ^= 0x00FFFFFFu;
            Tile(x, y, step, BlendColor(source ^ pattern, pattern, 0.55f));
        }
    });
}
} // namespace VisualPayloads
