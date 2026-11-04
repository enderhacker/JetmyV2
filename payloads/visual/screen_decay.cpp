#include "visual.h"

namespace VisualPayloads {
namespace {
uint32_t DecayHash(uint32_t value) {
    value ^= value >> 16; value *= 0x7FEB352Du;
    value ^= value >> 15; value *= 0x846CA68Bu;
    return value ^ (value >> 16);
}
}

void EffectScreenDecay(float time, float) {
    const int w = g_ScreenWidth, h = g_ScreenHeight;
    if (w <= 0 || h <= 0) return;
    // Reach full decay once, then keep animating rather than resetting the damage.
    const float decay = Smooth(time / 35.0f);
    const float heat = Smooth((time - 8.0f) / 27.0f);
    const float t = fmodf(time, 3600.0f);
    const int block = std::max(RasterStep(), 1 + int(decay * 23));
    const int columns = (w + block - 1) / block, rows = (h + block - 1) / block;
    const int shiftX = int(w * (0.008f + decay * 0.015f + sinf(t * 0.43f) * 0.006f));
    const int shiftY = int(h * (-0.004f - decay * 0.010f + sinf(t * 0.37f) * 0.006f));
    const unsigned lostBits = unsigned(decay * 5);
    const uint32_t channelMask = (255u << lostBits) & 255u;
    const uint32_t colorMask = channelMask * 0x010101u;
    const unsigned damageTick = unsigned(t * 4);

    // One held color per 32px coding tile, not an ever-growing screenshot history.
    // Update on the render thread; pixel workers only read this bounded state.
    constexpr int codingTile = 32;
    struct HeldTile { uint32_t color = 0, stamp = ~0u, damage = 0; };
    static std::vector<HeldTile> held;
    static std::vector<float> verticalHeat;
    static uint32_t seed = 0;
    static int width = 0, height = 0;
    const int codingColumns = (w + codingTile - 1) / codingTile;
    const int codingRows = (h + codingTile - 1) / codingTile;
    if (time == 0 || width != w || height != h) {
        width = w; height = h; seed = FastRand();
        held.assign(static_cast<size_t>(codingColumns) * codingRows, {});
        for (size_t i = 0; i < held.size(); ++i)
            held[i].damage = DecayHash(uint32_t(i) ^ seed);
    }
    const unsigned clock = unsigned(t * (18 - decay * 14));
    for (int row = 0; row < codingRows; ++row) {
        for (int col = 0; col < codingColumns; ++col) {
            size_t index = static_cast<size_t>(row) * codingColumns + col;
            auto& tile = held[index];
            unsigned period = 1 + unsigned(decay * 9) * (1 + (tile.damage & 3u));
            unsigned stamp = clock / period;
            if (tile.stamp != stamp) {
                tile.color = Sample(col * codingTile + codingTile / 2 - shiftX,
                    row * codingTile + codingTile / 2 - shiftY);
                tile.stamp = stamp;
            }
        }
    }
    verticalHeat.resize(columns);
    for (int col = 0; col < columns; ++col)
        verticalHeat[col] = sinf(col * block * 0.019f + t * 1.3f) * heat * h * 0.004f;

    Runtime::ParallelFor(rows, [&](int row) {
        const int y = row * block, bottom = std::min(h, y + block);
        const uint32_t band = DecayHash(uint32_t(y / codingTile) ^ seed ^ damageTick * 0x9E3779B9u);
        const bool torn = (band & 255u) < unsigned(decay * 52);
        const int tear = torn ? int((int((band >> 8) & 255u) - 128) * decay * w * 0.0005f) : 0;
        const float horizontalHeat = sinf(y * 0.022f - t * 1.7f) * heat * w * 0.006f;
        for (int col = 0; col < columns; ++col) {
            const int x = col * block, right = std::min(w, x + block);
            const int sx = int(x - shiftX + tear + horizontalHeat);
            const int sy = int(y - shiftY + verticalHeat[col]);
            uint32_t color = 0x070100;
            if (sx >= 0 && sx < w && sy >= 0 && sy < h) {
                const size_t index = static_cast<size_t>(y / codingTile) * codingColumns + x / codingTile;
                const uint32_t damage = held[index].damage;
                color = Sample(sx, sy);
                // Stalled/flat coding tiles, displaced strips, and missing data.
                if ((damage & 255u) < unsigned(decay * 65))
                    color = BlendColor(color, held[index].color, decay * 0.85f);
                if (torn && ((damage >> 8) & 15u) < unsigned(decay * 5))
                    color = BlendColor(color, 0x090102, decay * 0.9f);
                color &= colorMask;
                if (heat > 0) {
                    unsigned r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
                    uint32_t scorched = (std::min(255u, r + g / 3 + 40) << 16) | ((g / 3) << 8) | (b / 8);
                    color = BlendColor(color, scorched, heat * 0.72f);
                    float edgeX = fabsf(2.0f * x / std::max(1, w - 1) - 1);
                    float edgeY = fabsf(2.0f * y / std::max(1, h - 1) - 1);
                    color = BlendColor(color, 0x060000, heat * (0.12f + 0.28f * std::max(edgeX, edgeY)));
                    // Thin ember seams, not a full-screen white/inversion flash.
                    if (torn && (damage & 7u) == 0 && y % codingTile < block)
                        color = BlendColor(color, 0xF15A12, heat * 0.65f);
                }
            }
            for (int yy = y; yy < bottom; ++yy) {
                auto* destination = g_pFramePixels + static_cast<size_t>(yy) * w + x;
                std::fill_n(destination, right - x, color);
            }
        }
    });
}
} // namespace VisualPayloads
