#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <string>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

// ============================================================================
// GLOBAL APPLICATION STATE & PIPELINE BUFFERS
// ============================================================================
static bool g_Running = true;
static int g_ScreenWidth = 0;
static int g_ScreenHeight = 0;

// Working Composite Buffer (Target)
static HDC g_hMemDC = nullptr;
static HBITMAP g_hDIB = nullptr;
static uint32_t* g_pFramePixels = nullptr;

// Scratch Buffer (Source snapshot for the currently executing payload)
static HDC g_hScratchDC = nullptr;
static HBITMAP g_hScratchBmp = nullptr;
static uint32_t* g_pScratchPixels = nullptr;

// Raw Live Desktop Snapshot
static HDC g_hBaseDC = nullptr;
static HBITMAP g_hBaseBmp = nullptr;
static uint32_t* g_pBasePixels = nullptr;

// History buffer for Effect 18 (Time Echoes)
struct EchoFrame {
    int w, h;
    std::vector<uint32_t> pixels;
};
static const size_t MAX_ECHOES = 16;
static std::vector<EchoFrame> g_EchoHistory;
static size_t g_EchoHead = 0;

// PRNG
static std::mt19937 g_Rng(1337);
inline int RandInt(int min, int max) {
    std::uniform_int_distribution<int> d(min, max);
    return d(g_Rng);
}
inline float RandFloat(float min, float max) {
    std::uniform_real_distribution<float> d(min, max);
    return d(g_Rng);
}

inline uint32_t BlendColor(uint32_t c1, uint32_t c2, float t) {
    float inv = 1.0f - t;
    uint32_t r = static_cast<uint32_t>(((c1 >> 16) & 0xFF) * inv + ((c2 >> 16) & 0xFF) * t);
    uint32_t g = static_cast<uint32_t>(((c1 >> 8) & 0xFF) * inv + ((c2 >> 8) & 0xFF) * t);
    uint32_t b = static_cast<uint32_t>((c1 & 0xFF) * inv + (c2 & 0xFF) * t);
    return (r << 16) | (g << 8) | b;
}

// ============================================================================
// 25 STACKABLE PAYLOAD IMPLEMENTATIONS
// Each reads from g_hScratchDC / g_pScratchPixels and writes to g_hMemDC / g_pFramePixels
// ============================================================================

// 1. PIXEL FIREWORKS
struct FireworkParticle { float x, y, vx, vy; uint32_t color; float life; };
static std::vector<FireworkParticle> g_Fireworks;

void EffectPixelFireworks(float time, float dt) {
    if (RandInt(0, 12) == 0 || g_Fireworks.empty()) {
        float ox = RandFloat(g_ScreenWidth * 0.1f, g_ScreenWidth * 0.9f);
        float oy = RandFloat(g_ScreenHeight * 0.1f, g_ScreenHeight * 0.7f);
        uint32_t col = RGB(RandInt(80, 255), RandInt(80, 255), RandInt(80, 255));
        for (int i = 0; i < 40; ++i) {
            float ang = RandFloat(0, 6.28318f);
            float spd = RandFloat(60.0f, 300.0f);
            g_Fireworks.push_back({ ox, oy, cosf(ang) * spd, sinf(ang) * spd, col, RandFloat(0.4f, 1.2f) });
        }
    }
    for (size_t i = 0; i < g_Fireworks.size();) {
        auto& p = g_Fireworks[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 260.0f * dt;
        p.life -= dt;
        int ix = static_cast<int>(p.x);
        int iy = static_cast<int>(p.y);
        if (p.life > 0 && ix >= 0 && ix < g_ScreenWidth - 3 && iy >= 0 && iy < g_ScreenHeight - 3) {
            RECT r = { ix, iy, ix + 3, iy + 3 };
            HBRUSH b = CreateSolidBrush(p.color);
            FillRect(g_hMemDC, &r, b);
            DeleteObject(b);
            ++i;
        } else {
            g_Fireworks[i] = g_Fireworks.back();
            g_Fireworks.pop_back();
        }
    }
}

// 2. CHAOS RECTANGLES
void EffectChaosRectangles(float time, float dt) {
    for (int i = 0; i < 15; ++i) {
        int w = RandInt(30, 200);
        int h = RandInt(30, 200);
        int x = RandInt(0, g_ScreenWidth - w);
        int y = RandInt(0, g_ScreenHeight - h);
        if (i % 2 == 0) {
            PatBlt(g_hMemDC, x, y, w, h, DSTINVERT);
        } else {
            HBRUSH b = CreateSolidBrush(RGB(RandInt(0, 255), RandInt(0, 255), RandInt(0, 255)));
            RECT r = { x, y, x + w, y + h };
            FrameRect(g_hMemDC, &r, b);
            DeleteObject(b);
        }
    }
}

// 3. BLACK HOLE SINGULARITIES
void EffectBlackHoleSingularities(float time, float dt) {
    float hx = g_ScreenWidth * (0.5f + 0.3f * sinf(time * 1.4f));
    float hy = g_ScreenHeight * (0.5f + 0.25f * cosf(time * 1.1f));
    float radius = 220.0f;
    float twist = 3.5f;

    for (int y = 0; y < g_ScreenHeight; y += 2) {
        float dy = y - hy;
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            float dx = x - hx;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < radius) {
                float factor = 1.0f - (dist / radius);
                float a = atan2f(dy, dx) + factor * twist;
                float newDist = dist * (1.0f - factor * 0.45f);
                int sx = std::clamp(static_cast<int>(hx + cosf(a) * newDist), 0, g_ScreenWidth - 1);
                int sy = std::clamp(static_cast<int>(hy + sinf(a) * newDist), 0, g_ScreenHeight - 1);
                uint32_t col = g_pScratchPixels[sy * g_ScreenWidth + sx];
                g_pFramePixels[rowIdx + x] = col;
                g_pFramePixels[rowIdx + x + 1] = col;
                g_pFramePixels[rowIdx + g_ScreenWidth + x] = col;
                g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = col;
            }
        }
    }
}

// 4. DESKTOP GLASS SHATTER
void EffectDesktopGlassShatter(float time, float dt) {
    int cols = 8, rows = 6;
    int sw = g_ScreenWidth / cols;
    int sh = g_ScreenHeight / rows;
    float cycle = fmodf(time, 3.5f);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float delay = (c * 0.06f) + (r * 0.1f);
            float t = (std::max)(0.0f, cycle - delay);
            int drop = static_cast<int>(0.5f * 800.0f * t * t);
            if (drop > 0) {
                int sx = c * sw, sy = r * sh;
                int dy = (std::min)(g_ScreenHeight - sh, sy + drop);
                BitBlt(g_hMemDC, sx, dy, sw - 3, sh - 3, g_hScratchDC, sx, sy, SRCCOPY);
            }
        }
    }
}

// 5. LIVING DIGITAL INFECTION
void EffectLivingInfection(float time, float dt) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 120));
    HGDIOBJ old = SelectObject(g_hMemDC, pen);
    int nodes = static_cast<int>(fmodf(time * 40.0f, 250.0f)) + 10;
    std::mt19937 tempRng(444);
    for (int root = 0; root < 4; ++root) {
        int cx = (root * g_ScreenWidth) / 3;
        int cy = (root % 2 == 0) ? 0 : g_ScreenHeight;
        MoveToEx(g_hMemDC, cx, cy, NULL);
        for (int i = 0; i < nodes; ++i) {
            cx += std::uniform_int_distribution<int>(-18, 18)(tempRng);
            cy += std::uniform_int_distribution<int>((root % 2 == 0) ? 4 : -4, (root % 2 == 0) ? 22 : -22)(tempRng);
            LineTo(g_hMemDC, cx, cy);
        }
    }
    SelectObject(g_hMemDC, old);
    DeleteObject(pen);
}

// 6. IMPOSSIBLE WINDOWS
struct FakeWin { float x, y, vx, vy; int w, h; const char* title; };
static std::vector<FakeWin> g_FakeWindows = {
    { 120, 120, 160, 120, 260, 170, "Stack_Overflow.exe" },
    { 600, 220, -140, 150, 230, 150, "Memory_Fault" },
    { 400, 500, 170, -130, 280, 180, "Kernel_Interrupt" }
};

void EffectImpossibleWindows(float time, float dt) {
    for (auto& w : g_FakeWindows) {
        w.x += w.vx * dt;
        w.y += w.vy * dt;
        if (w.x < 0) { w.x = 0; w.vx = -w.vx; }
        if (w.x + w.w > g_ScreenWidth) { w.x = static_cast<float>(g_ScreenWidth - w.w); w.vx = -w.vx; }
        if (w.y < 0) { w.y = 0; w.vy = -w.vy; }
        if (w.y + w.h > g_ScreenHeight) { w.y = static_cast<float>(g_ScreenHeight - w.h); w.vy = -w.vy; }

        int ix = static_cast<int>(w.x), iy = static_cast<int>(w.y);
        RECT r = { ix, iy, ix + w.w, iy + w.h };
        FillRect(g_hMemDC, &r, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
        RECT rt = { ix, iy, ix + w.w, iy + 22 };
        HBRUSH tb = CreateSolidBrush(RGB(190, 25, 35));
        FillRect(g_hMemDC, &rt, tb);
        DeleteObject(tb);

        SetBkMode(g_hMemDC, TRANSPARENT);
        SetTextColor(g_hMemDC, RGB(255, 255, 255));
        TextOutA(g_hMemDC, ix + 6, iy + 3, w.title, static_cast<int>(strlen(w.title)));

        SetStretchBltMode(g_hMemDC, COLORONCOLOR);
        StretchBlt(g_hMemDC, ix + 3, iy + 24, w.w - 6, w.h - 27, g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, SRCCOPY);
    }
}

// 7. LIQUID DESKTOP
void EffectLiquidDesktop(float time, float dt) {
    float waveSpeed = 7.0f;
    float waveFreq = 0.03f;
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int xShift = static_cast<int>(sinf((y * waveFreq) + (time * waveSpeed)) * 16.0f);
            int yShift = static_cast<int>(cosf((x * waveFreq) + (time * waveSpeed)) * 12.0f);
            int sx = std::clamp(x + xShift, 0, g_ScreenWidth - 1);
            int sy = std::clamp(y + yShift, 0, g_ScreenHeight - 1);
            uint32_t col = g_pScratchPixels[sy * g_ScreenWidth + sx];
            g_pFramePixels[rowIdx + x] = col;
            g_pFramePixels[rowIdx + x + 1] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = col;
        }
    }
}

// 8. PIXEL SORTING ANOMALY
void EffectPixelSortingAnomaly(float time, float dt) {
    for (int b = 0; b < 8; ++b) {
        int boxW = RandInt(100, 300);
        int boxH = RandInt(30, 100);
        int sx = RandInt(0, g_ScreenWidth - boxW);
        int sy = RandInt(0, g_ScreenHeight - boxH);
        for (int y = sy; y < sy + boxH; ++y) {
            uint32_t* row = g_pFramePixels + (y * g_ScreenWidth) + sx;
            std::sort(row, row + boxW, [](uint32_t a, uint32_t b) {
                return (a & 0xFF) < (b & 0xFF);
            });
        }
    }
}

// 9. NEON FRACTAL INVASION
void EffectNeonFractalInvasion(float time, float dt) {
    float cr = -0.7f + sinf(time * 0.4f) * 0.08f;
    float ci = 0.27f + cosf(time * 0.3f) * 0.08f;
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        float zy0 = 1.4f * (y - g_ScreenHeight / 2) / (0.5f * g_ScreenHeight);
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            float zx = 1.4f * (x - g_ScreenWidth / 2) / (0.5f * g_ScreenHeight);
            float zy = zy0;
            int i = 16;
            while (zx * zx + zy * zy < 4.0f && i > 0) {
                float tmp = zx * zx - zy * zy + cr;
                zy = 2.0f * zx * zy + ci;
                zx = tmp;
                --i;
            }
            uint32_t neon = (i == 0) ? 0x000000 : ((i * 15) << 16) | ((i * 8) << 8) | (i * 26);
            uint32_t col = BlendColor(g_pScratchPixels[rowIdx + x], neon, 0.5f);
            g_pFramePixels[rowIdx + x] = col;
            g_pFramePixels[rowIdx + x + 1] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = col;
        }
    }
}

// 10. GHOST CURSOR SWARM
void EffectGhostCursorSwarm(float time, float dt) {
    POINT p; GetCursorPos(&p);
    HCURSOR hArr = LoadCursor(NULL, IDC_ARROW);
    for (int i = 0; i < 28; ++i) {
        float a = time * 3.0f + (i * 0.45f);
        float r = 40.0f + (i * 16.0f);
        int cx = static_cast<int>(p.x + cosf(a) * r);
        int cy = static_cast<int>(p.y + sinf(a) * r);
        DrawIcon(g_hMemDC, cx, cy, hArr);
    }
}

// 11. SPATIAL PORTAL NETWORK
void EffectSpatialPortalNetwork(float time, float dt) {
    int portalRadius = 110;
    for (int i = 0; i < 3; ++i) {
        int cx = static_cast<int>(g_ScreenWidth * 0.5f + cosf(time * 1.4f + i * 2.0f) * (g_ScreenWidth * 0.35f));
        int cy = static_cast<int>(g_ScreenHeight * 0.5f + sinf(time * 1.6f + i * 2.0f) * (g_ScreenHeight * 0.3f));
        int srcX = (cx + 400) % (g_ScreenWidth - portalRadius * 2);
        int srcY = (cy + 300) % (g_ScreenHeight - portalRadius * 2);

        HRGN hRgn = CreateEllipticRgn(cx - portalRadius, cy - portalRadius, cx + portalRadius, cy + portalRadius);
        SelectClipRgn(g_hMemDC, hRgn);
        BitBlt(g_hMemDC, cx - portalRadius, cy - portalRadius, portalRadius * 2, portalRadius * 2, g_hScratchDC, srcX, srcY, SRCCOPY);
        SelectClipRgn(g_hMemDC, NULL);
        DeleteObject(hRgn);

        HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 220, 255));
        HGDIOBJ old = SelectObject(g_hMemDC, pen);
        SelectObject(g_hMemDC, GetStockObject(NULL_BRUSH));
        Ellipse(g_hMemDC, cx - portalRadius, cy - portalRadius, cx + portalRadius, cy + portalRadius);
        SelectObject(g_hMemDC, old);
        DeleteObject(pen);
    }
}

// 12. CRT FAILURE SIMULATOR
void EffectCRTFailureSimulator(float time, float dt) {
    int roll = static_cast<int>(fmodf(time * 260.0f, static_cast<float>(g_ScreenHeight)));
    int jitter = RandInt(-6, 6);
    BitBlt(g_hMemDC, jitter, roll, g_ScreenWidth, g_ScreenHeight - roll, g_hScratchDC, 0, 0, SRCCOPY);
    BitBlt(g_hMemDC, jitter, 0, g_ScreenWidth, roll, g_hScratchDC, 0, g_ScreenHeight - roll, SRCCOPY);

    for (int y = 0; y < g_ScreenHeight; y += 4) {
        uint32_t* row = g_pFramePixels + (y * g_ScreenWidth);
        for (int x = 0; x < g_ScreenWidth; ++x) {
            row[x] = (row[x] & 0x007F7F7F) + ((RandInt(0, 20)) << 16);
        }
    }
}

// 13. VOXEL DESKTOP EXPLOSION
void EffectVoxelDesktopExplosion(float time, float dt) {
    int cols = 10, rows = 8;
    int bw = g_ScreenWidth / cols;
    int bh = g_ScreenHeight / rows;
    float t = fmodf(time * 1.5f, 3.0f);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float distFromCenter = sqrtf(powf(c - cols * 0.5f, 2) + powf(r - rows * 0.5f, 2));
            float z = 1.0f + (t * (distFromCenter * 0.2f));
            int curW = static_cast<int>(bw * z);
            int curH = static_cast<int>(bh * z);
            int curX = static_cast<int>(c * bw + (c - cols * 0.5f) * t * 25.0f);
            int curY = static_cast<int>(r * bh + (r - rows * 0.5f) * t * 25.0f);

            SetStretchBltMode(g_hMemDC, COLORONCOLOR);
            StretchBlt(g_hMemDC, curX, curY, curW, curH, g_hScratchDC, c * bw, r * bh, bw, bh, SRCCOPY);
        }
    }
}

// 14. MATRIX RECONSTRUCTION
void EffectMatrixReconstruction(float time, float dt) {
    for (int i = 0; i < g_ScreenWidth * g_ScreenHeight; i += 2) {
        g_pFramePixels[i] = ((g_pFramePixels[i] & 0x00FEFEFE) >> 1);
    }
    SetBkMode(g_hMemDC, TRANSPARENT);
    SetTextColor(g_hMemDC, RGB(40, 255, 90));
    const char glyphs[] = "0123456789ABCDEF$#@%&*";
    for (int col = 0; col < g_ScreenWidth; col += 32) {
        int streamHead = static_cast<int>(fmodf(time * 350.0f + col * 21.0f, g_ScreenHeight + 300.0f)) - 150;
        for (int step = 0; step < 8; ++step) {
            int y = streamHead - step * 18;
            if (y >= 0 && y < g_ScreenHeight - 18) {
                char ch = glyphs[(col + step + static_cast<int>(time * 10)) % (sizeof(glyphs) - 1)];
                TextOutA(g_hMemDC, col, y, &ch, 1);
            }
        }
    }
}

// 15. RGB SPECTRAL PHANTOMS
void EffectRGBSpectralPhantoms(float time, float dt) {
    int shiftR_x = static_cast<int>(sinf(time * 3.2f) * 25.0f);
    int shiftB_x = static_cast<int>(-cosf(time * 2.8f) * 25.0f);

    for (int y = 0; y < g_ScreenHeight; ++y) {
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; ++x) {
            int xR = std::clamp(x + shiftR_x, 0, g_ScreenWidth - 1);
            int xB = std::clamp(x + shiftB_x, 0, g_ScreenWidth - 1);

            uint32_t colR = g_pScratchPixels[rowIdx + xR];
            uint32_t colG = g_pScratchPixels[rowIdx + x];
            uint32_t colB = g_pScratchPixels[rowIdx + xB];

            g_pFramePixels[rowIdx + x] = (colR & 0x00FF0000) | (colG & 0x0000FF00) | (colB & 0x000000FF);
        }
    }
}

// 16. KALEIDOSCOPIC REALITY
void EffectKaleidoscopicReality(float time, float dt) {
    int cx = static_cast<int>(g_ScreenWidth * 0.5f + cosf(time * 1.7f) * (g_ScreenWidth * 0.2f));
    int cy = static_cast<int>(g_ScreenHeight * 0.5f + sinf(time * 1.5f) * (g_ScreenHeight * 0.15f));

    for (int y = 0; y < g_ScreenHeight; y += 2) {
        int dy = abs(y - cy);
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int dx = abs(x - cx);
            int sx = std::clamp((dx > dy) ? (cx + dx) : (cx + dy), 0, g_ScreenWidth - 1);
            int sy = std::clamp((dx > dy) ? (cy + dy) : (cy + dx), 0, g_ScreenHeight - 1);
            uint32_t col = g_pScratchPixels[sy * g_ScreenWidth + sx];
            g_pFramePixels[rowIdx + x] = col;
            g_pFramePixels[rowIdx + x + 1] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = col;
        }
    }
}

// 17. GRAVITY PAINT
struct PaintDrop { float x, y, vy; uint32_t col; };
static std::vector<PaintDrop> g_Paint;

void EffectGravityPaint(float time, float dt) {
    if (g_Paint.size() < 25) {
        g_Paint.push_back({ RandFloat(0, static_cast<float>(g_ScreenWidth)), 0, RandFloat(90, 240),
            static_cast<uint32_t>(RGB(RandInt(120, 255), RandInt(0, 255), RandInt(120, 255))) });
    }
    for (auto& p : g_Paint) {
        p.y += p.vy * dt;
        if (p.y > g_ScreenHeight - 5) p.y = 0;
        int ix = static_cast<int>(p.x);
        int iy = static_cast<int>(p.y);
        for (int trail = 0; trail < 18; ++trail) {
            int ty = iy - trail;
            if (ty >= 0 && ty < g_ScreenHeight && ix >= 0 && ix < g_ScreenWidth) {
                g_pFramePixels[ty * g_ScreenWidth + ix] = p.col;
            }
        }
    }
}

// 18. DESKTOP TIME ECHOES
void EffectDesktopTimeEchoes(float time, float dt) {
    if (g_EchoHistory.empty()) return;
    const auto& oldF = g_EchoHistory[(g_EchoHead + MAX_ECHOES - 8) % MAX_ECHOES];
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        int sy = (y * oldF.h) / g_ScreenHeight;
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int sx = (x * oldF.w) / g_ScreenWidth;
            uint32_t cOld = oldF.pixels[sy * oldF.w + sx];
            uint32_t cLive = g_pScratchPixels[rowIdx + x];
            uint32_t blended = BlendColor(cLive, cOld, 0.45f);
            g_pFramePixels[rowIdx + x] = blended;
            g_pFramePixels[rowIdx + x + 1] = blended;
            g_pFramePixels[rowIdx + g_ScreenWidth + x] = blended;
            g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = blended;
        }
    }
}

// 19. 3D WARP TUNNEL
void Effect3DWarpTunnel(float time, float dt) {
    float cx = g_ScreenWidth * 0.5f + cosf(time * 1.8f) * (g_ScreenWidth * 0.15f);
    float cy = g_ScreenHeight * 0.5f + sinf(time * 1.4f) * (g_ScreenHeight * 0.15f);
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        float dy = y - cy;
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            float dx = x - cx;
            float r = sqrtf(dx * dx + dy * dy) + 0.1f;
            float a = atan2f(dy, dx);
            int u = static_cast<int>(abs(a * g_ScreenWidth / 3.14159f)) % g_ScreenWidth;
            int v = static_cast<int>(abs((2500.0f / r) + (time * 600.0f))) % g_ScreenHeight;
            uint32_t col = g_pScratchPixels[v * g_ScreenWidth + u];
            g_pFramePixels[rowIdx + x] = col;
            g_pFramePixels[rowIdx + x + 1] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = col;
        }
    }
}

// 20. GEOMETRIC IMPLOSION
void EffectGeometricImplosion(float time, float dt) {
    float ax = g_ScreenWidth * 0.5f + cosf(time * 2.5f) * (g_ScreenWidth * 0.3f);
    float ay = g_ScreenHeight * 0.5f + sinf(time * 2.0f) * (g_ScreenHeight * 0.25f);
    float progress = fmodf(time * 2.0f, 1.0f);

    HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 0, 130));
    HGDIOBJ old = SelectObject(g_hMemDC, pen);
    for (int i = 0; i < 20; ++i) {
        float initAngle = i * (6.28318f / 20);
        float initDist = 400.0f * (1.0f - progress);
        int px = static_cast<int>(ax + cosf(initAngle) * initDist);
        int py = static_cast<int>(ay + sinf(initAngle) * initDist);
        POINT pts[3] = { { px, py }, { px + 20, py - 30 }, { px - 20, py - 30 } };
        Polygon(g_hMemDC, pts, 3);
    }
    SelectObject(g_hMemDC, old);
    DeleteObject(pen);
}

// 21. X-RAY SCANNER
void EffectXRayScanner(float time, float dt) {
    int scanX = static_cast<int>((sinf(time * 2.5f) + 1.0f) * 0.5f * (g_ScreenWidth - 200));
    int scanW = 200;

    for (int y = 0; y < g_ScreenHeight; ++y) {
        int rowIdx = y * g_ScreenWidth;
        for (int x = scanX; x < scanX + scanW; ++x) {
            uint32_t c = g_pScratchPixels[rowIdx + x];
            uint32_t cRight = g_pScratchPixels[rowIdx + std::min(x + 1, g_ScreenWidth - 1)];
            int edge = abs(static_cast<int>(c & 0xFF) - static_cast<int>(cRight & 0xFF));
            g_pFramePixels[rowIdx + x] = (edge > 20) ? 0x0000FFFF : 0x00200040;
        }
    }
}

// 22. SCREEN PEEL & FOLD
void EffectScreenPeelAndFold(float time, float dt) {
    float t = (sinf(time * 2.2f) + 1.0f) * 0.5f;
    int peelX = static_cast<int>(g_ScreenWidth * 0.5f * t);
    int peelY = static_cast<int>(g_ScreenHeight * 0.5f * t);

    POINT pts[3] = { { 0, 0 }, { peelX, 0 }, { 0, peelY } };
    HRGN hRgn = CreatePolygonRgn(pts, 3, WINDING);
    SelectClipRgn(g_hMemDC, hRgn);

    HBRUSH b = CreateHatchBrush(HS_DIAGCROSS, RGB(0, 255, 180));
    RECT r = { 0, 0, peelX, peelY };
    FillRect(g_hMemDC, &r, b);
    DeleteObject(b);

    SelectClipRgn(g_hMemDC, NULL);
    DeleteObject(hRgn);
}

// 23. HOLOGRAPHIC GLITCH OBJECTS
void EffectHolographicGlitch(float time, float dt) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 255));
    HGDIOBJ old = SelectObject(g_hMemDC, pen);

    for (int k = 0; k < 4; ++k) {
        float ox = g_ScreenWidth * (0.25f + 0.2f * k);
        float oy = g_ScreenHeight * 0.5f + sinf(time * 2.0f + k) * 150.0f;
        float size = 50.0f;
        float ang = time * 3.0f + k;

        POINT box[8];
        for (int i = 0; i < 8; ++i) {
            float vx = (i & 1) ? size : -size;
            float vy = (i & 2) ? size : -size;
            float vz = (i & 4) ? size : -size;
            float rx = vx * cosf(ang) - vz * sinf(ang);
            float rz = vx * sinf(ang) + vz * cosf(ang) + 200.0f;
            box[i].x = static_cast<LONG>(ox + (rx * 160.0f) / rz);
            box[i].y = static_cast<LONG>(oy + (vy * 160.0f) / rz);
        }
        int edges[12][2] = { {0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7} };
        for (auto& e : edges) {
            MoveToEx(g_hMemDC, box[e[0]].x, box[e[0]].y, NULL);
            LineTo(g_hMemDC, box[e[1]].x, box[e[1]].y);
        }
    }
    SelectObject(g_hMemDC, old);
    DeleteObject(pen);
}

// 24. SYNESTHESIA STORM
void EffectSynesthesiaStorm(float time, float dt) {
    int bars = 24;
    int bw = g_ScreenWidth / bars;
    for (int i = 0; i < bars; ++i) {
        float hNorm = (sinf(time * 7.0f + i * 0.5f) + 1.0f) * 0.5f;
        int barH = static_cast<int>(hNorm * (g_ScreenHeight * 0.35f));
        RECT r = { i * bw, g_ScreenHeight - barH, (i + 1) * bw - 2, g_ScreenHeight };
        HBRUSH b = CreateSolidBrush(RGB((i * 15) % 255, 255 - (i * 9) % 255, 200));
        FillRect(g_hMemDC, &r, b);
        DeleteObject(b);
    }
}

// 25. REALITY COLLAPSE FINALE
void EffectRealityCollapseFinale(float time, float dt) {
    float cycle = fmodf(time, 4.0f);
    float scale = (std::max)(0.05f, 1.0f - (cycle / 4.0f));

    int nw = static_cast<int>(g_ScreenWidth * scale);
    int nh = static_cast<int>(g_ScreenHeight * scale);
    int ox = (g_ScreenWidth - nw) / 2 + RandInt(-10, 10);
    int oy = (g_ScreenHeight - nh) / 2;

    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    StretchBlt(g_hMemDC, ox, oy, nw, nh, g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, SRCCOPY);
    if (cycle > 2.5f) {
        PatBlt(g_hMemDC, ox, oy, nw, nh, DSTINVERT);
    }
}

// ============================================================================
// REGISTRY
// ============================================================================
typedef void (*PayloadFn)(float, float);

struct PayloadRecord {
    const char* name;
    PayloadFn fn;
};

static const PayloadRecord g_Payloads[25] = {
    { "Pixel Fireworks",             EffectPixelFireworks },
    { "Chaos Rectangles",            EffectChaosRectangles },
    { "Black Hole Singularities",    EffectBlackHoleSingularities },
    { "Desktop Glass Shatter",       EffectDesktopGlassShatter },
    { "Living Digital Infection",    EffectLivingInfection },
    { "Impossible Windows",          EffectImpossibleWindows },
    { "Liquid Desktop",              EffectLiquidDesktop },
    { "Pixel Sorting Anomaly",       EffectPixelSortingAnomaly },
    { "Neon Fractal Invasion",       EffectNeonFractalInvasion },
    { "Ghost Cursor Swarm",          EffectGhostCursorSwarm },
    { "Spatial Portal Network",      EffectSpatialPortalNetwork },
    { "CRT Failure Simulator",       EffectCRTFailureSimulator },
    { "Voxel Desktop Explosion",     EffectVoxelDesktopExplosion },
    { "Matrix Reconstruction",       EffectMatrixReconstruction },
    { "RGB Spectral Phantoms",       EffectRGBSpectralPhantoms },
    { "Kaleidoscopic Reality",       EffectKaleidoscopicReality },
    { "Gravity Paint",               EffectGravityPaint },
    { "Desktop Time Echoes",         EffectDesktopTimeEchoes },
    { "3D Warp Tunnel",              Effect3DWarpTunnel },
    { "Geometric Implosion",         EffectGeometricImplosion },
    { "X-Ray Scanner",               EffectXRayScanner },
    { "Screen Peel & Fold",          EffectScreenPeelAndFold },
    { "Holographic Glitch Objects",  EffectHolographicGlitch },
    { "Synesthesia Storm",           EffectSynesthesiaStorm },
    { "Reality Collapse Finale",     EffectRealityCollapseFinale }
};

// ============================================================================
// WIN32 RUNTIME & STACKING SCHEDULER
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_HOTKEY && wParam == 101) { // Emergency Exit
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    const wchar_t CLASS_NAME[] = L"StackedChaosOverlay";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"Stacked Chaos Overlay", WS_POPUP,
        0, 0, g_ScreenWidth, g_ScreenHeight,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!hWnd) return 1;

    SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);
    RegisterHotKey(hWnd, 101, MOD_CONTROL | MOD_SHIFT, 'Q');

    // Framebuffer setup
    HDC hScreenDC = GetDC(NULL);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_ScreenWidth;
    bmi.bmiHeader.biHeight = -g_ScreenHeight; // Top-Down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // 1. Output Composite Buffer
    g_hMemDC = CreateCompatibleDC(hScreenDC);
    g_hDIB = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pFramePixels), NULL, 0);
    SelectObject(g_hMemDC, g_hDIB);

    // 2. Inter-Payload Scratch Buffer
    g_hScratchDC = CreateCompatibleDC(hScreenDC);
    g_hScratchBmp = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pScratchPixels), NULL, 0);
    SelectObject(g_hScratchDC, g_hScratchBmp);

    // 3. Raw Live Screen Capture Buffer
    g_hBaseDC = CreateCompatibleDC(hScreenDC);
    g_hBaseBmp = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pBasePixels), NULL, 0);
    SelectObject(g_hBaseDC, g_hBaseBmp);

    ReleaseDC(NULL, hScreenDC);
    ShowWindow(hWnd, SW_SHOWNOACTIVATE);

    // Allocate downsampled History Buffer for Effect 18
    int echoW = g_ScreenWidth / 2;
    int echoH = g_ScreenHeight / 2;
    g_EchoHistory.resize(MAX_ECHOES);
    for (auto& f : g_EchoHistory) {
        f.w = echoW; f.h = echoH;
        f.pixels.resize(echoW * echoH, 0);
    }

    // Prepare randomized sequence of 10 unique payloads
    std::vector<int> pool(25);
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), g_Rng);

    std::vector<int> activePayloads;
    activePayloads.push_back(pool[0]); // Start with 1 random payload immediately
    int payloadsAdded = 1;

    using clock = std::chrono::high_resolution_clock;
    auto startTime = clock::now();
    auto lastFrame = clock::now();

    float intervalTimer = 0.0f;
    const float STAGE_INTERVAL = 10.0f; // 10 seconds per stage

    POINT ptZero = { 0, 0 };
    SIZE szWnd = { g_ScreenWidth, g_ScreenHeight };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, 0 };

    MSG msg = {};
    while (g_Running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Secondary Emergency Exit: Ctrl + Shift + Esc
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
            (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
            break;
        }

        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - lastFrame).count();
        float totalTime = std::chrono::duration<float>(now - startTime).count();
        lastFrame = now;

        // ====================================================================
        // STAGE LIFECYCLE: 10s intervals -> add payload -> up to 10 -> stop
        // ====================================================================
        intervalTimer += dt;
        if (intervalTimer >= STAGE_INTERVAL) {
            intervalTimer = 0.0f;
            if (payloadsAdded < 10) {
                activePayloads.push_back(pool[payloadsAdded]);
                payloadsAdded++;
            } else {
                // All 10 payloads have run together for 10 seconds.
                // Stop everything and restore the screen to its original state.
                g_Running = false;
                break;
            }
        }

        // 1. INGEST LIVE DESKTOP
        HDC hLiveDC = GetDC(NULL);
        BitBlt(g_hBaseDC, 0, 0, g_ScreenWidth, g_ScreenHeight, hLiveDC, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hLiveDC);

        // Record history frame for time echoes
        static float echoTick = 0.0f;
        echoTick += dt;
        if (echoTick >= 0.08f) {
            echoTick = 0.0f;
            auto& f = g_EchoHistory[g_EchoHead % MAX_ECHOES];
            for (int y = 0; y < echoH; ++y) {
                int sy = (y * g_ScreenHeight) / echoH;
                for (int x = 0; x < echoW; ++x) {
                    int sx = (x * g_ScreenWidth) / echoW;
                    f.pixels[y * echoW + x] = g_pBasePixels[sy * g_ScreenWidth + sx];
                }
            }
            g_EchoHead++;
        }

        // Initialize the working buffer with the untouched live desktop
        BitBlt(g_hMemDC, 0, 0, g_ScreenWidth, g_ScreenHeight, g_hBaseDC, 0, 0, SRCCOPY);

        // 2. CHAIN-EXECUTE ALL ACTIVE PAYLOADS IN SEQUENCE
        // Each payload distorts the result of all previous payloads without wiping the canvas.
        for (int pIdx : activePayloads) {
            // Snapshot current composite state into scratch buffer
            BitBlt(g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, g_hMemDC, 0, 0, SRCCOPY);
            // Execute payload (reads from g_hScratchDC, writes into g_hMemDC)
            g_Payloads[pIdx].fn(totalTime, dt);
        }

        // 3. FLUSH LAYERED WINDOW TO DWM
        UpdateLayeredWindow(hWnd, NULL, &ptZero, &szWnd, g_hMemDC, &ptZero, 0, &blend, ULW_ALPHA);

        // ~60 FPS Frame Limiting
        Sleep(16);
    }

    // Teardown: Immediate restoration of pristine desktop
    UnregisterHotKey(hWnd, 101);
    DeleteObject(g_hDIB);
    DeleteDC(g_hMemDC);
    DeleteObject(g_hScratchBmp);
    DeleteDC(g_hScratchDC);
    DeleteObject(g_hBaseBmp);
    DeleteDC(g_hBaseDC);
    DestroyWindow(hWnd);

    // Invalidate screen to force Windows to redraw underneath cleanly
    RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);

    return 0;
}