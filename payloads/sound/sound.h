#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace SoundPayloads {
inline constexpr unsigned SampleRate = 8000;
// Every generated clip owns its oscillator/noise state; nothing leaks into the next sound.
struct Voice {
    std::mt19937& random;
    uint32_t noiseState;
    float phase = 0, secondPhase = 0;
    std::array<float, 10> phases{};
    std::array<float, 4> frequencies{};
    std::array<uint32_t, 4> timers{};
    std::array<float, 4> memory{};
    std::array<float, 512> buffer{};
    uint32_t cursor = 0, loopSize = 100;
    float position = 0;
    float Noise() {
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return float(noiseState >> 8) * (2.0f / 16777215.0f) - 1;
    }
};
using Generator = float (*)(uint32_t sample, Voice& voice);
struct PayloadRecord { const char* name; Generator generate; };
const std::vector<PayloadRecord>& Registry();

float BytebeatIndustrialScream(uint32_t, Voice&);
float EntropyHandshake(uint32_t, Voice&);
float BufferFreezeStutter(uint32_t, Voice&);
float DualToneSirenWarp(uint32_t, Voice&);
float BytebeatXenonTear(uint32_t, Voice&);
float BitRotQuantizer(uint32_t, Voice&);
float SubharmonicMeltdown(uint32_t, Voice&);
float ModemCarrierHell(uint32_t, Voice&);
float BytebeatAcidFracture(uint32_t, Voice&);
float PCSpeakerBeepStorm(uint32_t, Voice&);
float SquareWaveStrobe(uint32_t, Voice&);
float FMFeedbackHowl(uint32_t, Voice&);
float RadiationAvalanche(uint32_t, Voice&);
float ShepardPanicAscent(uint32_t, Voice&);
float RingModTritoneClash(uint32_t, Voice&);
float BytebeatVoidEngine(uint32_t, Voice&);
float MemoryBusGlitch(uint32_t, Voice&);
float CombFilterScreamer(uint32_t, Voice&);
float BrokenVowel(uint32_t, Voice&);
float RatchetCrash(uint32_t, Voice&);
float ReverseShrapnel(uint32_t, Voice&);
std::vector<int16_t> Synthesize(Generator generator, unsigned seconds, std::mt19937& random, bool loop = false);

// A single continuously queued output stream; selection changes crossfade inside it.
class Player {
public:
    Player() = default;
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    bool PlayRandom(unsigned seconds, std::mt19937& random);
    bool Play(size_t index, unsigned seconds, std::mt19937& random);
    bool IsPlaying() const { return healthy_.load(); }
    int ActiveIndex() const { return IsPlaying() ? active_ : -1; }
    void Stop();
private:
    static constexpr size_t BlockSamples = 256;
    struct Block {
        WAVEHDR header{};
        std::array<int16_t, BlockSamples> samples{};
        bool prepared = false, queued = false;
    };
    bool Open();
    void Stream();
    HWAVEOUT device_ = nullptr;
    HANDLE completed_ = nullptr, wake_ = nullptr;
    std::array<Block, 3> blocks_{};
    std::thread worker_;
    std::atomic<bool> stopping_{false}, healthy_{false};
    std::mutex mutex_;
    std::shared_ptr<const std::vector<int16_t>> requested_;
    int active_ = -1;
    size_t previous_ = static_cast<size_t>(-1);
};
} // namespace SoundPayloads
