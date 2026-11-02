#include "visual.h"

namespace VisualPayloads {
void EffectMatrixReconstruction(float time, float) {
    for (int i = 0; i < g_ScreenWidth * g_ScreenHeight; i += 3) g_pFramePixels[i] &= 0x007F7F7F;
    SetBkMode(g_hMemDC, TRANSPARENT);
    SetTextColor(g_hMemDC, RGB(40, 255, 120));
    const char glyphs[] = "0123456789ABCDEF$#@%&*+-=";
    for (int col = 0; col < g_ScreenWidth; col += 24) {
        int head = static_cast<int>(fmodf(time * 420 + col * 27, g_ScreenHeight + 350.0f)) - 160;
        for (int step = 0; step < 9; ++step) {
            int y = head - step * 18;
            if (y >= 0 && y < g_ScreenHeight - 18) {
                char ch = glyphs[(col + step + static_cast<int>(time * 15)) % (sizeof(glyphs) - 1)];
                TextOutA(g_hMemDC, col, y, &ch, 1);
            }
        }
    }
}
}
