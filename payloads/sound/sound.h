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
#include <cstdint>
#include <random>
#include <vector>

namespace SoundPayloads {
inline constexpr unsigned SampleRate = 8000;
// Every generated clip owns its oscillator/noise state; nothing leaks into the next sound.
struct Voice {
    std::mt19937& random;
    uint32_t noiseState;
    float phase = 0, secondPhase = 0, frequency = 440, sweep = 0, filteredNoise = 0;
    std::array<uint32_t, 8> songPhase{}, songIncrement{};
    std::array<int, 8> songNote{};
    std::array<float, 4> memory{};
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

float BytebeatPulse(uint32_t, Voice&);
float BytebeatMelody(uint32_t, Voice&);
float BytebeatArpeggio(uint32_t, Voice&);
float RandomChirps(uint32_t, Voice&);
float NoisePercussion(uint32_t, Voice&);
float BytebeatNeonDrive(uint32_t, Voice&);
float BytebeatPocketGroove(uint32_t, Voice&);
float BytebeatCrystalWaltz(uint32_t, Voice&);
float BytebeatArcadeQuest(uint32_t, Voice&);
float BytebeatMidnightKeys(uint32_t, Voice&);
float BytebeatDuskBreaks(uint32_t, Voice&);
float SonarSoundscape(uint32_t, Voice&);
float GlassBells(uint32_t, Voice&);
float RainOnMetal(uint32_t, Voice&);
float CosmicDrone(uint32_t, Voice&);
std::vector<int16_t> Synthesize(Generator generator, unsigned seconds, std::mt19937& random);

// One device and one owned buffer: replacing a sound always resets the old one.
class Player {
public:
    Player() = default;
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    bool PlayRandom(unsigned seconds, std::mt19937& random);
    void Stop();
private:
    HWAVEOUT device_ = nullptr;
    WAVEHDR header_ = {};
    bool prepared_ = false;
    std::vector<int16_t> samples_;
    size_t previous_ = static_cast<size_t>(-1);
};
} // namespace SoundPayloads
