#include "visual.h"

namespace VisualPayloads {
void EffectBlackHoleSingularities(float time, float dt) {
    static float hx = 0, hy = 0, heading = 0, speed = 0, targetSpeed = 0;
    static float turn = 0, targetTurn = 0, remaining = 0;
    static int width = 0, height = 0;
    float unit = float(std::max(1, std::min(g_ScreenWidth, g_ScreenHeight)));
    float radius = std::max(1.0f, unit * 0.28f);
    float marginX = std::min(radius * 0.45f, (g_ScreenWidth - 1) * 0.5f);
    float marginY = std::min(radius * 0.45f, (g_ScreenHeight - 1) * 0.5f);
    if (time == 0 || width != g_ScreenWidth || height != g_ScreenHeight) {
        width = g_ScreenWidth; height = g_ScreenHeight;
        hx = FastRandFloat(marginX, width - 1 - marginX);
        hy = FastRandFloat(marginY, height - 1 - marginY);
        heading = FastRandFloat(0, Tau);
        speed = targetSpeed = FastRandFloat(0.04f, 0.16f) * unit;
        turn = targetTurn = FastRandFloat(-0.8f, 0.8f);
        remaining = FastRandFloat(1.2f, 3.8f);
    }
    remaining -= dt;
    if (remaining <= 0) {
        remaining = FastRandFloat(1.2f, 3.8f);
        targetSpeed = FastRandFloat(0.04f, 0.16f) * unit;
        targetTurn = FastRandFloat(-0.8f, 0.8f);
    }
    float response = 1 - expf(-dt * 1.8f);
    speed += (targetSpeed - speed) * response;
    turn += (targetTurn - turn) * response;
    heading = fmodf(heading + turn * dt, Tau);
    hx += cosf(heading) * speed * dt;
    hy += sinf(heading) * speed * dt;
    if (hx < marginX || hx > width - 1 - marginX) {
        hx = std::clamp(hx, marginX, width - 1 - marginX);
        heading = Pi - heading;
        turn = -turn; targetTurn = -targetTurn;
    }
    if (hy < marginY || hy > height - 1 - marginY) {
        hy = std::clamp(hy, marginY, height - 1 - marginY);
        heading = -heading;
        turn = -turn; targetTurn = -targetTurn;
    }
    int step = RasterStep();
    int left = std::max(0, int(hx - radius)), top = std::max(0, int(hy - radius));
    int right = std::min(g_ScreenWidth, int(hx + radius + 1));
    int bottom = std::min(g_ScreenHeight, int(hy + radius + 1));
    Runtime::ParallelFor((bottom - top + step - 1) / step, [&](int row) {
        int y = top + row * step;
        for (int x = left; x < right; x += step) {
            float dx = (x - hx) / radius, dy = (y - hy) / radius;
            float r2 = dx * dx + dy * dy;
            if (r2 >= 1) continue;
            float r = sqrtf(r2), edge = Smooth((1 - r) * 5);
            float angle = atan2f(dy, dx);
            float twist = 2.8f * (1 - r) * (1 - r) + time * 0.24f;
            float c = cosf(twist), s = sinf(twist), pull = 1 - edge * 0.25f;
            uint32_t warped = Sample(int(hx + (dx * c - dy * s) * radius * pull),
                int(hy + (dx * s + dy * c) * radius * pull));
            float ring = std::max(0.0f, 1 - fabsf(r - 0.31f) * 22);
            uint32_t hot = BlendColor(0xD84919, 0xFFF0AB, 0.5f + 0.5f * sinf(angle * 3 - time * 2));
            uint32_t color = BlendColor(warped, 0x080712, Smooth((0.25f - r) * 30));
            color = BlendColor(color, hot, ring * 0.95f);
            Tile(x, y, step, BlendColor(Sample(x, y), color, edge));
        }
    });
}
}
