#include "visual.h"

namespace VisualPayloads {
void EffectScreenPeelAndFold(float time, float) {
    // A diagonal paper curl, with a reflected underside and a narrow rolling highlight.
    float amount = 0.08f + 0.65f * (0.5f - 0.5f * cosf(time * 0.55f));
    float unit = float(std::max(1, std::min(g_ScreenWidth, g_ScreenHeight)));
    float crease = amount * unit;
    float curl = std::max(2.0f, unit * 0.035f);
    int step = RasterStep();
    int limitY = std::min(g_ScreenHeight, int(crease + curl + 1));
    for (int y = 0; y < limitY; y += step) {
        int limitX = std::min(g_ScreenWidth, int(crease + curl - y + 1));
        for (int x = 0; x < limitX; x += step) {
            float distance = crease - (x + y);
            uint32_t color;
            if (distance > curl) {
                // Newly exposed backing, not a second copy of the original desktop.
                color = 0x000000;
            } else if (distance >= 0) {
                int sx = int(x + distance), sy = int(y + distance);
                float light = sinf(distance / curl * Pi);
                color = BlendColor(Sample(sx, sy), 0xF2E8CD, 0.55f + light * 0.35f);
            } else {
                color = BlendColor(Sample(x, y), 0x03060D, (1 + distance / curl) * 0.45f);
            }
            Tile(x, y, step, color);
        }
    }
}
}
