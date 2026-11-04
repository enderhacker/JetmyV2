#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectGhostCursorSwarm(float time, float dt) {
    static std::array<POINT, 64> trail{};
    static size_t head = 0;
    static float tick = 1;
    static bool initialized = false;
    if (time == 0) { initialized = false; tick = 1; head = 0; }
    static HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW); // Shared system handle; do not destroy.
    POINT mouse{};
    if (!GetCursorPos(&mouse) || !cursor) return;
    mouse.x -= g_ScreenLeft;
    mouse.y -= g_ScreenTop;
    if (!initialized) { trail.fill(mouse); initialized = true; }
    tick += dt;
    if (tick >= 0.025f) { tick = 0; trail[head++ % trail.size()] = mouse; }
    float radius = std::min(g_ScreenWidth, g_ScreenHeight) * 0.12f;
    // A mouse-history comet, not another rotating particle cloud.
    for (int i = 23; i >= 0; --i) {
        const auto& p = trail[(head + trail.size() - 1 - i * 2) % trail.size()];
        float spread = i / 23.0f;
        float a = time * 1.5f - i * 0.33f;
        int x = int(p.x + cosf(a) * radius * spread);
        int y = int(p.y + sinf(a * 1.4f) * radius * spread * 0.6f);
        int size = 12 + (23 - i) * 20 / 23;
        if (x < -size || y < -size || x >= g_ScreenWidth || y >= g_ScreenHeight) continue;
        DrawIconEx(g_hMemDC, x, y, cursor, size, size, 0, nullptr, DI_NORMAL);
    }
}
}
