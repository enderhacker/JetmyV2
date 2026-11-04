#include "visual.h"

namespace VisualPayloads {
namespace {
uint32_t CityHash(int x, int y) {
    uint32_t value = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
    return (value ^ (value >> 13)) * 1274126177u;
}
bool CityBuilding(int x, int y) {
    // Two-cell-wide avenues keep the moving camera out of the buildings.
    return ((x % 6 + 6) % 6) >= 2 && ((y % 6 + 6) % 6) >= 2;
}
}
void EffectInfiniteDesktopCity(float time, float) {
    const int w = g_ScreenWidth, h = g_ScreenHeight, step = RasterStep();
    const float cameraX = 0.9f + 0.28f * sinf(time * 0.31f), cameraY = time * 1.8f;
    const float heading = 0.16f * sinf(time * 0.23f);
    const float forwardX = sinf(heading), forwardY = cosf(heading);
    const float horizon = h * (0.43f + 0.035f * sinf(time * 0.37f));
    const float focal = w * 0.68f, eye = 0.72f;
    // Distance depends only on the scanline, not on the ray/column.
    static std::vector<float> streetDistance;
    streetDistance.resize(h);
    for (int y = 0; y < h; y += step)
        streetDistance[y] = eye * focal / std::max(1.0f, y - horizon);
    Runtime::ParallelFor((w + step - 1) / step, [&](int column) {
        int x = column * step;
        float screen = (x - w * 0.5f) / focal;
        float dx = forwardX + forwardY * screen, dy = forwardY - forwardX * screen;
        float invX = 1 / std::max(0.00001f, fabsf(dx));
        float invY = 1 / std::max(0.00001f, fabsf(dy));
        int cellX = int(floorf(cameraX)), cellY = int(floorf(cameraY));
        int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
        float sideX = (dx < 0 ? cameraX - cellX : cellX + 1 - cameraX) * invX;
        float sideY = (dy < 0 ? cameraY - cellY : cellY + 1 - cameraY) * invY;
        // Draw the street and a desktop-reflecting night sky first.
        for (int y = 0; y < h; y += step) {
            uint32_t color;
            if (y <= horizon) {
                color = BlendColor(Sample(x, h - 1 - y), 0x071025, 0.88f);
            } else {
                float distance = streetDistance[y];
                float wx = cameraX + dx * distance, wy = cameraY + dy * distance;
                float u = wx - floorf(wx), v = wy - floorf(wy);
                color = BlendColor(Sample(int(u * (w - 1)), int(v * (h - 1))), 0x081020, 0.75f);
                float marking = fabsf(wx - 1);
                if (marking < 0.025f && (int(floorf(wy * 2)) & 1) == 0) color = 0x63BCCA;
                color = BlendColor(color, 0x14233E, Smooth(distance / 35));
            }
            Tile(x, y, step, color);
        }
        // Front-to-back grid traversal with a skyline occlusion boundary.
        // Distant tall buildings remain visible above nearby shorter ones.
        int ceiling = int(h);
        for (int cell = 0; cell < 100 && ceiling > 0; ++cell) {
            float distance;
            bool side;
            if (sideX < sideY) { distance = sideX; sideX += invX; cellX += sx; side = false; }
            else { distance = sideY; sideY += invY; cellY += sy; side = true; }
            if (distance > 42) break;
            if (!CityBuilding(cellX, cellY)) continue;
            distance = std::max(0.05f, distance);
            uint32_t hash = CityHash(cellX, cellY);
            float height = 1.3f + (hash % 31) * 0.13f;
            float top = horizon - (height - eye) * focal / distance;
            float bottom = horizon + eye * focal / distance;
            float wall = side ? cameraX + distance * dx : cameraY + distance * dy;
            wall -= floorf(wall);
            int start = std::max(0, int(floorf(top / step)) * step);
            int end = std::min(ceiling, std::min(h, int(bottom) + step));
            float fog = Smooth(distance / 42);
            float windowX = wall * 6 - floorf(wall * 6);
            for (int y = start; y < end; y += step) {
                float v = std::clamp((y - top) / std::max(1.0f, bottom - top), 0.0f, 1.0f);
                uint32_t color = Sample(int(wall * (w - 1)), int(v * (h - 1)));
                color = BlendColor(color, side ? 0x162A48 : 0x070C19, side ? 0.25f : 0.5f);
                float windowY = v * height * 5 - floorf(v * height * 5);
                if (windowX < 0.08f || windowY < 0.08f) color = BlendColor(color, 0x55CFE0, 0.6f);
                Tile(x, y, step, BlendColor(color, 0x14233E, fog));
            }
            ceiling = std::min(ceiling, start);
        }
    });
}
} // namespace VisualPayloads
