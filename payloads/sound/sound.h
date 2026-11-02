#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <random>
#include <vector>

namespace SoundPayloads {
inline constexpr unsigned SampleRate = 8000;
using Generator = float (*)(uint32_t sample, std::mt19937& random);
struct PayloadRecord { const char* name; Generator generate; };
const std::vector<PayloadRecord>& Registry();

float BytebeatPulse(uint32_t, std::mt19937&);
float BytebeatMelody(uint32_t, std::mt19937&);
float BytebeatArpeggio(uint32_t, std::mt19937&);
float RandomChirps(uint32_t, std::mt19937&);
float NoisePercussion(uint32_t, std::mt19937&);
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
