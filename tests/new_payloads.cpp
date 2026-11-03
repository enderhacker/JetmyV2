#include "payloads/visual/visual.h"
#include "payloads/sound/sound.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <utility>

int main() {
    using namespace VisualPayloads;
    assert(Registry().size() == 34);
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
        for (float time : {0.0f, 0.499f, 0.5f, 0.999f, 1.0f, 1.499f, 1.5f, 60.0f}) {
            frame = source; // The runtime supplies fresh incoming pixels, never the previous inversion.
            g_pFramePixels = frame.data() + 1;
            EffectScreenColorInversion(time, 0);
            bool inverted = fmodf(time, 1.0f) < 0.5f;
            for (size_t i = 1; i <= count; ++i)
                assert(frame[i] == (source[i] ^ (inverted ? 0x00FFFFFFu : 0u)));
            assert(frame.front() == 0xDEADBEEFu && frame.back() == 0xDEADBEEFu);
        }
    }
    for (auto dimensions : {std::pair<int, int>{1, 1}, {3, 5}, {127, 73}, {2053, 7}, {7, 2053}}) {
        g_ScreenWidth = dimensions.first; g_ScreenHeight = dimensions.second;
        size_t count = static_cast<size_t>(g_ScreenWidth) * g_ScreenHeight;
        std::vector<uint32_t> source(count + 2, 0xDEADBEEFu), frame;
        for (size_t i = 1; i <= count; ++i) source[i] = static_cast<uint32_t>(i * 123457u) & 0x00FFFFFFu;
        const auto untouched = source;
        g_pScratchPixels = source.data() + 1;
        for (int i = 0; i < 150; ++i) {
            frame = source;
            g_pFramePixels = frame.data() + 1;
            EffectPixelSortingAnomaly(i / 20.0f, i ? 0.05f : 0);
            assert(frame.front() == 0xDEADBEEFu && frame.back() == 0xDEADBEEFu);
            assert(source == untouched);
            for (size_t j = 1; j <= count; ++j) assert((frame[j] & 0xFF000000u) == 0);
        }
        auto restart = [&]() {
            frame = source;
            g_pFramePixels = frame.data() + 1;
            g_FastState = 12345u;
            EffectPixelSortingAnomaly(0, 0);
            return frame;
        };
        auto first = restart();
        EffectPixelSortingAnomaly(2.0f, 0.1f);
        assert(restart() == first);
        if (dimensions == std::pair<int, int>{127, 73}) {
            size_t changed = 0;
            for (size_t i = 1; i <= count; ++i) changed += first[i] != source[i];
            assert(changed > count / 4); // Substantial corruption, not the old tiny ribbons.
        }
    }
    std::mt19937 random(12345);
    std::set<std::string> names;
    std::set<SoundPayloads::Generator> generators;
    for (const auto& payload : SoundPayloads::Registry()) {
        assert(names.insert(payload.name).second);
        assert(payload.generate && generators.insert(payload.generate).second);
        auto generator = payload.generate;
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
        // Exercise the longest supported clip, including many delay wraps and quantizer cycles.
        auto longest = SoundPayloads::Synthesize(generator, 120, random, true);
        assert(longest.size() == SoundPayloads::SampleRate * 120);
        for (auto sample : longest) assert(sample >= -6000 && sample <= 6000);
        // Fresh voices must not inherit feedback, delay or oscillator state from other clips.
        // Desktop-seeded Entropy Handshake intentionally depends on the live desktop.
        if (generator != SoundPayloads::EntropyHandshake) {
            std::mt19937 firstRandom(42), secondRandom(42);
            SoundPayloads::Voice first{firstRandom, 42u}, second{secondRandom, 42u};
            for (uint32_t i = 0; i < 2000; ++i) {
                float a = generator(i, first), b = generator(i, second);
                assert(a == b);
            }
        }
        std::cout << "Checked " << payload.name << '\n';
    }
    // Exact unsigned formula semantics, including variable shift counts >= 32.
    SoundPayloads::Voice formulaVoice{random, 1u};
    for (uint32_t t : {0u, 1u, 65535u, 65536u, 2097152u, std::numeric_limits<uint32_t>::max()}) {
        auto byte = [](uint32_t raw) { return float(raw & 255u) / 127.5f - 1.0f; };
        assert(SoundPayloads::BytebeatXenonTear(t, formulaVoice) == byte((t * ((t >> 5) | (t >> 8))) ^ (t >> 16)));
        assert(SoundPayloads::BytebeatAcidFracture(t, formulaVoice) == byte(
            (t * 5u & t >> 7) | (t * 3u & t >> 10) | (t * (((t >> 12) | (t >> 8)) & 63u & (t >> 4)))));
        assert(SoundPayloads::BytebeatVoidEngine(t, formulaVoice) == byte(
            ((t >> 6) | t | (t >> ((t >> 16) & 31u))) * 10u + ((t >> 11) & 7u)));
    }
    SoundPayloads::Voice strobe{random, 1u};
    for (uint32_t t = 0; t < SoundPayloads::SampleRate; ++t) {
        float value = SoundPayloads::SquareWaveStrobe(t, strobe);
        assert(t % 320u < 160u ? fabsf(value) == 1 : value == 0);
    }
    assert(SoundPayloads::Synthesize(nullptr, 10, random).empty());
    assert(SoundPayloads::Synthesize(SoundPayloads::SquareWaveStrobe, 0, random).empty());
    assert(SoundPayloads::Synthesize(SoundPayloads::SquareWaveStrobe, 121, random).empty());
    std::cout << "Payload smoke tests passed (all 18 sounds; no playback or overlay).\n";
}
