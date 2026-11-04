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
        { "Random Rectangles", EffectChaosRectangles, false },
        { "Black Hole", EffectBlackHoleSingularities },
        { "Shattered Screen", EffectDesktopGlassShatter, true, true },
        { "Growing Light Trails", EffectLivingInfection, false },
        { "Nested Windows", EffectImpossibleWindows },
        { "Wavy Screen", EffectLiquidDesktop, true, false, true },
        { "Sorted Color Bands", EffectPixelSortingAnomaly, false },
        { "Neon Fractal", EffectNeonFractalInvasion },
        { "Cursor Trail", EffectGhostCursorSwarm, false },
        { "Screen Portals", EffectSpatialPortalNetwork },
        { "Old TV Interference", EffectCRTFailureSimulator, true, false, true },
        { "Exploding Blocks", EffectVoxelDesktopExplosion, true, true },
        { "Split Colors", EffectRGBSpectralPhantoms, true, false, true },
        { "Kaleidoscope", EffectKaleidoscopicReality, true, false, true },
        { "Dripping Paint", EffectGravityPaint, false },
        { "Delayed Screen Copies", EffectDesktopTimeEchoes },
        { "Round Tunnel", Effect3DWarpTunnel, true, false, true },
        { "Desktop City", EffectInfiniteDesktopCity, true, false, true },
        { "Desktop Planet", EffectDesktopPlanet, true, false, true },
        { "Screen Collapse", EffectRealityCollapseFinale, true, true },
        { "Rotate Screen", EffectRotateScreen, true, false, true },
        { "Random Icons", EffectRandomStatusIcons, false },
        { "Melting Screen", EffectMeltingScreen, true, false, true },
        { "Repeating Screen Tunnel", EffectInfiniteTunnel },
        { "Color Patterns", EffectXorColorChaos, true, false, true },
        { "Flashing Inverted Colors", EffectScreenColorInversion, false },
        { "Fake Message Popups", EffectPopupCascade, false },
        { "Random Paint Squares", EffectRandomSquarePaint, false },
        { "Wavy Color Distortion", EffectChaosShader, true, false, true },
        { "Flashing Color Vortex", EffectHellShader, true, false, true },
        { "Sliding Screen Strips", EffectSeismicSliceAndDrift, true, false, true },
        { "Zooming Screen Glitch", EffectRecursiveRopCrucible, true, false, true },
        { "Flashing Brightness Bands", EffectBitPlaneTopography, true, false, true },
        { "Screen Decay", EffectScreenDecay, true, false, true },
        { "Paper Accordion", EffectPaperAccordion, true, false, true },
        { "Zipper Rift", EffectZipperRift, true, false, true },
        { "Error Barcode", EffectErrorBarcode, true, false, true },
        { "Circuit Etching", EffectCircuitEtching, true, false, true },
        { "ASCII Corruption", EffectAsciiCorruption, true, false, true }
    };
    return payloads;
}
} // namespace VisualPayloads
