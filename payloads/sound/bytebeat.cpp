#include "sound.h"

namespace SoundPayloads {
namespace {
float Normalize(uint32_t value) { return (static_cast<float>(value & 255) - 127.5f) / 127.5f; }
}
float BytebeatPulse(uint32_t t, std::mt19937&) {
    return Normalize(t * ((t >> 5) | (t >> 8)) >> (t >> 16 & 3));
}
float BytebeatMelody(uint32_t t, std::mt19937&) {
    return Normalize(t * (((t >> 12) | (t >> 8)) & 63 & (t >> 4)));
}
float BytebeatArpeggio(uint32_t t, std::mt19937&) {
    return Normalize((t * ((t >> 10 & 7) + 1)) ^ (t >> 4) ^ (t >> 7));
}
}
