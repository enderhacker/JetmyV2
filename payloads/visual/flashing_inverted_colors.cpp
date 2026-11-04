#include "visual.h"

namespace VisualPayloads {
void EffectScreenColorInversion(float time, float) {
    // Frames are freshly captured: skip alternate half-seconds rather than
    // repeatedly toggling a retained framebuffer. Activation begins inverted.
    if (fmodf(time, 1.0f) >= 0.5f) return;
    size_t count = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight;
    for (size_t i = 0; i < count; ++i) g_pFramePixels[i] ^= 0x00FFFFFFu;
}
} // namespace VisualPayloads
