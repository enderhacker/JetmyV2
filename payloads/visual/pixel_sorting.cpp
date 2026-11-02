#include "visual.h"

namespace VisualPayloads {
void EffectPixelSortingAnomaly(float, float) {
    int bands = FastRandInt(6, 14);
    for (int b = 0; b < bands; ++b) {
        int w = std::min(g_ScreenWidth, FastRandInt(80, 350));
        int h = std::min(g_ScreenHeight, FastRandInt(20, 80));
        int sx = FastRandInt(0, g_ScreenWidth - w), sy = FastRandInt(0, g_ScreenHeight - h);
        int mode = FastRandInt(0, 2);
        for (int y = sy; y < sy + h; ++y) {
            uint32_t* row = g_pFramePixels + y * g_ScreenWidth + sx;
            std::sort(row, row + w, [mode](uint32_t a, uint32_t b) {
                if (mode == 0) return (a & 0xFF) < (b & 0xFF);
                if (mode == 1) return ((a >> 16) & 0xFF) > ((b >> 16) & 0xFF);
                return ((a >> 8) & 0xFF) < ((b >> 8) & 0xFF);
            });
        }
    }
}
}
