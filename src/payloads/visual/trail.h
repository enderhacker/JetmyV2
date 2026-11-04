#pragma once
#include "visual.h"

namespace VisualPayloads {
// Retain only effect ink, never a screenshot of the desktop beneath it.
// Each pixel occurs at most once in active_, even in an indefinite preview.
class TrailCanvas {
public:
    void ResetIfNeeded(bool reset) {
        if (!reset && width_ == g_ScreenWidth && height_ == g_ScreenHeight) return;
        width_ = g_ScreenWidth; height_ = g_ScreenHeight;
        pixels_.assign(static_cast<size_t>(width_) * height_, 0);
        active_.clear();
        fadeRemainder_ = 0;
    }
    void Deposit(int x, int y, uint32_t color, unsigned opacity = 255) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_ || opacity == 0) return;
        size_t index = static_cast<size_t>(y) * width_ + x;
        uint32_t& previous = pixels_[index];
        unsigned oldOpacity = previous >> 24;
        if (!oldOpacity) active_.push_back(index);
        // Fresh marks replace the color and cannot weaken an existing stroke.
        previous = (color & 0x00FFFFFFu) | (std::max(oldOpacity, std::min(opacity, 255u)) << 24);
    }
    void Stroke(int x0, int y0, int x1, int y1, uint32_t color, unsigned opacity = 255) {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, error = dx + dy;
        for (;;) {
            Deposit(x0, y0, color, opacity);
            if (x0 == x1 && y0 == y1) break;
            int twice = error * 2;
            if (twice >= dy) { error += dy; x0 += sx; }
            if (twice <= dx) { error += dx; y0 += sy; }
        }
    }
    void Fade(float dt) {
        // Fixed-rate decay avoids frame-rate-dependent 8-bit rounding. No fade
        // is applied to permanent trails, which simply do not call this method.
        fadeRemainder_ += std::max(0.0f, dt);
        constexpr float tick = 1.0f / 120;
        int ticks = int(fadeRemainder_ / tick);
        if (!ticks) return;
        fadeRemainder_ -= ticks * tick;
        float decay = expf(-3.2f * tick);
        unsigned faded[256];
        for (unsigned value = 0; value < 256; ++value) {
            unsigned alpha = value;
            for (int i = 0; i < ticks && alpha >= 3; ++i) alpha = unsigned(alpha * decay);
            faded[value] = alpha < 3 ? 0 : alpha;
        }
        size_t kept = 0;
        for (size_t index : active_) {
            unsigned alpha = faded[pixels_[index] >> 24];
            if (!alpha) { pixels_[index] = 0; continue; }
            pixels_[index] = (pixels_[index] & 0x00FFFFFFu) | (alpha << 24);
            active_[kept++] = index;
        }
        active_.resize(kept);
    }
    void Composite() const {
        for (size_t index : active_) {
            uint32_t ink = pixels_[index];
            unsigned alpha = ink >> 24;
            g_pFramePixels[index] = alpha == 255 ? ink & 0x00FFFFFFu :
                BlendColor(g_pFramePixels[index], ink & 0x00FFFFFFu, alpha / 255.0f);
        }
    }
private:
    int width_ = 0, height_ = 0;
    float fadeRemainder_ = 0;
    std::vector<uint32_t> pixels_;
    std::vector<size_t> active_;
};
} // namespace VisualPayloads
