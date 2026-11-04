# Repository guidance

## Build and verification
- Windows-only C++17/Win32 application. From the repository root, build with `g++ --version` then `.\compile.cmd` (MinGW-w64 with threading support).
- **Build side effects:** `compile.cmd` forcibly stops running `JetmyV2` processes and deletes the existing executable before compilation. If the compiler path contains spaces, it also attempts to rename `default-manifest.o` inside the compiler installation. Do not treat this as a side-effect-free check.
- The build discovers `runtime/*.cpp`, `payloads/visual/*.cpp`, and `payloads/sound/*.cpp` automatically. Keep test entrypoints outside those directories. `version.rc` embeds the manifest/version metadata when `windres` is available.
- For off-screen checks, use the PowerShell compile/run block in `tests/README.md`. It exercises every registered visual and sound, including full-overwrite buffer exchange and worker coverage, but not the menu, finale, or live audio streaming. Keep assertions enabled (`-DNDEBUG` disables the checks).
- When invoking the compiler directly, prepend its `bin` directory to the process `PATH`, as the test instructions do; finding `g++.exe` alone may not suffice for its subprocesses/runtime DLLs.
- Direct builds linking `runtime/blue_screen.cpp` need `-lgdiplus`; keep its canvas at presentation pixel dimensions and use 1:1 blits, not low-resolution bitmap upscaling.
- Never launch `.\JetmyV2.exe` without arguments as an automated smoke test: it immediately starts flashing visuals and audio. For user-requested interactive checks, `.\JetmyV2.exe --menu` starts with all effects off. Exit with Ctrl+Shift+Q or Ctrl+Shift+Esc; Escape also exits during the finale.
- `*.exe` and `resources.o` are ignored build artifacts; a pre-existing executable does not establish that current sources were built.

## Runtime contracts
- `main.cpp` delegates to `Runtime::RunApplication`; `runtime/app.cpp` owns capture, framebuffers, stacking, and audio selection. The menu changes `Controls`; it does not render or play payloads itself.
- Manual stacking follows registry order, not the menu's alphabetical display order. Keep menu command IDs and selection state mapped to registry indices.
- `runtime/schedule.h` controls the automatic show: ten distinct visuals accumulate at ten-second intervals; audio changes every other stage. The actual blue-screen finale is separate (`runtime/finale.cpp`), not a visual registry entry. `Screen Collapse` is an ordinary visual, not the finale.
- Preserve the simulator boundary: effects modify temporary overlays/in-memory audio, not Windows settings or other applications. Preserve capture exclusion, the menu's overlay cutout, emergency exits, and cleanup. Normal visuals target the primary monitor; only the finale covers the virtual desktop.

## Visual payloads
- Declare effects in `payloads/visual/visual.h`, implement `void EffectName(float time, float dt)` in `VisualPayloads`, and register them in `payloads/visual/registry.cpp`.
- Every activation starts with `time == 0` and `dt == 0`; reset effect-owned animation/history then. Later `dt` is capped at 0.1 seconds. Manual previews have no time limit, so retained state must remain bounded.
- Each frame starts from a fresh desktop capture. Trails/feedback must retain their own ink/history; modifying the destination alone will not persist into the next frame. `trail.h` supplies a bounded ink-only canvas.
- `g_pFramePixels`/`g_hMemDC` are the destination. Scratch holds the incoming stacked image only when `needsSnapshot` is true. The runtime snapshots **before** clearing a `blackBackground` destination; do not sample scratch with snapshots disabled.
- GDI and direct DIB writes share buffers: flush GDI before dependent CPU pixel access inside an effect; the runtime's post-effect flush cannot synchronize earlier accesses within that effect. Restore changed DC state and deselect owned GDI objects before deleting them; shared system icons/cursors are not owned.
- `PreparePayloadFrame` may exchange the frame/scratch DIB roles. Use the current global DCs/pixel pointers, not the original framebuffer object's handles. Mark `writesAllPixels` only for effects that overwrite every pixel at every phase; partial effects retain a snapshot copy. Pixel workers must write disjoint regions, never call GDI/FastRand, and finish before the effect returns.
- Use bounds-safe `Sample`/`Tile`/drawing helpers and `RasterStep()` where suitable. Handle odd-sized edges; do not assume dimensions are divisible by the sampling step.

## Sound payloads
- Declare generators in `payloads/sound/sound.h` and register them in `payloads/sound/registry.cpp`. Signature: `float Generator(uint32_t sample, Voice&)`; oscillator/filter/noise state belongs to the clip-local `Voice`, not shared statics.
- `player.cpp` synthesizes clips before playback, smooths loop seams, filters/clamps samples to bounded PCM, and owns one queued WinMM stream with selection crossfades. Do not open separate playback devices in normal generators. The stream is 8 kHz mono: 4 kHz is Nyquist, not a usable ordinary carrier frequency.
- Idle/audio-only mode skips visual frame capture/rendering, but Digital Noise Bursts makes one temporary desktop capture during synthesis and uses 16 pixels to seed its LFSR. Its tests therefore read pixels even though they show no overlay and play no sound.

## Adding or changing payloads
- Keep one registered visual/sound per `.cpp` file, named after its display label in lowercase `snake_case` (replace spaces/hyphens with underscores). Shared helper and registry files are not payload implementations.
- Update declarations, registry entries, and `payloads.md` together. Reconcile totals in `README.md`/`payloads.md` and hard-coded registry-count assertions in `tests/performance.cpp` when adding/removing entries; menu counts are derived automatically.
