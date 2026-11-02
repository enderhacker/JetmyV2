#include "visual.h"

namespace VisualPayloads {
void EffectScreenPeelAndFold(float time, float) {
    float t = (sinf(time * 2.4f) + 1) * 0.5f;
    int peelX = static_cast<int>(g_ScreenWidth * 0.55f * t), peelY = static_cast<int>(g_ScreenHeight * 0.55f * t);
    for (int y = 0; y < peelY && y < g_ScreenHeight; ++y) {
        int maxX = static_cast<int>(peelX * (1 - float(y) / (peelY + 1)));
        for (int x = 0; x < maxX && x < g_ScreenWidth; ++x)
            g_pFramePixels[y * g_ScreenWidth + x] = ((x ^ y) & 8) ? 0x0000FFAA : 0x00101010;
    }
}
}
