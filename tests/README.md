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
counts, tiny/odd frame boundaries, restart behavior, initial monochrome masks,
finite audio synthesis and bounded, non-silent ten-second loops. No overlay is
shown and no audio is played. Entropy Handshake still reads its seed pixels.
These checks do not replace visual inspection or listening at low volume.
