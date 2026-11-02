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
        { "Pixel Fireworks", EffectPixelFireworks, false },
        { "Chaos Rectangles", EffectChaosRectangles, false },
        { "Black Hole Singularities", EffectBlackHoleSingularities },
        { "Desktop Glass Shatter", EffectDesktopGlassShatter },
        { "Living Digital Infection", EffectLivingInfection, false },
        { "Impossible Windows", EffectImpossibleWindows },
        { "Liquid Desktop", EffectLiquidDesktop },
        { "Pixel Sorting Anomaly", EffectPixelSortingAnomaly, false },
        { "Neon Fractal Invasion", EffectNeonFractalInvasion },
        { "Ghost Cursor Swarm", EffectGhostCursorSwarm, false },
        { "Spatial Portal Network", EffectSpatialPortalNetwork },
        { "CRT Failure Simulator", EffectCRTFailureSimulator },
        { "Voxel Desktop Explosion", EffectVoxelDesktopExplosion },
        { "Matrix Reconstruction", EffectMatrixReconstruction, false },
        { "RGB Spectral Phantoms", EffectRGBSpectralPhantoms },
        { "Kaleidoscopic Reality", EffectKaleidoscopicReality },
        { "Gravity Paint", EffectGravityPaint, false },
        { "Desktop Time Echoes", EffectDesktopTimeEchoes },
        { "3D Warp Tunnel", Effect3DWarpTunnel },
        { "Infinite Desktop City", EffectInfiniteDesktopCity },
        { "Desktop Planet", EffectDesktopPlanet },
        { "Mobius Reality Ribbon", EffectMobiusRealityRibbon },
        { "Geometric Implosion", EffectGeometricImplosion, false },
        { "X-Ray Scanner", EffectXRayScanner },
        { "Screen Peel & Fold", EffectScreenPeelAndFold },
        { "Holographic Glitch Objects", EffectHolographicGlitch, false },
        { "Synesthesia Storm", EffectSynesthesiaStorm, false },
        { "Reality Collapse Finale", EffectRealityCollapseFinale },
        { "Rotate Screen", EffectRotateScreen },
        { "Random Status Icons", EffectRandomStatusIcons, false },
        { "Melting Screen", EffectMeltingScreen },
        { "Infinite Tunnel", EffectInfiniteTunnel },
        { "XOR Color Chaos", EffectXorColorChaos },
        { "Screen Color Inversion", EffectScreenColorInversion, false },
        { "Popup Cascade", EffectPopupCascade, false }
    };
    return payloads;
}
} // namespace VisualPayloads
