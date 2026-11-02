#pragma once
#include "../payloads/visual/visual.h"
#include "blue_screen.h"
#include <string>

namespace Runtime {
struct Controls {
    explicit Controls(size_t count) : visuals(count, false) {}
    std::vector<bool> visuals;
    int audio = -1;
    uint64_t audioRevision = 0;
    bool closing = false;
    bool finaleRequested = false;
    BlueScreenStyle finaleStyle = BlueScreenStyle::Automatic;
};

// Native Win32 controls retain keyboard navigation, focus and accessibility.
class ControlPanel {
public:
    ControlPanel(HINSTANCE instance, Controls& controls) : instance_(instance), controls_(controls) {}
    ~ControlPanel();
    ControlPanel(const ControlPanel&) = delete;
    ControlPanel& operator=(const ControlPanel&) = delete;
    void Create();
    void Refresh(const std::wstring& error = {});
    void SetPreviewActive(bool active);
    bool HandleMessage(const MSG& message);
    HWND Window() const { return window_; }
private:
    struct Pane {
        ControlPanel* owner = nullptr;
        HWND window = nullptr;
        HWND empty = nullptr;
        std::vector<HWND> buttons;
        std::vector<std::wstring> names;
        std::vector<size_t> order, visible;
        int offset = 0, row = 44, inset = 8, wheel = 0;
        void Filter(const std::wstring& query);
        void Reflow();
        void Scroll(int position);
    };
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PaneProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK ActionProc(HWND, UINT, WPARAM, LPARAM);
    HWND Add(HWND parent, const wchar_t* kind, const std::wstring& text, DWORD style, int id = 0);
    int Scale(int value) const { return MulDiv(value, dpi_, 96); }
    void Layout();
    void Filter();
    void DrawButton(const DRAWITEMSTRUCT& item);
    void Command(int id);
    HINSTANCE instance_;
    Controls& controls_;
    HWND window_ = nullptr, title_ = nullptr, description_ = nullptr;
    HWND visualsTitle_ = nullptr, audioTitle_ = nullptr, stop_ = nullptr, close_ = nullptr, status_ = nullptr, hint_ = nullptr;
    HWND finale_ = nullptr;
    HWND finaleLabel_ = nullptr, finaleStyle_ = nullptr;
    HWND searchLabel_ = nullptr, search_ = nullptr, clear_ = nullptr;
    HWND visualTab_ = nullptr, audioTab_ = nullptr, visualsHelp_ = nullptr, audioHelp_ = nullptr;
    HWND hovered_ = nullptr;
    Pane visuals_, audio_;
    HFONT font_ = nullptr, titleFont_ = nullptr, sectionFont_ = nullptr, smallFont_ = nullptr;
    HBRUSH groundBrush_ = nullptr, surfaceBrush_ = nullptr, selectedBrush_ = nullptr;
    COLORREF ground_ = RGB(246, 247, 248), surface_ = RGB(255, 255, 255);
    COLORREF ink_ = RGB(24, 26, 28), muted_ = RGB(85, 89, 94), selected_ = RGB(235, 238, 240);
    WNDPROC buttonProc_ = nullptr;
    int dpi_ = 96;
    bool registered_ = false, paneRegistered_ = false;
    bool previewActive_ = false;
    bool compact_ = false, audioView_ = false, highContrast_ = false;
    std::wstring statusText_, errorText_;
};
} // namespace Runtime
