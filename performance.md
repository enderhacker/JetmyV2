# Payload performance audit

## Results and scope

Measured off-screen at **1920 × 1080**, on an **Intel i5-9600K (6 cores)**,
using MinGW-w64 GCC 16.2, 32-bit, C++17, `-O3 -ffast-math`, SSE2 and static
linking. See [tests/README.md](tests/README.md) for reproducible commands.
Sixteen sampled phases per effect, identical synthetic input and RNG seeds;
means exclude the activation frame. Numbers are milliseconds, rounded.

The problem is cumulative CPU work, not a special eighth-stage transition.
At stage eight, eight effects are still rendering, usually with full-screen
input snapshots. Several original effects individually exceed a 16.7 ms frame
budget. Random selection determines how severe the late-show slowdown is.

| Reproducible stack depth | Before, ms/frame | After, ms/frame |
| --- | ---: | ---: |
| 1 | 3.3 | 2.4 |
| 4 | 51.0 | 19.7 |
| 8 | 204.2 | 67.3 |
| 10 | 220.5 | 70.7 |

This is approximately **3× faster at eight to ten layers**, **not a claim of
60 FPS**. Capture, compositor/overlay presentation, live desktop content,
resolution, different stacks, and machine load add costs. The reported 1 FPS
live case was not reproduced by launching the flashing show. A ten-layer stack
of software effects can still exceed the frame budget after optimization.

## Every visual payload

| Payload | Before ms | After ms | Main cost / action |
| --- | ---: | ---: | --- |
| Chaos Rectangles | 0.04 | 0.04 | Small bounded panel fills; already cheap. |
| Black Hole Singularities | 31.20 | 9.28 | Per-pixel sqrt, angle, sine/cosine and sampling; parallel independent rows. |
| Desktop Glass Shatter | 15.22 | 6.35 | Triangle bounding-box rasterization; parallel rows within each shard, preserving shard order. Cost varies substantially through each break cycle. |
| Living Digital Infection | 0.02 | 0.02 | Bounded growth and sparse retained ink. Long previews replay more ink, but storage remains bounded by screen area. |
| Impossible Windows | 4.23 | 4.35 | Four GDI stretches, text and synchronization; essentially unchanged. |
| Liquid Desktop | 21.73 | 5.06 | Full-screen sampled blending; parallel rows and no redundant snapshot copy. |
| Pixel Sorting Anomaly | 8.74 | 1.60 | Sorting 24 scanlines, then smearing large bands. Replace per-pixel modulo with two contiguous copies; preserve the original sort and tie ordering. |
| Neon Fractal Invasion | 16.41 | 5.01 | Up to 24 Julia iterations per sampled pixel; parallel rows, same iteration count/sample grid. |
| Ghost Cursor Swarm | 0.68 | 0.63 | 24 small system-cursor draws; unchanged. Output depends on the live cursor. |
| Spatial Portal Network | 3.85 | 4.06 | Local circular sampling and sqrt; bounded affected area, unchanged. |
| CRT Failure Simulator | 17.52 | 4.06 | Full-screen blending; generate row noise in original RNG order, then parallelize pixel work and exchange buffers. |
| Voxel Desktop Explosion | 3.39 | 2.28 | 70 GDI textured blocks; remove the input copy by exchanging DIBs before the existing black clear. |
| RGB Spectral Phantoms | 5.27 | 1.48 | Three sampled channels per pixel; parallel rows and buffer exchange. |
| Kaleidoscopic Reality | 29.21 | 4.80 | Cached geometry, integer reflection/modulo and source sampling; halve reflection modulo operations, parallelize rendering and initial map construction, exchange buffers. |
| Gravity Paint | 0.06 | 0.06 | Sparse retained drips; unchanged. Replay cost grows with painted area but cannot grow unboundedly. |
| Desktop Time Echoes | 15.57 | 5.39 | Full-screen delayed-image blending; parallel rows, preserve interpolation and history timing. History allocation/update is additional runtime work. |
| 3D Warp Tunnel | 39.66 | 11.94 | Full-screen cached polar mapping, wrapping and blending; parallel map construction/rendering and buffer exchange. |
| Infinite Desktop City | 81.08 | 17.39 | Ray traversal plus street/wall sampling; parallel independent columns, precompute scanline street distance and wall-invariant window position, exchange buffers. |
| Desktop Planet | 97.53 | 28.61 | Spherical texture coordinates, ring intersections and expensive trig; parallel rows, skip calculations outside the unchanged halo/ring reach, exchange buffers. **Still the heaviest steady-state effect.** |
| Reality Collapse Finale | 2.43 | 1.25 | One shrinking GDI stretch; exchange buffers before the black clear. This is a normal visual, not the separate blue-screen finale. |
| Rotate Screen | 10.48 | 2.35 | Full-screen inverse-coordinate mapping; parallel rows and buffer exchange. |
| Random Status Icons | 0.31 | 0.30 | Bounded icon draws and small success seals; unchanged. |
| Melting Screen | 37.87 | 9.28 | Previously cache-hostile full-height column writes; precompute column parameters, write contiguous parallel rows, exchange buffers. |
| Infinite Tunnel | 8.73 | 9.00 | Repeated GDI stretches and required GDI/CPU synchronization; unchanged, within normal run-to-run variance. |
| XOR Color Chaos | 10.00 | 2.33 | Full-screen integer pattern/sampling; parallel rows and buffer exchange. |
| Screen Color Inversion | 0.33 | 0.25 | Simple contiguous XOR during alternate half-seconds; unchanged. |
| Popup Cascade | 1.20 | 1.17 | Bounded themed popup rendering, text and shadows; unchanged. |
| Random Square Paint | 0.01 | 0.02 | Small retained tile grid; unchanged. Replay grows only to screen coverage. |
| Chaos Shader | 109.19 | 12.11 | Originally recalculated the same column sine for every row; cache it once per column, parallelize rows and exchange buffers. |
| Hell Shader | 16.89 | 4.06 | Existing lookup-table shader; parallel rows and buffer exchange, retain the same sine table and sample grid. |
| Seismic Slice & Drift | 34.24 | 1.61 | Multiple integer divisions/modulos per pixel; precompute wrapped X maps, derive two source rows per output row, parallelize and exchange buffers. |
| Recursive ROP Crucible | 8.52 | 3.51 | Full-screen feedback/difference and history copy; reuse X-map allocation, parallel rows, exchange input DIBs. Feedback remains immutable during rendering. |
| Bit-Plane Topography | 9.77 | 2.40 | Full-screen luminance conversion; parallel rows and buffer exchange. |

Activation spikes matter too: Kaleidoscopic Reality's first frame fell from
about **207 to 47 ms**, and 3D Warp Tunnel's from **129 to 39 ms**. Geometry caches
are still rebuilt only when dimensions change. No effects were removed.

## Every sound payload

Normal audio clips are synthesized once per selection, then replayed by a
separate WinMM worker. These are **milliseconds to synthesize a ten-second
clip**, not recurring visual-frame costs. Only Entropy Handshake changed;
other measured differences between runs were ordinary system-load variation.

| Sound | Original clip synthesis ms | Assessment |
| --- | ---: | --- |
| Bytebeat Industrial Scream | 1.49 | Cheap integer formula and oscillator. |
| Entropy Handshake | 268.98 | **Largest selection stall.** Sixteen display-DC `GetPixel` calls synchronized repeatedly; now one temporary capture supplies the same 16 seed positions. Observed replacement cost ~29–77 ms, compositor-dependent. |
| Buffer Freeze Stutter | 0.97 | Small clip-local buffer; cheap. |
| Dual-Tone Siren Warp | 5.12 | Two sine-driven oscillators. |
| Bytebeat Xenon Tear | 0.39 | Cheap integer bytebeat. |
| Bit-Rot Quantizer | 2.70 | Sample holding and quantization. |
| Subharmonic Meltdown | 9.34 | Multiple oscillators and saturation. |
| Modem Carrier Hell | 7.83 | Trig, filtering and saturation. |
| Bytebeat Acid Fracture | 0.45 | Cheap integer bytebeat. |
| PC Speaker Beep Storm | 3.77 | Four bounded clip-local voices. |
| Square-Wave Strobe | 0.76 | Cheap square wave and gate. |
| FM Feedback Howl | 7.44 | Nested sine modulation. |
| Radiation Avalanche | 6.14 | Exponentials and saturation. |
| Shepard Panic Ascent | 65.95 | **Heaviest CPU-only synthesizer:** ten voices with exponential/logarithmic envelopes. Unchanged to preserve the waveform. Occasional selection hitch, not sustained late-stage FPS loss. |
| Ring-Mod Tritone Clash | 7.71 | Coupled sine oscillators. |
| Bytebeat Void Engine | 0.42 | Cheap integer bytebeat. |
| Memory Bus Glitch | 0.94 | Cheap synthetic register patterns. |
| Comb Filter Screamer | 12.55 | Fractional delay, interpolation and saturation. |

Entropy Handshake's temporary capture is released immediately after mixing the
16 sampled values into its seed. It neither saves nor transmits the image.
PCM limits, generators, loop seams, crossfades, and device ownership are unchanged.
The separate blue-screen finale is not part of these payload timings; its artwork
and error-tone behavior were not modified.

## Shared renderer changes and visual preservation

- A persistent worker pool processes **independent pixels**, never concurrent
  effects. Layer order, RNG order, retained history and GDI calls stay on the
  render thread. Work is bounded to at most eight rendering threads including
  the caller, leaving one logical CPU unused where more than two are available.
  Threads sleep between jobs. `JETMY_RENDER_THREADS=1` forces sequential work.
- Full-overwrite effects exchange the frame/scratch DIB roles instead of copying
  the entire incoming image. Partial-coverage effects still get a real snapshot;
  black-backed effects still preserve input **before** clearing the destination.
  Presentation and cursor capture use the current DIB roles.
- No adaptive resolution, reduced sampling, fewer particles, fewer fractal
  iterations, skipped layers, frame reuse, or altered show timing. Capture
  exclusion, menu cutout, emergency exits and cleanup are preserved.
- Of 32 deterministic visuals, **26 matched all sampled output hashes exactly**.
  Six have sparse floating-point code-generation differences under `-ffast-math`:
  Black Hole, Warp Tunnel, City, Planet, Melting Screen and Chaos Shader. In the
  compared final 1080p frames, each matched **at least 99.9947% of pixels**.
  Chaos Shader differences were at most one channel level; the others include
  occasional one-pixel texture-coordinate rounding. This is not a claim of
  bit-for-bit equality for every possible input/phase. Ghost cursors depend on
  the live pointer. Single-thread and pooled output hashes also matched.
- Checks passed at odd native-resolution edges, both existing coarse sampling
  steps, and across repeated cycle changes. Full-overwrite metadata is checked
  against a poisoned destination to catch invalid buffer-exchange assumptions.

## Measuring an actual slow stack

Optional runtime tracing separates effects from capture and presentation.
It is off by default and writes **timings only**, not screenshots/audio:

```powershell
$env:JETMY_PROFILE_CSV = "$env:LOCALAPPDATA\Temp\jetmy-frame-times.csv"
.\JetmyV2.exe --menu
# Select the problematic effects manually, then close normally.
Remove-Item Env:JETMY_PROFILE_CSV
```

The requested CSV is overwritten on startup and limited to 6000 rendered frames.
Each effect's time includes its snapshot/clear, rendering and GDI flush.
Capture/history and presentation are separate rows. Frame total includes audio
selection/control work and excludes the frame-pacing sleep. Component rows do
not include all control work, so they need not sum exactly to the total. Profiling
itself adds some overhead. Automatic stages in the file are zero-based; in manual
mode that column is only elapsed ten-second intervals, not an active-layer count.

If real FPS remains substantially worse than the off-screen results, use this
trace to distinguish slow payload combinations from desktop-capture/compositor
costs instead of lowering visual quality blindly.
