#include "visual.h"

namespace VisualPayloads {
void EffectMouseDrift(float time, float dt) {
    static POINT anchor{}, expected{};
    static float idle = 0, countdown = 0;
    POINT current{};
    if (!GetCursorPos(&current)) return;
    if (time == 0 || current.x != expected.x || current.y != expected.y) {
        anchor = expected = current;
        idle = 0; countdown = 0;
        return;
    }
    // Never compete with a drag, click, typing, or a recent user movement.
    if ((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON) |
        GetAsyncKeyState(VK_MBUTTON)) & 0x8000) { idle = 0; return; }
    LASTINPUTINFO input{sizeof(LASTINPUTINFO), 0};
    if (!GetLastInputInfo(&input) || GetTickCount() - input.dwTime < 2000) { idle = 0; return; }
    idle += dt;
    countdown -= dt;
    if (idle < 2 || countdown > 0) return;
    countdown = FastRandFloat(0.7f, 1.2f);
    RECT clip{};
    if (!GetClipCursor(&clip) || clip.right <= clip.left || clip.bottom <= clip.top) return;
    // Bounded around the last user position: no random walk across the desktop.
    int x = std::clamp(int(anchor.x) + FastRandInt(-3, 3), int(clip.left), int(clip.right - 1));
    int y = std::clamp(int(anchor.y) + FastRandInt(-3, 3), int(clip.top), int(clip.bottom - 1));
    if (SetCursorPos(x, y)) GetCursorPos(&expected);
}
} // namespace VisualPayloads
