#include "../payloads/visual/visual.h"
#include "../payloads/sound/sound.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <numeric>

namespace {
using Clock = std::chrono::steady_clock;
struct Canvas {
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP bitmap = nullptr;
    HGDIOBJ old = nullptr;
    uint32_t* pixels = nullptr;
    Canvas(int w, int h) {
        BITMAPINFO info{};
        info.bmiHeader = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
        bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), nullptr, 0);
        assert(dc && bitmap && pixels);
        old = SelectObject(dc, bitmap);
    }
    ~Canvas() { SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); }
};
uint64_t Hash(const uint32_t* pixels, size_t count) {
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < count; ++i) hash = (hash ^ (pixels[i] & 0x00FFFFFFu)) * 1099511628211ull;
    return hash;
}
void Prepare(const VisualPayloads::PayloadRecord& effect) {
#ifdef JETMY_REFERENCE
    using namespace VisualPayloads;
    const size_t count = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight;
    if (effect.needsSnapshot) std::copy_n(g_pFramePixels, count, g_pScratchPixels);
    if (effect.blackBackground) std::fill_n(g_pFramePixels, count, 0u);
#else
    VisualPayloads::PreparePayloadFrame(effect);
#endif
}
}

// No presentation window or audio device. Cursor Trail and Digital Noise Bursts
// still query the live cursor/desktop, so their hashes vary.
int main(int argc, char** argv) {
    using namespace VisualPayloads;
    const int w = argc > 1 ? std::atoi(argv[1]) : 1920;
    const int h = argc > 2 ? std::atoi(argv[2]) : 1080;
    const int frames = argc > 3 ? std::atoi(argv[3]) : 12;
    assert(w > 0 && h > 0 && frames > 1);
    g_ScreenWidth = w; g_ScreenHeight = h;
    Canvas frame(w, h), scratch(w, h);
    g_hMemDC = frame.dc; g_pFramePixels = frame.pixels;
    g_hScratchDC = scratch.dc; g_pScratchPixels = scratch.pixels;
    const size_t count = static_cast<size_t>(w) * h;
    assert(Registry().size() == 34 && SoundPayloads::Registry().size() == 18);
    std::vector<uint32_t> input(count);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
        input[static_cast<size_t>(y) * w + x] = ((x * 19 + y * 3) & 255) << 16 |
            ((y * 17 + x / 3) & 255) << 8 | ((x * 7 ^ y * 13) & 255);
    g_EchoHistory.resize(MAX_ECHOES); g_EchoHead = MAX_ECHOES;
    const int echoScale = std::max(1, int(std::ceil(std::sqrt(double(w) * h / 2100000.0))));
    const int echoW = (w + echoScale - 1) / echoScale, echoH = (h + echoScale - 1) / echoScale;
    for (size_t i = 0; i < MAX_ECHOES; ++i) {
        auto& echo = g_EchoHistory[i]; echo.w = echoW; echo.h = echoH;
        echo.pixels.resize(static_cast<size_t>(echoW) * echoH);
        for (int y = 0; y < echoH; ++y) for (int x = 0; x < echoW; ++x)
            echo.pixels[static_cast<size_t>(y) * echoW + x] = input[static_cast<size_t>(y * h / echoH) * w + x * w / echoW];
        for (auto& pixel : echo.pixels) pixel ^= static_cast<uint32_t>(i) * 0x030507;
    }
    std::cout << "payload,first_ms,mean_ms,p95_ms,hash\n" << std::fixed << std::setprecision(3);
    for (const auto& effect : Registry()) {
        g_FastState = 0x8A5CD791;
        std::vector<double> timings;
        uint64_t hash = 0;
        double first = 0;
        for (int f = 0; f < frames; ++f) {
            std::copy(input.begin(), input.end(), g_pFramePixels);
            const auto begin = Clock::now();
            Prepare(effect);
            effect.fn(f == 0 ? 0.0f : f * 0.731f, f == 0 ? 0.0f : 0.1f);
            GdiFlush();
            double ms = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
            if (f == 0) first = ms; else timings.push_back(ms);
            hash = (hash ^ Hash(g_pFramePixels, count)) * 1099511628211ull + static_cast<uint64_t>(f);
            if (argc > 4 && f == frames - 1) {
                std::ofstream dump(std::string(argv[4]) + "/" + effect.name + ".bin", std::ios::binary);
                dump.write(reinterpret_cast<const char*>(g_pFramePixels), count * sizeof(uint32_t));
                assert(dump.good());
            }
        }
        const double mean = std::accumulate(timings.begin(), timings.end(), 0.0) / timings.size();
        std::sort(timings.begin(), timings.end());
        std::cout << effect.name << ',' << first << ',' << mean << ','
                  << timings[(timings.size() * 95 + 99) / 100 - 1] << ',' << hash << '\n';
    }
    // Reproducible cumulative stack: same selection/order in both builds.
    std::vector<size_t> order(Registry().size()); std::iota(order.begin(), order.end(), 0);
    std::mt19937 random(12345); std::shuffle(order.begin(), order.end(), random);
    for (size_t depth : {size_t(1), size_t(4), size_t(8), size_t(10)}) {
        g_FastState = 0x8A5CD791;
        double elapsed = 0; uint64_t hash = 0;
        for (int f = 0; f < frames; ++f) {
            std::copy(input.begin(), input.end(), g_pFramePixels);
            const auto begin = Clock::now();
            for (size_t i = 0; i < depth; ++i) {
                const auto& effect = Registry()[order[i]];
                Prepare(effect);
                effect.fn(f == 0 ? 0.0f : f * 0.731f, f == 0 ? 0.0f : 0.1f);
                GdiFlush();
            }
            if (f) elapsed += std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
            hash = (hash ^ Hash(g_pFramePixels, count)) * 1099511628211ull + static_cast<uint64_t>(f);
        }
        std::cout << "Stack " << depth << ",0," << elapsed / (frames - 1) << ",0," << hash << '\n';
    }
    for (const auto& sound : SoundPayloads::Registry()) {
        std::mt19937 seed(12345);
        const auto begin = Clock::now();
        auto samples = SoundPayloads::Synthesize(sound.generate, 10, seed, true);
        const double elapsed = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
        assert(samples.size() == SoundPayloads::SampleRate * 10);
        for (int16_t sample : samples) { assert(sample >= -6000 && sample <= 6000); }
        std::cout << sound.name << ",0," << elapsed << ",0,0\n";
    }
#ifndef JETMY_REFERENCE
    // Prove that buffer exchange is safe: full-overwrite effects must produce
    // identical output even when their destination initially contains garbage.
    for (const auto& effect : Registry()) {
        if (!effect.needsSnapshot || !(effect.writesAllPixels || effect.blackBackground)) continue;
        std::copy(input.begin(), input.end(), g_pFramePixels);
        std::copy(input.begin(), input.end(), g_pScratchPixels);
        if (effect.blackBackground) std::fill_n(g_pFramePixels, count, 0u);
        g_FastState = 0x8A5CD791;
        effect.fn(0, 0); GdiFlush();
        std::vector<uint32_t> expected(g_pFramePixels, g_pFramePixels + count);
        std::copy(input.begin(), input.end(), g_pFramePixels);
        std::fill_n(g_pScratchPixels, count, 0xA5A5A5A5u);
        g_FastState = 0x8A5CD791;
        Prepare(effect); effect.fn(0, 0); GdiFlush();
        assert(std::equal(expected.begin(), expected.end(), g_pFramePixels));
    }
    // Every row is visited exactly once, including non-divisible job sizes.
    for (int rows : {1, 63, 64, 65, 181, 1081}) {
        std::vector<int> visits(rows);
        Runtime::ParallelFor(rows, [&](int row) { ++visits[row]; });
        assert(std::all_of(visits.begin(), visits.end(), [](int count) { return count == 1; }));
    }
#endif
}
