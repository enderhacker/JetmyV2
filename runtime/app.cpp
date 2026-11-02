#include "../payloads/visual/visual.h"
#include "../payloads/sound/sound.h"
#include "schedule.h"
#include "menu.h"
#include "app.h"
#include "finale.h"
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
    void ProtectMenu(HWND menu) {
        RECT rect{};
        bool visible = menu && IsWindowVisible(menu) && !IsIconic(menu) && GetWindowRect(menu, &rect);
        if (visible) InflateRect(&rect, 4, 4);
        if (visible == hasHole_ && (!visible || EqualRect(&rect, &hole_))) return;
        if (!visible) {
            SetWindowRgn(window, nullptr, TRUE);
        } else {
            HRGN region = CreateRectRgn(0, 0, VisualPayloads::g_ScreenWidth, VisualPayloads::g_ScreenHeight);
            HRGN hole = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
            bool valid = region && hole && CombineRgn(region, region, hole, RGN_DIFF) != ERROR;
            if (hole) DeleteObject(hole);
            if (!valid || !SetWindowRgn(window, region, TRUE)) {
                if (region) DeleteObject(region);
                throw std::runtime_error("Cannot protect the control panel from visual effects.");
            }
            // On success Windows owns the window region.
        }
        hasHole_ = visible; hole_ = rect;
    }
private:
    HINSTANCE instance_;
    bool registered_ = false;
    bool hasHole_ = false;
    RECT hole_{};
};

bool ExitRequested() {
    return ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
        ((GetAsyncKeyState('Q') & 0x8000) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)));
}

int Run(HINSTANCE instance, bool menuMode) {
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
    const auto& visuals = Registry();
    Runtime::Controls controls(visuals.size());
    Runtime::ControlPanel menu(instance, controls);
    if (menuMode) menu.Create();
    SoundPayloads::Player sound; // Destroy sound before removing overlay and buffers.
    int echoW = std::max(1, g_ScreenWidth / 4), echoH = std::max(1, g_ScreenHeight / 4);
    g_EchoHistory.resize(MAX_ECHOES);
    for (auto& echo : g_EchoHistory) {
        echo.w = echoW; echo.h = echoH;
        echo.pixels.resize(static_cast<size_t>(echoW) * echoH);
    }

    std::mt19937 random(std::random_device{}());
    g_FastState = random() | 1u;
    if (visuals.size() < Runtime::VisualCount) throw std::runtime_error("Not enough visual payloads.");
    std::vector<size_t> pool(visuals.size());
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), random);

    const auto start = Clock::now();
    auto lastFrame = start;
    int currentStage = -1;
    int soundSelection = -1;
    uint64_t audioRevision = 0;
    std::vector<bool> wasEnabled(visuals.size(), false);
    std::vector<Clock::time_point> activated(visuals.size(), start);
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
            if (menuMode && IsDialogMessageW(menu.Window(), &msg)) continue;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running || controls.closing || ExitRequested()) break;
        const auto now = Clock::now();
        int stage = Runtime::StageAt(now - start);
        if ((!menuMode && stage >= Runtime::VisualCount) || controls.finaleRequested) {
            controls.finaleRequested = false;
            std::fill(controls.visuals.begin(), controls.visuals.end(), false);
            std::fill(wasEnabled.begin(), wasEnabled.end(), false);
            controls.audio = -1; ++controls.audioRevision;
            audioRevision = controls.audioRevision;
            sound.Stop();
            ShowWindow(overlay.window, SW_HIDE); shown = false;
            if (menuMode) {
                menu.SetPreviewActive(false); menu.Refresh();
                ShowWindow(menu.Window(), SW_HIDE);
            }
            bool completed = Runtime::RunBlueScreenFinale(instance, [&] {
                return !running || controls.closing || ExitRequested();
            });
            if (!menuMode || !completed || !running || controls.closing || ExitRequested()) break;
            historyReady = false; echoTick = 0; g_EchoHead = 0;
            lastFrame = Clock::now();
            ShowWindow(menu.Window(), SW_SHOW);
            SetForegroundWindow(menu.Window());
            menu.Refresh();
            continue;
        }
        float dt = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;
        if (!menuMode && stage != currentStage) {
            for (int i = 0; i <= stage; ++i) controls.visuals[pool[i]] = true;
            int selection = Runtime::SoundSelectionAt(stage);
            if (selection != soundSelection) {
                if (!sound.PlayRandom(Runtime::StageSeconds, random))
                    OutputDebugStringA("Sound unavailable; retaining previous audio if possible.\n");
                soundSelection = selection;
            }
            currentStage = stage;
        }
        if (menuMode) {
            if (audioRevision != controls.audioRevision) {
                bool success = true;
                if (controls.audio < 0) sound.Stop();
                else success = sound.Play(static_cast<size_t>(controls.audio), Runtime::StageSeconds, random);
                controls.audio = sound.ActiveIndex();
                audioRevision = controls.audioRevision;
                if (success) menu.Refresh();
                else menu.Refresh(L"Audio could not start. Check your output device and try again.");
            } else if (controls.audio >= 0 && !sound.IsPlaying()) {
                sound.Stop(); controls.audio = -1;
                menu.Refresh(L"Audio stopped. Check the device and select a sound to retry.");
            }
            overlay.ProtectMenu(menu.Window());
        }
        bool anyVisual = false;
        for (size_t i = 0; i < controls.visuals.size(); ++i) {
            if (!controls.visuals[i]) wasEnabled[i] = false;
            else anyVisual = true;
        }
        if (menuMode) menu.SetPreviewActive(anyVisual);
        if (!anyVisual) {
            if (shown) { ShowWindow(overlay.window, SW_HIDE); shown = false; }
            historyReady = false; echoTick = 0; g_EchoHead = 0;
            // Audio-only and idle menu modes do no screen capture or rendering.
            MsgWaitForMultipleObjectsEx(0, nullptr, 32, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            continue;
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
        // Automatic mode retains shuffled stacking order; manual mode uses registry order.
        for (size_t order = 0; order < visuals.size(); ++order) {
            size_t index = menuMode ? order : pool[order];
            if (!controls.visuals[index]) continue;
            bool firstFrame = !wasEnabled[index];
            if (firstFrame) activated[index] = now;
            float age = firstFrame ? 0 : std::chrono::duration<float>(now - activated[index]).count();
            const auto& payload = visuals[index];
            if (payload.needsSnapshot) memcpy(scratch.pixels, frame.pixels, totalBytes);
            payload.fn(age, firstFrame ? 0 : std::min(dt, 0.1f));
            wasEnabled[index] = true;
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

namespace Runtime {
int RunApplication(HINSTANCE instance, bool menuMode) {
    running = true;
    return Run(instance, menuMode);
}
} // namespace Runtime
