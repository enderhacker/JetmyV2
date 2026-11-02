#include "visual.h"

namespace VisualPayloads {
void EffectGeometricImplosion(float time, float) {
    float ax = g_ScreenWidth * (0.5f + cosf(time * 2.2f) * 0.3f);
    float ay = g_ScreenHeight * (0.5f + sinf(time * 1.9f) * 0.25f);
    float progress = fmodf(time * 2, 1.0f);
    for (int i = 0; i < 18; ++i) {
        float angle = i * (6.28318f / 18) + time, dist = 450 * (1 - progress);
        FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight,
            static_cast<int>(ax + cosf(angle) * dist), static_cast<int>(ay + sinf(angle) * dist),
            static_cast<int>(ax + cosf(angle + 0.3f) * dist * 0.8f),
            static_cast<int>(ay + sinf(angle + 0.3f) * dist * 0.8f), 0x00FF007F);
    }
}
}
