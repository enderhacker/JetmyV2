#include "visual.h"

namespace VisualPayloads {
void EffectChaosRectangles(float, float) {
    int count = FastRandInt(10, 25);
    for (int i = 0; i < count; ++i) {
        int w = FastRandInt(20, 260), h = FastRandInt(20, 260);
        int x = FastRandInt(0, g_ScreenWidth - w), y = FastRandInt(0, g_ScreenHeight - h);
        int mode = FastRandInt(0, 3);
        if (mode == 0) FastInvertRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, w, h);
        else if (mode == 1) FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, w, h, FastRand() & 0x00FFFFFF);
        else {
            uint32_t col = FastRand() & 0x00FFFFFF;
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, x + w, y, col);
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x + w, y, x + w, y + h, col);
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x + w, y + h, x, y + h, col);
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y + h, x, y, col);
        }
    }
}
}
