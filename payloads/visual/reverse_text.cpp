#include "visual.h"
#include <string>

namespace VisualPayloads {
namespace {
struct Label { RECT rect; std::wstring text; bool title; };
struct Capture { std::vector<Label> labels; int visited = 0; };
std::wstring Reverse(std::wstring text) {
    std::reverse(text.begin(), text.end());
    // Preserve UTF-16 surrogate pairs instead of turning emoji into invalid text.
    for (size_t i = 0; i + 1 < text.size(); ++i) {
        if (text[i] >= 0xDC00 && text[i] <= 0xDFFF && text[i + 1] >= 0xD800 && text[i + 1] <= 0xDBFF) {
            std::swap(text[i], text[i + 1]); ++i;
        }
    }
    return text;
}
BOOL CALLBACK CollectLabel(HWND window, LPARAM parameter) {
    auto& capture = *reinterpret_cast<Capture*>(parameter);
    if (++capture.visited > 384 || capture.labels.size() >= 96) return FALSE;
    DWORD process = 0; GetWindowThreadProcessId(window, &process);
    if (process == GetCurrentProcessId() || !IsWindowVisible(window) || IsIconic(window)) return TRUE;
    RECT rect{};
    if (!GetWindowRect(window, &rect) || rect.right <= 0 || rect.bottom <= 0 ||
        rect.left >= g_ScreenWidth || rect.top >= g_ScreenHeight) return TRUE;
    wchar_t text[256]{};
    // Read captions only. Never send WM_SETTEXT, inject code, or alter another app.
    if (!GetWindowTextW(window, text, 256)) return TRUE;
    bool title = (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD) == 0;
    if (title) {
        if (!(GetWindowLongPtrW(window, GWL_STYLE) & WS_CAPTION)) return TRUE;
        rect.left += 32; rect.right -= 140;
        rect.top += GetSystemMetrics(SM_CYFRAME);
        rect.bottom = rect.top + GetSystemMetrics(SM_CYCAPTION);
    }
    if (rect.right - rect.left < 12 || rect.bottom - rect.top < 10) return TRUE;
    capture.labels.push_back({rect, Reverse(text), title});
    return TRUE;
}
BOOL CALLBACK CollectWindow(HWND window, LPARAM parameter) {
    auto& capture = *reinterpret_cast<Capture*>(parameter);
    if (!CollectLabel(window, parameter)) return FALSE;
    DWORD process = 0; GetWindowThreadProcessId(window, &process);
    if (process != GetCurrentProcessId() && IsWindowVisible(window) && !IsIconic(window))
        EnumChildWindows(window, CollectLabel, parameter);
    return capture.visited < 384 && capture.labels.size() < 96;
}
}

void EffectReverseText(float time, float dt) {
    static Capture capture;
    static float refresh = 0;
    if (time == 0) refresh = 0;
    refresh -= dt;
    if (refresh <= 0) {
        capture.labels.clear(); capture.visited = 0;
        EnumWindows(CollectWindow, reinterpret_cast<LPARAM>(&capture));
        refresh = 0.75f;
    }
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    SelectObject(g_hMemDC, GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(g_hMemDC, TRANSPARENT);
    SetTextColor(g_hMemDC, RGB(30, 34, 40));
    for (auto it = capture.labels.rbegin(); it != capture.labels.rend(); ++it) {
        RECT rect = it->rect;
        Fill(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, it->title ? 0xE6EAF0 : 0xF3F4F6);
        DrawTextW(g_hMemDC, it->text.c_str(), -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        GdiFlush();
    }
    // Modern search fields/custom-drawn UI do not expose Win32 captions. Show a
    // clearly simulated reversed search box instead of editing their real content.
    int w = std::min(300, g_ScreenWidth), h = std::min(36, g_ScreenHeight);
    int x = std::max(0, (g_ScreenWidth - w) / 2), y = std::max(0, g_ScreenHeight - h - 8);
    Fill(x, y, w, h, 0xE8EBEF);
    RECT search{x + 12, y, x + w - 12, y + h};
    DrawTextW(g_hMemDC, L"hcraeS  /  weiverp txet desreveR", -1, &search, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    GdiFlush(); RestoreDC(g_hMemDC, saved);
}
} // namespace VisualPayloads
