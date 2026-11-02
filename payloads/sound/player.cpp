#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads {
std::vector<int16_t> Synthesize(Generator generator, unsigned seconds, std::mt19937& random) {
    // Bound allocation and WAVEHDR's 32-bit byte count even for future callers.
    if (!generator || seconds == 0 || seconds > 120) return {};
    std::vector<int16_t> samples(static_cast<size_t>(SampleRate) * seconds);
    Voice voice{random, static_cast<uint32_t>(random()) | 1u};
    constexpr size_t fadeSamples = SampleRate * 3 / 100; // 30 ms clip-edge fades.
    float lastInput = 0, highpass = 0, lowpass = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        float fade = std::min({1.0f, float(i) / fadeSamples, float(samples.size() - 1 - i) / fadeSamples});
        float value = generator(static_cast<uint32_t>(i), voice);
        if (!std::isfinite(value)) value = 0;
        value = std::clamp(value, -1.0f, 1.0f);
        highpass = value - lastInput + 0.995f * highpass;
        lastInput = value;
        lowpass += 0.65f * (highpass - lowpass);
        samples[i] = static_cast<int16_t>(std::clamp(lowpass, -1.0f, 1.0f) * fade * 6000);
    }
    return samples;
}

Player::~Player() { Stop(); }

void Player::Stop() {
    if (device_) {
        // Reset returns queued buffers before releasing their memory.
        waveOutReset(device_);
        if (prepared_) waveOutUnprepareHeader(device_, &header_, sizeof(header_));
        waveOutClose(device_);
    }
    device_ = nullptr;
    prepared_ = false;
    header_ = {};
    samples_.clear();
}

bool Player::PlayRandom(unsigned seconds, std::mt19937& random) {
    Stop();
    const auto& payloads = Registry();
    if (payloads.empty() || seconds == 0 || seconds > 120) return false;
    size_t index;
    if (payloads.size() > 1 && previous_ < payloads.size()) {
        index = std::uniform_int_distribution<size_t>(0, payloads.size() - 2)(random);
        if (index >= previous_) ++index;
    } else index = std::uniform_int_distribution<size_t>(0, payloads.size() - 1)(random);
    samples_ = Synthesize(payloads[index].generate, seconds, random);
    if (samples_.empty()) return false;

    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = SampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = SampleRate * format.nBlockAlign;
    if (waveOutOpen(&device_, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        device_ = nullptr;
        samples_.clear();
        return false;
    }
    header_.lpData = reinterpret_cast<LPSTR>(samples_.data());
    header_.dwBufferLength = static_cast<DWORD>(samples_.size() * sizeof(int16_t));
    if (waveOutPrepareHeader(device_, &header_, sizeof(header_)) != MMSYSERR_NOERROR) {
        Stop();
        return false;
    }
    prepared_ = true;
    if (waveOutWrite(device_, &header_, sizeof(header_)) != MMSYSERR_NOERROR) {
        Stop();
        return false;
    }
    previous_ = index;
    return true;
}
}
