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
// ULTRA-FAST INLINE PRNG & UTILITIES
// ============================================================================
static uint32_t g_FastState = 0x8A5CD791;
inline uint32_t FastRand() {
    g_FastState ^= g_FastState << 13;
    g_FastState ^= g_FastState >> 17;
    g_FastState ^= g_FastState << 5;
    return g_FastState;
}
inline int FastRandInt(int min, int max) {
    if (min >= max) return min;
    return min + static_cast<int>(FastRand() % (max - min + 1));
}
inline float FastRandFloat(float min, float max) {
    return min + (FastRand() * (1.0f / 4294967295.0f)) * (max - min);
}

// Inline Fast Blend
inline uint32_t BlendColor(uint32_t c1, uint32_t c2, float t) {
    uint32_t inv = static_cast<uint32_t>((1.0f - t) * 256.0f);
    uint32_t w = static_cast<uint32_t>(t * 256.0f);

    uint32_t rb1 = c1 & 0x00FF00FF;
    uint32_t g1  = c1 & 0x0000FF00;
    uint32_t rb2 = c2 & 0x00FF00FF;
    uint32_t g2  = c2 & 0x0000FF00;

    uint32_t rb = ((rb1 * inv + rb2 * w) >> 8) & 0x00FF00FF;
    uint32_t g  = ((g1 * inv + g2 * w) >> 8) & 0x0000FF00;
    return rb | g;
}

// Fast Direct Rasterization Primitives on 32-bit DIB
inline void FastDrawPixel(uint32_t* buf, int w, int h, int x, int y, uint32_t color) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        buf[y * w + x] = color;
    }
}

inline void FastDrawLine(uint32_t* buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) buf[y0 * w + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

inline void FastFillRect(uint32_t* buf, int w, int h, int rx, int ry, int rw, int rh, uint32_t color) {
    int x0 = (std::max)(0, rx);
    int y0 = (std::max)(0, ry);
    int x1 = (std::min)(w, rx + rw);
    int y1 = (std::min)(h, ry + rh);
    for (int y = y0; y < y1; ++y) {
        uint32_t* row = buf + y * w;
        for (int x = x0; x < x1; ++x) {
            row[x] = color;
        }
    }
}

inline void FastInvertRect(uint32_t* buf, int w, int h, int rx, int ry, int rw, int rh) {
    int x0 = (std::max)(0, rx);
    int y0 = (std::max)(0, ry);
    int x1 = (std::min)(w, rx + rw);
    int y1 = (std::min)(h, ry + rh);
    for (int y = y0; y < y1; ++y) {
        uint32_t* row = buf + y * w;
        for (int x = x0; x < x1; ++x) {
            row[x] = ~row[x] & 0x00FFFFFF;
        }
    }
}

// ============================================================================
// GLOBAL APPLICATION STATE & PIPELINE BUFFERS
// ============================================================================
static bool g_Running = true;
static int g_ScreenWidth = 0;
static int g_ScreenHeight = 0;

static HDC g_hMemDC = nullptr;
static HBITMAP g_hDIB = nullptr;
static uint32_t* g_pFramePixels = nullptr;

static HDC g_hScratchDC = nullptr;
static HBITMAP g_hScratchBmp = nullptr;
static uint32_t* g_pScratchPixels = nullptr;

static HDC g_hBaseDC = nullptr;
static HBITMAP g_hBaseBmp = nullptr;
static uint32_t* g_pBasePixels = nullptr;

struct EchoFrame {
    int w, h;
    std::vector<uint32_t> pixels;
};
static const size_t MAX_ECHOES = 16;
static std::vector<EchoFrame> g_EchoHistory;
static size_t g_EchoHead = 0;

// ============================================================================
// 25 OPTIMIZED & HYPER-RANDOMIZED PAYLOADS
// ============================================================================

// 1. PIXEL FIREWORKS
struct Particle { float x, y, vx, vy; uint32_t col; float life; };
static std::vector<Particle> g_Fireworks;

void EffectPixelFireworks(float time, float dt) {
    if (FastRandInt(0, 5) == 0 || g_Fireworks.empty()) {
        float ox = FastRandFloat(g_ScreenWidth * 0.05f, g_ScreenWidth * 0.95f);
        float oy = FastRandFloat(g_ScreenHeight * 0.05f, g_ScreenHeight * 0.65f);
        uint32_t col = (FastRand() & 0x00FFFFFF) | 0x00404040;
        int count = FastRandInt(30, 70);
        float baseSpd = FastRandFloat(100.0f, 450.0f);
        for (int i = 0; i < count; ++i) {
            float ang = FastRandFloat(0.0f, 6.28318f);
            float spd = baseSpd * FastRandFloat(0.3f, 1.2f);
            g_Fireworks.push_back({ ox, oy, cosf(ang) * spd, sinf(ang) * spd, col, FastRandFloat(0.4f, 1.4f) });
        }
    }
    for (size_t i = 0; i < g_Fireworks.size();) {
        auto& p = g_Fireworks[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 320.0f * dt;
        p.life -= dt;
        int ix = static_cast<int>(p.x);
        int iy = static_cast<int>(p.y);
        if (p.life > 0.0f && ix >= 0 && ix < g_ScreenWidth - 2 && iy >= 0 && iy < g_ScreenHeight - 2) {
            FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ix, iy, 3, 3, p.col);
            ++i;
        } else {
            g_Fireworks[i] = g_Fireworks.back();
            g_Fireworks.pop_back();
        }
    }
}

// 2. CHAOS RECTANGLES
void EffectChaosRectangles(float time, float dt) {
    int rectCount = FastRandInt(10, 25);
    for (int i = 0; i < rectCount; ++i) {
        int w = FastRandInt(20, 260);
        int h = FastRandInt(20, 260);
        int x = FastRandInt(0, g_ScreenWidth - w);
        int y = FastRandInt(0, g_ScreenHeight - h);
        int mode = FastRandInt(0, 3);
        if (mode == 0) {
            FastInvertRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, w, h);
        } else if (mode == 1) {
            FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, w, h, FastRand() & 0x00FFFFFF);
        } else {
            uint32_t col = FastRand() & 0x00FFFFFF;
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, x + w, y, col);
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x + w, y, x + w, y + h, col);
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x + w, y + h, x, y + h, col);
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y + h, x, y, col);
        }
    }
}

// 3. BLACK HOLE SINGULARITIES (Bounded Optimization)
void EffectBlackHoleSingularities(float time, float dt) {
    float hx = g_ScreenWidth * (0.5f + 0.35f * sinf(time * 1.8f + 1.2f));
    float hy = g_ScreenHeight * (0.5f + 0.30f * cosf(time * 1.3f));
    float radius = 220.0f + 40.0f * sinf(time * 3.0f);
    float twist = 4.2f + sinf(time * 2.0f) * 1.5f;

    int minX = (std::max)(0, static_cast<int>(hx - radius));
    int maxX = (std::min)(g_ScreenWidth - 2, static_cast<int>(hx + radius));
    int minY = (std::max)(0, static_cast<int>(hy - radius));
    int maxY = (std::min)(g_ScreenHeight - 2, static_cast<int>(hy + radius));

    float radSq = radius * radius;
    for (int y = minY; y <= maxY; y += 2) {
        float dy = y - hy;
        int rowIdx = y * g_ScreenWidth;
        for (int x = minX; x <= maxX; x += 2) {
            float dx = x - hx;
            float distSq = dx * dx + dy * dy;
            if (distSq < radSq) {
                float dist = sqrtf(distSq);
                float factor = 1.0f - (dist / radius);
                float a = atan2f(dy, dx) + factor * twist;
                float newDist = dist * (1.0f - factor * 0.55f);
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
struct Shard { int sx, sy, w, h; float vx, vy; };
static std::vector<Shard> g_Shards;

void EffectDesktopGlassShatter(float time, float dt) {
    if (g_Shards.empty() || fmodf(time, 4.0f) < dt) {
        g_Shards.clear();
        int cols = 9, rows = 7;
        int sw = g_ScreenWidth / cols;
        int sh = g_ScreenHeight / rows;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                g_Shards.push_back({ c * sw, r * sh, sw, sh, FastRandFloat(-120.0f, 120.0f), FastRandFloat(40.0f, 280.0f) });
            }
        }
    }
    float cycle = fmodf(time, 4.0f);
    for (auto& s : g_Shards) {
        int dx = std::clamp(static_cast<int>(s.sx + s.vx * cycle), 0, g_ScreenWidth - s.w);
        int dy = std::clamp(static_cast<int>(s.sy + s.vy * cycle * cycle), 0, g_ScreenHeight - s.h);
        BitBlt(g_hMemDC, dx, dy, s.w - 4, s.h - 4, g_hScratchDC, s.sx, s.sy, SRCCOPY);
    }
}

// 5. LIVING DIGITAL INFECTION (Direct Fast Lines)
void EffectLivingInfection(float time, float dt) {
    int roots = 5;
    uint32_t colors[] = { 0x0000FF7F, 0x00FF0055, 0x0000E5FF, 0x00FFD700, 0x009900FF };
    for (int root = 0; root < roots; ++root) {
        int cx = FastRandInt(0, g_ScreenWidth);
        int cy = (root % 2 == 0) ? 0 : g_ScreenHeight - 1;
        uint32_t col = colors[root % 5];
        for (int i = 0; i < 40; ++i) {
            int nx = cx + FastRandInt(-22, 22);
            int ny = cy + ((root % 2 == 0) ? FastRandInt(6, 26) : FastRandInt(-26, -6));
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, cx, cy, nx, ny, col);
            cx = nx; cy = ny;
        }
    }
}

// 6. IMPOSSIBLE WINDOWS
struct GlitchWin { float x, y, vx, vy; int w, h; const char* title; uint32_t col; };
static std::vector<GlitchWin> g_GlitchWins;

void EffectImpossibleWindows(float time, float dt) {
    static const char* titles[] = {
        "CRITICAL_PROCESS_DIED", "PAGE_FAULT_IN_NONPAGED_AREA",
        "SYSTEM_SERVICE_EXCEPTION", "STACK_HASH_CORRUPTION",
        "QUANTUM_CORE_HALTED", "HEAP_SEGMENT_OVERFLOW"
    };
    if (g_GlitchWins.empty()) {
        for (int i = 0; i < 5; ++i) {
            g_GlitchWins.push_back({
                FastRandFloat(50, g_ScreenWidth - 350), FastRandFloat(50, g_ScreenHeight - 250),
                FastRandFloat(-250.0f, 250.0f), FastRandFloat(-220.0f, 220.0f),
                FastRandInt(220, 340), FastRandInt(140, 220),
                titles[i % 6],
                (FastRand() & 0x007F1F1F) | 0x00800000
            });
        }
    }
    for (auto& w : g_GlitchWins) {
        w.x += w.vx * dt;
        w.y += w.vy * dt;
        if (w.x < 0 || w.x + w.w > g_ScreenWidth) { w.vx = -w.vx; w.x = std::clamp(w.x, 0.0f, (float)(g_ScreenWidth - w.w)); }
        if (w.y < 0 || w.y + w.h > g_ScreenHeight) { w.vy = -w.vy; w.y = std::clamp(w.y, 0.0f, (float)(g_ScreenHeight - w.h)); }

        int ix = static_cast<int>(w.x), iy = static_cast<int>(w.y);
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ix, iy, w.w, 24, w.col);
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ix, iy + 24, w.w, w.h - 24, 0x001A1A1A);

        SetBkMode(g_hMemDC, TRANSPARENT);
        SetTextColor(g_hMemDC, RGB(255, 255, 255));
        TextOutA(g_hMemDC, ix + 8, iy + 4, w.title, static_cast<int>(strlen(w.title)));
        StretchBlt(g_hMemDC, ix + 2, iy + 26, w.w - 4, w.h - 28, g_hScratchDC, ix % 100, iy % 100, g_ScreenWidth / 3, g_ScreenHeight / 3, SRCCOPY);
    }
}

// 7. LIQUID DESKTOP (Precomputed 1D Wave Lookup)
void EffectLiquidDesktop(float time, float dt) {
    float waveSpeed = 6.5f;
    float freqX = 0.024f + 0.006f * sinf(time * 0.7f);
    float freqY = 0.028f + 0.005f * cosf(time * 0.9f);

    std::vector<int> xShifts(g_ScreenHeight);
    for (int y = 0; y < g_ScreenHeight; ++y) {
        xShifts[y] = static_cast<int>(sinf((y * freqX) + (time * waveSpeed)) * 18.0f);
    }
    std::vector<int> yShifts(g_ScreenWidth);
    for (int x = 0; x < g_ScreenWidth; ++x) {
        yShifts[x] = static_cast<int>(cosf((x * freqY) + (time * waveSpeed)) * 14.0f);
    }

    for (int y = 0; y < g_ScreenHeight; y += 2) {
        int xShift = xShifts[y];
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int sx = std::clamp(x + xShift, 0, g_ScreenWidth - 1);
            int sy = std::clamp(y + yShifts[x], 0, g_ScreenHeight - 1);
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
    int bands = FastRandInt(6, 14);
    for (int b = 0; b < bands; ++b) {
        int boxW = FastRandInt(80, 350);
        int boxH = FastRandInt(20, 80);
        int sx = FastRandInt(0, g_ScreenWidth - boxW);
        int sy = FastRandInt(0, g_ScreenHeight - boxH);
        int sortMode = FastRandInt(0, 2);

        for (int y = sy; y < sy + boxH; ++y) {
            uint32_t* row = g_pFramePixels + (y * g_ScreenWidth) + sx;
            if (sortMode == 0) {
                std::sort(row, row + boxW, [](uint32_t a, uint32_t b) { return (a & 0xFF) < (b & 0xFF); });
            } else if (sortMode == 1) {
                std::sort(row, row + boxW, [](uint32_t a, uint32_t b) { return ((a >> 16) & 0xFF) > ((b >> 16) & 0xFF); });
            } else {
                std::sort(row, row + boxW, [](uint32_t a, uint32_t b) { return ((a >> 8) & 0xFF) < ((b >> 8) & 0xFF); });
            }
        }
    }
}

// 9. NEON FRACTAL INVASION
void EffectNeonFractalInvasion(float time, float dt) {
    float cr = -0.72f + sinf(time * 0.45f) * 0.12f;
    float ci = 0.26f + cosf(time * 0.35f) * 0.12f;
    for (int y = 0; y < g_ScreenHeight; y += 3) {
        float zy0 = 1.3f * (y - g_ScreenHeight / 2) / (0.5f * g_ScreenHeight);
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 3) {
            float zx = 1.3f * (x - g_ScreenWidth / 2) / (0.5f * g_ScreenHeight);
            float zy = zy0;
            int i = 14;
            while (zx * zx + zy * zy < 4.0f && i > 0) {
                float tmp = zx * zx - zy * zy + cr;
                zy = 2.0f * zx * zy + ci;
                zx = tmp;
                --i;
            }
            uint32_t neon = (i == 0) ? 0x000000 : ((i * 18) << 16) | ((i * 10) << 8) | (i * 24);
            uint32_t col = BlendColor(g_pScratchPixels[rowIdx + x], neon, 0.45f);
            for (int dy = 0; dy < 3 && y + dy < g_ScreenHeight; ++dy) {
                for (int dx = 0; dx < 3 && x + dx < g_ScreenWidth; ++dx) {
                    g_pFramePixels[(y + dy) * g_ScreenWidth + (x + dx)] = col;
                }
            }
        }
    }
}

// 10. GHOST CURSOR SWARM
void EffectGhostCursorSwarm(float time, float dt) {
    POINT p; GetCursorPos(&p);
    HCURSOR hArr = LoadCursor(NULL, IDC_ARROW);
    int count = 32;
    for (int i = 0; i < count; ++i) {
        float a = time * 3.2f + (i * 0.35f) + sinf(time + i);
        float r = 30.0f + (i * 18.0f) + 15.0f * sinf(time * 2.0f + i);
        int cx = static_cast<int>(p.x + cosf(a) * r);
        int cy = static_cast<int>(p.y + sinf(a) * r);
        DrawIcon(g_hMemDC, cx, cy, hArr);
    }
}

// 11. SPATIAL PORTAL NETWORK
void EffectSpatialPortalNetwork(float time, float dt) {
    for (int i = 0; i < 3; ++i) {
        int radius = 90 + static_cast<int>(25.0f * sinf(time * 2.0f + i));
        int cx = static_cast<int>(g_ScreenWidth * 0.5f + cosf(time * 1.5f + i * 2.1f) * (g_ScreenWidth * 0.35f));
        int cy = static_cast<int>(g_ScreenHeight * 0.5f + sinf(time * 1.7f + i * 1.9f) * (g_ScreenHeight * 0.3f));
        int srcX = (cx + 350 + i * 120) % (g_ScreenWidth - radius * 2);
        int srcY = (cy + 250 + i * 90) % (g_ScreenHeight - radius * 2);

        int minX = (std::max)(0, cx - radius), maxX = (std::min)(g_ScreenWidth - 1, cx + radius);
        int minY = (std::max)(0, cy - radius), maxY = (std::min)(g_ScreenHeight - 1, cy + radius);
        int rSq = radius * radius;

        for (int y = minY; y <= maxY; ++y) {
            int dy = y - cy;
            for (int x = minX; x <= maxX; ++x) {
                int dx = x - cx;
                int dSq = dx * dx + dy * dy;
                if (dSq <= rSq) {
                    if (abs(dSq - rSq) < (radius * 4)) {
                        g_pFramePixels[y * g_ScreenWidth + x] = 0x0000E5FF;
                    } else {
                        int sx = srcX + (dx + radius);
                        int sy = srcY + (dy + radius);
                        g_pFramePixels[y * g_ScreenWidth + x] = g_pScratchPixels[sy * g_ScreenWidth + sx];
                    }
                }
            }
        }
    }
}

// 12. CRT FAILURE SIMULATOR
void EffectCRTFailureSimulator(float time, float dt) {
    int roll = static_cast<int>(fmodf(time * 300.0f, static_cast<float>(g_ScreenHeight)));
    int jitter = FastRandInt(-8, 8);

    BitBlt(g_hMemDC, jitter, roll, g_ScreenWidth, g_ScreenHeight - roll, g_hScratchDC, 0, 0, SRCCOPY);
    BitBlt(g_hMemDC, jitter, 0, g_ScreenWidth, roll, g_hScratchDC, 0, g_ScreenHeight - roll, SRCCOPY);

    for (int y = 0; y < g_ScreenHeight; y += 3) {
        uint32_t* row = g_pFramePixels + (y * g_ScreenWidth);
        uint32_t noise = (FastRand() & 0x001F1F1F);
        for (int x = 0; x < g_ScreenWidth; ++x) {
            row[x] = ((row[x] & 0x00FEFEFE) >> 1) + noise;
        }
    }
}

// 13. VOXEL DESKTOP EXPLOSION
void EffectVoxelDesktopExplosion(float time, float dt) {
    int cols = 12, rows = 9;
    int bw = g_ScreenWidth / cols;
    int bh = g_ScreenHeight / rows;
    float t = fmodf(time * 1.8f, 3.0f);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float dc = c - cols * 0.5f;
            float dr = r - rows * 0.5f;
            float dist = sqrtf(dc * dc + dr * dr);
            float scale = 1.0f + (t * (dist * 0.22f));
            int curW = static_cast<int>(bw * scale);
            int curH = static_cast<int>(bh * scale);
            int curX = static_cast<int>(c * bw + dc * t * 30.0f);
            int curY = static_cast<int>(r * bh + dr * t * 30.0f);

            SetStretchBltMode(g_hMemDC, COLORONCOLOR);
            StretchBlt(g_hMemDC, curX, curY, curW, curH, g_hScratchDC, c * bw, r * bh, bw, bh, SRCCOPY);
        }
    }
}

// 14. MATRIX RECONSTRUCTION
void EffectMatrixReconstruction(float time, float dt) {
    for (int i = 0; i < g_ScreenWidth * g_ScreenHeight; i += 3) {
        g_pFramePixels[i] = (g_pFramePixels[i] & 0x007F7F7F);
    }
    SetBkMode(g_hMemDC, TRANSPARENT);
    SetTextColor(g_hMemDC, RGB(40, 255, 120));
    const char glyphs[] = "0123456789ABCDEF$#@%&*+-=";
    for (int col = 0; col < g_ScreenWidth; col += 24) {
        int streamHead = static_cast<int>(fmodf(time * 420.0f + col * 27.0f, g_ScreenHeight + 350.0f)) - 160;
        for (int step = 0; step < 9; ++step) {
            int y = streamHead - step * 18;
            if (y >= 0 && y < g_ScreenHeight - 18) {
                char ch = glyphs[(col + step + static_cast<int>(time * 15)) % (sizeof(glyphs) - 1)];
                TextOutA(g_hMemDC, col, y, &ch, 1);
            }
        }
    }
}

// 15. RGB SPECTRAL PHANTOMS (Chromatic Aberration)
void EffectRGBSpectralPhantoms(float time, float dt) {
    int shiftR_x = static_cast<int>(sinf(time * 3.5f) * 28.0f);
    int shiftR_y = static_cast<int>(cosf(time * 2.8f) * 8.0f);
    int shiftB_x = static_cast<int>(-cosf(time * 3.1f) * 28.0f);
    int shiftB_y = static_cast<int>(-sinf(time * 2.4f) * 8.0f);

    for (int y = 0; y < g_ScreenHeight; ++y) {
        int yR = std::clamp(y + shiftR_y, 0, g_ScreenHeight - 1);
        int yB = std::clamp(y + shiftB_y, 0, g_ScreenHeight - 1);
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; ++x) {
            int xR = std::clamp(x + shiftR_x, 0, g_ScreenWidth - 1);
            int xB = std::clamp(x + shiftB_x, 0, g_ScreenWidth - 1);

            uint32_t colR = g_pScratchPixels[yR * g_ScreenWidth + xR];
            uint32_t colG = g_pScratchPixels[rowIdx + x];
            uint32_t colB = g_pScratchPixels[yB * g_ScreenWidth + xB];

            g_pFramePixels[rowIdx + x] = (colR & 0x00FF0000) | (colG & 0x0000FF00) | (colB & 0x000000FF);
        }
    }
}

// 16. KALEIDOSCOPIC REALITY
void EffectKaleidoscopicReality(float time, float dt) {
    int cx = static_cast<int>(g_ScreenWidth * 0.5f + cosf(time * 1.8f) * (g_ScreenWidth * 0.22f));
    int cy = static_cast<int>(g_ScreenHeight * 0.5f + sinf(time * 1.6f) * (g_ScreenHeight * 0.18f));

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
struct PaintDrop { float x, y, vy; uint32_t col; int thickness; };
static std::vector<PaintDrop> g_Paint;

void EffectGravityPaint(float time, float dt) {
    if (g_Paint.size() < 35) {
        g_Paint.push_back({
            FastRandFloat(0, static_cast<float>(g_ScreenWidth)), 0,
            FastRandFloat(120.0f, 380.0f),
            (FastRand() & 0x00FFFFFF) | 0x00550055,
            FastRandInt(1, 3)
        });
    }
    for (auto& p : g_Paint) {
        p.y += p.vy * dt;
        if (p.y > g_ScreenHeight - 5) {
            p.y = 0;
            p.x = FastRandFloat(0, static_cast<float>(g_ScreenWidth));
        }
        int ix = static_cast<int>(p.x);
        int iy = static_cast<int>(p.y);
        for (int trail = 0; trail < 24; ++trail) {
            int ty = iy - trail;
            if (ty >= 0 && ty < g_ScreenHeight) {
                for (int tx = 0; tx < p.thickness; ++tx) {
                    if (ix + tx < g_ScreenWidth) g_pFramePixels[ty * g_ScreenWidth + (ix + tx)] = p.col;
                }
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
    float cx = g_ScreenWidth * 0.5f + cosf(time * 2.0f) * (g_ScreenWidth * 0.18f);
    float cy = g_ScreenHeight * 0.5f + sinf(time * 1.5f) * (g_ScreenHeight * 0.18f);
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        float dy = y - cy;
        int rowIdx = y * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            float dx = x - cx;
            float r = sqrtf(dx * dx + dy * dy) + 0.1f;
            float a = atan2f(dy, dx);
            int u = static_cast<int>(abs(a * g_ScreenWidth / 3.14159f)) % g_ScreenWidth;
            int v = static_cast<int>(abs((2600.0f / r) + (time * 700.0f))) % g_ScreenHeight;
            uint32_t col = g_pScratchPixels[v * g_ScreenWidth + u];
            g_pFramePixels[rowIdx + x] = col;
            g_pFramePixels[rowIdx + x + 1] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x] = col;
            g_pFramePixels[rowIdx + g_ScreenWidth + x + 1] = col;
        }
    }
}

// 20. GEOMETRIC IMPLOSION (Direct Fast Lines)
void EffectGeometricImplosion(float time, float dt) {
    float ax = g_ScreenWidth * 0.5f + cosf(time * 2.2f) * (g_ScreenWidth * 0.3f);
    float ay = g_ScreenHeight * 0.5f + sinf(time * 1.9f) * (g_ScreenHeight * 0.25f);
    float progress = fmodf(time * 2.0f, 1.0f);
    int sides = 18;

    for (int i = 0; i < sides; ++i) {
        float initAngle = i * (6.28318f / sides) + time;
        float initDist = 450.0f * (1.0f - progress);
        int px = static_cast<int>(ax + cosf(initAngle) * initDist);
        int py = static_cast<int>(ay + sinf(initAngle) * initDist);
        int p2x = static_cast<int>(ax + cosf(initAngle + 0.3f) * (initDist * 0.8f));
        int p2y = static_cast<int>(ay + sinf(initAngle + 0.3f) * (initDist * 0.8f));
        FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, px, py, p2x, p2y, 0x00FF007F);
    }
}

// 21. X-RAY SCANNER
void EffectXRayScanner(float time, float dt) {
    int scanX = static_cast<int>((sinf(time * 2.8f) + 1.0f) * 0.5f * (g_ScreenWidth - 240));
    int scanW = 240;

    for (int y = 0; y < g_ScreenHeight; ++y) {
        int rowIdx = y * g_ScreenWidth;
        for (int x = scanX; x < scanX + scanW; ++x) {
            uint32_t c = g_pScratchPixels[rowIdx + x];
            uint32_t cRight = g_pScratchPixels[rowIdx + (std::min)(x + 1, g_ScreenWidth - 1)];
            int edge = abs(static_cast<int>(c & 0xFF) - static_cast<int>(cRight & 0xFF));
            g_pFramePixels[rowIdx + x] = (edge > 18) ? 0x0000FFFF : 0x00250045;
        }
    }
}

// 22. SCREEN PEEL & FOLD
void EffectScreenPeelAndFold(float time, float dt) {
    float t = (sinf(time * 2.4f) + 1.0f) * 0.5f;
    int peelX = static_cast<int>(g_ScreenWidth * 0.55f * t);
    int peelY = static_cast<int>(g_ScreenHeight * 0.55f * t);

    for (int y = 0; y < peelY && y < g_ScreenHeight; ++y) {
        int maxX = static_cast<int>(peelX * (1.0f - (static_cast<float>(y) / (peelY + 1))));
        for (int x = 0; x < maxX && x < g_ScreenWidth; ++x) {
            g_pFramePixels[y * g_ScreenWidth + x] = ((x ^ y) & 8) ? 0x0000FFAA : 0x00101010;
        }
    }
}

// 23. HOLOGRAPHIC GLITCH OBJECTS
void EffectHolographicGlitch(float time, float dt) {
    for (int k = 0; k < 4; ++k) {
        float ox = g_ScreenWidth * (0.2f + 0.2f * k);
        float oy = g_ScreenHeight * 0.5f + sinf(time * 2.2f + k * 1.5f) * 160.0f;
        float size = 55.0f;
        float ang = time * 3.2f + k;

        POINT box[8];
        for (int i = 0; i < 8; ++i) {
            float vx = (i & 1) ? size : -size;
            float vy = (i & 2) ? size : -size;
            float vz = (i & 4) ? size : -size;
            float rx = vx * cosf(ang) - vz * sinf(ang);
            float rz = vx * sinf(ang) + vz * cosf(ang) + 220.0f;
            box[i].x = static_cast<LONG>(ox + (rx * 180.0f) / rz);
            box[i].y = static_cast<LONG>(oy + (vy * 180.0f) / rz);
        }
        int edges[12][2] = {
            {0,1},{1,3},{3,2},{2,0},
            {4,5},{5,7},{7,6},{6,4},
            {0,4},{1,5},{2,6},{3,7}
        };
        uint32_t col = (k % 2 == 0) ? 0x0000FFFF : 0x00FF00AA;
        for (auto& e : edges) {
            FastDrawLine(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, box[e[0]].x, box[e[0]].y, box[e[1]].x, box[e[1]].y, col);
        }
    }
}

// 24. SYNESTHESIA STORM
void EffectSynesthesiaStorm(float time, float dt) {
    int bars = 32;
    int bw = g_ScreenWidth / bars;
    for (int i = 0; i < bars; ++i) {
        float hNorm = (sinf(time * 7.5f + i * 0.6f) + 1.0f) * 0.5f;
        int barH = static_cast<int>(hNorm * (g_ScreenHeight * 0.4f));
        uint32_t col = ((i * 8) << 16) | ((255 - i * 6) << 8) | 220;
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, i * bw, g_ScreenHeight - barH, bw - 2, barH, col);
    }
}

// 25. REALITY COLLAPSE FINALE
void EffectRealityCollapseFinale(float time, float dt) {
    float cycle = fmodf(time, 4.0f);
    float scale = (std::max)(0.03f, 1.0f - (cycle / 4.0f));

    int nw = static_cast<int>(g_ScreenWidth * scale);
    int nh = static_cast<int>(g_ScreenHeight * scale);
    int ox = (g_ScreenWidth - nw) / 2 + FastRandInt(-12, 12);
    int oy = (g_ScreenHeight - nh) / 2;

    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    StretchBlt(g_hMemDC, ox, oy, nw, nh, g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, SRCCOPY);
    if (cycle > 2.5f) {
        FastInvertRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, ox, oy, nw, nh);
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
    if (msg == WM_HOTKEY && wParam == 101) {
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    size_t totalBytes = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight * sizeof(uint32_t);

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

    // Fast Top-Down 32-bit Framebuffers
    HDC hScreenDC = GetDC(NULL);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_ScreenWidth;
    bmi.bmiHeader.biHeight = -g_ScreenHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_hMemDC = CreateCompatibleDC(hScreenDC);
    g_hDIB = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pFramePixels), NULL, 0);
    SelectObject(g_hMemDC, g_hDIB);

    g_hScratchDC = CreateCompatibleDC(hScreenDC);
    g_hScratchBmp = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pScratchPixels), NULL, 0);
    SelectObject(g_hScratchDC, g_hScratchBmp);

    g_hBaseDC = CreateCompatibleDC(hScreenDC);
    g_hBaseBmp = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pBasePixels), NULL, 0);
    SelectObject(g_hBaseDC, g_hBaseBmp);

    ReleaseDC(NULL, hScreenDC);
    ShowWindow(hWnd, SW_SHOWNOACTIVATE);

    // Allocate downsampled History Buffer for Time Echoes
    int echoW = g_ScreenWidth / 2;
    int echoH = g_ScreenHeight / 2;
    g_EchoHistory.resize(MAX_ECHOES);
    for (auto& f : g_EchoHistory) {
        f.w = echoW; f.h = echoH;
        f.pixels.resize(echoW * echoH, 0);
    }

    std::mt19937 randGen(std::random_device{}());
    std::vector<int> pool(25);
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), randGen);

    std::vector<int> activePayloads;
    activePayloads.push_back(pool[0]);
    int payloadsAdded = 1;

    using clock = std::chrono::high_resolution_clock;
    auto startTime = clock::now();
    auto lastFrame = clock::now();

    float intervalTimer = 0.0f;
    const float STAGE_INTERVAL = 10.0f;

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

        intervalTimer += dt;
        if (intervalTimer >= STAGE_INTERVAL) {
            intervalTimer = 0.0f;
            if (payloadsAdded < 10) {
                activePayloads.push_back(pool[payloadsAdded]);
                payloadsAdded++;
            } else {
                g_Running = false;
                break;
            }
        }

        // 1. INGEST LIVE SCREEN INTO BASE BUFFER
        HDC hLiveDC = GetDC(NULL);
        BitBlt(g_hBaseDC, 0, 0, g_ScreenWidth, g_ScreenHeight, hLiveDC, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hLiveDC);

        // Record history snapshot
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

        // Copy base into working buffer
        memcpy(g_pFramePixels, g_pBasePixels, totalBytes);

        // 2. CHAIN-EXECUTE ACTIVE PAYLOADS
        for (int pIdx : activePayloads) {
            // Snapshot current composite for the payload
            memcpy(g_pScratchPixels, g_pFramePixels, totalBytes);
            g_Payloads[pIdx].fn(totalTime, dt);
        }

        // 3. FLUSH COMPOSITED FRAME TO DWM
        UpdateLayeredWindow(hWnd, NULL, &ptZero, &szWnd, g_hMemDC, &ptZero, 0, &blend, ULW_ALPHA);

        // Solid ~60 FPS Limiter
        Sleep(16);
    }

    // Teardown: Clean up and restore desktop
    UnregisterHotKey(hWnd, 101);
    DeleteObject(g_hDIB);
    DeleteDC(g_hMemDC);
    DeleteObject(g_hScratchBmp);
    DeleteDC(g_hScratchDC);
    DeleteObject(g_hBaseBmp);
    DeleteDC(g_hBaseDC);
    DestroyWindow(hWnd);

    RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);

    return 0;
}