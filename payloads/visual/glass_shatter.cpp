#include "visual.h"

namespace VisualPayloads {
void EffectDesktopGlassShatter(float time, float) {
    // Triangular fragments rotate independently; unlike voxels these have no box faces.
    constexpr int cols = 7, rows = 5;
    float phase = fmodf(time, 6.0f);
    float fall = Smooth((phase - 0.6f) / 3.8f);
    float settle = 1 - Smooth((phase - 5.0f) / 0.8f);
    int step = RasterStep();
    for (int row = 0; row < rows; ++row) {
        int y0 = row * g_ScreenHeight / rows, y1 = (row + 1) * g_ScreenHeight / rows;
        for (int col = 0; col < cols; ++col) {
            int x0 = col * g_ScreenWidth / cols, x1 = (col + 1) * g_ScreenWidth / cols;
            if (x1 == x0 || y1 == y0) continue;
            for (int half = 0; half < 2; ++half) {
                int id = (row * cols + col) * 2 + half;
                float angle = (id % 2 ? 1 : -1) * fall * (0.2f + (id % 5) * 0.09f) * settle;
                float c = cosf(angle), s = sinf(angle);
                float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
                float shiftX = sinf(id * 12.3f) * g_ScreenWidth * 0.12f * fall * settle;
                float shiftY = g_ScreenHeight * 0.7f * fall * fall * settle;
                // Tight rotated bounds avoid scanning a large square for every shard.
                float extentX = (fabsf(c) * (x1 - x0) + fabsf(s) * (y1 - y0)) * 0.5f;
                float extentY = (fabsf(s) * (x1 - x0) + fabsf(c) * (y1 - y0)) * 0.5f;
                float invW = 1.0f / (x1 - x0), invH = 1.0f / (y1 - y0);
                for (int y = std::max(0, int(cy + shiftY - extentY) - 1); y < std::min(g_ScreenHeight, int(cy + shiftY + extentY) + 2); y += step) {
                    for (int x = std::max(0, int(cx + shiftX - extentX) - 1); x < std::min(g_ScreenWidth, int(cx + shiftX + extentX) + 2); x += step) {
                        float dx = x - cx - shiftX, dy = y - cy - shiftY;
                        float sx = cx + dx * c + dy * s, sy = cy - dx * s + dy * c;
                        if (sx < x0 || sx >= x1 || sy < y0 || sy >= y1) continue;
                        float diagonal = (sx - x0) * invW - (sy - y0) * invH;
                        if ((diagonal >= 0) != (half == 0)) continue;
                        uint32_t color = Sample(int(sx), int(sy));
                        float seam = std::min({sx - x0, x1 - sx, sy - y0, y1 - sy,
                            fabsf(diagonal) * std::min(x1 - x0, y1 - y0)});
                        color = BlendColor(color, 0xC5EBFF, seam < 2 ? 0.8f : 0.07f);
                        Tile(x, y, step, color);
                    }
                }
            }
        }
    }
}
}
