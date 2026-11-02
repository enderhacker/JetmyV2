#include "visual.h"

namespace VisualPayloads {
void EffectLivingInfection(float, float) {
    uint32_t colors[] = {0x0000FF7F, 0x00FF0055, 0x0000E5FF, 0x00FFD700, 0x009900FF};
    for (int root = 0; root < 5; ++root) {
        int cx = FastRandInt(0, g_ScreenWidth);
        int cy = root % 2 == 0 ? 0 : g_ScreenHeight - 1;
        for (int i = 0; i < 40; ++i) {
            int nx = cx + FastRandInt(-22, 22);
            int ny = cy + (root % 2 == 0 ? FastRandInt(6, 26) : FastRandInt(-26, -6));
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, cx, cy, nx, ny, colors[root]);
            cx = nx; cy = ny;
        }
    }
}
}
