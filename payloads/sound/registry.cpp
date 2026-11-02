#include "sound.h"

namespace SoundPayloads {
const std::vector<PayloadRecord>& Registry() {
    static const std::vector<PayloadRecord> payloads = {
        {"Bytebeat Pulse", BytebeatPulse},
        {"Bytebeat Melody", BytebeatMelody},
        {"Bytebeat Arpeggio", BytebeatArpeggio},
        {"Random Chirps", RandomChirps},
        {"Noise Percussion", NoisePercussion}
    };
    return payloads;
}
}
