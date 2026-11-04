#include "chaos.h"

namespace SoundPayloads {
float BytebeatAcidFracture(uint32_t t, Voice&) {
    return Chaos::Byte((t * 5u & t >> 7) | (t * 3u & t >> 10) |
        (t * (((t >> 12) | (t >> 8)) & 63u & (t >> 4))));
}
} // namespace SoundPayloads
