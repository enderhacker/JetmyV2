# Off-screen performance and regression checks

`performance.cpp` exercises **all 33 registered visuals and all 18 sounds**.
It creates memory DIBs, not presentation windows, and synthesizes audio without
opening an audio device. It never starts the show or finale. Cursor Trail
queries the live cursor; Digital Noise Bursts briefly captures the desktop to seed
its noise. No captured image is retained or written by these tests.

From the repository root in PowerShell:

```powershell
$compiler = 'C:\mingw32\bin\g++.exe' # Adjust for your installation.
$env:PATH = "$(Split-Path $compiler);$env:PATH"
$out = "$env:LOCALAPPDATA\Temp\opencode"
$sources = @('tests/performance.cpp', 'runtime/parallel.cpp') + @(
    Get-ChildItem payloads/visual/*.cpp, payloads/sound/*.cpp |
    Select-Object -ExpandProperty FullName
)
& $compiler @sources -o "$out\jetmy-performance.exe" -std=c++17 -I. -pthread `
    -O3 -ffast-math -msse2 -mfpmath=sse -mstackrealign -static `
    -Wall -Wextra -Wno-unused-parameter -lgdi32 -luser32 -lwinmm -lmsimg32 -ldwmapi
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
& "$out\jetmy-performance.exe" 1920 1080 16 | Set-Content "$out\performance.csv"
if ($LASTEXITCODE -ne 0) { throw 'Checks failed' }
```

Do **not** add `-DNDEBUG`: assertions validate counts, bounded PCM, independent
worker coverage, and full-frame buffer-exchange equivalence against initialized
destinations. This direct build does not stop running instances, replace the
release executable, or modify the compiler installation.

Arguments are width, height, and number of frames (at least two). Each visual
starts with `(time, dt) == (0, 0)` and is sampled at later phases with `dt == 0.1`.
The first frame is reported separately; means and p95 exclude it. Visual timings
include input preparation and GDI synchronization, but exclude synthetic-input
resetting, hashing, real screen capture, and presentation. Sound timings are
**per ten-second clip**, not per video frame. The stack selection is a fixed
shuffle on the same compiler/library, with depths 1, 4, 8, and 10.

Suggested additional checks:

```powershell
& "$out\jetmy-performance.exe" 1921 1081 4 # Odd native-resolution edges
& "$out\jetmy-performance.exe" 2561 1563 4 # RasterStep == 2, odd edges
& "$out\jetmy-performance.exe" 4001 4001 3 # RasterStep == 4, odd edges; more RAM
& "$out\jetmy-performance.exe" 321 181 160 # Repeated activations/cycle changes
$env:JETMY_RENDER_THREADS = '1'
& "$out\jetmy-performance.exe" 1920 1080 16
Remove-Item Env:JETMY_RENDER_THREADS
```

An optional fourth argument is an **existing** output directory. It writes each
visual's final *synthetic* frame as raw top-down 32-bit BGRX (`.bin`), never a
desktop capture. Compare two sets with:

```powershell
.\tests\compare_frames.ps1 -Before "$out\before-pixels" -After "$out\after-pixels"
```

For comparisons against pre-optimization sources, compile this harness in that
checkout with `-DJETMY_REFERENCE` and omit `runtime/parallel.cpp`. That switches
input preparation to the original copy/clear path. Cursor-dependent hashes can
vary; floating-point code generation under `-ffast-math` can cause sparse
one-pixel texture-coordinate or one-level color rounding differences. Inspect
differences rather than treating every nonmatching hash as a changed design.

These checks do not verify the menu, overlay/capture exclusion, live audio
streaming, emergency exits, or blue-screen finale. Do not launch the automatic
show as an unattended smoke test. See [performance.md](../performance.md) for
the payload-by-payload results and limitations.

The timing logger also has a standalone, graphics/audio-free check:

```powershell
& $compiler tests/profiler.cpp -o "$out\jetmy-profiler-test.exe" -std=c++17 -static
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
& "$out\jetmy-profiler-test.exe" # Disabled by default
$env:JETMY_PROFILE_CSV = "$out\profiler-test.csv"
& "$out\jetmy-profiler-test.exe" # Validates fields and the 6000-frame limit
if ($LASTEXITCODE -ne 0) { throw 'Profiler checks failed' }
Remove-Item Env:JETMY_PROFILE_CSV
```
