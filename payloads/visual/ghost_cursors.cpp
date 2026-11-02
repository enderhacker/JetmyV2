#include "visual.h"

namespace VisualPayloads {
void EffectGhostCursorSwarm(float time, float) {
    POINT p; GetCursorPos(&p);
    HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW);
    for (int i = 0; i < 32; ++i) {
        float a = time * 3.2f + i * 0.35f + sinf(time + i);
        float r = 30.0f + i * 18.0f + 15.0f * sinf(time * 2.0f + i);
        DrawIcon(g_hMemDC, static_cast<int>(p.x + cosf(a) * r), static_cast<int>(p.y + sinf(a) * r), cursor);
    }
}
}
