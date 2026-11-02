#include "menu.h"
#include "../payloads/sound/sound.h"
#include <stdexcept>

namespace Runtime {
namespace {
constexpr wchar_t MenuClass[] = L"JetmyPayloadControlPanel";
constexpr wchar_t PaneClass[] = L"JetmyPayloadControlPane";
constexpr int VisualBase = 1000, AudioBase = 2000, StopAll = 10, Close = 11;
std::wstring Wide(const char* text) { return std::wstring(text, text + strlen(text)); } // Registry names are ASCII.
}

ControlPanel::~ControlPanel() {
    if (window_) DestroyWindow(window_);
    if (font_) DeleteObject(font_);
    if (titleFont_) DeleteObject(titleFont_);
    if (registered_) UnregisterClassW(MenuClass, instance_);
    if (paneRegistered_) UnregisterClassW(PaneClass, instance_);
}

HWND ControlPanel::Add(HWND parent, const wchar_t* kind, const std::wstring& text, DWORD style, int id) {
    HWND child = CreateWindowExW(0, kind, text.c_str(), WS_CHILD | WS_VISIBLE | style,
        0, 0, 1, 1, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    if (!child) throw std::runtime_error("Cannot create menu control.");
    SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    return child;
}

void ControlPanel::Create() {
    HDC screen = GetDC(nullptr);
    if (screen) { dpi_ = GetDeviceCaps(screen, LOGPIXELSY); ReleaseDC(nullptr, screen); }
    font_ = CreateFontW(-Scale(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    titleFont_ = CreateFontW(-Scale(23), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    if (!font_ || !titleFont_) throw std::runtime_error("Cannot create menu fonts.");
    WNDCLASSW wc{};
    wc.hInstance = instance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = WindowProc; wc.lpszClassName = MenuClass;
    if (!RegisterClassW(&wc)) throw std::runtime_error("Cannot register menu window.");
    registered_ = true;
    wc.lpfnWndProc = PaneProc; wc.lpszClassName = PaneClass;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) throw std::runtime_error("Cannot register menu lists.");
    paneRegistered_ = true;
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int width = std::min(Scale(940), int(work.right - work.left));
    int height = std::min(Scale(760), int(work.bottom - work.top));
    window_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_CONTROLPARENT, MenuClass, L"JetmyV2 - Payload controls",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, work.left + (work.right - work.left - width) / 2,
        work.top + (work.bottom - work.top - height) / 2, width, height,
        nullptr, nullptr, instance_, this);
    if (!window_) throw std::runtime_error("Cannot create payload menu.");
    title_ = Add(window_, L"STATIC", L"Payload controls", SS_LEFT);
    SendMessageW(title_, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont_), FALSE);
    description_ = Add(window_, L"STATIC", L"Manual mode. Visuals combine freely; audio selections replace one another. Everything starts off.", SS_LEFT);
    visualsTitle_ = Add(window_, L"STATIC", L"Visual payloads (" + std::to_wstring(controls_.visuals.size()) + L")", SS_LEFT);
    audioTitle_ = Add(window_, L"STATIC", L"Looping audio (" + std::to_wstring(SoundPayloads::Registry().size()) + L")", SS_LEFT);
    stop_ = Add(window_, L"BUTTON", L"Stop all", BS_PUSHBUTTON | WS_TABSTOP, StopAll);
    close_ = Add(window_, L"BUTTON", L"Close", BS_PUSHBUTTON | WS_TABSTOP, Close);
    status_ = Add(window_, L"STATIC", std::wstring{}, SS_LEFT);
    hint_ = Add(window_, L"STATIC", L"Flashing visuals / electronic audio. Emergency exit: Ctrl+Shift+Q or Ctrl+Shift+Esc.", SS_LEFT);
    for (auto* pane : {&visuals_, &audio_}) {
        pane->row = Scale(38);
        pane->window = CreateWindowExW(WS_EX_CONTROLPARENT, PaneClass, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
            0, 0, 1, 1, window_, nullptr, instance_, pane);
        if (!pane->window) throw std::runtime_error("Cannot create scrollable payload list.");
    }
    DWORD toggle = BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_NOTIFY | WS_TABSTOP;
    for (size_t i = 0; i < controls_.visuals.size(); ++i)
        visuals_.buttons.push_back(Add(visuals_.window, L"BUTTON", Wide(VisualPayloads::Registry()[i].name), toggle, VisualBase + int(i)));
    for (size_t i = 0; i < SoundPayloads::Registry().size(); ++i)
        audio_.buttons.push_back(Add(audio_.window, L"BUTTON", Wide(SoundPayloads::Registry()[i].name), toggle, AudioBase + int(i)));
    Layout(); Refresh();
    ShowWindow(window_, SW_SHOW); UpdateWindow(window_);
    SetFocus(visuals_.buttons.empty() ? stop_ : visuals_.buttons.front());
}

void ControlPanel::Pane::Reflow() {
    RECT rect{}; GetClientRect(window, &rect);
    int total = int(buttons.size()) * row + 12;
    offset = std::clamp(offset, 0, std::max(0, total - int(rect.bottom)));
    SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL,
        0, total - 1, static_cast<UINT>(rect.bottom), offset, 0};
    SetScrollInfo(window, SB_VERT, &scroll, TRUE);
    GetClientRect(window, &rect);
    for (size_t i = 0; i < buttons.size(); ++i)
        MoveWindow(buttons[i], 6, 6 + int(i) * row - offset, std::max(1, int(rect.right) - 12), row - 4, TRUE);
}
void ControlPanel::Pane::Scroll(int position) { offset = position; Reflow(); }

void ControlPanel::Layout() {
    if (!title_) return; // Initial WM_SIZE precedes child creation.
    RECT rect{}; GetClientRect(window_, &rect);
    int w = rect.right, h = rect.bottom, pad = Scale(24), gap = Scale(16);
    int column = std::max(1, (w - pad * 2 - gap) / 2), right = pad + column + gap;
    auto move = [](HWND child, int x, int y, int width, int height) { MoveWindow(child, x, y, std::max(1, width), std::max(1, height), TRUE); };
    move(title_, pad, Scale(18), w - pad * 2 - Scale(140), Scale(32));
    move(stop_, w - pad - Scale(120), Scale(18), Scale(120), Scale(34));
    move(description_, pad, Scale(60), w - pad * 2, Scale(42));
    move(visualsTitle_, pad, Scale(112), column, Scale(24));
    move(audioTitle_, right, Scale(112), column, Scale(24));
    int top = Scale(144), bottom = h - Scale(112);
    if (visuals_.window) move(visuals_.window, pad, top, column, bottom - top);
    if (audio_.window) move(audio_.window, right, top, w - pad - right, bottom - top);
    move(status_, pad, h - Scale(94), w - pad * 2 - Scale(110), Scale(44));
    move(close_, w - pad - Scale(96), h - Scale(94), Scale(96), Scale(34));
    move(hint_, pad, h - Scale(42), w - pad * 2, Scale(36));
}

void ControlPanel::Refresh(const std::wstring& error) {
    if (!window_) return;
    size_t count = 0;
    for (size_t i = 0; i < visuals_.buttons.size(); ++i) {
        bool enabled = controls_.visuals[i]; count += enabled;
        SendMessageW(visuals_.buttons[i], BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    for (size_t i = 0; i < audio_.buttons.size(); ++i)
        SendMessageW(audio_.buttons[i], BM_SETCHECK, controls_.audio == int(i) ? BST_CHECKED : BST_UNCHECKED, 0);
    std::wstring text = std::to_wstring(count) + L" visual" + (count == 1 ? L" active. " : L"s active. ");
    text += controls_.audio < 0 ? L"Audio off." : L"Looping: " + Wide(SoundPayloads::Registry()[controls_.audio].name);
    if (!error.empty()) text += L"\r\n" + error;
    if (text != statusText_) { statusText_ = text; SetWindowTextW(status_, text.c_str()); }
}

void ControlPanel::SetPreviewActive(bool active) {
    if (!window_ || active == previewActive_) return;
    // Exclude the panel only while capturing visual source imagery. Idle/audio-only
    // controls remain ordinary capturable windows for screenshots and screen readers.
    SetWindowDisplayAffinity(window_, active ? 0x00000011 : 0);
    previewActive_ = active;
}

void ControlPanel::Command(int id) {
    if (id >= VisualBase && id < VisualBase + int(controls_.visuals.size())) {
        size_t index = static_cast<size_t>(id - VisualBase);
        controls_.visuals[index] = !controls_.visuals[index];
    } else if (id >= AudioBase && id < AudioBase + int(SoundPayloads::Registry().size())) {
        int index = id - AudioBase;
        controls_.audio = controls_.audio == index ? -1 : index;
        ++controls_.audioRevision;
    } else if (id == StopAll) {
        std::fill(controls_.visuals.begin(), controls_.visuals.end(), false);
        controls_.audio = -1; ++controls_.audioRevision;
    } else if (id == Close) controls_.closing = true;
    Refresh();
}

LRESULT CALLBACK ControlPanel::WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* panel = reinterpret_cast<ControlPanel*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        panel = static_cast<ControlPanel*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
    }
    if (panel) {
        if (msg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED) { panel->Command(LOWORD(wParam)); return 0; }
        if (msg == WM_SIZE) { panel->Layout(); return 0; }
        if (msg == WM_CLOSE || msg == WM_DISPLAYCHANGE || (msg == WM_ENDSESSION && wParam)) { panel->controls_.closing = true; return 0; }
        if (msg == WM_GETMINMAXINFO) {
            auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
            limits->ptMinTrackSize = {panel->Scale(660), panel->Scale(480)};
            return 0;
        }
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}

LRESULT CALLBACK ControlPanel::PaneProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* pane = reinterpret_cast<Pane*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        pane = static_cast<Pane*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        pane->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pane));
    }
    if (pane) {
        if (msg == WM_SIZE) { pane->Reflow(); return 0; }
        if (msg == WM_MOUSEWHEEL) {
            pane->wheel += static_cast<short>(HIWORD(wParam));
            int lines = pane->wheel / WHEEL_DELTA; pane->wheel %= WHEEL_DELTA;
            pane->Scroll(pane->offset - lines * pane->row * 3); return 0;
        }
        if (msg == WM_VSCROLL) {
            SCROLLINFO info{sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0}; GetScrollInfo(window, SB_VERT, &info);
            int position = pane->offset;
            switch (LOWORD(wParam)) {
            case SB_LINEUP: position -= pane->row; break;
            case SB_LINEDOWN: position += pane->row; break;
            case SB_PAGEUP: position -= int(info.nPage); break;
            case SB_PAGEDOWN: position += int(info.nPage); break;
            case SB_THUMBTRACK: position = info.nTrackPos; break;
            case SB_TOP: position = 0; break;
            case SB_BOTTOM: position = info.nMax; break;
            default: break;
            }
            pane->Scroll(position); return 0;
        }
        if (msg == WM_COMMAND) {
            if (HIWORD(wParam) == BN_SETFOCUS) {
                RECT child{}, client{}; GetWindowRect(reinterpret_cast<HWND>(lParam), &child);
                MapWindowPoints(nullptr, window, reinterpret_cast<POINT*>(&child), 2); GetClientRect(window, &client);
                if (child.top < 0) pane->Scroll(pane->offset + child.top - 6);
                else if (child.bottom > client.bottom) pane->Scroll(pane->offset + child.bottom - client.bottom + 6);
            }
            return SendMessageW(GetParent(window), msg, wParam, lParam);
        }
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}
} // namespace Runtime
