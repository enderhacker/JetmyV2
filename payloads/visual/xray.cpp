#include "visual.h"

namespace VisualPayloads {
namespace {
int Luminance(uint32_t color) {
    return int(((color >> 16 & 255) * 77 + (color >> 8 & 255) * 150 + (color & 255) * 29) >> 8);
}
}
void EffectXRayScanner(float time, float) {
    int width = std::max(1, int(g_ScreenWidth * 0.23f));
    int left = int((0.5f + 0.5f * sinf(time * 0.42f)) * (g_ScreenWidth - width));
    int step = RasterStep();
    for (int y = 0; y < g_ScreenHeight; y += step) {
        for (int x = left; x < left + width; x += step) {
            uint32_t base = Sample(x, y);
            int gx = Luminance(Sample(x + 1, y)) - Luminance(Sample(x - 1, y));
            int gy = Luminance(Sample(x, y + 1)) - Luminance(Sample(x, y - 1));
            float edge = std::min(1.0f, (abs(gx) + abs(gy)) / 100.0f);
            uint32_t film = BlendColor(0x10283D, 0x82D7F5, (255 - Luminance(base)) / 510.0f);
            film = BlendColor(film, 0xECFBFF, edge);
            float feather = Smooth(std::min(x - left, left + width - x) / 12.0f);
            Tile(x, y, step, BlendColor(base, film, feather * 0.92f));
        }
    }
    Line(left, 0, left, g_ScreenHeight - 1, 0x8AE7F6);
    Line(left + width - 1, 0, left + width - 1, g_ScreenHeight - 1, 0x8AE7F6);
    for (int y = 0; y < g_ScreenHeight; y += 32) {
        int tick = y % 128 == 0 ? 14 : 6;
        Line(left, y, std::min(left + tick, g_ScreenWidth - 1), y, 0xE2FAFF);
    }
}
}
