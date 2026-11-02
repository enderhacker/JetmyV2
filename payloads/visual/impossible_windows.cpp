#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectImpossibleWindows(float time, float dt) {
    struct Window { float x, y, vx, vy; };
    static std::array<Window, 4> windows{};
    static bool initialized = false;
    if (time == 0) initialized = false;
    int w = std::min(310, g_ScreenWidth), h = std::min(184, g_ScreenHeight);
    if (!initialized) {
        for (auto& win : windows) win = {FastRandFloat(0, float(g_ScreenWidth - w)),
            FastRandFloat(0, float(g_ScreenHeight - h)), FastRandFloat(30, 90), FastRandFloat(22, 65)};
        initialized = true;
    }
    constexpr const char* titles[] = {"Recursive viewport", "Window inside a window", "Frame buffer / 03", "Unreachable desktop"};
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    for (size_t i = 0; i < windows.size(); ++i) {
        auto& win = windows[i];
        win.x += win.vx * dt; win.y += win.vy * dt;
        if (win.x < 0 || win.x > g_ScreenWidth - w) win.vx = -win.vx;
        if (win.y < 0 || win.y > g_ScreenHeight - h) win.vy = -win.vy;
        win.x = std::clamp(win.x, 0.0f, float(g_ScreenWidth - w));
        win.y = std::clamp(win.y, 0.0f, float(g_ScreenHeight - h));
        int x = int(win.x), y = int(win.y);
        Tint(x + 5, y + 6, w, h, 0x080B18, 0.45f);
        Fill(x, y, w, h, 0x1A2133);
        Fill(x, y, w, std::min(24, h), i % 2 ? 0x53427C : 0x285A70);
        if (w > 48 && h > 48) {
            int panX = int((0.5f + 0.5f * sinf(time * 0.4f + i)) * std::max(0, g_ScreenWidth - w));
            StretchBlt(g_hMemDC, x + 4, y + 28, w - 8, h - 32, g_hScratchDC,
                panX, 0, w, g_ScreenHeight, SRCCOPY);
            GdiFlush();
            // Nested title bars make the recursive-window identity explicit.
            for (int inset = 18; inset < std::min(w, h) / 2; inset += 22) {
                Fill(x + inset, y + 28 + inset / 2, w - inset * 2, 9, 0x5D7395);
                Line(x + inset, y + 37 + inset / 2, x + inset, y + h - inset / 2, 0xAAC7E4);
            }
        }
        if (w > 100 && h >= 24) {
            SetBkMode(g_hMemDC, TRANSPARENT); SetTextColor(g_hMemDC, RGB(235, 243, 255));
            RECT text = {x + 8, y + 3, x + w - 26, y + 22};
            DrawTextA(g_hMemDC, titles[i], -1, &text, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            // Flush text before the next window touches pixels with CPU helpers.
            GdiFlush();
            Line(x + w - 16, y + 8, x + w - 9, y + 15, 0xFFFFFF);
            Line(x + w - 9, y + 8, x + w - 16, y + 15, 0xFFFFFF);
        }
    }
    RestoreDC(g_hMemDC, saved);
}
}
