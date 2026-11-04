#include "visual.h"
#include "trail.h"

namespace VisualPayloads {
void EffectLivingInfection(float time, float dt) {
    struct Tip { float x, y, angle, age; uint32_t color; };
    static std::vector<Tip> tips;
    static TrailCanvas trail;
    static int width = 0, height = 0;
    static float tick = 1;
    bool reset = time == 0 || width != g_ScreenWidth || height != g_ScreenHeight;
    if (reset) {
        tips.clear(); tick = 1;
        width = g_ScreenWidth; height = g_ScreenHeight;
    }
    trail.ResetIfNeeded(reset);
    constexpr uint32_t colors[] = {0x75FFA1, 0xA8E873, 0x42DCD0};
    if (tips.capacity() < 72) tips.reserve(72);
    tick += dt;
    if (tick >= 0.045f) {
        tick = 0;
        if (tips.size() < 8) tips.push_back({FastRandFloat(0, float(g_ScreenWidth)),
            float(g_ScreenHeight - 1), -Pi * 0.5f, 0, colors[FastRandInt(0, 2)]});
        size_t initial = tips.size();
        for (size_t i = 0; i < initial; ++i) {
            auto p = tips[i];
            p.angle += FastRandFloat(-0.38f, 0.38f);
            float length = std::max(3.0f, g_ScreenHeight * 0.009f);
            float nx = p.x + cosf(p.angle) * length, ny = p.y + sinf(p.angle) * length;
            trail.Stroke(int(p.x), int(p.y), int(nx), int(ny), p.color);
            p.x = nx; p.y = ny; p.age += 0.045f;
            if (nx < 0 || nx >= g_ScreenWidth || ny < 0 || ny >= g_ScreenHeight || p.age > 4) p.age = 100;
            tips[i] = p;
            if (p.age < 3 && tips.size() < 72 && FastRandInt(0, 20) == 0) {
                p.angle += FastRandInt(0, 1) ? 0.7f : -0.7f;
                tips.push_back(p);
            }
        }
        tips.erase(std::remove_if(tips.begin(), tips.end(), [](const Tip& p) { return p.age > 4; }), tips.end());
    }
    // Replay all deposited filaments without fading or evicting old segments.
    trail.Composite();
    for (const auto& p : tips) Fill(int(p.x) - 1, int(p.y) - 1, 3, 3, 0xE7FFD9);
}
}
