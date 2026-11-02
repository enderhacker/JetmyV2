#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectGravityPaint(float, float dt) {
    struct Drip { float x, head, speed, phase; int width; uint32_t color; };
    static std::array<Drip, 24> drips{};
    static bool initialized = false;
    constexpr uint32_t colors[] = {0xFF638B, 0xF8C663, 0x6EDAC4, 0x9B8AFB};
    auto reset = [&colors](Drip& d, bool initial) {
        d.x = FastRandFloat(0, float(g_ScreenWidth - 1));
        d.head = initial ? FastRandFloat(-180, 0) : -20;
        d.speed = FastRandFloat(0.06f, 0.18f) * g_ScreenHeight;
        d.width = FastRandInt(3, 10); d.phase = FastRandFloat(0, Tau);
        d.color = colors[FastRandInt(0, 3)];
    };
    if (!initialized) { for (auto& d : drips) reset(d, true); initialized = true; }
    int tail = std::max(30, g_ScreenHeight / 3);
    for (auto& d : drips) {
        d.head += d.speed * dt;
        if (d.head - tail > g_ScreenHeight) reset(d, false);
        int top = std::max(0, int(d.head) - tail), bottom = std::min(g_ScreenHeight, int(d.head) + 1);
        for (int y = top; y < bottom; ++y) {
            float age = (d.head - y) / tail;
            int x = int(d.x + sinf(y * 0.012f + d.phase) * d.width * 0.6f);
            int width = std::max(1, int(d.width * (1 - age * 0.7f)));
            uint32_t body = BlendColor(0x281C39, d.color, 1 - age * 0.45f);
            Fill(x - width / 2, y, width, 1, body);
            Pixel(x - width / 2, y, BlendColor(body, 0xFFFFFF, 0.55f));
        }
        // Rounded heavy droplet at the leading edge of each viscous rivulet.
        int cx = int(d.x + sinf(d.head * 0.012f + d.phase) * d.width * 0.6f);
        for (int y = -d.width / 2; y <= d.width / 2; ++y) {
            int half = int(sqrtf(std::max(0.0f, d.width * d.width * 0.25f - y * y)));
            Fill(cx - half, int(d.head) + y, half * 2 + 1, 1, d.color);
        }
    }
}
}
