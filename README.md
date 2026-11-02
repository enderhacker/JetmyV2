# Visual and sound payload demo

Windows desktop overlay with 25 visual effects and five synthesized sounds.
Effects only draw into the overlay; they do not modify the desktop itself.

## Sequence

- 0–10 seconds: one random visual, no sound.
- 10–20 seconds: add a second visual and play a random sound.
- 20–30 seconds: add a third visual and stop sound.
- Continue alternating silent/sound stages every 10 seconds.
- 90–100 seconds: ten stacked visuals and one sound.
- At 100 seconds: stop audio, destroy the overlay, refresh the desktop, and quit.

Visuals are selected without repeats. Existing visuals remain active as new ones
are added. Audio never stacks: one owned WinMM playback buffer is stopped before
replacement or a silent stage. Audio uses bounded amplitude and short fades, and
does not change system volume. If no audio device is available, visuals continue.

**Exit early:** Ctrl+Shift+Q or Ctrl+Shift+Esc.
Warning: flashing visuals and electronic sounds; avoid use with photosensitivity.

## Layout and build

- `main.cpp`: overlay, resource cleanup, capture/compositing loop.
- `runtime/schedule.h`: ten-stage timing and sound policy.
- `payloads/visual/`: `VisualPayloads`, shared raster helpers, registry, one effect per `.cpp`.
- `payloads/sound/`: `SoundPayloads`, bytebeat/random generators, registry, single-player audio.

Run `compile.cmd` from this directory with MinGW-w64 `g++` on PATH.
It compiles all modules and links GDI, User32, and WinMM into `JetmyV2.exe`.
The existing build script stops a running `JetmyV2.exe` before replacing it.

## Payload identities

Every visual receives its own elapsed time from activation. Particle counts,
history, branches, and sorting regions are bounded. Full-screen raster effects
use resolution-scaled sampling; tunnel and kaleidoscope mappings are cached.
Drawing-only effects avoid unnecessary full-frame snapshots. A display-mode
change exits cleanly rather than continuing with stale framebuffer dimensions.

| Visual | Distinct behavior |
| --- | --- |
| Pixel Fireworks | Rising rockets explode into gravity-driven sparks with fading tails. |
| Chaos Rectangles | Grid-aligned architectural panels; one panel changes per tick. |
| Black Hole Singularities | Local gravitational lens, dark event horizon, hot accretion rim. |
| Desktop Glass Shatter | Independently rotating triangular image fragments. |
| Living Digital Infection | Persistent, branching filaments with bright growing tips. |
| Impossible Windows | Moving recursive desktop viewports with nested title bars. |
| Liquid Desktop | Slow crossed wave fields with subtle water caustics. |
| Pixel Sorting Anomaly | Narrow, stable channel-sorted ribbons. |
| Neon Fractal Invasion | Slowly rotating Julia-set contours in a fixed neon palette. |
| Ghost Cursor Swarm | Mouse-history cursor comet with delayed, spreading followers. |
| Spatial Portal Network | Paired blue/orange gateways sample one another's locations. |
| CRT Failure Simulator | Scanlines, edge vignette, rolling interference and horizontal jitter. |
| Voxel Desktop Explosion | Extruded rectangular image blocks with shaded side faces. |
| Matrix Reconstruction | Independently moving green glyph streams with pale leading characters. |
| RGB Spectral Phantoms | Independently displaced red, green, and blue image channels. |
| Kaleidoscopic Reality | Six-sector mirrored image symmetry with slow source rotation. |
| Gravity Paint | Glossy, winding liquid rivulets with rounded heavy droplets. |
| Desktop Time Echoes | Four moving panes blend different capture-history delays. |
| 3D Warp Tunnel | Perspective image-textured tunnel with radial ribs and traveling rings. |
| Geometric Implosion | Flat nested nine-sided guilloche outlines converge inward. |
| X-Ray Scanner | Feathered inspection pane reveals luminance edges and inverted film. |
| Screen Peel & Fold | Diagonal paper curl, reflected underside, backing and cast shadow. |
| Holographic Glitch Objects | Projected cube, octahedron and tetrahedron with ghost edges. |
| Synesthesia Storm | Smoothed radial pseudo-spectrum, peak markers and procedural scope traces. |
| Reality Collapse Finale | Vertical screen compression, horizontal collapse, afterglow and reopening. |

The spectrum is procedural, not microphone input or audio analysis. Sounds have
independent per-clip state, DC removal, mild low-pass filtering, bounded output,
and 30 ms clip-edge fades. Consecutive sound stages never select the same sound.

| Sound | Distinct behavior |
| --- | --- |
| Bytebeat Pulse | Syncopated low bit-crushed pulse bass. |
| Bytebeat Melody | Warm, plucked pentatonic triangle melody with bytebeat texture. |
| Bytebeat Arpeggio | Fast high-register dual-oscillator chip arpeggio. |
| Random Chirps | Randomized ascending/descending whistles separated by silence. |
| Noise Percussion | Sixteen-step kick, snare and high-passed noise-hat pattern. |
