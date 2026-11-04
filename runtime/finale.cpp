#include "finale.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
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
    HDC frozenCanvas = nullptr;
    HBITMAP frozenBitmap = nullptr;
    HGDIOBJ previousFrozenBitmap = nullptr;
    bool frozen = true;
    BlueScreenArtwork artwork;
    bool registered = false, dismissed = false;
    int progress = -1;
    std::chrono::steady_clock::time_point presentedAt{};
    BlueScreen(HINSTANCE app, BlueScreenStyle style) : instance(app), artwork(style) {}
    ~BlueScreen() {
        bool restoreFocus = window && GetForegroundWindow() == window;
        if (window) { KillTimer(window, 1); DestroyWindow(window); }
        if (restoreFocus && IsWindow(previousForeground) && IsWindowVisible(previousForeground))
            SetForegroundWindow(previousForeground);
        if (canvas && previousBitmap) SelectObject(canvas, previousBitmap);
        if (bitmap) DeleteObject(bitmap);
        if (canvas) DeleteDC(canvas);
        if (frozenCanvas && previousFrozenBitmap) SelectObject(frozenCanvas, previousFrozenBitmap);
        if (frozenBitmap) DeleteObject(frozenBitmap);
        if (frozenCanvas) DeleteDC(frozenCanvas);
        if (registered) UnregisterClassW(FinaleClass, instance);
        // Cursor visibility was only changed by WM_SETCURSOR, never ShowCursor.
        POINT cursor{};
        if (GetCursorPos(&cursor)) {
            HWND target = WindowFromPoint(cursor);
            if (target) PostMessageW(target, WM_SETCURSOR, reinterpret_cast<WPARAM>(target), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        }
    }
    void Create() {
        // Capture once before showing our window. Windows and other apps keep running.
        HDC desktop = GetDC(nullptr);
        if (!desktop) throw std::runtime_error("Cannot capture frozen desktop.");
        frozenCanvas = CreateCompatibleDC(desktop);
        frozenBitmap = CreateCompatibleBitmap(desktop, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
        if (frozenCanvas && frozenBitmap) previousFrozenBitmap = SelectObject(frozenCanvas, frozenBitmap);
        bool captured = previousFrozenBitmap && previousFrozenBitmap != HGDI_ERROR &&
            BitBlt(frozenCanvas, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
                desktop, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), SRCCOPY | CAPTUREBLT);
        ReleaseDC(nullptr, desktop);
        if (previousFrozenBitmap == HGDI_ERROR) previousFrozenBitmap = nullptr;
        if (!captured) throw std::runtime_error("Cannot capture frozen desktop.");
        CURSORINFO cursorInfo{};
        cursorInfo.cbSize = sizeof(cursorInfo);
        if (GetCursorInfo(&cursorInfo) && (cursorInfo.flags & CURSOR_SHOWING)) {
            ICONINFO icon{};
            if (GetIconInfo(cursorInfo.hCursor, &icon)) {
                DrawIconEx(frozenCanvas, cursorInfo.ptScreenPos.x - GetSystemMetrics(SM_XVIRTUALSCREEN) - icon.xHotspot,
                    cursorInfo.ptScreenPos.y - GetSystemMetrics(SM_YVIRTUALSCREEN) - icon.yHotspot,
                    cursorInfo.hCursor, 0, 0, 0, nullptr, DI_NORMAL);
                if (icon.hbmMask) DeleteObject(icon.hbmMask);
                if (icon.hbmColor) DeleteObject(icon.hbmColor);
            }
        }
        canvas = CreateCompatibleDC(nullptr);
        if (!canvas) throw std::runtime_error("Cannot create blue-screen canvas.");
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = artwork.Width(); info.bmiHeader.biHeight = -artwork.Height();
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        void* pixels = nullptr;
        bitmap = CreateDIBSection(canvas, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!bitmap) throw std::runtime_error("Cannot allocate blue-screen canvas.");
        previousBitmap = SelectObject(canvas, bitmap);
        if (!previousBitmap || previousBitmap == HGDI_ERROR) {
            previousBitmap = nullptr;
            throw std::runtime_error("Cannot select blue-screen canvas.");
        }
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
        artwork.Draw(canvas, progress, initial);
        // The text is cosmetic. No memory dump, file write, crash, or reboot occurs.
        if (initial) InvalidateRect(window, nullptr, FALSE);
        else {
            RECT dirty = artwork.ProgressRect();
            RECT image = ImageRect();
            OffsetRect(&dirty, image.left, image.top); // Artwork is already in presentation pixels.
            InvalidateRect(window, &dirty, FALSE);
        }
    }
    RECT ImageRect() const {
        // Keep the 4:3 image centered on the primary display; other monitors stay black.
        int screenW = GetSystemMetrics(SM_CXSCREEN), screenH = GetSystemMetrics(SM_CYSCREEN);
        int height = artwork.Height(), width = artwork.Width();
        int x = (screenW - width) / 2 - GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = (screenH - height) / 2 - GetSystemMetrics(SM_YVIRTUALSCREEN);
        return {x, y, x + width, y + height};
    }
    void Paint() {
        PAINTSTRUCT paint{}; HDC dc = BeginPaint(window, &paint);
        RECT client{}; GetClientRect(window, &client);
        if (frozen) {
            BitBlt(dc, 0, 0, client.right, client.bottom, frozenCanvas, 0, 0, SRCCOPY);
            EndPaint(window, &paint);
            return;
        }
        RECT image = ImageRect();
        // Paint letterboxing only: never flash black underneath the blue image.
        RECT bars[] = {{client.left, client.top, client.right, image.top},
            {client.left, image.bottom, client.right, client.bottom},
            {client.left, image.top, image.left, image.bottom},
            {image.right, image.top, client.right, image.bottom}};
        for (const auto& bar : bars)
            if (bar.right > bar.left && bar.bottom > bar.top)
                FillRect(dc, &bar, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        // Canvas and presentation have identical physical dimensions. In particular,
        // do not nearest-neighbor upscale classic text or the modern QR code.
        BitBlt(dc, image.left, image.top, artwork.Width(), artwork.Height(), canvas, 0, 0, SRCCOPY);
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

bool RunBlueScreenFinale(HINSTANCE instance, const std::function<bool()>& shouldExit, BlueScreenStyle style) {
    using Clock = std::chrono::steady_clock;
    BlueScreen screen(instance, style);
    screen.Create();
    auto start = screen.presentedAt;
    const auto freezeDeadline = start + std::chrono::seconds(2);
    const auto duration = std::chrono::seconds(FinaleSeconds);
    auto deadline = freezeDeadline + duration;
    while (!screen.dismissed && !shouldExit()) {
        MSG msg{};
        for (int processed = 0; processed < 64 && PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE); ++processed) {
            if (msg.message == WM_QUIT) { PostQuitMessage(static_cast<int>(msg.wParam)); return false; }
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (screen.dismissed || shouldExit()) return false;
        const auto now = Clock::now();
        if (screen.frozen) {
            if (now >= freezeDeadline) {
                screen.frozen = false;
                InvalidateRect(screen.window, nullptr, FALSE);
                UpdateWindow(screen.window);
                start = Clock::now();
                deadline = start + duration;
            }
            MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            continue;
        }
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
