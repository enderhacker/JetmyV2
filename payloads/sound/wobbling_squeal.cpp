#include "chaos.h"

namespace SoundPayloads {
float FMFeedbackHowl(uint32_t t, Voice& voice) {
    using namespace Chaos;
    float movement = 0.5f + 0.5f * sinf(Seconds(t) * 1.9f);
    float carrier = Tau * Advance(voice.phase, 45 + 2650 * movement * movement);
    float modulator = Tau * Advance(voice.secondPhase, 19 + 1180 * movement);
    float output = sinf(carrier + 4 * sinf(modulator + 1.2f * voice.memory[0]));
    voice.memory[0] = output;
    return output;
}
} // namespace SoundPayloads
