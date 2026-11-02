#include "visual.h"

namespace VisualPayloads {
namespace {
struct Particle { float x, y, vx, vy; uint32_t col; float life; };
std::vector<Particle> particles;
}
void EffectPixelFireworks(float, float dt) {
    if (FastRandInt(0, 5) == 0 || particles.empty()) {
        float ox = FastRandFloat(g_ScreenWidth * 0.05f, g_ScreenWidth * 0.95f);
        float oy = FastRandFloat(g_ScreenHeight * 0.05f, g_ScreenHeight * 0.65f);
        uint32_t col = (FastRand() & 0x00FFFFFF) | 0x00404040;
        int count = FastRandInt(30, 70);
        float baseSpd = FastRandFloat(100.0f, 450.0f);
        for (int i = 0; i < count; ++i) {
            float ang = FastRandFloat(0.0f, 6.28318f);
            float spd = baseSpd * FastRandFloat(0.3f, 1.2f);
            particles.push_back({ox, oy, cosf(ang) * spd, sinf(ang) * spd, col, FastRandFloat(0.4f, 1.4f)});
        }
    }
    for (size_t i = 0; i < particles.size();) {
        auto& p = particles[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 320.0f * dt;
        p.life -= dt;
        int ix = static_cast<int>(p.x), iy = static_cast<int>(p.y);
        if (p.life > 0 && ix >= 0 && ix < g_ScreenWidth - 2 && iy >= 0 && iy < g_ScreenHeight - 2) {
            FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ix, iy, 3, 3, p.col);
            ++i;
        } else {
            particles[i] = particles.back();
            particles.pop_back();
        }
    }
}
}
