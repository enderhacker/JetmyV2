# Payloads

All 53 payloads: 34 visual effects, one finale and 18 sounds. Use `JetmyV2.exe --menu`
to preview them individually. Visual effects draw into an overlay without changing
the desktop or Windows display settings. Audio payloads loop while selected.

## Visual payloads

| Payload | Quick summary |
| --- | --- |
| Random Rectangles | Random architectural panels appear and change across the screen. |
| Black Hole | A randomly placed black hole wanders with smoothly changing random direction and speed, bending the desktop around its dark center and glowing rim. |
| Shattered Screen | The desktop breaks into irregular triangular shards with randomized falls and spins. |
| Growing Light Trails | Branching digital filaments spread over the desktop with glowing tips and permanent trails until restarted. |
| Nested Windows | Moving windows show recursive desktop views with nested title bars. |
| Wavy Screen | Gentle intersecting waves distort the desktop with watery highlights. |
| Sorted Color Bands | 24 large overlapping color-channel-sorted bands smear the desktop into torn, displaced scanlines; four bands reshuffle every 50 ms. |
| Neon Fractal | Slowly rotating neon Julia-set fractal contours cover the desktop. |
| Cursor Trail | Ghost cursors follow the mouse's recent path in a spreading comet trail. |
| Screen Portals | Paired blue and orange portals show the desktop at each other's locations. |
| Old TV Interference | Simulates an old failing display with scanlines, interference and horizontal jitter. |
| Exploding Blocks | Desktop fragments become extruded blocks with shaded sides, a new random blast direction and randomized fragment paths each loop. |
| Split Colors | Separates and shifts the desktop's red, green and blue channels. |
| Kaleidoscope | Mirrors the desktop into six rotating kaleidoscope sectors. |
| Dripping Paint | Thin paint drips leave winding trails that remain while the effect is enabled. |
| Delayed Screen Copies | Moving panes blend detailed delayed snapshots of the desktop. |
| Round Tunnel | Maps the desktop onto a perspective tunnel with rings and radial ribs. |
| Desktop City | Flies down a night-time avenue between desktop-textured skyscrapers, with luminous window grids, street reflections and distance fog. |
| Desktop Planet | Wraps the desktop around a rotating globe with atmospheric glow and a tilted, textured ring that passes in front and behind. |
| Screen Collapse | Compresses the screen vertically and horizontally, then reopens it with an afterglow. |
| Rotate Screen | Continuously rotates the captured desktop image around its center. |
| Random Icons | Places warning, success, information and error icons at random positions and sizes. |
| Melting Screen | Stretches the desktop downward in uneven folds, then fades in a fresh image for the next melt. |
| Repeating Screen Tunnel | Repeated shrinking desktop copies, including the visible cursor, form a continuous rectangular tunnel. |
| Color Patterns | Animated XOR patterns mix the desktop's colors into moving geometric interference. |
| Flashing Inverted Colors | Alternates inverted and normal desktop colors every 500 ms, starting inverted; full-screen flashing without display-setting changes. |
| Fake Message Popups | 24 simulated system messages with OS-aware XP, Vista/7, Windows 10 or Windows 11 styling, system fonts, DPI-scaled layouts and status icons; no real dialogs open. |
| Random Paint Squares | Continuously paints random squares, including previously painted ones, until the screen is covered and beyond. |
| Wavy Color Distortion | Warps the incoming image beneath animated, full-screen color interference. |
| Flashing Color Vortex | Rapidly flashing, full-screen vortex distortion and intense shifting colors. |
| Sliding Screen Strips | Alternating 2–8-pixel horizontal strips shear in counter-phase; two-pixel vertical columns join halfway through each cycle to form a shredded woven grid. |
| Zooming Screen Glitch | Expands its previous overlay frame by approximately 101–102% per nominal frame, combining XOR and brightened difference feedback with a periodic desktop-refresh sweep. |
| Flashing Brightness Bands | Cycles luminance bit-planes from bit 7 to bit 0 as monochrome and neon binary contours, with brief frame-visible inverted flashes. |
| Screen Decay | Slowly drifts the captured screen slightly off the edges, increases blockiness and color loss, stalls coding tiles and tears strips, then builds dark red/orange heat distortion over 35 seconds. Stays corrupted and animated until disabled; restarting resets the decay. All damage and low-bitrate artifacts are simulated. |

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
| Harsh Digital Buzz | Abrasive recursive-shift bytebeat, syncopated clock dividers and ring-modulated, bit-crushed sawtooth sweeps. |
| Digital Noise Bursts | LFSR bursts seeded from 16 pixels in one temporary desktop capture alternate high-pass and band-pass noise with collision clicks and sharp carrier bursts; uses 3 kHz rather than a 4 kHz Nyquist-limit carrier in the 8 kHz stream. |
| Stuttering Noise | Loops an approximately 12 ms synthetic buffer fragment with pitch jitter, granular slips and clipped distortion. |
| Two-Tone Siren | Out-of-phase square-wave sirens rapidly sweep between 450 Hz and 3.8 kHz. |
| Metallic Clicks | Unsigned shift/multiply/XOR formula fractures into metallic ticks and noise bursts. |
| Crunchy Digital Noise | Degrades from 8-bit quantization to single-bit PWM with sample-hold aliasing and offset jumps. |
| Rattling Bass | A 32 Hz square bass collides with unstable carriers and rattling difference tones. |
| Distorted Modem | Stylized, distorted handshake pilots, phase reversals and high-pass demodulation hash; not a V.34 implementation. |
| Rapid Digital Notes | High-speed bitwise arpeggios fracture across registers with jagged algorithmic rhythms. |
| Rapid Computer Beeps | Four independent BIOS-tone square voices collide with parity flips and brief silence gaps. |
| Pulsing Beep | An 800 Hz square wave chopped by a 25 Hz, 50%-duty amplitude gate. |
| Wobbling Squeal | Two-operator FM with previous-sample feedback alternates screeching and motorboating. |
| Building Static | Poisson-distributed crackles accelerate exponentially into saturated static over each ten-second cycle. |
| Rising Buzz | Rising octave-spaced sawtooth bank with increasing pulse tempo and harsh tritone partners. |
| Clashing Tones | Hard-clipped waves a tritone apart (frequency ratio sqrt(2)) continuously cross-modulate and ring-modulate. |
| Grinding Digital Noise | Grinding unsigned shift/multiply formula produces mechanical chugs and digital squeals; variable shift counts wrap modulo 32. |
| Glitchy Noise Bursts | Synthetic register-like patterns become pseudo-PCM squeals and packet bursts; no actual memory/register access. |
| Whistling Noise | Noise feeds a bounded, swept fractional-delay feedback comb with approximately 1–3.6 kHz fundamental resonances. |

The new visuals contain intense motion and flashing; all sounds are intentionally
harsh. Start with low system volume. The 8 kHz mono stream cannot reproduce tones
above its 4 kHz Nyquist limit; aliasing is intentional in the digital effects.
Generators clip/soft-clip their distortion, and the player preserves filtering,
bounded PCM (maximum magnitude 6000), loop smoothing and selection crossfades.
Amplitude limits do not guarantee safe listening levels. Existing emergency
shortcuts stop all effects; no system volume or audio-driver settings are changed.
