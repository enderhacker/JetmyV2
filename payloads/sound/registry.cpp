#include "sound.h"

namespace SoundPayloads {
const std::vector<PayloadRecord>& Registry() {
    static const std::vector<PayloadRecord> payloads = {
        {"Bytebeat Pulse", BytebeatPulse},
        {"Bytebeat Melody", BytebeatMelody},
        {"Bytebeat Arpeggio", BytebeatArpeggio},
        {"Random Chirps", RandomChirps},
        {"Noise Percussion", NoisePercussion},
        {"Bytebeat Neon Drive", BytebeatNeonDrive},
        {"Bytebeat Pocket Groove", BytebeatPocketGroove},
        {"Bytebeat Crystal Waltz", BytebeatCrystalWaltz},
        {"Bytebeat Arcade Quest", BytebeatArcadeQuest},
        {"Bytebeat Midnight Keys", BytebeatMidnightKeys},
        {"Bytebeat Dusk Breaks", BytebeatDuskBreaks},
        {"Sonar Soundscape", SonarSoundscape},
        {"Glass Bells", GlassBells},
        {"Rain on Metal", RainOnMetal},
        {"Cosmic Drone", CosmicDrone},
        {"Bytebeat Industrial Scream", BytebeatIndustrialScream},
        {"Shepard-Risset Descent", ShepardRissetDescent},
        {"Entropy Handshake", EntropyHandshake}
    };
    return payloads;
}
}
