#include "visual.h"

namespace VisualPayloads {
void EffectBitPlaneTopography(float time, float) {
    const unsigned tick = static_cast<unsigned>(time * 24.0f);
    const unsigned mask = 1u << (7 - tick % 8);
    // Brief, frame-visible inversions; sub-frame flashes cannot be presented at 60 Hz.
    const bool invert = tick % 19 == 18;
    const int step = RasterStep();
    for (int y = 0; y < g_ScreenHeight; y += step) {
        for (int x = 0; x < g_ScreenWidth; x += step) {
            uint32_t source = Sample(x, y);
            unsigned r = (source >> 16) & 255, g = (source >> 8) & 255, b = source & 255;
            unsigned luminance = (77 * r + 150 * g + 29 * b) >> 8;
            uint32_t color = (luminance & mask) ? 0x00FFFFFFu : 0;
            // Monochrome bit-plane contours periodically acquire a neon phosphor tint.
            if ((tick / 8) & 1) {
                constexpr uint32_t palette[] = {0x40FFD0, 0xFF50E0, 0x60B0FF};
                color &= palette[(tick / 16) % 3];
            }
            if (invert) color ^= 0x00FFFFFFu;
            Tile(x, y, step, color);
        }
    }
}
} // namespace VisualPayloads
