#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectDesktopGlassShatter(float time, float) {
    struct Point { float x, y; };
    struct Shard {
        Point a, b, c, center;
        float delay, speed, drift, spin;
    };
    constexpr int cols = 7, rows = 5;
    static std::array<Shard, cols * rows * 2> shards{};
    static int lastCycle = -1, width = 0, height = 0;
    int cycle = int(time / 6.0f);
    float phase = fmodf(time, 6.0f);
    if (time == 0 || cycle != lastCycle || width != g_ScreenWidth || height != g_ScreenHeight) {
        lastCycle = cycle; width = g_ScreenWidth; height = g_ScreenHeight;
        std::array<Point, (cols + 1) * (rows + 1)> vertices{};
        for (int row = 0; row <= rows; ++row) {
            for (int col = 0; col <= cols; ++col) {
                vertices[row * (cols + 1) + col] = {
                    (col + (col > 0 && col < cols ? FastRandFloat(-0.38f, 0.38f) : 0)) * width / cols,
                    (row + (row > 0 && row < rows ? FastRandFloat(-0.38f, 0.38f) : 0)) * height / rows};
            }
        }
        Point impact{FastRandFloat(0, float(width)), FastRandFloat(0, float(height))};
        size_t index = 0;
        auto add = [&](Point a, Point b, Point c) {
            Point center{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3};
            float dx = (center.x - impact.x) / std::max(1, width);
            float dy = (center.y - impact.y) / std::max(1, height);
            shards[index++] = {a, b, c, center,
                0.15f + sqrtf(dx * dx + dy * dy) * 0.65f + FastRandFloat(0, 0.4f),
                FastRandFloat(0.6f, 1.3f), FastRandFloat(-0.22f, 0.22f), FastRandFloat(-1.8f, 1.8f)};
        };
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                Point a = vertices[row * (cols + 1) + col], b = vertices[row * (cols + 1) + col + 1];
                Point c = vertices[(row + 1) * (cols + 1) + col], d = vertices[(row + 1) * (cols + 1) + col + 1];
                if (FastRandInt(0, 1)) { add(a, b, d); add(a, d, c); }
                else { add(a, b, c); add(b, d, c); }
            }
        }
    }
    int step = RasterStep();
    // A brief reset restores the desktop before the next independently randomized break.
    float settle = 1 - Smooth((phase - 5.0f) / 0.8f);
    for (const auto& shard : shards) {
        float age = std::max(0.0f, phase - shard.delay);
        float angle = shard.spin * age * settle;
        float c = cosf(angle), s = sinf(angle);
        float shiftX = shard.drift * g_ScreenWidth * age * settle;
        float shiftY = g_ScreenHeight * 0.20f * shard.speed * age * age * settle;
        auto transform = [&](Point p) -> Point {
            float dx = p.x - shard.center.x, dy = p.y - shard.center.y;
            return {shard.center.x + shiftX + dx * c - dy * s,
                shard.center.y + shiftY + dx * s + dy * c};
        };
        Point a = transform(shard.a), b = transform(shard.b), d = transform(shard.c);
        int left = std::max(0, int(std::min({a.x, b.x, d.x})) - 1);
        int right = std::min(g_ScreenWidth, int(std::max({a.x, b.x, d.x})) + 2);
        int top = std::max(0, int(std::min({a.y, b.y, d.y})) - 1);
        int bottom = std::min(g_ScreenHeight, int(std::max({a.y, b.y, d.y})) + 2);
        float determinant = (shard.b.y - shard.c.y) * (shard.a.x - shard.c.x) +
            (shard.c.x - shard.b.x) * (shard.a.y - shard.c.y);
        if (fabsf(determinant) < 0.001f) continue;
        float edgeA = std::hypot(shard.b.x - shard.c.x, shard.b.y - shard.c.y);
        float edgeB = std::hypot(shard.c.x - shard.a.x, shard.c.y - shard.a.y);
        float edgeC = std::hypot(shard.a.x - shard.b.x, shard.a.y - shard.b.y);
        Runtime::ParallelFor(std::max(0, (bottom - top + step - 1) / step), [&](int row) {
            int y = top + row * step;
            for (int x = left; x < right; x += step) {
                float dx = x - shard.center.x - shiftX, dy = y - shard.center.y - shiftY;
                float sx = shard.center.x + dx * c + dy * s, sy = shard.center.y - dx * s + dy * c;
                float u = ((shard.b.y - shard.c.y) * (sx - shard.c.x) + (shard.c.x - shard.b.x) * (sy - shard.c.y)) / determinant;
                float v = ((shard.c.y - shard.a.y) * (sx - shard.c.x) + (shard.a.x - shard.c.x) * (sy - shard.c.y)) / determinant;
                float w = 1 - u - v;
                if (u < 0 || v < 0 || w < 0) continue;
                float seam = fabsf(determinant) * std::min({u / std::max(0.001f, edgeA),
                    v / std::max(0.001f, edgeB), w / std::max(0.001f, edgeC)});
                Tile(x, y, step, BlendColor(Sample(int(sx), int(sy)), 0xC5EBFF, seam < 0.7f ? 0.45f : 0.025f));
            }
        });
    }
}
} // namespace VisualPayloads
