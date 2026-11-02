#include "visual.h"

namespace VisualPayloads {
void EffectRotateScreen(float time, float) {
    // Rotate the captured image, not the Windows display orientation.
    float angle = fmodf(time * 0.32f, Tau);
    float c = cosf(angle), s = sinf(angle);
    float cx = (g_ScreenWidth - 1) * 0.5f, cy = (g_ScreenHeight - 1) * 0.5f;
    int step = RasterStep();
    for (int y = 0; y < g_ScreenHeight; y += step) {
        float dy = y - cy;
        for (int x = 0; x < g_ScreenWidth; x += step) {
            float dx = x - cx;
            float sx = cx + dx * c + dy * s, sy = cy - dx * s + dy * c;
            uint32_t color = 0x000000;
            if (sx >= 0 && sx < g_ScreenWidth && sy >= 0 && sy < g_ScreenHeight)
                color = Sample(int(sx), int(sy));
            Tile(x, y, step, color);
        }
    }
}
} // namespace VisualPayloads
