#include "../payloads/visual/visual.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <utility>

namespace VisualPayloads {
int g_ScreenWidth = 0, g_ScreenHeight = 0;
uint32_t* g_pFramePixels = nullptr;
uint32_t* g_pScratchPixels = nullptr;
uint32_t g_FastState = 0;
}

namespace {
uint64_t Hash(const uint32_t* pixels, size_t count) {
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < count; ++i) hash = (hash ^ pixels[i]) * 1099511628211ull;
    return hash;
}
}

int main() {
    using namespace VisualPayloads;
    constexpr uint32_t guard = 0xD15EA5EDu, poison = 0xA5A5A5A5u;
    for (const auto& size : {std::pair<int, int>{1, 1}, {3, 5}, {321, 181}, {1920, 1080},
                            {1921, 1081}, {2561, 1563}, {4001, 4001}}) {
        g_ScreenWidth = size.first; g_ScreenHeight = size.second;
        const size_t count = static_cast<size_t>(size.first) * size.second;
        std::vector<uint32_t> frame(count + 2, guard), input(count + 2, guard);
        g_pFramePixels = frame.data() + 1; g_pScratchPixels = input.data() + 1;
        std::fill_n(g_pScratchPixels, count, 0x808080u);
        const auto original = Hash(g_pScratchPixels, count);
        uint64_t initial = 0;
        double initialRedBias = 0;
        for (float time : {0.0f, 0.1f, 4.0f, 8.0f, 17.0f, 35.0f, 70.0f, 3600.0f, 1000000.0f}) {
            std::fill_n(g_pFramePixels, count, poison);
            g_FastState = 12345;
            const auto begin = std::chrono::steady_clock::now();
            EffectScreenDecay(time, time == 0 ? 0 : 0.1f);
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
            if (time == 35) std::cout << size.first << 'x' << size.second << " full decay: " << ms << " ms\n";
            assert(frame.front() == guard && frame.back() == guard);
            assert(input.front() == guard && input.back() == guard);
            assert(Hash(g_pScratchPixels, count) == original);
            double redBias = 0;
            for (size_t i = 0; i < count; ++i) {
                assert((g_pFramePixels[i] & 0xFF000000u) == 0); // Full overwrite, even at odd edges.
                redBias += int((g_pFramePixels[i] >> 16) & 255) - int((g_pFramePixels[i] >> 8) & 255);
            }
            redBias /= count;
            if (time == 0) { initial = Hash(g_pFramePixels, count); initialRedBias = redBias; }
            if (time >= 35 && size.first > 32) assert(redBias > initialRedBias + 25);
        }
        // Re-enabling is independent of all previous damage, ticks and grid sizes.
        g_FastState = 12345;
        EffectScreenDecay(0, 0);
        assert(Hash(g_pFramePixels, count) == initial);
        std::cout << size.first << 'x' << size.second << ": passed\n";
    }
    // A patterned input makes the initial off-screen displacement observable.
    g_ScreenWidth = 321; g_ScreenHeight = 181;
    const int w = g_ScreenWidth, h = g_ScreenHeight;
    std::vector<uint32_t> frame(static_cast<size_t>(w) * h), input(frame.size());
    g_pFramePixels = frame.data(); g_pScratchPixels = input.data();
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
        input[static_cast<size_t>(y) * w + x] = ((x & 255u) << 16) | ((y & 255u) << 8) | 127u;
    EffectScreenDecay(0, 0);
    const int shiftX = int(w * 0.008f), shiftY = int(h * -0.004f);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        int sx = x - shiftX, sy = y - shiftY;
        uint32_t expected = sx >= 0 && sx < w && sy >= 0 && sy < h ? input[static_cast<size_t>(sy) * w + sx] : 0x070100u;
        assert(frame[static_cast<size_t>(y) * w + x] == expected);
    }
    EffectScreenDecay(35, 0.1f);
    constexpr int block = 24;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
        assert(frame[static_cast<size_t>(y) * w + x] == frame[static_cast<size_t>(y / block * block) * w + x / block * block]);
    std::cout << "Off-screen shift and compression blocks: passed\n";
}
