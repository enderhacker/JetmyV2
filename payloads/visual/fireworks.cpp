#include "visual.h"
#include "trail.h"

namespace VisualPayloads {
namespace {
struct Spark { float x, y, vx, vy, age, life; uint32_t color; };
struct Rocket { float x, y, targetY; uint32_t color; };
}
void EffectPixelFireworks(float time, float dt) {
    static std::vector<Spark> sparks;
    static std::vector<Rocket> rockets;
    static TrailCanvas trail;
    static int width = 0, height = 0;
    static float spawn = 1;
    bool reset = time == 0 || width != g_ScreenWidth || height != g_ScreenHeight;
    if (reset) {
        sparks.clear(); rockets.clear(); spawn = 1;
        width = g_ScreenWidth; height = g_ScreenHeight;
    }
    trail.ResetIfNeeded(reset);
    trail.Fade(dt);
    if (sparks.capacity() < 320) { sparks.reserve(320); rockets.reserve(4); }
    constexpr uint32_t colors[] = {0xFFB454, 0x65E6FF, 0xFF72AB, 0xBAFF86, 0xCE9CFF};
    spawn += dt;
    if (spawn >= 0.85f && rockets.size() < 4) {
        spawn = 0;
        rockets.push_back({FastRandFloat(0.1f, 0.9f) * g_ScreenWidth, float(g_ScreenHeight),
            FastRandFloat(0.16f, 0.55f) * g_ScreenHeight, colors[FastRandInt(0, 4)]});
    }
    for (size_t i = 0; i < rockets.size();) {
        auto& r = rockets[i];
        float previousY = r.y;
        r.y -= std::max(100.0f, g_ScreenHeight * 0.85f) * dt;
        trail.Stroke(int(r.x), int(previousY), int(r.x), int(r.y), r.color, 210);
        if (r.y > r.targetY) { ++i; continue; }
        for (int j = 0; j < 64 && sparks.size() < 320; ++j) {
            float a = j * Tau / 64 + FastRandFloat(-0.035f, 0.035f);
            float speed = FastRandFloat(0.1f, 0.3f) * std::min(g_ScreenWidth, g_ScreenHeight);
            sparks.push_back({r.x, r.y, cosf(a) * speed, sinf(a) * speed, 0, FastRandFloat(1.1f, 1.8f), r.color});
        }
        rockets[i] = rockets.back(); rockets.pop_back();
    }
    for (size_t i = 0; i < sparks.size();) {
        auto& p = sparks[i];
        p.age += dt;
        if (p.age >= p.life) { sparks[i] = sparks.back(); sparks.pop_back(); continue; }
        float previousX = p.x, previousY = p.y;
        p.x += p.vx * dt; p.y += p.vy * dt;
        p.vy += g_ScreenHeight * 0.12f * dt;
        float brightness = 1 - p.age / p.life;
        trail.Stroke(int(previousX), int(previousY), int(p.x), int(p.y),
            p.color, unsigned(80 + brightness * 175));
        ++i;
    }
    trail.Composite();
    for (const auto& r : rockets)
        Line(int(r.x), int(r.y), int(r.x), int(r.y + 8), BlendColor(r.color, 0xFFFFFF, 0.45f));
    for (const auto& p : sparks) {
        float brightness = 1 - p.age / p.life;
        Fill(int(p.x), int(p.y), 2, 2, BlendColor(p.color, 0xFFFFFF, brightness * 0.6f));
    }
}
}
