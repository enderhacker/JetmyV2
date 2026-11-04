#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectDesktopTimeEchoes(float time, float) {
    if (g_EchoHistory.size() != MAX_ECHOES || g_EchoHead == 0) return;
    int step = 1;
    // Four translucent time slices, each with a different actual capture delay.
    std::array<const EchoFrame*, 4> frames{};
    for (size_t panel = 0; panel < frames.size(); ++panel) {
        frames[panel] = &g_EchoHistory[(g_EchoHead + MAX_ECHOES - 1 - (3 + panel * 4)) % MAX_ECHOES];
        if (frames[panel]->w <= 0 || frames[panel]->h <= 0 || frames[panel]->pixels.empty()) return;
    }
    struct Column { int panel, sx; float fraction, opacity; };
    static std::vector<Column> columns;
    columns.resize((g_ScreenWidth + step - 1) / step);
    float drift = sinf(time * 0.4f) * g_ScreenWidth * 0.06f;
    int panelWidth = std::max(1, g_ScreenWidth / 4);
    for (size_t i = 0; i < columns.size(); ++i) {
        int x = int(i) * step;
        int panel = std::clamp(int((x + drift) / panelWidth), 0, 3);
        float sourceX = float(x) * frames[panel]->w / g_ScreenWidth;
        int sx = std::min(frames[panel]->w - 1, int(sourceX));
        int position = (x + int(drift)) % panelWidth;
        if (position < 0) position += panelWidth;
        float opacity = (0.32f + panel * 0.12f) * Smooth(std::min(position, panelWidth - position) / 14.0f);
        columns[i] = {panel, sx, sourceX - sx, opacity};
    }
    // Walk contiguous rows, not cache-hostile full-height columns.
    Runtime::ParallelFor((g_ScreenHeight + step - 1) / step, [&](int row) {
        int y = row * step;
        std::array<const uint32_t*, 4> rows{};
        std::array<const uint32_t*, 4> nextRows{};
        std::array<float, 4> fractions{};
        for (size_t i = 0; i < frames.size(); ++i) {
            float sourceY = float(y) * frames[i]->h / g_ScreenHeight;
            int sy = std::min(frames[i]->h - 1, int(sourceY));
            rows[i] = frames[i]->pixels.data() + static_cast<size_t>(sy) * frames[i]->w;
            nextRows[i] = frames[i]->pixels.data() + static_cast<size_t>(std::min(sy + 1, frames[i]->h - 1)) * frames[i]->w;
            fractions[i] = sourceY - sy;
        }
        for (size_t i = 0; i < columns.size(); ++i) {
            const auto& col = columns[i];
            int x = int(i) * step;
            uint32_t echo = rows[col.panel][col.sx];
            if (frames[col.panel]->w != g_ScreenWidth || frames[col.panel]->h != g_ScreenHeight) {
                int next = std::min(col.sx + 1, frames[col.panel]->w - 1);
                uint32_t top = BlendColor(echo, rows[col.panel][next], col.fraction);
                uint32_t bottom = BlendColor(nextRows[col.panel][col.sx], nextRows[col.panel][next], col.fraction);
                echo = BlendColor(top, bottom, fractions[col.panel]);
            }
            Tile(x, y, step, BlendColor(Sample(x, y), echo, col.opacity));
        }
    });
}
}
