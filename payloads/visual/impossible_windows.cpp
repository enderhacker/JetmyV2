#include "visual.h"

namespace VisualPayloads {
namespace {
struct GlitchWin { float x, y, vx, vy; int w, h; const char* title; uint32_t col; };
std::vector<GlitchWin> windows;
}
void EffectImpossibleWindows(float, float dt) {
    static const char* titles[] = {
        "CRITICAL_PROCESS_DIED", "PAGE_FAULT_IN_NONPAGED_AREA", "SYSTEM_SERVICE_EXCEPTION",
        "STACK_HASH_CORRUPTION", "QUANTUM_CORE_HALTED", "HEAP_SEGMENT_OVERFLOW"
    };
    if (windows.empty()) {
        for (int i = 0; i < 5; ++i) {
            windows.push_back({FastRandFloat(0, std::max(0, g_ScreenWidth - 350)),
                FastRandFloat(0, std::max(0, g_ScreenHeight - 250)),
                FastRandFloat(-250, 250), FastRandFloat(-220, 220),
                std::min(g_ScreenWidth, FastRandInt(220, 340)),
                std::min(g_ScreenHeight, FastRandInt(140, 220)), titles[i],
                (FastRand() & 0x007F1F1F) | 0x00800000});
        }
    }
    for (auto& w : windows) {
        w.x += w.vx * dt; w.y += w.vy * dt;
        if (w.x < 0 || w.x + w.w > g_ScreenWidth) {
            w.vx = -w.vx; w.x = std::clamp(w.x, 0.0f, float(g_ScreenWidth - w.w));
        }
        if (w.y < 0 || w.y + w.h > g_ScreenHeight) {
            w.vy = -w.vy; w.y = std::clamp(w.y, 0.0f, float(g_ScreenHeight - w.h));
        }
        int ix = static_cast<int>(w.x), iy = static_cast<int>(w.y);
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ix, iy, w.w, 24, w.col);
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ix, iy + 24, w.w, w.h - 24, 0x001A1A1A);
        SetBkMode(g_hMemDC, TRANSPARENT);
        SetTextColor(g_hMemDC, RGB(255, 255, 255));
        TextOutA(g_hMemDC, ix + 8, iy + 4, w.title, static_cast<int>(strlen(w.title)));
        StretchBlt(g_hMemDC, ix + 2, iy + 26, w.w - 4, w.h - 28, g_hScratchDC,
            ix % 100, iy % 100, g_ScreenWidth / 3, g_ScreenHeight / 3, SRCCOPY);
    }
}
}
