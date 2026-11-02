#include "visual.h"

namespace VisualPayloads {
void EffectHolographicGlitch(float time, float) {
    for (int k = 0; k < 4; ++k) {
        float ox = g_ScreenWidth * (0.2f + 0.2f * k);
        float oy = g_ScreenHeight * 0.5f + sinf(time * 2.2f + k * 1.5f) * 160;
        float ang = time * 3.2f + k;
        POINT box[8];
        for (int i = 0; i < 8; ++i) {
            float vx = (i & 1) ? 55.0f : -55.0f, vy = (i & 2) ? 55.0f : -55.0f, vz = (i & 4) ? 55.0f : -55.0f;
            float rx = vx * cosf(ang) - vz * sinf(ang), rz = vx * sinf(ang) + vz * cosf(ang) + 220;
            box[i] = {static_cast<LONG>(ox + rx * 180 / rz), static_cast<LONG>(oy + vy * 180 / rz)};
        }
        const int edges[12][2] = {{0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges)
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, box[e[0]].x, box[e[0]].y,
                box[e[1]].x, box[e[1]].y, k % 2 == 0 ? 0x0000FFFF : 0x00FF00AA);
    }
}
}
