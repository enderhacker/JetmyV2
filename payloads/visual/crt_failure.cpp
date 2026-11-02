#include "visual.h"

namespace VisualPayloads {
void EffectCRTFailureSimulator(float time, float) {
    int roll = static_cast<int>(fmodf(time * 300, static_cast<float>(g_ScreenHeight)));
    int jitter = FastRandInt(-8, 8);
    BitBlt(g_hMemDC, jitter, roll, g_ScreenWidth, g_ScreenHeight - roll, g_hScratchDC, 0, 0, SRCCOPY);
    BitBlt(g_hMemDC, jitter, 0, g_ScreenWidth, roll, g_hScratchDC, 0, g_ScreenHeight - roll, SRCCOPY);
    GdiFlush();
    for (int y = 0; y < g_ScreenHeight; y += 3) {
        uint32_t noise = FastRand() & 0x001F1F1F;
        for (int x = 0; x < g_ScreenWidth; ++x) {
            auto& pixel = g_pFramePixels[y * g_ScreenWidth + x];
            pixel = ((pixel & 0x00FEFEFE) >> 1) + noise;
        }
    }
}
}
