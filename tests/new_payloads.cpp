#include "payloads/visual/visual.h"
#include "payloads/sound/sound.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <utility>

int main() {
    using namespace VisualPayloads;
    assert(Registry().size() == 41);
    assert(SoundPayloads::Registry().size() == 18);
    // Guard pixels exercise odd, tiny and ordinary frame sizes without displaying effects.
    for (auto dimensions : {std::pair<int, int>{1, 1}, {3, 5}, {127, 73}}) {
        g_ScreenWidth = dimensions.first; g_ScreenHeight = dimensions.second;
        size_t count = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight;
        std::vector<uint32_t> frame(count + 2, 0xDEADBEEFu), source(count + 2, 0xDEADBEEFu);
        for (size_t i = 1; i <= count; ++i) source[i] = static_cast<uint32_t>(i * 123457u) & 0x00FFFFFFu;
        g_pFramePixels = frame.data() + 1; g_pScratchPixels = source.data() + 1;
        for (auto effect : {EffectSeismicSliceAndDrift, EffectRecursiveRopCrucible, EffectBitPlaneTopography}) {
            for (int i = 0; i < 150; ++i) {
                effect(i / 20.0f, i ? 0.05f : 0);
                assert(frame.front() == 0xDEADBEEFu && frame.back() == 0xDEADBEEFu);
                assert(source.front() == 0xDEADBEEFu && source.back() == 0xDEADBEEFu);
                for (size_t j = 1; j <= count; ++j) assert((frame[j] & 0xFF000000u) == 0);
            }
            effect(0, 0);
            auto restarted = frame;
            effect(1.0f, 0.05f);
            effect(0, 0);
            assert(frame == restarted);
        }
        EffectSeismicSliceAndDrift(0, 0);
        assert(std::equal(frame.begin() + 1, frame.end() - 1, source.begin() + 1));
        EffectBitPlaneTopography(0, 0);
        for (size_t i = 1; i <= count; ++i) assert(frame[i] == 0 || frame[i] == 0x00FFFFFFu);
    }
    std::mt19937 random(12345);
    for (auto generator : {SoundPayloads::BytebeatIndustrialScream, SoundPayloads::ShepardRissetDescent,
                           SoundPayloads::EntropyHandshake}) {
        SoundPayloads::Voice voice{random, 12345u};
        for (uint32_t i = 0; i < SoundPayloads::SampleRate * 20; ++i) {
            float value = generator(i, voice);
            assert(std::isfinite(value) && fabsf(value) <= 1.1f);
        }
        auto samples = SoundPayloads::Synthesize(generator, 10, random, true);
        assert(samples.size() == SoundPayloads::SampleRate * 10);
        int peak = 0;
        for (auto sample : samples) {
            assert(sample >= -6000 && sample <= 6000);
            peak = std::max(peak, std::abs(int(sample)));
        }
        assert(peak > 100);
    }
    std::cout << "New payload smoke tests passed (no playback or overlay).\n";
}
