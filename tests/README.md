# Payload smoke tests

From the repository root in PowerShell, with MinGW-w64 `g++` on `PATH`:

```powershell
$compiler = (Get-Command g++.exe).Source
$env:PATH = (Split-Path $compiler) + ';' + $env:PATH
$sources = @('tests/new_payloads.cpp') + @(Get-ChildItem payloads/visual/*.cpp, payloads/sound/*.cpp | ForEach-Object { $_.FullName })
$output = Join-Path $env:TEMP 'jetmy-new-payloads-tests.exe'
& $compiler @sources -o $output -std=c++17 -I. -O2 -Wall -Wextra -Wno-unused-parameter -pthread -lgdi32 -luser32 -lwinmm -lmsimg32 -ldwmapi
if ($LASTEXITCODE -eq 0) { & $output }
```

Assertions must remain enabled (do not pass `-DNDEBUG`). Tests check registry
counts, tiny/odd frame boundaries, restart behavior and initial monochrome masks
for the three newest visuals; all 18 sound generators are checked for finite,
bounded output, isolated clip state, non-silent ten-second PCM loops and bounded
120-second maximum-length clips. Additional
checks cover bytebeat formula/shift boundaries and the 25 Hz strobe gate. No overlay is
shown and no audio is played. Entropy Handshake still reads its seed pixels.
These checks do not replace visual inspection or listening at low volume.

Visual checks also cover Pixel Sorting Anomaly's guard pixels, seeded restarts,
large-band coverage and dimensions beyond its 2048-pixel sort cap, plus Color
Inversion's 500 ms boundaries using freshly restored input on every frame.

## Blue-screen artwork (off-screen only)

This separate test checks all five presets at 640x480, 1440x1080 and 2880x2160,
native canvas sizing, antialiased glyphs, and partial progress redraws against full
redraws. It does not exercise the finale window, audio, focus or exit handling.

```powershell
$compiler = (Get-Command g++.exe).Source
$env:PATH = (Split-Path $compiler) + ';' + $env:PATH
$output = Join-Path $env:TEMP 'jetmy-blue-screen-tests.exe'
& $compiler tests/blue_screen.cpp runtime/blue_screen.cpp -o $output -std=c++17 -I. -O2 -Wall -Wextra -lgdi32 -lgdiplus -luser32
if ($LASTEXITCODE -eq 0) { & $output }
```

Optionally pass a BMP file path to the test executable to save a Windows 11 blue
1440x1080 artwork snapshot, and a second path for a Windows 7 snapshot. No overlay
is displayed and no sound is played.
