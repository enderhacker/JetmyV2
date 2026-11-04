#include "chaos.h"

namespace SoundPayloads {
float BytebeatXenonTear(uint32_t t, Voice&) {
    return Chaos::Byte((t * ((t >> 5) | (t >> 8))) ^ (t >> 16));
}
} // namespace SoundPayloads
