#include "menu.h"
#include "../payloads/sound/sound.h"
#include <commctrl.h>
#include <cwctype>
#include <numeric>
#include <stdexcept>

namespace Runtime {
namespace {
constexpr wchar_t MenuClass[] = L"JetmyPayloadControlPanel";
constexpr wchar_t PaneClass[] = L"JetmyPayloadControlPane";
constexpr int VisualBase = 1000, AudioBase = 2000, StopAll = 10, Close = 11, Finale = 12, FinaleStyle = 13;
constexpr int VisualTab = 16, AudioTab = 17;
std::wstring Wide(const char* text) { return std::wstring(text, text + strlen(text)); } // Registry names are ASCII.
std::wstring Lower(std::wstring text) {
    for (auto& ch : text) ch = static_cast<wchar_t>(std::towlower(ch));
    return text;
}
void Text(HWND window, const wchar_t* text) { SetWindowTextW(window, text); }
void Text(HWND window, const std::wstring& text) { Text(window, text.c_str()); }
}

ControlPanel::~ControlPanel() {
    if (window_) DestroyWindow(window_);
    for (auto font : {font_, titleFont_, sectionFont_, smallFont_}) if (font) DeleteObject(font);
    for (auto brush : {groundBrush_, surfaceBrush_, selectedBrush_}) if (brush) DeleteObject(brush);
    if (registered_) UnregisterClassW(MenuClass, instance_);
    if (paneRegistered_) UnregisterClassW(PaneClass, instance_);
}

HWND ControlPanel::Add(HWND parent, const wchar_t* kind, const std::wstring& text, DWORD style, int id) {
    HWND child = CreateWindowExW(0, kind, text.c_str(), WS_CHILD | WS_VISIBLE | style,
        0, 0, 1, 1, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    if (!child) throw std::runtime_error("Cannot create menu control.");
    SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    if (wcscmp(kind, L"BUTTON") == 0 && (style & BS_TYPEMASK) == BS_OWNERDRAW) {
        SetWindowLongPtrW(child, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        auto original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(child, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ActionProc)));
        if (!buttonProc_) buttonProc_ = original;
    }
    return child;
}

void ControlPanel::Create() {
    HDC screen = GetDC(nullptr);
    if (screen) { dpi_ = GetDeviceCaps(screen, LOGPIXELSY); ReleaseDC(nullptr, screen); }
    HIGHCONTRASTW contrast{sizeof(HIGHCONTRASTW), 0, nullptr};
    highContrast_ = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) && (contrast.dwFlags & HCF_HIGHCONTRASTON);
    if (highContrast_) {
        ground_ = GetSysColor(COLOR_BTNFACE); surface_ = GetSysColor(COLOR_WINDOW);
        ink_ = GetSysColor(COLOR_WINDOWTEXT); muted_ = ink_; selected_ = GetSysColor(COLOR_HIGHLIGHT);
    }
    groundBrush_ = CreateSolidBrush(ground_); surfaceBrush_ = CreateSolidBrush(surface_);
    selectedBrush_ = CreateSolidBrush(selected_);
    auto font = [&](int size, int weight) {
        return CreateFontW(-Scale(size), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    };
    font_ = font(14, FW_NORMAL); titleFont_ = font(24, FW_SEMIBOLD);
    sectionFont_ = font(16, FW_SEMIBOLD); smallFont_ = font(13, FW_NORMAL);
    if (!font_ || !titleFont_ || !sectionFont_ || !smallFont_ || !groundBrush_ || !surfaceBrush_ || !selectedBrush_)
        throw std::runtime_error("Cannot create menu styling resources.");
    WNDCLASSW wc{};
    wc.hInstance = instance_; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpfnWndProc = WindowProc; wc.lpszClassName = MenuClass;
    if (!RegisterClassW(&wc)) throw std::runtime_error("Cannot register menu window.");
    registered_ = true;
    wc.lpfnWndProc = PaneProc; wc.lpszClassName = PaneClass;
    if (!RegisterClassW(&wc)) throw std::runtime_error("Cannot register menu lists.");
    paneRegistered_ = true;
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int width = std::min(Scale(980), int(work.right - work.left));
    int height = std::min(Scale(820), int(work.bottom - work.top));
    window_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_CONTROLPARENT, MenuClass, L"JetmyV2 - Payload controls",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, work.left + (work.right - work.left - width) / 2,
        work.top + (work.bottom - work.top - height) / 2, width, height,
        nullptr, nullptr, instance_, this);
    if (!window_) throw std::runtime_error("Cannot create payload menu.");
    title_ = Add(window_, L"STATIC", L"JetmyV2 controls", SS_LEFT);
    SendMessageW(title_, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont_), FALSE);
    description_ = Add(window_, L"STATIC", L"Combine visuals. Choose one audio loop. Click again to turn off.", SS_LEFT);
    stop_ = Add(window_, L"BUTTON", L"Stop all", BS_OWNERDRAW | WS_TABSTOP, StopAll);
    visualsTitle_ = Add(window_, L"STATIC", L"Visual effects", SS_LEFT);
    audioTitle_ = Add(window_, L"STATIC", L"Audio loops", SS_LEFT);
    for (auto heading : {visualsTitle_, audioTitle_})
        SendMessageW(heading, WM_SETFONT, reinterpret_cast<WPARAM>(sectionFont_), FALSE);
    visualTab_ = Add(window_, L"BUTTON", L"Visual effects", BS_OWNERDRAW | WS_TABSTOP, VisualTab);
    audioTab_ = Add(window_, L"BUTTON", L"Audio loops", BS_OWNERDRAW | WS_TABSTOP, AudioTab);
    visualsHelp_ = Add(window_, L"STATIC", L"A-Z  /  Enable any combination", SS_LEFT);
    audioHelp_ = Add(window_, L"STATIC", L"A-Z  /  One loop at a time", SS_LEFT);
    close_ = Add(window_, L"BUTTON", L"Close", BS_OWNERDRAW | WS_TABSTOP, Close);
    finale_ = Add(window_, L"BUTTON", L"Blue screen (10s)", BS_OWNERDRAW | WS_TABSTOP, Finale);
    finaleLabel_ = Add(window_, L"STATIC", L"Blue-screen style", SS_LEFT);
    finaleStyle_ = Add(window_, L"COMBOBOX", std::wstring{}, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, FinaleStyle);
    for (const auto* style : {L"Automatic (this PC)", L"Windows XP", L"Windows Vista", L"Windows 7", L"Windows 10", L"Windows 11 (blue)"})
        SendMessageW(finaleStyle_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(style));
    SendMessageW(finaleStyle_, CB_SETCURSEL, 0, 0);
    SendMessageW(finaleStyle_, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), Scale(24));
    status_ = Add(window_, L"STATIC", std::wstring{}, SS_LEFT);
    hint_ = Add(window_, L"STATIC", L"Flashing visuals / audio. Exit: Ctrl+Shift+Q or Ctrl+Shift+Esc.", SS_LEFT);
    for (auto label : {visualsHelp_, audioHelp_, finaleLabel_, hint_})
        SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont_), FALSE);
    auto createPane = [&](Pane& pane, const wchar_t* name, int base) {
        pane.owner = this; pane.row = Scale(44); pane.inset = Scale(8);
        pane.window = CreateWindowExW(WS_EX_CONTROLPARENT, PaneClass, name,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
            0, 0, 1, 1, window_, nullptr, instance_, &pane);
        if (!pane.window) throw std::runtime_error("Cannot create scrollable payload list.");
        pane.order.resize(pane.names.size()); std::iota(pane.order.begin(), pane.order.end(), size_t(0));
        std::stable_sort(pane.order.begin(), pane.order.end(), [&](size_t a, size_t b) { return Lower(pane.names[a]) < Lower(pane.names[b]); });
        pane.buttons.resize(pane.names.size());
        // Create in display order for alphabetical Tab navigation, but retain
        // registry-indexed storage and command IDs for all playback state.
        for (size_t index : pane.order)
            pane.buttons[index] = Add(pane.window, L"BUTTON", pane.names[index],
                BS_AUTOCHECKBOX | BS_NOTIFY | BS_LEFT | BS_VCENTER | BS_MULTILINE | WS_TABSTOP, base + int(index));
    };
    for (const auto& visual : VisualPayloads::Registry()) visuals_.names.push_back(Wide(visual.name));
    for (const auto& audio : SoundPayloads::Registry()) audio_.names.push_back(Wide(audio.name));
    createPane(visuals_, L"Visual effects", VisualBase); createPane(audio_, L"Audio loops", AudioBase);
    for (auto child : {visualTab_, audioTab_, visuals_.window, audio_.window,
            finaleStyle_, finale_, stop_, close_})
        SetWindowPos(child, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    Refresh(); Layout();
    ShowWindow(window_, SW_SHOW); UpdateWindow(window_);
    SetFocus(compact_ ? visualTab_ : (visuals_.order.empty() ? stop_ : visuals_.buttons[visuals_.order.front()]));
}

void ControlPanel::Pane::Reflow() {
    if (buttons.size() != order.size() || (!order.empty() && !buttons[order.back()])) return;
    RECT rect{}; GetClientRect(window, &rect);
    int total = int(order.size()) * row + inset * 2;
    offset = std::clamp(offset, 0, std::max(0, total - int(rect.bottom)));
    SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS,
        0, std::max(0, total - 1), static_cast<UINT>(rect.bottom), offset, 0};
    SetScrollInfo(window, SB_VERT, &scroll, TRUE);
    GetClientRect(window, &rect);
    HDWP positions = BeginDeferWindowPos(int(order.size()));
    auto move = [&](HWND child, int x, int y, int width, int height) {
        if (positions) positions = DeferWindowPos(positions, child, nullptr, x, y, std::max(1, width), height, SWP_NOZORDER | SWP_NOACTIVATE);
        else MoveWindow(child, x, y, std::max(1, width), height, TRUE);
    };
    for (size_t display = 0; display < order.size(); ++display)
        move(buttons[order[display]], inset, inset + int(display) * row - offset,
            rect.right - inset * 2, row - owner->Scale(4));
    if (positions) EndDeferWindowPos(positions);
    InvalidateRect(window, nullptr, TRUE);
}
void ControlPanel::Pane::Scroll(int position) {
    RECT rect{}; GetClientRect(window, &rect);
    position = std::clamp(position, 0, std::max(0, int(order.size()) * row + inset * 2 - int(rect.bottom)));
    if (position != offset) { offset = position; Reflow(); }
}

void ControlPanel::Layout() {
    if (!audio_.window) return; // Initial WM_SIZE precedes child creation.
    RECT rect{}; GetClientRect(window_, &rect);
    int w = rect.right, h = rect.bottom;
    compact_ = w < Scale(720);
    int pad = Scale(compact_ ? 16 : 24), gap = Scale(16), available = w - pad * 2;
    int column = std::max(1, (available - gap) / 2), right = pad + column + gap;
    auto move = [](HWND child, int x, int y, int width, int height) { MoveWindow(child, x, y, std::max(1, width), std::max(1, height), TRUE); };
    auto show = [](HWND child, bool visible) { ShowWindow(child, visible ? SW_SHOW : SW_HIDE); };
    // Both columns close on the same outer edges: 2*column + gap = available.
    move(title_, pad, Scale(18), available - Scale(128), Scale(34));
    move(stop_, w - pad - Scale(112), Scale(18), Scale(112), Scale(36));
    move(description_, pad, Scale(60), available, Scale(34));
    show(visualsTitle_, !compact_); show(audioTitle_, !compact_);
    show(visualTab_, compact_); show(audioTab_, compact_);
    show(visualsHelp_, !compact_ || !audioView_); show(audioHelp_, !compact_ || audioView_);
    show(visuals_.window, !compact_ || !audioView_); show(audio_.window, !compact_ || audioView_);
    int statusHeight = Scale(errorText_.empty() ? 44 : (compact_ ? 76 : 60));
    int statusY = h - Scale(54) - statusHeight;
    int actionsY = statusY - Scale(50);
    int top = Scale(compact_ ? 160 : 146);
    int bottom = actionsY - Scale(compact_ ? 46 : 18);
    if (compact_) {
        move(visualTab_, pad, Scale(90), (available - Scale(8)) / 2, Scale(34));
        move(audioTab_, pad + (available + Scale(8)) / 2, Scale(90), (available - Scale(8)) / 2, Scale(34));
        move(visualsHelp_, pad, Scale(132), available, Scale(20));
        move(audioHelp_, pad, Scale(132), available, Scale(20));
        move(visuals_.window, pad, top, available, bottom - top);
        move(audio_.window, pad, top, available, bottom - top);
        move(finaleLabel_, pad, actionsY - Scale(26), available, Scale(20));
        move(finaleStyle_, pad, actionsY, available - Scale(172), Scale(220));
        move(finale_, w - pad - Scale(160), actionsY, Scale(160), Scale(34));
    } else {
        move(visualsTitle_, pad, Scale(90), column, Scale(24));
        move(audioTitle_, right, Scale(90), w - pad - right, Scale(24));
        move(visualsHelp_, pad, Scale(118), column, Scale(20));
        move(audioHelp_, right, Scale(118), w - pad - right, Scale(20));
        move(visuals_.window, pad, top, column, bottom - top);
        move(audio_.window, right, top, w - pad - right, bottom - top);
        move(finaleLabel_, pad, actionsY + Scale(8), Scale(128), Scale(20));
        move(finaleStyle_, pad + Scale(136), actionsY, Scale(196), Scale(220));
        move(finale_, w - pad - Scale(160), actionsY, Scale(160), Scale(34));
    }
    move(status_, pad, statusY, available, statusHeight);
    move(hint_, pad, h - Scale(46), available - Scale(100), Scale(40));
    move(close_, w - pad - Scale(88), h - Scale(46), Scale(88), Scale(34));
    // Never leave keyboard focus inside a pane hidden by the compact layout.
    HWND focus = GetFocus();
    if (compact_ && IsChild(audioView_ ? visuals_.window : audio_.window, focus))
        SetFocus(audioView_ ? audioTab_ : visualTab_);
    else if (!compact_ && (focus == visualTab_ || focus == audioTab_)) {
        Pane& pane = focus == audioTab_ ? audio_ : visuals_;
        if (!pane.order.empty()) SetFocus(pane.buttons[pane.order.front()]);
    }
    InvalidateRect(window_, nullptr, TRUE);
}

void ControlPanel::Refresh(const std::wstring& error) {
    if (!window_) return;
    bool errorChanged = errorText_ != error; errorText_ = error;
    size_t count = 0;
    for (size_t i = 0; i < visuals_.buttons.size(); ++i) {
        bool enabled = controls_.visuals[i]; count += enabled;
        HWND button = visuals_.buttons[i];
        LRESULT check = enabled ? BST_CHECKED : BST_UNCHECKED;
        if (SendMessageW(button, BM_GETCHECK, 0, 0) != check) { SendMessageW(button, BM_SETCHECK, check, 0); InvalidateRect(button, nullptr, TRUE); }
    }
    for (size_t i = 0; i < audio_.buttons.size(); ++i) {
        HWND button = audio_.buttons[i];
        LRESULT check = controls_.audio == int(i) ? BST_CHECKED : BST_UNCHECKED;
        if (SendMessageW(button, BM_GETCHECK, 0, 0) != check) { SendMessageW(button, BM_SETCHECK, check, 0); InvalidateRect(button, nullptr, TRUE); }
    }
    auto heading = [](const Pane& pane, const std::wstring& name) {
        return name + L" (" + std::to_wstring(pane.names.size()) + L")";
    };
    Text(visualsTitle_, heading(visuals_, L"Visual effects")); Text(audioTitle_, heading(audio_, L"Audio loops"));
    Text(visualTab_, heading(visuals_, L"Visuals")); Text(audioTab_, heading(audio_, L"Audio"));
    bool playing = count > 0 || controls_.audio >= 0;
    EnableWindow(stop_, playing);
    std::wstring text = std::to_wstring(count) + (count == 1 ? L" visual active" : L" visuals active");
    text += controls_.audio < 0 ? L"  /  Audio off" : L"  /  Audio: " + Wide(SoundPayloads::Registry()[controls_.audio].name);
    if (!error.empty()) text += L"\r\n" + error;
    if (text != statusText_) { statusText_ = text; Text(status_, text); }
    if (errorChanged) Layout();
}

void ControlPanel::SetPreviewActive(bool active) {
    if (!window_ || active == previewActive_) return;
    // Idle/audio-only controls remain capturable for screenshots and accessibility.
    SetWindowDisplayAffinity(window_, active ? 0x00000011 : 0);
    previewActive_ = active;
}

bool ControlPanel::HandleMessage(const MSG& message) {
    (void)message;
    return false;
}

void ControlPanel::Command(int id) {
    if (id >= VisualBase && id < VisualBase + int(controls_.visuals.size())) {
        size_t index = static_cast<size_t>(id - VisualBase);
        controls_.visuals[index] = !controls_.visuals[index];
    } else if (id >= AudioBase && id < AudioBase + int(SoundPayloads::Registry().size())) {
        int index = id - AudioBase;
        controls_.audio = controls_.audio == index ? -1 : index; ++controls_.audioRevision;
    } else if (id == StopAll) {
        std::fill(controls_.visuals.begin(), controls_.visuals.end(), false);
        controls_.audio = -1; ++controls_.audioRevision;
    } else if (id == VisualTab || id == AudioTab) {
        audioView_ = id == AudioTab; Layout();
        InvalidateRect(visualTab_, nullptr, TRUE); InvalidateRect(audioTab_, nullptr, TRUE);
        return;
    } else if (id == Finale) controls_.finaleRequested = true;
    else if (id == Close) controls_.closing = true;
    Refresh();
}

void ControlPanel::DrawButton(const DRAWITEMSTRUCT& item) {
    HDC dc = item.hDC; int saved = SaveDC(dc);
    bool disabled = (item.itemState & ODS_DISABLED) != 0, pressed = (item.itemState & ODS_SELECTED) != 0;
    bool tab = item.CtlID == VisualTab || item.CtlID == AudioTab;
    bool selected = tab && (audioView_ ? item.CtlID == AudioTab : item.CtlID == VisualTab);
    bool primary = item.CtlID == StopAll && !disabled;
    COLORREF fill = primary ? ink_ : selected ? selected_ : surface_;
    COLORREF text = primary ? surface_ : disabled ? GetSysColor(COLOR_GRAYTEXT) : ink_;
    COLORREF border = selected || primary ? ink_ : RGB(193, 197, 201);
    if (pressed || (hovered_ == item.hwndItem && !disabled)) fill = primary ? RGB(56, 59, 62) : selected_;
    if (highContrast_) {
        fill = selected || pressed ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_BTNFACE);
        text = disabled ? GetSysColor(COLOR_GRAYTEXT) : selected || pressed ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_BTNTEXT);
        border = GetSysColor(COLOR_BTNTEXT);
    }
    FillRect(dc, &item.rcItem, groundBrush_);
    SelectObject(dc, GetStockObject(DC_BRUSH)); SetDCBrushColor(dc, fill);
    SelectObject(dc, GetStockObject(DC_PEN)); SetDCPenColor(dc, border);
    RoundRect(dc, item.rcItem.left, item.rcItem.top, item.rcItem.right, item.rcItem.bottom, Scale(8), Scale(8));
    wchar_t label[128]{}; GetWindowTextW(item.hwndItem, label, 128);
    SelectObject(dc, font_); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, text);
    RECT rect = item.rcItem; InflateRect(&rect, -Scale(8), 0);
    DrawTextW(dc, label, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_HIDEPREFIX);
    if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) {
        rect = item.rcItem; InflateRect(&rect, -Scale(4), -Scale(4)); DrawFocusRect(dc, &rect);
    }
    RestoreDC(dc, saved);
}

LRESULT CALLBACK ControlPanel::ActionProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* panel = reinterpret_cast<ControlPanel*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_MOUSEMOVE && panel->hovered_ != window) {
        panel->hovered_ = window;
        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0}; TrackMouseEvent(&track);
        InvalidateRect(window, nullptr, FALSE);
    } else if (msg == WM_MOUSELEAVE) {
        if (panel->hovered_ == window) panel->hovered_ = nullptr;
        InvalidateRect(window, nullptr, FALSE);
    }
    return CallWindowProcW(panel->buttonProc_, window, msg, wParam, lParam);
}

LRESULT CALLBACK ControlPanel::WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* panel = reinterpret_cast<ControlPanel*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        panel = static_cast<ControlPanel*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
    }
    if (panel) {
        if (msg == WM_ERASEBKGND) {
            RECT rect{}; GetClientRect(window, &rect);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, panel->groundBrush_); return 1;
        }
        if (msg == WM_COMMAND && LOWORD(wParam) == FinaleStyle && HIWORD(wParam) == CBN_SELCHANGE) {
            LRESULT selection = SendMessageW(panel->finaleStyle_, CB_GETCURSEL, 0, 0);
            if (selection >= 0 && selection <= 5) panel->controls_.finaleStyle = static_cast<BlueScreenStyle>(selection);
            return 0;
        }
        if (msg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED) { panel->Command(LOWORD(wParam)); return 0; }
        if (msg == WM_DRAWITEM) { panel->DrawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam)); return TRUE; }
        if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX) {
            HDC dc = reinterpret_cast<HDC>(wParam); HWND child = reinterpret_cast<HWND>(lParam);
            bool field = msg != WM_CTLCOLORSTATIC;
            bool muted = child == panel->description_ || child == panel->hint_ || child == panel->visualsHelp_ || child == panel->audioHelp_ || child == panel->finaleLabel_;
            SetTextColor(dc, muted ? panel->muted_ : panel->ink_);
            SetBkColor(dc, field ? panel->surface_ : panel->ground_);
            return reinterpret_cast<LRESULT>(field ? panel->surfaceBrush_ : panel->groundBrush_);
        }
        if (msg == WM_SIZE) { panel->Layout(); return 0; }
        if (msg == WM_MOUSEWHEEL) {
            POINT point{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            for (auto* pane : {&panel->visuals_, &panel->audio_}) {
                RECT rect{}; GetWindowRect(pane->window, &rect);
                if (IsWindowVisible(pane->window) && PtInRect(&rect, point)) return SendMessageW(pane->window, msg, wParam, lParam);
            }
        }
        if (msg == WM_CLOSE || msg == WM_DISPLAYCHANGE || (msg == WM_ENDSESSION && wParam)) { panel->controls_.closing = true; return 0; }
        if (msg == WM_GETMINMAXINFO) {
            auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
            RECT minimum{0, 0, panel->Scale(400), panel->Scale(540)}, work{};
            AdjustWindowRectEx(&minimum, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_TOPMOST | WS_EX_CONTROLPARENT);
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
            limits->ptMinTrackSize = {std::min(LONG(minimum.right - minimum.left), work.right - work.left),
                std::min(LONG(minimum.bottom - minimum.top), work.bottom - work.top)};
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
        if (msg == WM_ERASEBKGND) {
            RECT rect{}; GetClientRect(window, &rect);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, pane->owner->surfaceBrush_); return 1;
        }
        if (msg == WM_SIZE) { pane->Reflow(); return 0; }
        if (msg == WM_CTLCOLORBTN || msg == WM_CTLCOLORSTATIC) {
            // Windows can send WM_CTLCOLORSTATIC for checkbox backgrounds too.
            bool selected = SendMessageW(reinterpret_cast<HWND>(lParam), BM_GETCHECK, 0, 0) == BST_CHECKED;
            auto* panel = pane->owner; HDC dc = reinterpret_cast<HDC>(wParam);
            SetTextColor(dc, selected && panel->highContrast_ ? GetSysColor(COLOR_HIGHLIGHTTEXT) : panel->ink_);
            SetBkColor(dc, selected ? panel->selected_ : panel->surface_);
            return reinterpret_cast<LRESULT>(selected ? panel->selectedBrush_ : panel->surfaceBrush_);
        }
        if (msg == WM_MOUSEWHEEL) {
            UINT lines = 3; SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
            RECT rect{}; GetClientRect(window, &rect);
            int distance = lines == WHEEL_PAGESCROLL ? int(rect.bottom) : int(std::min(lines, UINT(100))) * pane->row;
            pane->wheel += static_cast<short>(HIWORD(wParam)) * distance;
            int pixels = pane->wheel / WHEEL_DELTA; pane->wheel %= WHEEL_DELTA;
            pane->Scroll(pane->offset - pixels); return 0;
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
            case SB_THUMBPOSITION: position = info.nTrackPos; break;
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
                if (child.top < 0) pane->Scroll(pane->offset + child.top - pane->inset);
                else if (child.bottom > client.bottom) pane->Scroll(pane->offset + child.bottom - client.bottom + pane->inset);
            }
            return SendMessageW(GetParent(window), msg, wParam, lParam);
        }
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}
} // namespace Runtime
