#include "chaos.h"

namespace SoundPayloads {
float BytebeatVoidEngine(uint32_t t, Voice&) {
    // Explicit modulo-32 shift count: the literal formula otherwise has undefined shifts.
    return Chaos::Byte(((t >> 6) | t | (t >> ((t >> 16) & 31u))) * 10u + ((t >> 11) & 7u));
}
} // namespace SoundPayloads
