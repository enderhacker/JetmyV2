#include "visual.h"

namespace VisualPayloads {
void EffectAsciiCorruption(float time, float) {
    GdiFlush();
    // Tiny bitmap glyphs avoid font/DC allocation and stay crisp at every DPI.
    static constexpr uint16_t glyphs[] = {0, 0x0080, 0x05d0, 0x2eba, 0x7b6f, 0x7fff};
    const int scale = std::max(2, RasterStep()), cw = 4 * scale, ch = 6 * scale;
    const int tick = static_cast<int>(fmodf(time * 9, 1024));
    for (int y = 0; y < g_ScreenHeight; y += ch)
        for (int x = 0; x < g_ScreenWidth; x += cw) {
            uint32_t source = Sample(x + cw / 2, y + ch / 2);
            int luminance = ((source >> 16 & 255) * 3 + (source >> 8 & 255) * 6 + (source & 255)) / 10;
            bool corrupt = ((x / cw * 17 + y / ch * 31 + tick) % 23) < 3;
            uint16_t glyph = glyphs[corrupt ? 5 : luminance * 5 / 255];
            Fill(x, y, cw, ch, 0x020604);
            for (int row = 0; row < 5; ++row)
                for (int col = 0; col < 3; ++col)
                    if (glyph & (1u << (row * 3 + col)))
                        Fill(x + col * scale, y + row * scale, scale, scale,
                            corrupt ? 0xff493c : BlendColor(source, 0x60ff87, 0.65f));
        }
}
}
