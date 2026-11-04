#include "sound.h"

namespace SoundPayloads {
const std::vector<PayloadRecord>& Registry() {
    static const std::vector<PayloadRecord> payloads = {
        {"Harsh Digital Buzz", BytebeatIndustrialScream},
        {"Digital Noise Bursts", EntropyHandshake},
        {"Stuttering Noise", BufferFreezeStutter},
        {"Two-Tone Siren", DualToneSirenWarp},
        {"Metallic Clicks", BytebeatXenonTear},
        {"Crunchy Digital Noise", BitRotQuantizer},
        {"Rattling Bass", SubharmonicMeltdown},
        {"Distorted Modem", ModemCarrierHell},
        {"Rapid Digital Notes", BytebeatAcidFracture},
        {"Rapid Computer Beeps", PCSpeakerBeepStorm},
        {"Pulsing Beep", SquareWaveStrobe},
        {"Wobbling Squeal", FMFeedbackHowl},
        {"Building Static", RadiationAvalanche},
        {"Rising Buzz", ShepardPanicAscent},
        {"Clashing Tones", RingModTritoneClash},
        {"Grinding Digital Noise", BytebeatVoidEngine},
        {"Glitchy Noise Bursts", MemoryBusGlitch},
        {"Whistling Noise", CombFilterScreamer}
    };
    return payloads;
}
}
