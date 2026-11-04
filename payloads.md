# Payloads

All 52 payloads: 33 visual effects, one finale and 18 sounds. Use `JetmyV2.exe --menu`
to preview them individually. Visual effects draw into an overlay without changing
the desktop or Windows display settings. Audio payloads loop while selected.

## Visual payloads

| Payload | Quick summary |
| --- | --- |
| Chaos Rectangles | Random architectural panels appear and change across the screen. |
| Black Hole Singularities | A randomly placed black hole wanders with smoothly changing random direction and speed, bending the desktop around its dark center and glowing rim. |
| Desktop Glass Shatter | The desktop breaks into irregular triangular shards with randomized falls and spins. |
| Living Digital Infection | Branching digital filaments spread over the desktop with glowing tips and permanent trails until restarted. |
| Impossible Windows | Moving windows show recursive desktop views with nested title bars. |
| Liquid Desktop | Gentle intersecting waves distort the desktop with watery highlights. |
| Pixel Sorting Anomaly | 24 large overlapping color-channel-sorted bands smear the desktop into torn, displaced scanlines; four bands reshuffle every 50 ms. |
| Neon Fractal Invasion | Slowly rotating neon Julia-set fractal contours cover the desktop. |
| Ghost Cursor Swarm | Ghost cursors follow the mouse's recent path in a spreading comet trail. |
| Spatial Portal Network | Paired blue and orange portals show the desktop at each other's locations. |
| CRT Failure Simulator | Simulates an old failing display with scanlines, interference and horizontal jitter. |
| Voxel Desktop Explosion | Desktop fragments become extruded blocks with shaded sides, a new random blast direction and randomized fragment paths each loop. |
| RGB Spectral Phantoms | Separates and shifts the desktop's red, green and blue channels. |
| Kaleidoscopic Reality | Mirrors the desktop into six rotating kaleidoscope sectors. |
| Gravity Paint | Thin paint drips leave winding trails that remain while the effect is enabled. |
| Desktop Time Echoes | Moving panes blend detailed delayed snapshots of the desktop. |
| 3D Warp Tunnel | Maps the desktop onto a perspective tunnel with rings and radial ribs. |
| Infinite Desktop City | Flies down a night-time avenue between desktop-textured skyscrapers, with luminous window grids, street reflections and distance fog. |
| Desktop Planet | Wraps the desktop around a rotating globe with atmospheric glow and a tilted, textured ring that passes in front and behind. |
| Reality Collapse Finale | Compresses the screen vertically and horizontally, then reopens it with an afterglow. |
| Rotate Screen | Continuously rotates the captured desktop image around its center. |
| Random Status Icons | Places warning, success, information and error icons at random positions and sizes. |
| Melting Screen | Stretches the desktop downward in uneven folds, then fades in a fresh image for the next melt. |
| Infinite Tunnel | Repeated shrinking desktop copies, including the visible cursor, form a continuous rectangular tunnel. |
| XOR Color Chaos | Animated XOR patterns mix the desktop's colors into moving geometric interference. |
| Screen Color Inversion | Alternates inverted and normal desktop colors every 500 ms, starting inverted; full-screen flashing without display-setting changes. |
| Popup Cascade | 24 simulated system messages with OS-aware XP, Vista/7, Windows 10 or Windows 11 styling, system fonts, DPI-scaled layouts and status icons; no real dialogs open. |
| Random Square Paint | Continuously paints random squares, including previously painted ones, until the screen is covered and beyond. |
| Chaos Shader | Warps the incoming image beneath animated, full-screen color interference. |
| Hell Shader | Rapidly flashing, full-screen vortex distortion and intense shifting colors. |
| Seismic Slice & Drift | Alternating 2–8-pixel horizontal strips shear in counter-phase; two-pixel vertical columns join halfway through each cycle to form a shredded woven grid. |
| Recursive ROP Crucible | Expands its previous overlay frame by approximately 101–102% per nominal frame, combining XOR and brightened difference feedback with a periodic desktop-refresh sweep. |
| Bit-Plane Topography | Cycles luminance bit-planes from bit 7 to bit 0 as monochrome and neon binary contours, with brief frame-visible inverted flashes. |

## Finale

**Simulated Blue Screen:** always follows the automatic 100-second show, or runs
from the menu's **Blue screen (10s)** button. Stops all visuals and audio, then
shows a centered 4:3 version-specific crash screen for 10 seconds with a continuous 800 Hz
error tone at bounded amplitude and black surrounding
space and the cursor hidden over the presentation. Other monitors are blacked out.
Artwork uses a native-resolution canvas, antialiased outline fonts and whole-pixel
QR modules, without upscaling low-resolution screenshots or changing system settings.
Afterward, automatic mode exits; manual mode returns with all payloads off.
No crash, reboot, memory dump, display-mode change or global input lock occurs.
Ordinary input is consumed only while its window has focus. Escape and emergency
shortcuts can exit early; system shortcuts remain available.
Automatic mode detects the Windows version; the menu can also preview XP, Vista,
7, 10, and the historical Windows 11 blue design individually. Classic styles use
monospaced STOP diagnostics; modern styles use Segoe UI, a sad face, progress,
stop-code details, and a locally generated QR code linking to Microsoft's support
page. The Windows 11 preset intentionally does not use its newer black design.

## Sound payloads

| Payload | Quick summary |
| --- | --- |
| Bytebeat Industrial Scream | Abrasive recursive-shift bytebeat, syncopated clock dividers and ring-modulated, bit-crushed sawtooth sweeps. |
| Entropy Handshake | Desktop-pixel-seeded LFSR bursts alternate high-pass and band-pass noise with collision clicks and sharp carrier bursts; uses 3 kHz rather than a 4 kHz Nyquist-limit carrier in the 8 kHz stream. |
| Buffer Freeze Stutter | Loops an approximately 12 ms synthetic buffer fragment with pitch jitter, granular slips and clipped distortion. |
| Dual-Tone Siren Warp | Out-of-phase square-wave sirens rapidly sweep between 450 Hz and 3.8 kHz. |
| Bytebeat Xenon Tear | Unsigned shift/multiply/XOR formula fractures into metallic ticks and noise bursts. |
| Bit-Rot Quantizer | Degrades from 8-bit quantization to single-bit PWM with sample-hold aliasing and offset jumps. |
| Subharmonic Meltdown | A 32 Hz square bass collides with unstable carriers and rattling difference tones. |
| Modem Carrier Hell | Stylized, distorted handshake pilots, phase reversals and high-pass demodulation hash; not a V.34 implementation. |
| Bytebeat Acid Fracture | High-speed bitwise arpeggios fracture across registers with jagged algorithmic rhythms. |
| PC Speaker Beep Storm | Four independent BIOS-tone square voices collide with parity flips and brief silence gaps. |
| Square-Wave Strobe | An 800 Hz square wave chopped by a 25 Hz, 50%-duty amplitude gate. |
| FM Feedback Howl | Two-operator FM with previous-sample feedback alternates screeching and motorboating. |
| Radiation Avalanche | Poisson-distributed crackles accelerate exponentially into saturated static over each ten-second cycle. |
| Shepard Panic Ascent | Rising octave-spaced sawtooth bank with increasing pulse tempo and harsh tritone partners. |
| Ring-Mod Tritone Clash | Hard-clipped waves a tritone apart (frequency ratio sqrt(2)) continuously cross-modulate and ring-modulate. |
| Bytebeat Void Engine | Grinding unsigned shift/multiply formula produces mechanical chugs and digital squeals; variable shift counts wrap modulo 32. |
| Memory Bus Glitch | Synthetic register-like patterns become pseudo-PCM squeals and packet bursts; no actual memory/register access. |
| Comb Filter Screamer | Noise feeds a bounded, swept fractional-delay feedback comb with approximately 1–3.6 kHz fundamental resonances. |

The new visuals contain intense motion and flashing; all sounds are intentionally
harsh. Start with low system volume. The 8 kHz mono stream cannot reproduce tones
above its 4 kHz Nyquist limit; aliasing is intentional in the digital effects.
Generators clip/soft-clip their distortion, and the player preserves filtering,
bounded PCM (maximum magnitude 6000), loop smoothing and selection crossfades.
Amplitude limits do not guarantee safe listening levels. Existing emergency
shortcuts stop all effects; no system volume or audio-driver settings are changed.
