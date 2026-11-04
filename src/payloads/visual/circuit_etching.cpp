#include "visual.h"

namespace VisualPayloads {
void EffectCircuitEtching(float time, float) {
    GdiFlush();
    const int step = RasterStep(), cell = 36;
    float pulse = fmodf(time * 55, 4096);
    for (int y = 0; y < g_ScreenHeight; y += step)
        for (int x = 0; x < g_ScreenWidth; x += step) {
            int cx = x / cell, cy = y / cell, lx = x % cell, ly = y % cell;
            uint32_t hash = static_cast<uint32_t>(cx * 73 + cy * 151);
            bool turn = (hash ^ (hash >> 3)) & 1;
            int path = turn ? abs(lx - ly) : abs(lx + ly - cell);
            bool wire = path < 2 || (lx < 2 && (cy & 1));
            float signal = 0.5f + 0.5f * sinf((x + y - pulse) * 0.04f);
            uint32_t c = BlendColor(Sample(x, y), 0x020c09, 0.78f);
            if (wire) c = BlendColor(0x176d48, 0xcaff78, signal * signal);
            if ((lx - 18) * (lx - 18) + (ly - 18) * (ly - 18) < 20)
                c = signal > 0.8f ? 0xff6339 : 0x183e29;
            Tile(x, y, step, c);
        }
}
}
