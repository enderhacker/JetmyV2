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
- `tests/payload_tests.cpp`: schedule boundaries, PCM checks, offscreen visual smoke tests.

Run `compile.cmd` from this directory with MinGW-w64 `g++` on PATH.
It compiles all modules and links GDI, User32, and WinMM into `JetmyV2.exe`.
The existing build script stops a running `JetmyV2.exe` before replacing it.

Tests can be built without launching the demo:

```powershell
$sources = @(Get-ChildItem payloads\visual\*.cpp, payloads\sound\*.cpp | ForEach-Object { $_.FullName })
g++ tests\payload_tests.cpp @sources -std=c++17 -O2 -lgdi32 -luser32 -lwinmm -o "$env:TEMP\payload_tests.exe"
& "$env:TEMP\payload_tests.exe"
& "$env:TEMP\payload_tests.exe" small
```

Tests create offscreen bitmaps only and do not play audio or capture the desktop.
