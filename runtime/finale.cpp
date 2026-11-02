#include "finale.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <mmsystem.h>
#include <stdexcept>
#include <vector>

namespace Runtime {
namespace {
constexpr wchar_t FinaleClass[] = L"JetmySimulatedBlueScreen";
class ErrorTone {
    HWAVEOUT device_ = nullptr;
    WAVEHDR header_{};
    std::vector<int16_t> samples_;
    bool prepared_ = false;
public:
    ~ErrorTone() {
        if (!device_) return;
        waveOutReset(device_);
        if (prepared_) waveOutUnprepareHeader(device_, &header_, sizeof(header_));
        waveOutClose(device_);
    }
    void Prepare() {
        constexpr unsigned rate = 8000;
        samples_.resize(rate * FinaleSeconds);
        for (size_t i = 0; i < samples_.size(); ++i) {
            // A single steady 800 Hz tone, with only 5 ms edge ramps to avoid clicks.
            float envelope = std::min({1.0f, float(i) / 40, float(samples_.size() - 1 - i) / 40});
            samples_[i] = static_cast<int16_t>(3500 * envelope * std::sin(6.28318530718 * 800 * i / rate));
        }
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 1; format.nSamplesPerSec = rate; format.wBitsPerSample = 16;
        format.nBlockAlign = 2; format.nAvgBytesPerSec = rate * 2;
        if (waveOutOpen(&device_, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
            device_ = nullptr; return;
        }
        header_.lpData = reinterpret_cast<LPSTR>(samples_.data());
        header_.dwBufferLength = static_cast<DWORD>(samples_.size() * sizeof(int16_t));
        prepared_ = waveOutPrepareHeader(device_, &header_, sizeof(header_)) == MMSYSERR_NOERROR;
    }
    void Start() {
        if (!prepared_ || waveOutWrite(device_, &header_, sizeof(header_)) != MMSYSERR_NOERROR)
            OutputDebugStringA("Finale tone unavailable; continuing blue-screen presentation silently.\n");
    }
};
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
    std::chrono::steady_clock::time_point presentedAt{};
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
        presentedAt = std::chrono::steady_clock::now();
        POINT cursor{}; RECT bounds{};
        if (GetCursorPos(&cursor) && GetWindowRect(window, &bounds) && PtInRect(&bounds, cursor)) SetCursor(nullptr);
    }
    void Render(int percent) {
        if (percent == progress) return;
        bool initial = progress < 0;
        progress = percent;
        RECT rect = initial ? RECT{0, 0, 640, 480} : RECT{20, 438, 620, 454};
        HBRUSH blue = CreateSolidBrush(RGB(0, 0, 170));
        if (!blue) throw std::runtime_error("Cannot create blue-screen background.");
        FillRect(canvas, &rect, blue); DeleteObject(blue);
        int saved = SaveDC(canvas);
        if (!saved) throw std::runtime_error("Cannot save blue-screen drawing state.");
        SelectObject(canvas, font);
        SetTextColor(canvas, RGB(255, 255, 255)); SetBkMode(canvas, TRANSPARENT);
        auto line = [&](int row, const wchar_t* text) { TextOutW(canvas, 20, 22 + row * 16, text, int(std::wcslen(text))); };
        if (initial) {
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
        }
        wchar_t dump[80]{};
        std::swprintf(dump, 80, L"Dumping physical memory to disk: %d", progress);
        line(26, dump);
        RestoreDC(canvas, saved);
        GdiFlush();
        // The text is cosmetic. No memory dump, file write, crash, or reboot occurs.
        if (initial) InvalidateRect(window, nullptr, FALSE);
        else {
            RECT image = ImageRect();
            RECT dirty{image.left + rect.left * (image.right - image.left) / 640,
                image.top + rect.top * (image.bottom - image.top) / 480,
                image.left + (rect.right * (image.right - image.left) + 639) / 640,
                image.top + (rect.bottom * (image.bottom - image.top) + 479) / 480};
            InvalidateRect(window, &dirty, FALSE);
        }
    }
    RECT ImageRect() const {
        // Keep the 4:3 image centered on the primary display; other monitors stay black.
        int screenW = GetSystemMetrics(SM_CXSCREEN), screenH = GetSystemMetrics(SM_CYSCREEN);
        int height = std::min(screenH, screenW * 3 / 4), width = height * 4 / 3;
        int x = (screenW - width) / 2 - GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = (screenH - height) / 2 - GetSystemMetrics(SM_YVIRTUALSCREEN);
        return {x, y, x + width, y + height};
    }
    void Paint() {
        PAINTSTRUCT paint{}; HDC dc = BeginPaint(window, &paint);
        RECT client{}; GetClientRect(window, &client);
        RECT image = ImageRect();
        // Paint letterboxing only: never flash black underneath the blue image.
        RECT bars[] = {{client.left, client.top, client.right, image.top},
            {client.left, image.bottom, client.right, client.bottom},
            {client.left, image.top, image.left, image.bottom},
            {image.right, image.top, client.right, image.bottom}};
        for (const auto& bar : bars)
            if (bar.right > bar.left && bar.bottom > bar.top)
                FillRect(dc, &bar, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        SetStretchBltMode(dc, COLORONCOLOR);
        StretchBlt(dc, image.left, image.top, image.right - image.left, image.bottom - image.top,
            canvas, 0, 0, 640, 480, SRCCOPY);
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
    ErrorTone tone;
    tone.Prepare(); // Allocate/open audio before the visible ten-second countdown.
    screen.Create();
    const auto start = screen.presentedAt;
    const auto duration = std::chrono::seconds(FinaleSeconds);
    const auto deadline = start + duration;
    tone.Start(); // Nonblocking playback; emergency exits remain responsive.
    while (!screen.dismissed && !shouldExit()) {
        MSG msg{};
        for (int processed = 0; processed < 64 && PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE); ++processed) {
            if (msg.message == WM_QUIT) { PostQuitMessage(static_cast<int>(msg.wParam)); return false; }
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (screen.dismissed || shouldExit()) return false;
        const auto now = Clock::now();
        if (now >= deadline) return true;
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        screen.Render(std::min(99, int(elapsedMs * 100 / (FinaleSeconds * 1000))));
        // Keep pumping messages and checking emergency exits; never sleep for ten seconds.
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count();
        DWORD wait = static_cast<DWORD>(std::clamp<long long>(remaining, 0, 30));
        MsgWaitForMultipleObjectsEx(0, nullptr, wait, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
    return false;
}
} // namespace Runtime
