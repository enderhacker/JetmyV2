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
