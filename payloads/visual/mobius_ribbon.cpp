#include "visual.h"
#include <limits>

namespace VisualPayloads {
namespace {
struct RibbonVertex { float x, y, z, u, v; };
float Edge(const RibbonVertex& a, const RibbonVertex& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}
}
void EffectMobiusRealityRibbon(float time, float) {
    const int w = g_ScreenWidth, h = g_ScreenHeight, step = RasterStep();
    const int columns = (w + step - 1) / step, rows = (h + step - 1) / step;
    static std::vector<float> depth;
    depth.assign(static_cast<size_t>(columns) * rows, std::numeric_limits<float>::max());
    for (int y = 0; y < h; y += step)
        for (int x = 0; x < w; x += step)
            Tile(x, y, step, BlendColor(Sample(x, y), 0x050919, 0.95f));

    constexpr int segments = 160, strips = 12;
    RibbonVertex vertices[segments + 1][strips + 1];
    float yaw = time * 0.23f, pitch = 0.65f + 0.35f * sinf(time * 0.29f);
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    float scale = std::min(w, h) * 1.28f;
    for (int i = 0; i <= segments; ++i) {
        float a = float(i) / segments * Tau;
        for (int j = 0; j <= strips; ++j) {
            float across = (float(j) / strips - 0.5f) * 1.1f;
            float radius = 1.12f + across * cosf(a * 0.5f);
            float x = radius * cosf(a), y = radius * sinf(a), z = across * sinf(a * 0.5f);
            float rx = x * cy + z * sy, rz = z * cy - x * sy;
            float ry = y * cp - rz * sp;
            float cameraZ = 4.4f + y * sp + rz * cp;
            vertices[i][j] = {w * 0.5f + rx * scale / cameraZ,
                h * 0.5f + ry * scale / cameraZ, cameraZ, float(i) / segments, float(j) / strips};
        }
    }
    auto triangle = [&](RibbonVertex a, RibbonVertex b, RibbonVertex c) {
        float area = Edge(a, b, c.x, c.y);
        if (fabsf(area) < 0.001f) return;
        int left = std::max(0, int(std::floor(std::min({a.x, b.x, c.x}) / step)));
        int right = std::min(columns - 1, int(std::ceil(std::max({a.x, b.x, c.x}) / step)));
        int top = std::max(0, int(std::floor(std::min({a.y, b.y, c.y}) / step)));
        int bottom = std::min(rows - 1, int(std::ceil(std::max({a.y, b.y, c.y}) / step)));
        float light = area < 0 ? 0.94f : 0.55f;
        for (int row = top; row <= bottom; ++row) {
            for (int col = left; col <= right; ++col) {
                float x = float(col * step), y = float(row * step);
                float wa = Edge(b, c, x, y) / area, wb = Edge(c, a, x, y) / area, wc = 1 - wa - wb;
                if (wa < -0.0001f || wb < -0.0001f || wc < -0.0001f) continue;
                float inverseZ = wa / a.z + wb / b.z + wc / c.z;
                float z = 1 / inverseZ;
                size_t index = static_cast<size_t>(row) * columns + col;
                if (z >= depth[index]) continue;
                depth[index] = z;
                // Perspective-correct desktop texture; the back face is deliberately
                // visible, making the half-twist and impossible topology readable.
                float u = (wa * a.u / a.z + wb * b.u / b.z + wc * c.u / c.z) * z;
                float v = (wa * a.v / a.z + wb * b.v / b.z + wc * c.v / c.z) * z;
                float travel = u * 2 + time * 0.065f;
                travel -= floorf(travel);
                uint32_t color = BlendColor(0x111B35,
                    Sample(int(travel * (w - 1)), int(v * (h - 1))), light);
                float edge = std::min(v, 1 - v);
                if (edge < 0.035f) color = BlendColor(color, area < 0 ? 0x77EBEF : 0xD68CFF, Smooth(1 - edge / 0.035f));
                float seam = u * 24 - floorf(u * 24);
                if (seam < 0.025f) color = BlendColor(color, 0xC3E9FF, 0.3f);
                Tile(col * step, row * step, step, color);
            }
        }
    };
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < strips; ++j) {
            triangle(vertices[i][j], vertices[i + 1][j], vertices[i + 1][j + 1]);
            triangle(vertices[i][j], vertices[i + 1][j + 1], vertices[i][j + 1]);
        }
    }
}
} // namespace VisualPayloads
