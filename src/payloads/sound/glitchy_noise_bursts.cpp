#include "chaos.h"

namespace SoundPayloads {
float MemoryBusGlitch(uint32_t t, Voice& voice) {
    // Synthetic register patterns only: never read process memory or hardware registers.
    uint32_t address = t * 1664525u + 1013904223u;
    uint32_t lane = (t / 1600u) & 3u;
    uint32_t data = lane == 0 ? ((t * 13u) ^ (t >> 3)) :
        lane == 1 ? ((address >> ((t >> 9) & 15u)) ^ t) :
        lane == 2 ? ((t << 3) | (t >> 5)) : ((t * ((t >> 8) & 31u)) ^ (address >> 24));
    float carrier = Chaos::Square(Chaos::Advance(voice.phase, 700 + float((t >> 7) & 15u) * 170));
    float packet = t % 800u < 640u ? 1.0f : 0.08f;
    return packet * (0.7f * Chaos::Byte(data) + 0.3f * carrier);
}
} // namespace SoundPayloads
