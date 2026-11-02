#pragma once
#include "../payloads/visual/visual.h"
#include <string>

namespace Runtime {
struct Controls {
    explicit Controls(size_t count) : visuals(count, false) {}
    std::vector<bool> visuals;
    int audio = -1;
    uint64_t audioRevision = 0;
    bool closing = false;
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
    HWND Window() const { return window_; }
private:
    struct Pane {
        HWND window = nullptr;
        std::vector<HWND> buttons;
        int offset = 0, row = 38, wheel = 0;
        void Reflow();
        void Scroll(int position);
    };
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PaneProc(HWND, UINT, WPARAM, LPARAM);
    HWND Add(HWND parent, const wchar_t* kind, const std::wstring& text, DWORD style, int id = 0);
    int Scale(int value) const { return MulDiv(value, dpi_, 96); }
    void Layout();
    void Command(int id);
    HINSTANCE instance_;
    Controls& controls_;
    HWND window_ = nullptr, title_ = nullptr, description_ = nullptr;
    HWND visualsTitle_ = nullptr, audioTitle_ = nullptr, stop_ = nullptr, close_ = nullptr, status_ = nullptr, hint_ = nullptr;
    Pane visuals_, audio_;
    HFONT font_ = nullptr, titleFont_ = nullptr;
    int dpi_ = 96;
    bool registered_ = false, paneRegistered_ = false;
    bool previewActive_ = false;
    std::wstring statusText_;
};
} // namespace Runtime
