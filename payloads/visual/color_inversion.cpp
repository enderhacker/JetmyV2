#include "visual.h"

namespace VisualPayloads {
void EffectScreenColorInversion(float, float) {
    size_t count = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight;
    for (size_t i = 0; i < count; ++i) g_pFramePixels[i] ^= 0x00FFFFFFu;
}
} // namespace VisualPayloads
