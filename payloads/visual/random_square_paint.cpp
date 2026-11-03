#include "visual.h"
#include <numeric>

namespace VisualPayloads {
void EffectRandomSquarePaint(float time, float) {
    constexpr int cell = 32;
    static int columns = 0, rows = 0;
    static size_t painted = 0;
    static std::vector<size_t> order;
    static std::vector<uint32_t> colors;
    int newColumns = (g_ScreenWidth + cell - 1) / cell;
    int newRows = (g_ScreenHeight + cell - 1) / cell;
    if (time == 0 || columns != newColumns || rows != newRows) {
        columns = newColumns; rows = newRows; painted = 0;
        order.resize(static_cast<size_t>(columns) * rows);
        colors.resize(order.size());
        std::iota(order.begin(), order.end(), size_t(0));
        // Shuffle once so every square is eventually painted, without repeats.
        for (size_t i = order.size(); i > 1; --i)
            std::swap(order[i - 1], order[FastRand() % i]);
    }
    if (painted < order.size()) colors[painted++] = FastRand() & 0x00FFFFFFu;
    // The frame is recaptured each time, so replay the retained squares.
    for (size_t i = 0; i < painted; ++i) {
        size_t index = order[i];
        Fill(static_cast<int>(index % columns) * cell, static_cast<int>(index / columns) * cell,
            cell, cell, colors[i]);
    }
}
} // namespace VisualPayloads
