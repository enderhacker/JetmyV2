#include "sound.h"
#include <cmath>

namespace SoundPayloads {
float BytebeatIndustrialScream(uint32_t t, Voice& voice) {
    uint32_t raw = (t * (((t >> 9) | (t >> 13)) & 15u)) ^ ((t >> 5) & (t >> 7));
    float byte = (float(raw & 255u) - 127.5f) / 127.5f;
    float sweep = float(t % (SampleRate * 2)) / (SampleRate * 2);
    voice.phase += (110.0f + 1800.0f * sweep * sweep) / SampleRate;
    voice.phase -= floorf(voice.phase);
    float saw = floorf(voice.phase * 16) / 7.5f - 1;
    float clock = ((t >> 8) ^ (t >> 11)) & 1u ? 1.0f : 0.35f;
    float ring = ((t >> 3) & 1u) ? 1.0f : -1.0f;
    return (0.68f * byte + 0.22f * saw * ring) * clock;
}
} // namespace SoundPayloads
