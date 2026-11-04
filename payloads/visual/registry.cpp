#include "visual.h"

namespace VisualPayloads {
int g_ScreenWidth = 0, g_ScreenHeight = 0;
HDC g_hMemDC = nullptr, g_hScratchDC = nullptr;
uint32_t *g_pFramePixels = nullptr, *g_pScratchPixels = nullptr;
std::vector<EchoFrame> g_EchoHistory;
size_t g_EchoHead = 0;
uint32_t g_FastState = 0x8A5CD791;

const std::vector<PayloadRecord>& Registry() {
    static const std::vector<PayloadRecord> payloads = {
        { "Chaos Rectangles", EffectChaosRectangles, false },
        { "Black Hole Singularities", EffectBlackHoleSingularities },
        { "Desktop Glass Shatter", EffectDesktopGlassShatter, true, true },
        { "Living Digital Infection", EffectLivingInfection, false },
        { "Impossible Windows", EffectImpossibleWindows },
        { "Liquid Desktop", EffectLiquidDesktop, true, false, true },
        { "Pixel Sorting Anomaly", EffectPixelSortingAnomaly, false },
        { "Neon Fractal Invasion", EffectNeonFractalInvasion },
        { "Ghost Cursor Swarm", EffectGhostCursorSwarm, false },
        { "Spatial Portal Network", EffectSpatialPortalNetwork },
        { "CRT Failure Simulator", EffectCRTFailureSimulator, true, false, true },
        { "Voxel Desktop Explosion", EffectVoxelDesktopExplosion, true, true },
        { "RGB Spectral Phantoms", EffectRGBSpectralPhantoms, true, false, true },
        { "Kaleidoscopic Reality", EffectKaleidoscopicReality, true, false, true },
        { "Gravity Paint", EffectGravityPaint, false },
        { "Desktop Time Echoes", EffectDesktopTimeEchoes },
        { "3D Warp Tunnel", Effect3DWarpTunnel, true, false, true },
        { "Infinite Desktop City", EffectInfiniteDesktopCity, true, false, true },
        { "Desktop Planet", EffectDesktopPlanet, true, false, true },
        { "Reality Collapse Finale", EffectRealityCollapseFinale, true, true },
        { "Rotate Screen", EffectRotateScreen, true, false, true },
        { "Random Status Icons", EffectRandomStatusIcons, false },
        { "Melting Screen", EffectMeltingScreen, true, false, true },
        { "Infinite Tunnel", EffectInfiniteTunnel },
        { "XOR Color Chaos", EffectXorColorChaos, true, false, true },
        { "Screen Color Inversion", EffectScreenColorInversion, false },
        { "Popup Cascade", EffectPopupCascade, false },
        { "Random Square Paint", EffectRandomSquarePaint, false },
        { "Chaos Shader", EffectChaosShader, true, false, true },
        { "Hell Shader", EffectHellShader, true, false, true },
        { "Seismic Slice & Drift", EffectSeismicSliceAndDrift, true, false, true },
        { "Recursive ROP Crucible", EffectRecursiveRopCrucible, true, false, true },
        { "Bit-Plane Topography", EffectBitPlaneTopography, true, false, true }
    };
    return payloads;
}
} // namespace VisualPayloads
