#include "sound.h"
#include <cmath>

namespace SoundPayloads {
float EntropyHandshake(uint32_t t, Voice& voice) {
    if (t == 0) {
        // Fold a few desktop pixels into the clip-local seed; retain no image data.
        HDC desktop = GetDC(nullptr);
        if (desktop) {
            int w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN);
            for (int i = 1; i <= 16; ++i) {
                COLORREF pixel = GetPixel(desktop, (w - 1) * i / 17, (h - 1) * ((i * 7) % 17) / 17);
                if (pixel != CLR_INVALID) voice.noiseState = (voice.noiseState << 5 | voice.noiseState >> 27) ^ pixel;
            }
            ReleaseDC(nullptr, desktop);
        }
        if (!voice.noiseState) voice.noiseState = 1;
    }
    // Maximal-length 32-bit Galois LFSR, separate from the other generators' PRNG.
    uint32_t bit = voice.noiseState & 1u;
    voice.noiseState = (voice.noiseState >> 1) ^ (bit ? 0x80200003u : 0u);
    float noise = (float(voice.noiseState >> 8) / 8388607.5f) - 1;
    voice.memory[0] += 0.12f * (noise - voice.memory[0]);
    float high = noise - voice.memory[0];
    voice.memory[1] += 0.56f * (noise - voice.memory[1]);
    voice.memory[2] += 0.08f * (noise - voice.memory[2]);
    float band = voice.memory[1] - voice.memory[2];
    float filtered = ((t / 640) & 1u) ? high : band;
    uint32_t packet = t / 240, age = t % 240;
    float gate = age < 170 ? 1.0f : 0.12f;
    // 3 kHz instead of 4 kHz: a carrier exactly at Nyquist is not reproducible.
    float frequency = (packet & 3u) == 0 ? 3000.0f : 900.0f + float(packet % 7) * 260;
    voice.phase += frequency / SampleRate;
    voice.phase -= floorf(voice.phase);
    float carrier = voice.phase < 0.5f ? 1.0f : -1.0f;
    float click = age < 5 ? (age & 1u ? -0.3f : 0.3f) : 0;
    return (0.48f * filtered + 0.26f * carrier) * gate + click;
}
} // namespace SoundPayloads
