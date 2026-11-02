#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectRandomStatusIcons(float time, float dt) {
    struct Stamp { int x = 0, y = 0, size = 0, kind = 0; float remaining = 0; };
    static std::array<Stamp, 36> stamps{};
    static float countdown = 0;
    static size_t next = 0;
    // These are shared system resources and must not be destroyed.
    static const std::array<HICON, 3> icons{
        LoadIcon(nullptr, IDI_WARNING), LoadIcon(nullptr, IDI_INFORMATION), LoadIcon(nullptr, IDI_ERROR)};
    if (time == 0) { stamps = {}; countdown = 0; next = 0; }
    for (auto& stamp : stamps) stamp.remaining = std::max(0.0f, stamp.remaining - dt);
    countdown -= dt;
    if (countdown <= 0) {
        int limit = std::max(1, std::min({192, g_ScreenWidth, g_ScreenHeight}));
        int size = FastRandInt(std::min(20, limit), limit);
        stamps[next] = {FastRandInt(0, g_ScreenWidth - size), FastRandInt(0, g_ScreenHeight - size),
            size, FastRandInt(0, 3), FastRandFloat(4, 9)};
        next = (next + 1) % stamps.size();
        countdown = FastRandFloat(0.18f, 0.55f);
    }
    // Paint in insertion order so the newest icon appears above older ones.
    for (size_t i = 0; i < stamps.size(); ++i) {
        const auto& stamp = stamps[(next + i) % stamps.size()];
        if (stamp.remaining <= 0) continue;
        if (stamp.kind < 3) {
            DrawIconEx(g_hMemDC, stamp.x, stamp.y, icons[stamp.kind], stamp.size, stamp.size, 0, nullptr, DI_NORMAL);
            GdiFlush(); // Subsequent success marks use direct DIB writes.
        } else {
            // Windows has no standard success icon: draw a scalable green check seal.
            float radius = stamp.size * 0.47f;
            float cx = stamp.x + stamp.size * 0.5f, cy = stamp.y + stamp.size * 0.5f;
            for (int y = stamp.y; y < stamp.y + stamp.size; ++y) {
                float dy = y - cy;
                int half = int(sqrtf(std::max(0.0f, radius * radius - dy * dy)));
                if (fabsf(dy) <= radius) Fill(int(cx) - half, y, half * 2 + 1, 1, 0x249849);
            }
            int thickness = std::max(1, stamp.size / 18);
            for (int offset = -thickness; offset <= thickness; ++offset) {
                Line(stamp.x + stamp.size / 4, stamp.y + stamp.size / 2 + offset,
                    stamp.x + stamp.size * 43 / 100, stamp.y + stamp.size * 68 / 100 + offset, 0xFFFFFF);
                Line(stamp.x + stamp.size * 43 / 100, stamp.y + stamp.size * 68 / 100 + offset,
                    stamp.x + stamp.size * 77 / 100, stamp.y + stamp.size * 32 / 100 + offset, 0xFFFFFF);
            }
        }
    }
}
} // namespace VisualPayloads
