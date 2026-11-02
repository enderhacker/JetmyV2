#include "sound.h"
#include <algorithm>

namespace SoundPayloads {
std::vector<int16_t> Synthesize(Generator generator, unsigned seconds, std::mt19937& random) {
    std::vector<int16_t> samples(static_cast<size_t>(SampleRate) * seconds);
    constexpr size_t fadeSamples = SampleRate / 50; // 20 ms fade-in/out, no hard edges.
    for (size_t i = 0; i < samples.size(); ++i) {
        float fade = std::min({1.0f, float(i) / fadeSamples, float(samples.size() - 1 - i) / fadeSamples});
        float value = std::clamp(generator(static_cast<uint32_t>(i), random), -1.0f, 1.0f);
        samples[i] = static_cast<int16_t>(value * fade * 6500); // ~20% amplitude; never change system volume.
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
    if (payloads.empty() || seconds == 0) return false;
    size_t index = std::uniform_int_distribution<size_t>(0, payloads.size() - 1)(random);
    if (payloads.size() > 1 && index == previous_) index = (index + 1) % payloads.size();
    previous_ = index;
    samples_ = Synthesize(payloads[index].generate, seconds, random);

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
    return true;
}
}
