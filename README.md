# Visual and sound payload demo

Windows desktop overlay with 25 visual effects and 15 synthesized sounds.
Effects only draw into the overlay; they do not modify the desktop itself.

## Sequence

- At startup: one random visual and a random looping sound.
- Every 10 seconds: add another visual, keeping previous visuals active.
- At visuals #3, #5, #7 and #9 (20, 40, 60 and 80 seconds): switch to a
  different random sound. Even-numbered visuals leave the sound playing.
- 90–100 seconds: ten stacked visuals; audio continues looping.
- At 100 seconds: stop audio, destroy the overlay, refresh the desktop, and quit.

Visuals are selected without repeats. Existing visuals remain active as new ones
are added. A dedicated worker continuously queues one WinMM output stream.
Sounds loop with smoothed seams, and replacements crossfade for 50 ms without
closing the device or inserting a silent stage. A new clip is synthesized while
the old one keeps playing. Output is bounded and system volume is never changed.
If no audio device is available, visuals continue.

**Exit early:** Ctrl+Shift+Q or Ctrl+Shift+Esc.
Warning: flashing visuals and electronic sounds; avoid use with photosensitivity.

## Manual controls

```powershell
.\JetmyV2.exe --menu
```

This opens a native, resizable control panel instead of starting the show.
Everything starts **off**, and there is no 100-second timeout.

- Toggle any visual on or off; enable just one to inspect it independently, or
  combine several. Re-enabling a visual restarts its animation state.
- Toggle a sound on to loop it indefinitely. Selecting another sound replaces it
  with a short crossfade; clicking the selected sound again turns audio off.
- Audio works with all visuals off; visuals work with audio off.
- **Stop all** clears every toggle, stops audio, and removes the overlay.
- Both lists scroll independently; Tab/Shift+Tab and Space work on the buttons.
- The control panel is protected from visual effects and stays usable while
  previews run. Close it, or use an emergency shortcut, to stop everything.

No screen capture or rendering runs while all visual toggles are off.
Use `--help` for command-line usage; unknown arguments do not start the show.

## Layout and build

- `main.cpp`: exact command-line argument parsing and application entry point.
- `runtime/app.cpp`: overlay, resource cleanup, capture/compositing loop and mode coordination.
- `runtime/menu.cpp`: native control panel and scrollable toggle lists.
- `runtime/schedule.h`: ten-stage timing and sound policy.
- `payloads/visual/`: `VisualPayloads`, shared raster helpers, registry, one effect per `.cpp`.
- `payloads/sound/`: `SoundPayloads`, bytebeat/random generators, registry, single-player audio.

Run `compile.cmd` from this directory with MinGW-w64 `g++` on PATH.
It compiles all modules and links GDI, User32, WinMM, and Shell32 into `JetmyV2.exe`.
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
30 ms loop-seam smoothing and short transition fades. Automatic sound changes
never select the same sound as the previous selection.

| Sound | Distinct behavior |
| --- | --- |
| Bytebeat Pulse | Syncopated low bit-crushed pulse bass. |
| Bytebeat Melody | Warm, plucked pentatonic triangle melody with bytebeat texture. |
| Bytebeat Arpeggio | Fast high-register dual-oscillator chip arpeggio. |
| Random Chirps | Randomized ascending/descending whistles separated by silence. |
| Noise Percussion | Sixteen-step kick, snare and high-passed noise-hat pattern. |
| Bytebeat Neon Drive | Minor-key synthwave song with pulse lead, filtered bass, arpeggios and drums. |
| Bytebeat Pocket Groove | Swung pentatonic groove with plucked bass and offbeat chord stabs. |
| Bytebeat Crystal Waltz | Six-bar music-box waltz with melody, alternating bass and chords. |
| Bytebeat Arcade Quest | Major-key game-theme song with square-wave harmony and counter percussion. |
| Bytebeat Midnight Keys | Mellow electric-piano-style jazz voicings, melodic fills and brushed noise. |
| Bytebeat Dusk Breaks | Minor acid-bass sequence with a syncopated breakbeat and bright arpeggio. |
| Sonar Soundscape | Spaced underwater pings, delayed echoes and a low ambient bed. |
| Glass Bells | Clear pitched bells with independently decaying overtones. |
| Rain on Metal | Filtered rainfall with randomized metallic droplets. |
| Cosmic Drone | Slowly swelling root/fifth/ninth harmony and soft noise breath. |

The six new songs are original bytebeat/chiptune-style compositions, not recordings
or recreations of existing songs. Integer phase counters generate quantized voices;
explicit note sequences, rests, envelopes and percussion provide musical structure.
Each arrangement fits a ten-second loop, including a six-bar 3/4 waltz.
All 15 sounds share the same single-player replacement and output-limiting path.
