#include "visual.h"

namespace VisualPayloads {
void EffectZipperRift(float time, float) {
    GdiFlush();
    const int step = RasterStep();
    float head = (0.5f + 0.5f * sinf(time * 0.8f)) * g_ScreenHeight;
    for (int y = 0; y < g_ScreenHeight; y += step) {
        float seam = g_ScreenWidth * (0.5f + 0.12f * sinf(y * 0.008f + time));
        float gap = std::max(0.0f, head - y) * 0.38f;
        for (int x = 0; x < g_ScreenWidth; x += step) {
            float distance = fabsf(x - seam);
            uint32_t c;
            if (distance < gap) {
                int grid = (x / 13 + y / 13 + static_cast<int>(fmodf(time * 12, 2))) & 1;
                c = grid ? 0x670016 : 0x080008;
            } else {
                c = Sample(static_cast<int>(x + (x < seam ? gap : -gap)), y);
                if (distance - gap < 10 && ((y / 9) & 1) == (x < seam ? 0 : 1))
                    c = 0xe8e8c0;
            }
            if (fabsf(y - head) < 12 && distance < 16) c = 0xff2222;
            Tile(x, y, step, c);
        }
    }
}
}
