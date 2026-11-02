#include "visual.h"

namespace VisualPayloads {
void EffectMatrixReconstruction(float time, float dt) {
    struct Stream { float head, speed; int length; uint32_t seed; };
    static std::vector<Stream> streams;
    int cell = std::max(10, std::min(20, g_ScreenWidth / 35));
    size_t columns = static_cast<size_t>((g_ScreenWidth + cell - 1) / cell);
    if (streams.size() != columns) {
        streams.resize(columns);
        for (auto& s : streams) s = {FastRandFloat(-float(g_ScreenHeight), float(g_ScreenHeight)),
            FastRandFloat(65, 170), FastRandInt(6, 16), FastRand()};
    }
    constexpr char glyphs[] = "0123456789ABCDEF<>+*:$";
    for (size_t column = 0; column < streams.size(); ++column) {
        auto& stream = streams[column];
        stream.head += stream.speed * dt;
        if (stream.head - stream.length * 16 > g_ScreenHeight) stream.head = -16;
        int x = int(column) * cell;
        // Darken only this stream's footprint, leaving other payloads visible.
        Tint(x, int(stream.head) - stream.length * 16, cell - 1, stream.length * 16 + 16, 0x02120C, 0.6f);
    }
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    SelectObject(g_hMemDC, GetStockObject(ANSI_FIXED_FONT)); SetBkMode(g_hMemDC, TRANSPARENT);
    for (size_t column = 0; column < streams.size(); ++column) {
        const auto& stream = streams[column];
        for (int i = stream.length - 1; i >= 0; --i) {
            int y = int(stream.head) - i * 16;
            if (y < -16 || y >= g_ScreenHeight) continue;
            float fade = 1 - float(i) / stream.length;
            SetTextColor(g_hMemDC, i == 0 ? RGB(217, 255, 227) : RGB(22, int(50 + 180 * fade), int(35 + 60 * fade)));
            uint32_t hash = stream.seed ^ (uint32_t(i) * 747796405u) ^ (uint32_t(time * 8) * 2891336453u);
            char ch = glyphs[(hash ^ (hash >> 16)) % (sizeof(glyphs) - 1)];
            TextOutA(g_hMemDC, int(column) * cell, y, &ch, 1);
        }
    }
    RestoreDC(g_hMemDC, saved);
}
}
