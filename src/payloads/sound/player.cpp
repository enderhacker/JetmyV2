#include "sound.h"
#include <algorithm>
#include <cmath>

namespace SoundPayloads {
std::vector<int16_t> Synthesize(Generator generator, unsigned seconds, std::mt19937& random, bool loop) {
    if (!generator || seconds == 0 || seconds > 120) return {};
    constexpr size_t seam = SampleRate * 3 / 100;
    size_t length = static_cast<size_t>(SampleRate) * seconds;
    std::vector<int16_t> samples(length + (loop ? seam : 0));
    Voice voice{random, static_cast<uint32_t>(random()) | 1u};
    float lastInput = 0, highpass = 0, lowpass = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        float fade = loop ? 1.0f : std::min({1.0f, float(i) / seam, float(length - 1 - i) / seam});
        float value = generator(static_cast<uint32_t>(i), voice);
        if (!std::isfinite(value)) value = 0;
        value = std::clamp(value, -1.0f, 1.0f);
        highpass = value - lastInput + 0.995f * highpass;
        lastInput = value;
        lowpass += 0.65f * (highpass - lowpass);
        samples[i] = static_cast<int16_t>(std::clamp(lowpass, -1.0f, 1.0f) * fade * 6000);
    }
    if (loop) {
        // Continue beyond the end, then blend that continuation into the start.
        // This preserves the exact loop length without a mute/fade-out at the seam.
        for (size_t i = 0; i < seam; ++i) {
            float mix = float(i) / float(seam - 1);
            mix = mix * mix * (3 - 2 * mix);
            samples[i] = static_cast<int16_t>(samples[length + i] * (1 - mix) + samples[i] * mix);
        }
        samples.resize(length);
    }
    return samples;
}

Player::~Player() { Stop(); }

void Player::Stop() {
    stopping_ = true;
    if (wake_) SetEvent(wake_);
    if (worker_.joinable()) worker_.join();
    if (device_) {
        waveOutReset(device_); // Return all buffers before releasing their memory.
        for (auto& block : blocks_) {
            if (block.prepared) waveOutUnprepareHeader(device_, &block.header, sizeof(WAVEHDR));
        }
        waveOutClose(device_);
    }
    device_ = nullptr;
    if (completed_) CloseHandle(completed_);
    if (wake_) CloseHandle(wake_);
    completed_ = wake_ = nullptr;
    for (auto& block : blocks_) block = {};
    { std::lock_guard<std::mutex> lock(mutex_); requested_.reset(); }
    healthy_ = false;
    active_ = -1;
}

bool Player::Open() {
    completed_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    wake_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!completed_ || !wake_) { Stop(); return false; }
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = SampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = SampleRate * 2;
    if (waveOutOpen(&device_, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(completed_), 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        device_ = nullptr; Stop(); return false;
    }
    for (auto& block : blocks_) {
        block.header.lpData = reinterpret_cast<LPSTR>(block.samples.data());
        block.header.dwBufferLength = static_cast<DWORD>(sizeof(block.samples));
        if (waveOutPrepareHeader(device_, &block.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
            Stop(); return false;
        }
        block.prepared = true;
    }
    stopping_ = false;
    return true;
}

bool Player::Play(size_t index, unsigned seconds, std::mt19937& random) {
    if (index >= Registry().size() || seconds == 0 || seconds > 120) return false;
    try {
        // Keep the old audio running while the replacement is synthesized.
        auto clip = std::make_shared<const std::vector<int16_t>>(Synthesize(Registry()[index].generate, seconds, random, true));
        if (clip->empty()) return false;
        if (device_ && !healthy_) Stop();
        if (!device_ && !Open()) return false;
        { std::lock_guard<std::mutex> lock(mutex_); requested_ = std::move(clip); }
        if (!worker_.joinable()) {
            healthy_ = true;
            try { worker_ = std::thread(&Player::Stream, this); }
            catch (...) { Stop(); return false; }
        }
        SetEvent(wake_);
        previous_ = index;
        active_ = static_cast<int>(index);
        return true;
    } catch (const std::exception&) { return false; }
}

bool Player::PlayRandom(unsigned seconds, std::mt19937& random) {
    size_t count = Registry().size();
    if (count == 0) return false;
    size_t index;
    if (count > 1 && previous_ < count) {
        index = std::uniform_int_distribution<size_t>(0, count - 2)(random);
        if (index >= previous_) ++index;
    } else index = std::uniform_int_distribution<size_t>(0, count - 1)(random);
    return Play(index, seconds, random);
}

void Player::Stream() {
    using Clip = std::shared_ptr<const std::vector<int16_t>>;
    Clip current, outgoing;
    size_t cursor = 0, oldCursor = 0, fade = 0, attack = 0;
    constexpr size_t FadeSamples = SampleRate / 20;
    HANDLE events[] = {wake_, completed_};
    while (!stopping_) {
        for (auto& block : blocks_) {
            if (stopping_) break;
            if (block.queued && !(block.header.dwFlags & WHDR_DONE)) continue;
            Clip next;
            { std::lock_guard<std::mutex> lock(mutex_); next = requested_; }
            if (next && next != current) {
                outgoing = current; oldCursor = cursor;
                current = std::move(next); cursor = 0; fade = 0;
            }
            if (!current) continue;
            for (auto& sample : block.samples) {
                float value = (*current)[cursor];
                cursor = (cursor + 1) % current->size();
                if (outgoing) {
                    float mix = std::min(1.0f, float(fade++) / FadeSamples);
                    mix = mix * mix * (3 - 2 * mix);
                    value = (*outgoing)[oldCursor] * (1 - mix) + value * mix;
                    oldCursor = (oldCursor + 1) % outgoing->size();
                    if (fade > FadeSamples) outgoing.reset();
                }
                value *= std::min(1.0f, float(attack) / FadeSamples);
                if (attack < FadeSamples) ++attack;
                sample = static_cast<int16_t>(std::clamp(value, -6000.0f, 6000.0f));
            }
            if (waveOutWrite(device_, &block.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
                healthy_ = false;
                return;
            }
            block.queued = true;
        }
        if (!stopping_ && WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_FAILED) {
            healthy_ = false;
            return;
        }
    }
}
} // namespace SoundPayloads
