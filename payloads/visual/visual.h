#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace VisualPayloads {

extern int g_ScreenWidth, g_ScreenHeight;
extern HDC g_hMemDC, g_hScratchDC;
extern uint32_t *g_pFramePixels, *g_pScratchPixels;

struct EchoFrame {
    int w = 0, h = 0;
    std::vector<uint32_t> pixels;
};
inline constexpr size_t MAX_ECHOES = 16;
extern std::vector<EchoFrame> g_EchoHistory;
extern size_t g_EchoHead;
extern uint32_t g_FastState;

inline uint32_t FastRand() {
    g_FastState ^= g_FastState << 13;
    g_FastState ^= g_FastState >> 17;
    g_FastState ^= g_FastState << 5;
    return g_FastState;
}
inline int FastRandInt(int min, int max) {
    return min >= max ? min : min + static_cast<int>(FastRand() % (max - min + 1));
}
inline float FastRandFloat(float min, float max) {
    return min + (FastRand() * (1.0f / 4294967295.0f)) * (max - min);
}
inline uint32_t BlendColor(uint32_t c1, uint32_t c2, float t) {
    uint32_t inv = static_cast<uint32_t>((1.0f - t) * 256.0f);
    uint32_t w = static_cast<uint32_t>(t * 256.0f);
    return (((((c1 & 0x00FF00FF) * inv + (c2 & 0x00FF00FF) * w) >> 8) & 0x00FF00FF) |
            ((((c1 & 0x0000FF00) * inv + (c2 & 0x0000FF00) * w) >> 8) & 0x0000FF00));
}
inline void FastDrawLine(uint32_t* buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) buf[y0 * w + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
inline void FastFillRect(uint32_t* buf, int w, int h, int rx, int ry, int rw, int rh, uint32_t color) {
    for (int y = std::max(0, ry); y < std::min(h, ry + rh); ++y)
        for (int x = std::max(0, rx); x < std::min(w, rx + rw); ++x)
            buf[y * w + x] = color;
}
inline void FastInvertRect(uint32_t* buf, int w, int h, int rx, int ry, int rw, int rh) {
    for (int y = std::max(0, ry); y < std::min(h, ry + rh); ++y)
        for (int x = std::max(0, rx); x < std::min(w, rx + rw); ++x)
            buf[y * w + x] = ~buf[y * w + x] & 0x00FFFFFF;
}
// Bounds-safe block writes also support odd-sized displays.
inline void DrawBlock(int x, int y, uint32_t color) {
    FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, 2, 2, color);
}

using PayloadFn = void (*)(float time, float dt);
struct PayloadRecord { const char* name; PayloadFn fn; };
const std::vector<PayloadRecord>& Registry();

void EffectPixelFireworks(float, float);
void EffectChaosRectangles(float, float);
void EffectBlackHoleSingularities(float, float);
void EffectDesktopGlassShatter(float, float);
void EffectLivingInfection(float, float);
void EffectImpossibleWindows(float, float);
void EffectLiquidDesktop(float, float);
void EffectPixelSortingAnomaly(float, float);
void EffectNeonFractalInvasion(float, float);
void EffectGhostCursorSwarm(float, float);
void EffectSpatialPortalNetwork(float, float);
void EffectCRTFailureSimulator(float, float);
void EffectVoxelDesktopExplosion(float, float);
void EffectMatrixReconstruction(float, float);
void EffectRGBSpectralPhantoms(float, float);
void EffectKaleidoscopicReality(float, float);
void EffectGravityPaint(float, float);
void EffectDesktopTimeEchoes(float, float);
void Effect3DWarpTunnel(float, float);
void EffectGeometricImplosion(float, float);
void EffectXRayScanner(float, float);
void EffectScreenPeelAndFold(float, float);
void EffectHolographicGlitch(float, float);
void EffectSynesthesiaStorm(float, float);
void EffectRealityCollapseFinale(float, float);

} // namespace VisualPayloads
