#include "visual.h"

namespace VisualPayloads {
void EffectErrorBarcode(float time, float) {
    GdiFlush();
    const int step = RasterStep();
    const uint32_t tick = static_cast<uint32_t>(fmodf(time * 7, 65536));
    for (int x = 0; x < g_ScreenWidth; x += step) {
        uint32_t hash = static_cast<uint32_t>(x / 5) * 747796405u + tick * 2891336453u;
        hash = (hash ^ (hash >> 16)) * 2246822519u;
        int height = g_ScreenHeight / 5 + static_cast<int>(hash % std::max(1, g_ScreenHeight * 3 / 5));
        for (int y = 0; y < g_ScreenHeight; y += step) {
            uint32_t c = Sample(x, y);
            if (abs(y - g_ScreenHeight / 2) < height / 2) {
                uint32_t ink = (hash & 4) ? 0xeae3cb : 0x090909;
                c = BlendColor(c, ink, 0.88f);
            }
            float scan = (0.5f + 0.5f * sinf(time * 2)) * g_ScreenHeight;
            if (fabsf(y - scan) < 4) c = 0xff1515;
            Tile(x, y, step, c);
        }
    }
}
}
