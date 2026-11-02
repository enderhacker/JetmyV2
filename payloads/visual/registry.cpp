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
        { "Pixel Fireworks", EffectPixelFireworks },
        { "Chaos Rectangles", EffectChaosRectangles },
        { "Black Hole Singularities", EffectBlackHoleSingularities },
        { "Desktop Glass Shatter", EffectDesktopGlassShatter },
        { "Living Digital Infection", EffectLivingInfection },
        { "Impossible Windows", EffectImpossibleWindows },
        { "Liquid Desktop", EffectLiquidDesktop },
        { "Pixel Sorting Anomaly", EffectPixelSortingAnomaly },
        { "Neon Fractal Invasion", EffectNeonFractalInvasion },
        { "Ghost Cursor Swarm", EffectGhostCursorSwarm },
        { "Spatial Portal Network", EffectSpatialPortalNetwork },
        { "CRT Failure Simulator", EffectCRTFailureSimulator },
        { "Voxel Desktop Explosion", EffectVoxelDesktopExplosion },
        { "Matrix Reconstruction", EffectMatrixReconstruction },
        { "RGB Spectral Phantoms", EffectRGBSpectralPhantoms },
        { "Kaleidoscopic Reality", EffectKaleidoscopicReality },
        { "Gravity Paint", EffectGravityPaint },
        { "Desktop Time Echoes", EffectDesktopTimeEchoes },
        { "3D Warp Tunnel", Effect3DWarpTunnel },
        { "Geometric Implosion", EffectGeometricImplosion },
        { "X-Ray Scanner", EffectXRayScanner },
        { "Screen Peel & Fold", EffectScreenPeelAndFold },
        { "Holographic Glitch Objects", EffectHolographicGlitch },
        { "Synesthesia Storm", EffectSynesthesiaStorm },
        { "Reality Collapse Finale", EffectRealityCollapseFinale }
    };
    return payloads;
}
} // namespace VisualPayloads
