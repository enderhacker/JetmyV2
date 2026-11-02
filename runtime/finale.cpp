#include "finale.h"
#include <algorithm>
#include <chrono>
#include <cwchar>
#include <stdexcept>

namespace Runtime {
namespace {
constexpr wchar_t FinaleClass[] = L"JetmySimulatedBlueScreen";
class BlueScreen {
public:
    HINSTANCE instance;
    HWND window = nullptr;
    HWND previousForeground = nullptr;
    HDC canvas = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ previousBitmap = nullptr;
    HFONT font = nullptr;
    bool registered = false, dismissed = false;
    int progress = -1;
    explicit BlueScreen(HINSTANCE app) : instance(app) {}
    ~BlueScreen() {
        bool restoreFocus = window && GetForegroundWindow() == window;
        if (window) { KillTimer(window, 1); DestroyWindow(window); }
        if (restoreFocus && IsWindow(previousForeground) && IsWindowVisible(previousForeground))
            SetForegroundWindow(previousForeground);
        if (canvas && previousBitmap) SelectObject(canvas, previousBitmap);
        if (bitmap) DeleteObject(bitmap);
        if (canvas) DeleteDC(canvas);
        if (font) DeleteObject(font);
        if (registered) UnregisterClassW(FinaleClass, instance);
        // Cursor visibility was only changed by WM_SETCURSOR, never ShowCursor.
        POINT cursor{};
        if (GetCursorPos(&cursor)) {
            HWND target = WindowFromPoint(cursor);
            if (target) PostMessageW(target, WM_SETCURSOR, reinterpret_cast<WPARAM>(target), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        }
    }
    void Create() {
        canvas = CreateCompatibleDC(nullptr);
        if (!canvas) throw std::runtime_error("Cannot create blue-screen canvas.");
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = 640; info.bmiHeader.biHeight = -480;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        void* pixels = nullptr;
        bitmap = CreateDIBSection(canvas, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!bitmap) throw std::runtime_error("Cannot allocate blue-screen canvas.");
        previousBitmap = SelectObject(canvas, bitmap);
        if (!previousBitmap || previousBitmap == HGDI_ERROR) {
            previousBitmap = nullptr;
            throw std::runtime_error("Cannot select blue-screen canvas.");
        }
        font = CreateFontW(-12, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH, L"Lucida Console");
        if (!font) throw std::runtime_error("Cannot create blue-screen font.");
        WNDCLASSW wc{};
        wc.lpfnWndProc = Procedure; wc.hInstance = instance; wc.lpszClassName = FinaleClass;
        // No class cursor: the cursor is hidden only over this presentation window.
        if (!RegisterClassW(&wc)) throw std::runtime_error("Cannot register blue-screen window.");
        registered = true;
        previousForeground = GetForegroundWindow();
        window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, FinaleClass, L"JetmyV2 - Simulated blue screen",
            WS_POPUP, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
            nullptr, nullptr, instance, this);
        if (!window) throw std::runtime_error("Cannot create blue-screen window.");
        Render(0);
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetForegroundWindow(window); SetFocus(window);
        UpdateWindow(window);
        POINT cursor{}; RECT bounds{};
        if (GetCursorPos(&cursor) && GetWindowRect(window, &bounds) && PtInRect(&bounds, cursor)) SetCursor(nullptr);
    }
    void Render(int percent) {
        if (percent == progress) return;
        progress = percent;
        RECT rect{0, 0, 640, 480};
        HBRUSH blue = CreateSolidBrush(RGB(0, 0, 170));
        if (!blue) throw std::runtime_error("Cannot create blue-screen background.");
        FillRect(canvas, &rect, blue); DeleteObject(blue);
        int saved = SaveDC(canvas);
        if (!saved) throw std::runtime_error("Cannot save blue-screen drawing state.");
        SelectObject(canvas, font);
        SetTextColor(canvas, RGB(255, 255, 255)); SetBkMode(canvas, TRANSPARENT);
        auto line = [&](int row, const wchar_t* text) { TextOutW(canvas, 20, 22 + row * 16, text, int(std::wcslen(text))); };
        line(0, L"A problem has been detected and Windows has been shut down to prevent");
        line(1, L"damage to your computer.");
        line(3, L"PAGE_FAULT_IN_NONPAGED_AREA");
        line(5, L"If this is the first time you've seen this Stop error screen,");
        line(6, L"restart your computer. If this screen appears again, follow");
        line(7, L"these steps:");
        line(9, L"Check to make sure any new hardware or software is properly installed.");
        line(10, L"If this is a new installation, ask your hardware or software manufacturer");
        line(11, L"for any Windows updates you might need.");
        line(13, L"If problems continue, disable or remove any newly installed hardware");
        line(14, L"or software. Disable BIOS memory options such as caching or shadowing.");
        line(15, L"If you need to use Safe Mode to remove or disable components, restart");
        line(16, L"your computer, press F8 to select Advanced Startup Options, and then");
        line(17, L"select Safe Mode.");
        line(19, L"Technical information:");
        line(21, L"*** STOP: 0x00000050 (0xE1A7D000, 0x00000000, 0xBF8013B2, 0x00000001)");
        line(23, L"*** win32k.sys - Address BF8013B2 base at BF800000, DateStamp 3b7de698");
        line(25, L"Beginning dump of physical memory");
        wchar_t dump[80]{};
        std::swprintf(dump, 80, L"Dumping physical memory to disk: %d", progress);
        line(26, dump);
        RestoreDC(canvas, saved);
        // The text is cosmetic. No memory dump, file write, crash, or reboot occurs.
        InvalidateRect(window, nullptr, FALSE);
    }
    void Paint() {
        PAINTSTRUCT paint{}; HDC dc = BeginPaint(window, &paint);
        RECT client{}; GetClientRect(window, &client);
        FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        // Keep the 4:3 image centered on the primary display; other monitors stay black.
        int screenW = GetSystemMetrics(SM_CXSCREEN), screenH = GetSystemMetrics(SM_CYSCREEN);
        int height = std::min(screenH, screenW * 3 / 4), width = height * 4 / 3;
        int x = (screenW - width) / 2 - GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = (screenH - height) / 2 - GetSystemMetrics(SM_YVIRTUALSCREEN);
        SetStretchBltMode(dc, COLORONCOLOR);
        StretchBlt(dc, x, y, width, height, canvas, 0, 0, 640, 480, SRCCOPY);
        EndPaint(window, &paint);
    }
    static LRESULT CALLBACK Procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<BlueScreen*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            self = static_cast<BlueScreen*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            self->window = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) {
            if (msg == WM_PAINT) { self->Paint(); return 0; }
            if (msg == WM_ERASEBKGND) return 1;
            if (msg == WM_SETCURSOR) { SetCursor(nullptr); return TRUE; }
            if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) { self->dismissed = true; return 0; }
            if (msg == WM_CLOSE || msg == WM_DISPLAYCHANGE || (msg == WM_ENDSESSION && wParam)) {
                self->dismissed = true; return 0;
            }
            if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR ||
                (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)) return 0;
        }
        return DefWindowProcW(window, msg, wParam, lParam);
    }
};
}

bool RunBlueScreenFinale(HINSTANCE instance, const std::function<bool()>& shouldExit) {
    using Clock = std::chrono::steady_clock;
    BlueScreen screen(instance);
    screen.Create();
    const auto start = Clock::now();
    while (!screen.dismissed && !shouldExit()) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { PostQuitMessage(static_cast<int>(msg.wParam)); return false; }
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (screen.dismissed || shouldExit()) return false;
        float elapsed = std::chrono::duration<float>(Clock::now() - start).count();
        if (elapsed >= FinaleSeconds) return true;
        screen.Render(std::min(100, int(elapsed / (FinaleSeconds - 0.5f) * 100)));
        // Keep pumping messages and checking emergency exits; never sleep for ten seconds.
        MsgWaitForMultipleObjectsEx(0, nullptr, 30, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
    return false;
}
} // namespace Runtime
