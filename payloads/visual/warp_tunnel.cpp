#include "visual.h"

namespace VisualPayloads {
void Effect3DWarpTunnel(float time, float) {
    struct Coordinate { float u, depth, light; };
    static std::vector<Coordinate> map;
    static int cachedW = 0, cachedH = 0;
    int step = RasterStep();
    int columns = (g_ScreenWidth + step - 1) / step, rows = (g_ScreenHeight + step - 1) / step;
    if (cachedW != g_ScreenWidth || cachedH != g_ScreenHeight) {
        map.resize(static_cast<size_t>(columns) * rows);
        float unit = float(std::max(1, std::min(g_ScreenWidth, g_ScreenHeight)));
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < columns; ++col) {
                float dx = (col * step - g_ScreenWidth * 0.5f) / unit;
                float dy = (row * step - g_ScreenHeight * 0.5f) / unit;
                float radius = sqrtf(dx * dx + dy * dy);
                map[static_cast<size_t>(row) * columns + col] = {
                    atan2f(dy, dx) / Tau + 0.5f, 0.22f / std::max(0.025f, radius), Smooth(radius * 5)};
            }
        }
        cachedW = g_ScreenWidth; cachedH = g_ScreenHeight;
    }
    float travel = time * 0.45f, spin = time * 0.025f;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const auto& p = map[static_cast<size_t>(row) * columns + col];
            float u = p.u + spin, v = p.depth - travel;
            u -= floorf(u); v -= floorf(v);
            int sx = std::min(g_ScreenWidth - 1, int(u * g_ScreenWidth));
            int sy = std::min(g_ScreenHeight - 1, int(v * g_ScreenHeight));
            uint32_t color = Sample(sx, sy);
            float ring = v * 10 - floorf(v * 10), rib = u * 16 - floorf(u * 16);
            if (ring < 0.055f || rib < 0.025f) color = BlendColor(color, 0x50D9E6, 0.7f);
            Tile(col * step, row * step, step, BlendColor(0x090D1C, color, p.light));
        }
    }
}
}
