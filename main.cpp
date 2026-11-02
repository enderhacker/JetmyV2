#include "payloads/visual/visual.h"
#include "payloads/sound/sound.h"
#include "runtime/schedule.h"
#include <chrono>
#include <numeric>
#include <random>
#include <stdexcept>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

namespace {
bool running = true;
constexpr int ExitHotkey = 101;
constexpr wchar_t ClassName[] = L"StackedChaosOverlay";

LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    if ((msg == WM_HOTKEY && wParam == ExitHotkey) || msg == WM_CLOSE ||
        msg == WM_DISPLAYCHANGE || (msg == WM_ENDSESSION && wParam)) {
        // Stop cleanly rather than using stale framebuffer dimensions after a display change.
        running = false;
        return 0;
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}

// Own selected bitmaps as well as their DCs, including partial initialization.
class Framebuffer {
public:
    HDC dc = nullptr;
    uint32_t* pixels = nullptr;
    Framebuffer() = default;
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    ~Framebuffer() {
        if (dc && previous_) SelectObject(dc, previous_);
        if (bitmap_) DeleteObject(bitmap_);
        if (dc) DeleteDC(dc);
    }
    void Create(int width, int height) {
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        dc = CreateCompatibleDC(nullptr);
        if (!dc) throw std::runtime_error("Cannot create framebuffer DC.");
        bitmap_ = CreateDIBSection(dc, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), nullptr, 0);
        if (!bitmap_ || !pixels) throw std::runtime_error("Cannot allocate framebuffer.");
        previous_ = SelectObject(dc, bitmap_);
        if (!previous_ || previous_ == HGDI_ERROR) {
            previous_ = nullptr;
            throw std::runtime_error("Cannot select framebuffer bitmap.");
        }
    }
private:
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ previous_ = nullptr;
};

class Overlay {
public:
    HWND window = nullptr;
    explicit Overlay(HINSTANCE instance) : instance_(instance) {}
    ~Overlay() {
        if (window) {
            UnregisterHotKey(window, ExitHotkey);
            ShowWindow(window, SW_HIDE);
            DestroyWindow(window);
            // Effects only touch the overlay: never overwrite live desktop pixels.
            RedrawWindow(nullptr, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        if (registered_) UnregisterClassW(ClassName, instance_);
    }
    void Create(int width, int height) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = instance_;
        wc.lpszClassName = ClassName;
        if (!RegisterClassW(&wc)) throw std::runtime_error("Cannot register overlay window.");
        registered_ = true;
        window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED |
            WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, ClassName, L"Visual Payloads", WS_POPUP,
            0, 0, width, height, nullptr, nullptr, instance_, nullptr);
        if (!window) throw std::runtime_error("Cannot create overlay window.");
        if (!SetWindowDisplayAffinity(window, WDA_EXCLUDEFROMCAPTURE))
            throw std::runtime_error("Cannot exclude overlay from screen capture.");
        RegisterHotKey(window, ExitHotkey, MOD_CONTROL | MOD_SHIFT, 'Q');
    }
private:
    HINSTANCE instance_;
    bool registered_ = false;
};

bool ExitRequested() {
    return ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
        ((GetAsyncKeyState('Q') & 0x8000) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)));
}

int Run(HINSTANCE instance) {
    using namespace VisualPayloads;
    using Clock = std::chrono::steady_clock;
    g_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (g_ScreenWidth <= 0 || g_ScreenHeight <= 0) throw std::runtime_error("Invalid screen dimensions.");
    const size_t totalBytes = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight * sizeof(uint32_t);

    Framebuffer frame, scratch, base;
    frame.Create(g_ScreenWidth, g_ScreenHeight);
    scratch.Create(g_ScreenWidth, g_ScreenHeight);
    base.Create(g_ScreenWidth, g_ScreenHeight);
    g_hMemDC = frame.dc; g_pFramePixels = frame.pixels;
    g_hScratchDC = scratch.dc; g_pScratchPixels = scratch.pixels;

    Overlay overlay(instance);
    overlay.Create(g_ScreenWidth, g_ScreenHeight);
    SoundPayloads::Player sound; // Destroy sound before removing overlay and buffers.
    int echoW = std::max(1, g_ScreenWidth / 4), echoH = std::max(1, g_ScreenHeight / 4);
    g_EchoHistory.resize(MAX_ECHOES);
    for (auto& echo : g_EchoHistory) {
        echo.w = echoW; echo.h = echoH;
        echo.pixels.resize(static_cast<size_t>(echoW) * echoH);
    }

    std::mt19937 random(std::random_device{}());
    g_FastState = random() | 1u;
    const auto& visuals = Registry();
    if (visuals.size() < Runtime::VisualCount) throw std::runtime_error("Not enough visual payloads.");
    std::vector<size_t> pool(visuals.size());
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), random);

    const auto start = Clock::now();
    auto lastFrame = start;
    int currentStage = -1;
    float echoTick = 0;
    bool historyReady = false;
    bool shown = false;
    POINT origin = {0, 0};
    SIZE size = {g_ScreenWidth, g_ScreenHeight};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, 0};

    while (running) {
        MSG msg = {};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running || ExitRequested()) break;
        const auto now = Clock::now();
        int stage = Runtime::StageAt(now - start);
        if (stage >= Runtime::VisualCount) break; // Give the tenth visual its full 10 seconds.
        float dt = std::chrono::duration<float>(now - lastFrame).count();
        float time = std::chrono::duration<float>(now - start).count();
        lastFrame = now;
        if (stage != currentStage) {
            sound.Stop(); // Silent stages really are silent; old sounds never overlap.
            if (Runtime::HasSound(stage) && !sound.PlayRandom(Runtime::StageSeconds, random))
                OutputDebugStringA("Sound unavailable; continuing visual stage silently.\n");
            currentStage = stage;
        }

        HDC desktop = GetDC(nullptr);
        if (!desktop) throw std::runtime_error("Cannot capture desktop.");
        BOOL captured = BitBlt(base.dc, 0, 0, g_ScreenWidth, g_ScreenHeight, desktop, 0, 0, SRCCOPY);
        ReleaseDC(nullptr, desktop);
        if (!captured) throw std::runtime_error("Desktop capture failed.");
        GdiFlush(); // Synchronize GDI writes before accessing DIB memory.
        echoTick += dt;
        if (!historyReady || echoTick >= 0.08f) {
            echoTick = 0;
            auto& echo = g_EchoHistory[g_EchoHead % MAX_ECHOES];
            for (int y = 0; y < echoH; ++y)
                for (int x = 0; x < echoW; ++x)
                    echo.pixels[y * echoW + x] = base.pixels[(y * g_ScreenHeight / echoH) * g_ScreenWidth + x * g_ScreenWidth / echoW];
            ++g_EchoHead;
            if (!historyReady) {
                for (size_t i = 1; i < MAX_ECHOES; ++i) g_EchoHistory[i].pixels = echo.pixels;
                historyReady = true;
            }
        }
        memcpy(frame.pixels, base.pixels, totalBytes);
        for (int i = 0; i <= stage; ++i) {
            const auto& payload = visuals[pool[i]];
            if (payload.needsSnapshot) memcpy(scratch.pixels, frame.pixels, totalBytes);
            payload.fn(std::max(0.0f, time - i * Runtime::StageSeconds), std::min(dt, 0.1f));
            GdiFlush();
        }
        if (!UpdateLayeredWindow(overlay.window, nullptr, &origin, &size, frame.dc, &origin, 0, &blend, ULW_ALPHA))
            throw std::runtime_error("Cannot present overlay frame.");
        if (!shown) { ShowWindow(overlay.window, SW_SHOWNOACTIVATE); shown = true; }
        // Rendering time counts toward the frame budget instead of adding to it.
        auto remaining = std::chrono::milliseconds(16) - (Clock::now() - now);
        if (remaining > Clock::duration::zero())
            Sleep(static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count()));
    }
    sound.Stop();
    return 0;
}
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    try { return Run(instance); }
    catch (const std::exception& error) {
        // Run's resource owners have already stopped audio and restored the desktop.
        MessageBoxA(nullptr, error.what(), "Visual Payloads", MB_OK | MB_ICONERROR);
        return 1;
    }
}
