#include "visual.h"

namespace VisualPayloads {
void EffectXorColorChaos(float time, float) {
    int step = RasterStep();
    uint32_t tick = static_cast<uint32_t>(fmodf(time * 24, 65536));
    for (int y = 0; y < g_ScreenHeight; y += step) {
        for (int x = 0; x < g_ScreenWidth; x += step) {
            uint32_t px = static_cast<uint32_t>(x), py = static_cast<uint32_t>(y);
            uint32_t pattern = ((px + tick * 3) ^ (py + tick * 2)) + ((px * py) >> 6);
            uint32_t mask = ((pattern & 255u) << 16) | (((pattern * 3 + tick) & 255u) << 8) |
                ((pattern * 7 + tick * 2) & 255u);
            Tile(x, y, step, Sample(x, y) ^ mask);
        }
    }
}
} // namespace VisualPayloads
