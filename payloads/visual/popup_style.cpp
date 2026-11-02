#include "popup_style.h"
#include <uxtheme.h>
#include <vssym32.h>

namespace VisualPayloads {
namespace {
enum class Era { Classic, XP, Aero, Modern, Rounded };
template<class Function> Function Resolve(HMODULE module, const char* name) {
    FARPROC address = module ? GetProcAddress(module, name) : nullptr;
    Function function = nullptr;
    static_assert(sizeof(function) == sizeof(address), "Windows function pointer size mismatch");
    std::memcpy(&function, &address, sizeof(function));
    return function;
}
class Style {
public:
    using Open = HTHEME (WINAPI*)(HWND, LPCWSTR);
    using Close = HRESULT (WINAPI*)(HTHEME);
    using Draw = HRESULT (WINAPI*)(HTHEME, HDC, int, int, const RECT*, const RECT*);
    Era era = Era::Modern;
    int dpi = 96;
    HFONT body = nullptr, caption = nullptr;
    HMODULE library = nullptr;
    HTHEME window = nullptr, button = nullptr;
    Close close = nullptr;
    Draw draw = nullptr;
    Style() {
        OSVERSIONINFOW version{}; version.dwOSVersionInfoSize = sizeof(version);
        using Version = LONG (WINAPI*)(OSVERSIONINFOW*);
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        auto getVersion = Resolve<Version>(ntdll, "RtlGetVersion");
        if (getVersion && getVersion(&version) == 0) {
            if (version.dwMajorVersion < 5) era = Era::Classic;
            else if (version.dwMajorVersion == 5) era = Era::XP;
            else if (version.dwMajorVersion == 6 && version.dwMinorVersion <= 1) era = Era::Aero;
            else if (version.dwMajorVersion >= 10 && version.dwBuildNumber >= 22000) era = Era::Rounded;
        }
        HDC screen = GetDC(nullptr);
        if (screen) { dpi = GetDeviceCaps(screen, LOGPIXELSY); ReleaseDC(nullptr, screen); }
        NONCLIENTMETRICSW metrics{}; metrics.cbSize = sizeof(metrics);
        BOOL obtained = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
        if (!obtained) { // XP's structure predates iPaddedBorderWidth.
            metrics.cbSize = sizeof(metrics) - sizeof(int);
            obtained = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
        }
        if (obtained) {
            body = CreateFontIndirectW(&metrics.lfMessageFont);
            caption = CreateFontIndirectW(&metrics.lfCaptionFont);
        }
        library = LoadLibraryW(L"uxtheme.dll");
        if (library) {
            auto open = Resolve<Open>(library, "OpenThemeData");
            close = Resolve<Close>(library, "CloseThemeData");
            draw = Resolve<Draw>(library, "DrawThemeBackground");
            if (open && close && draw) {
                window = open(GetDesktopWindow(), L"WINDOW");
                button = open(GetDesktopWindow(), L"BUTTON");
            }
        }
    }
    ~Style() {
        if (window && close) close(window);
        if (button && close) close(button);
        if (library) FreeLibrary(library);
        if (body) DeleteObject(body);
        if (caption) DeleteObject(caption);
    }
    int Px(int value) const { return MulDiv(value, dpi, 96); }
    bool Themed(HTHEME theme, int part, int state, const RECT& rect) const {
        return theme && draw && SUCCEEDED(draw(theme, g_hMemDC, part, state, &rect, nullptr));
    }
};
Style& CurrentStyle() { static Style style; return style; }
void Solid(const RECT& rect, COLORREF color) {
    SetDCBrushColor(g_hMemDC, color);
    FillRect(g_hMemDC, &rect, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}
void Text(const wchar_t* text, RECT rect, UINT flags) {
    DrawTextW(g_hMemDC, text, -1, &rect, flags | DT_NOPREFIX);
}
}

SIZE PopupDimensions(const wchar_t* message) {
    auto& style = CurrentStyle();
    bool old = style.era == Era::Classic || style.era == Era::XP;
    int frame = style.Px(old ? 4 : style.era == Era::Aero ? 7 : 1);
    int captionHeight = style.Px(old ? 28 : style.era == Era::Aero ? 30 : 34);
    int w = std::min(style.Px(420), g_ScreenWidth);
    int saved = SaveDC(g_hMemDC);
    int textHeight = style.Px(48);
    if (saved) {
        SelectObject(g_hMemDC, style.body ? style.body : GetStockObject(DEFAULT_GUI_FONT));
        RECT text{0, 0, std::max(1, w - 2 * frame - style.Px(92)), 0};
        DrawTextW(g_hMemDC, message, -1, &text, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        textHeight = std::max(textHeight, int(text.bottom));
        RestoreDC(g_hMemDC, saved);
    }
    return {w, std::min(g_ScreenHeight, textHeight + 2 * frame + captionHeight + style.Px(97))};
}

void DrawPopup(int x, int y, int width, int height, const wchar_t* title,
    const wchar_t* message, const wchar_t* buttonText, int kind) {
    auto& style = CurrentStyle();
    auto px = [&style](int value) { return style.Px(value); };
    bool old = style.era == Era::Classic || style.era == Era::XP;
    bool aero = style.era == Era::Aero;
    bool rounded = style.era == Era::Rounded;
    int frame = old ? px(4) : aero ? px(7) : px(1);
    int captionHeight = px(old ? 28 : aero ? 30 : 34);
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    // Soft stepped shadow; no real windows, focus changes, or blocking dialogs.
    GdiFlush();
    for (int inset = px(6); inset >= px(2); inset -= std::max(1, px(2)))
        Tint(x + inset, y + inset + px(2), width, height, 0x000000, 0.07f);
    RECT outer{x, y, x + width, y + height};
    if (rounded) {
        HRGN clip = CreateRoundRectRgn(x, y, x + width + 1, y + height + 1, px(16), px(16));
        if (clip) { ExtSelectClipRgn(g_hMemDC, clip, RGN_AND); DeleteObject(clip); }
    }
    Solid(outer, old ? GetSysColor(COLOR_3DFACE) : RGB(255, 255, 255));
    RECT captionRect{x + frame, y + frame, x + width - frame, y + frame + captionHeight};
    COLORREF titleColor = GetSysColor(COLOR_WINDOWTEXT);
    if (old || aero) {
        if (!style.Themed(style.window, WP_CAPTION, CS_ACTIVE, captionRect)) {
            // Classic/Aero fallback also works when visual styles are disabled.
            for (int row = 0; row < captionHeight; ++row) {
                float mix = float(row) / std::max(1, captionHeight - 1);
                uint32_t color = aero ? BlendColor(0xDBEFFB, 0x86B2D4, mix) : BlendColor(0x0756B9, 0x3188ED, mix);
                Solid({captionRect.left, captionRect.top + row, captionRect.right, captionRect.top + row + 1},
                    RGB((color >> 16) & 255, (color >> 8) & 255, color & 255));
            }
        }
        titleColor = aero ? RGB(15, 25, 35) : GetSysColor(COLOR_CAPTIONTEXT);
        RECT left{x, y + frame + captionHeight, x + frame, y + height - frame};
        RECT right{x + width - frame, left.top, x + width, left.bottom};
        RECT bottom{x, y + height - frame, x + width, y + height};
        if (!style.Themed(style.window, WP_FRAMELEFT, FS_ACTIVE, left)) Solid(left, GetSysColor(COLOR_ACTIVEBORDER));
        if (!style.Themed(style.window, WP_FRAMERIGHT, FS_ACTIVE, right)) Solid(right, GetSysColor(COLOR_ACTIVEBORDER));
        if (!style.Themed(style.window, WP_FRAMEBOTTOM, FS_ACTIVE, bottom)) Solid(bottom, GetSysColor(COLOR_ACTIVEBORDER));
    } else {
        SetDCPenColor(g_hMemDC, RGB(190, 190, 190));
        SelectObject(g_hMemDC, GetStockObject(DC_PEN));
        SelectObject(g_hMemDC, GetStockObject(NULL_BRUSH));
        if (rounded) RoundRect(g_hMemDC, x, y, x + width, y + height, px(16), px(16));
        else Rectangle(g_hMemDC, x, y, x + width, y + height);
    }
    RECT closeRect{x + width - frame - px(old ? 25 : 44), captionRect.top,
        x + width - frame, captionRect.bottom};
    if (!(old || aero) || !style.Themed(style.window, WP_CLOSEBUTTON, CBS_NORMAL, closeRect)) {
        int cx = (closeRect.left + closeRect.right) / 2, cy = (closeRect.top + closeRect.bottom) / 2;
        HPEN pen = CreatePen(PS_SOLID, std::max(1, px(1)), titleColor);
        HGDIOBJ previous = pen ? SelectObject(g_hMemDC, pen) : nullptr;
        int radius = px(5);
        MoveToEx(g_hMemDC, cx - radius, cy - radius, nullptr); LineTo(g_hMemDC, cx + radius, cy + radius);
        MoveToEx(g_hMemDC, cx + radius, cy - radius, nullptr); LineTo(g_hMemDC, cx - radius, cy + radius);
        if (previous) SelectObject(g_hMemDC, previous);
        if (pen) DeleteObject(pen);
    }
    SetBkMode(g_hMemDC, TRANSPARENT);
    SetTextColor(g_hMemDC, titleColor);
    SelectObject(g_hMemDC, style.caption ? style.caption : GetStockObject(DEFAULT_GUI_FONT));
    RECT titleRect{captionRect.left + px(10), captionRect.top, closeRect.left - px(8), captionRect.bottom};
    Text(title, titleRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    int footerTop = y + height - frame - px(58);
    RECT footer{x + frame, footerTop, x + width - frame, y + height - frame};
    if (!old) Solid(footer, rounded ? RGB(243, 243, 243) : RGB(240, 240, 240));
    if (!old) Solid({footer.left, footer.top, footer.right, footer.top + 1}, RGB(222, 222, 222));
    static const HICON icons[] = {LoadIcon(nullptr, IDI_WARNING), LoadIcon(nullptr, IDI_INFORMATION),
        LoadIcon(nullptr, IDI_ERROR), LoadIcon(nullptr, IDI_INFORMATION)};
    int iconSize = px(32), contentTop = captionRect.bottom + px(23);
    DrawIconEx(g_hMemDC, x + frame + px(22), contentTop, icons[kind % 4], iconSize, iconSize, 0, nullptr, DI_NORMAL);
    SelectObject(g_hMemDC, style.body ? style.body : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(g_hMemDC, GetSysColor(COLOR_WINDOWTEXT));
    RECT body{x + frame + px(68), contentTop, x + width - frame - px(24), footerTop - px(16)};
    Text(message, body, DT_WORDBREAK);
    RECT button{x + width - frame - px(102), footerTop + px(15), x + width - frame - px(18), footerTop + px(41)};
    if (!style.Themed(style.button, BP_PUSHBUTTON, PBS_DEFAULTED, button)) {
        if (old || aero) {
            Solid(button, GetSysColor(COLOR_BTNFACE));
            DrawEdge(g_hMemDC, &button, EDGE_RAISED, BF_RECT);
        } else {
            SelectObject(g_hMemDC, GetStockObject(DC_BRUSH));
            SelectObject(g_hMemDC, GetStockObject(DC_PEN));
            SetDCBrushColor(g_hMemDC, RGB(255, 255, 255));
            SetDCPenColor(g_hMemDC, RGB(0, 95, 184));
            if (rounded) RoundRect(g_hMemDC, button.left, button.top, button.right, button.bottom, px(8), px(8));
            else Rectangle(g_hMemDC, button.left, button.top, button.right, button.bottom);
        }
    }
    Text(buttonText, button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    GdiFlush();
    RestoreDC(g_hMemDC, saved);
}
} // namespace VisualPayloads
