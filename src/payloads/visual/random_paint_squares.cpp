#include "visual.h"

namespace VisualPayloads {
void EffectRandomSquarePaint(float time, float) {
    constexpr int cell = 32;
    static int columns = 0, rows = 0;
    static std::vector<uint32_t> colors;
    int newColumns = (g_ScreenWidth + cell - 1) / cell;
    int newRows = (g_ScreenHeight + cell - 1) / cell;
    if (time == 0 || columns != newColumns || rows != newRows) {
        columns = newColumns; rows = newRows;
        colors.assign(static_cast<size_t>(columns) * rows, 0);
    }
    if (!colors.empty()) colors[FastRand() % colors.size()] = (FastRand() & 0x00FFFFFFu) | 0xFF000000u;
    // The frame is recaptured each time, so replay the retained squares.
    for (size_t index = 0; index < colors.size(); ++index) {
        if (!colors[index]) continue;
        Fill(static_cast<int>(index % columns) * cell, static_cast<int>(index / columns) * cell,
            cell, cell, colors[index] & 0x00FFFFFFu);
    }
}
} // namespace VisualPayloads
