#include "visual.h"

namespace VisualPayloads {

void EffectAsciiCorruption(float, float) {
    GdiFlush();

    // 16 bitmap glyphs covering every density level (0 to 15 active pixels in a 3x5 grid).
    // This provides the maximum theoretical density precision possible for this cell resolution.
    static constexpr uint16_t glyphs[16] = {
        0x0000, // [0]  ' ' (0 bits)  - Space
        0x0080, // [1]  '.' (1 bit)   - Center dot
        0x0410, // [2]  ':' (2 bits)  - Colon
        0x01c0, // [3]  '-' (3 bits)  - Dash
        0x2482, // [4]  'i' (4 bits)  - Lowercase i
        0x05d0, // [5]  '+' (5 bits)  - Plus
        0x04ba, // [6]  'v' (6 bits)  - Dagger / Arrow
        0x2497, // [7]  'T' (7 bits)  - Capital T
        0x2eba, // [8]  '#' (8 bits)  - Hash / Double cross
        0x724f, // [9]  'C' (9 bits)  - Capital C
        0x5be2, // [10] 'A' (10 bits) - Capital A
        0x73cf, // [11] 'E' (11 bits) - Capital E
        0x7b6f, // [12] 'O' (12 bits) - Box / Capital O
        0x7bef, // [13] '8' (13 bits) - Figure 8
        0x7f7f, // [14] [X] (14 bits) - Dense block with center cutout
        0x7fff  // [15] [█] (15 bits) - Full solid block
    };

    const int scale = std::max(2, RasterStep());
    const int cw = 4 * scale;
    const int ch = 6 * scale;

    for (int y = 0; y < g_ScreenHeight; y += ch) {
        for (int x = 0; x < g_ScreenWidth; x += cw) {
            // Stratified 4-point quadrant sampling to compute an accurate average color
            const uint32_t s0 = Sample(x + cw / 4,     y + ch / 4);
            const uint32_t s1 = Sample(x + 3 * cw / 4, y + ch / 4);
            const uint32_t s2 = Sample(x + cw / 4,     y + 3 * ch / 4);
            const uint32_t s3 = Sample(x + 3 * cw / 4, y + 3 * ch / 4);

            const uint32_t r = ((s0 >> 16 & 0xFF) + (s1 >> 16 & 0xFF) + (s2 >> 16 & 0xFF) + (s3 >> 16 & 0xFF) + 2) >> 2;
            const uint32_t g = ((s0 >>  8 & 0xFF) + (s1 >>  8 & 0xFF) + (s2 >>  8 & 0xFF) + (s3 >>  8 & 0xFF) + 2) >> 2;
            const uint32_t b = ((s0       & 0xFF) + (s1       & 0xFF) + (s2       & 0xFF) + (s3       & 0xFF) + 2) >> 2;

            const uint32_t avgColor = (r << 16) | (g << 8) | b;
            const int luminance = static_cast<int>(r * 3 + g * 6 + b) / 10;
            const int glyphIdx = std::clamp((luminance * 15 + 127) / 255, 0, 15);
            const uint16_t glyph = glyphs[glyphIdx];

            // Clear cell background to black for maximum ASCII contrast
            Fill(x, y, cw, ch, 0x000000);

            // Fast paths for common extremes
            if (glyph == 0) continue;

            if (glyph == 0x7fff) {
                Fill(x, y, 3 * scale, 5 * scale, avgColor);
                continue;
            }

            // Stream through bits without recalculating coordinates
            uint16_t bits = glyph;
            for (int row = 0; row < 5; ++row) {
                for (int col = 0; col < 3; ++col, bits >>= 1) {
                    if (bits & 1) {
                        Fill(x + col * scale, y + row * scale, scale, scale, avgColor);
                    }
                }
            }
        }
    }
}

} // namespace VisualPayloads