#include "visual.h"

namespace VisualPayloads {
void EffectPaperAccordion(float time, float) {
    GdiFlush();
    const int step = RasterStep();
    const float fold = 0.25f + 0.65f * (0.5f + 0.5f * sinf(time * 1.7f));
    const float width = std::max(24.0f, g_ScreenWidth / 12.0f);
    for (int y = 0; y < g_ScreenHeight; y += step)
        for (int x = 0; x < g_ScreenWidth; x += step) {
            float u = x / width, face = floorf(u), local = u - face;
            bool back = (static_cast<int>(face) & 1) != 0;
            float ridge = back ? local : 1 - local;
            int sy = static_cast<int>((y - g_ScreenHeight * 0.5f) /
                (1 - ridge * fold * 0.65f) + g_ScreenHeight * 0.5f);
            int sx = static_cast<int>((face + (back ? 1 - local : local)) * width);
            uint32_t c = Sample(sx, sy);
            c = BlendColor(c, back ? 0x160026 : 0xffd7b0, back ? fold * 0.75f : ridge * 0.3f);
            if (local < 0.025f) c = 0xff4040;
            Tile(x, y, step, c);
        }
}
}
