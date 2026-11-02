#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectGravityPaint(float time, float dt) {
    struct Drip { float x, head, speed, phase; int width; uint32_t color; };
    static std::array<Drip, 24> drips{};
    static std::vector<uint32_t> paint;
    static std::vector<size_t> painted;
    static bool initialized = false;
    size_t area = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight;
    if (time == 0 || paint.size() != area) {
        initialized = false;
        paint.assign(area, 0);
        painted.clear();
    }
    auto deposit = [](int x, int y, uint32_t color) {
        if (x < 0 || y < 0 || x >= g_ScreenWidth || y >= g_ScreenHeight) return;
        size_t index = static_cast<size_t>(y) * g_ScreenWidth + x;
        if (paint[index] == 0) painted.push_back(index);
        paint[index] = color | 0xFF000000u;
    };
    constexpr uint32_t colors[] = {0xFF638B, 0xF8C663, 0x6EDAC4, 0x9B8AFB};
    auto reset = [&colors](Drip& d, bool initial) {
        d.x = FastRandFloat(0, float(g_ScreenWidth - 1));
        d.head = initial ? FastRandFloat(-180, 0) : -20;
        d.speed = FastRandFloat(0.06f, 0.18f) * g_ScreenHeight;
        d.width = FastRandInt(2, 4); d.phase = FastRandFloat(0, Tau);
        d.color = colors[FastRandInt(0, 3)];
    };
    if (!initialized) { for (auto& d : drips) reset(d, true); initialized = true; }
    for (auto& d : drips) {
        float previous = d.head;
        d.head += d.speed * dt;
        int top = std::max(0, int(previous)), bottom = std::min(g_ScreenHeight, int(d.head) + 1);
        for (int y = top; y < bottom; ++y) {
            int x = int(d.x + sinf(y * 0.012f + d.phase) * d.width * 0.6f);
            for (int dx = 0; dx < d.width; ++dx)
                deposit(x - d.width / 2 + dx, y, dx == 0 ? BlendColor(d.color, 0xFFFFFF, 0.35f) : d.color);
        }
        if (d.head > g_ScreenHeight + d.width) reset(d, false);
    }
    // Replay the retained paint over the freshly captured desktop; never fade old strokes.
    // Each screen pixel is stored at most once, even during an indefinite manual preview.
    for (size_t index : painted) g_pFramePixels[index] = paint[index] & 0x00FFFFFFu;
    for (const auto& d : drips) {
        // Rounded heavy droplet at the leading edge of each viscous rivulet.
        int cx = int(d.x + sinf(d.head * 0.012f + d.phase) * d.width * 0.6f);
        for (int y = -d.width / 2; y <= d.width / 2; ++y) {
            int half = int(sqrtf(std::max(0.0f, d.width * d.width * 0.25f - y * y)));
            Fill(cx - half, int(d.head) + y, half * 2 + 1, 1, d.color);
        }
    }
}
}
