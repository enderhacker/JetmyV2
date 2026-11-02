#include "visual.h"

namespace VisualPayloads {
void EffectDesktopPlanet(float time, float) {
    const int w = g_ScreenWidth, h = g_ScreenHeight, step = RasterStep();
    const float unit = float(std::max(1, std::min(w, h)));
    const float cx = w * (0.5f + 0.08f * sinf(time * 0.19f));
    const float cy = h * (0.5f + 0.05f * cosf(time * 0.23f));
    // A tilted ring is intersected as a plane, so its near half occludes the
    // globe and its far half disappears behind it (rather than a flat ellipse).
    const float tilt = 0.65f + 0.14f * sinf(time * 0.27f);
    const float sn = sinf(tilt), cs = cosf(tilt);
    for (int y = 0; y < h; y += step) {
        for (int x = 0; x < w; x += step) {
            float px = (x - cx) / (unit * 0.31f), py = (y - cy) / (unit * 0.31f);
            float radius2 = px * px + py * py;
            uint32_t color = BlendColor(Sample(x, y), 0x030714, 0.94f);
            // Static sparse stars: no random full-screen flashing.
            uint32_t hash = uint32_t(x / step) * 374761393u ^ uint32_t(y / step) * 668265263u;
            hash = (hash ^ (hash >> 13)) * 1274126177u;
            if (hash % 3203u == 0) color = 0x8FACC8;
            float surfaceZ = -10;
            if (radius2 < 1) {
                surfaceZ = sqrtf(1 - radius2);
                float latitude = py * cs + surfaceZ * sn;
                float z = surfaceZ * cs - py * sn;
                float u = atan2f(px, z) / Tau + 0.5f + time * 0.045f;
                u -= floorf(u);
                float v = acosf(std::clamp(latitude, -1.0f, 1.0f)) / Pi;
                uint32_t texture = Sample(int(u * (w - 1)), int(v * (h - 1)));
                float light = std::max(0.0f, -px * 0.45f - py * 0.35f + surfaceZ * 0.75f);
                color = BlendColor(0x030917, texture, 0.12f + 0.88f * light);
                float rim = powf(1 - surfaceZ, 3.0f);
                color = BlendColor(color, 0x65CFFF, rim * 0.8f);
            } else {
                float halo = std::max(0.0f, 1 - (sqrtf(radius2) - 1) / 0.11f);
                color = BlendColor(color, 0x247BAA, halo * halo * 0.5f);
            }
            float ringY = py / cs, ringZ = ringY * sn;
            float ringR = sqrtf(px * px + ringY * ringY);
            if (ringR > 1.3f && ringR < 1.95f && ringZ > surfaceZ) {
                float angle = atan2f(ringY, px) / Tau + time * 0.025f;
                angle -= floorf(angle);
                float band = (ringR - 1.3f) / 0.65f;
                uint32_t texture = Sample(int(angle * (w - 1)), int(band * (h - 1)));
                float grooves = 0.5f + 0.5f * sinf(ringR * 160);
                texture = BlendColor(texture, 0xE3B77F, 0.25f + grooves * 0.25f);
                float edge = Smooth(band * 30) * Smooth((1 - band) * 30);
                color = BlendColor(color, texture, edge * 0.9f);
            }
            Tile(x, y, step, color);
        }
    }
}
} // namespace VisualPayloads
