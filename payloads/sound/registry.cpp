#include "sound.h"

namespace SoundPayloads {
const std::vector<PayloadRecord>& Registry() {
    static const std::vector<PayloadRecord> payloads = {
        {"Bytebeat Industrial Scream", BytebeatIndustrialScream},
        {"Entropy Handshake", EntropyHandshake},
        {"Buffer Freeze Stutter", BufferFreezeStutter},
        {"Dual-Tone Siren Warp", DualToneSirenWarp},
        {"Bytebeat Xenon Tear", BytebeatXenonTear},
        {"Bit-Rot Quantizer", BitRotQuantizer},
        {"Subharmonic Meltdown", SubharmonicMeltdown},
        {"Modem Carrier Hell", ModemCarrierHell},
        {"Bytebeat Acid Fracture", BytebeatAcidFracture},
        {"PC Speaker Beep Storm", PCSpeakerBeepStorm},
        {"Square-Wave Strobe", SquareWaveStrobe},
        {"FM Feedback Howl", FMFeedbackHowl},
        {"Radiation Avalanche", RadiationAvalanche},
        {"Shepard Panic Ascent", ShepardPanicAscent},
        {"Ring-Mod Tritone Clash", RingModTritoneClash},
        {"Bytebeat Void Engine", BytebeatVoidEngine},
        {"Memory Bus Glitch", MemoryBusGlitch},
        {"Comb Filter Screamer", CombFilterScreamer}
    };
    return payloads;
}
}
