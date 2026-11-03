<# :
@echo off
setlocal
cd /d "%~dp0"
set "SCRIPT_FILE=%~f0"

:: Auto-detect if launched by double-clicking in Windows Explorer
set "IS_INTERACTIVE=0"
echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && (
    echo %cmdcmdline% | findstr /i /c:"/c" >nul && set "IS_INTERACTIVE=1"
)

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "& ([ScriptBlock]::Create((Get-Content -LiteralPath $env:SCRIPT_FILE -Raw -Encoding UTF8)))" %*
set "EXIT_CODE=%ERRORLEVEL%"

if "%IS_INTERACTIVE%"=="1" (
    echo.
    pause
)
exit /b %EXIT_CODE%
#>

# ==============================================================================
# PowerShell Engine & Build Stopwatch
# ==============================================================================
$ErrorActionPreference = "Stop"
$BuildTimer = [System.Diagnostics.Stopwatch]::StartNew()

function Format-Elapsed([TimeSpan]$ts) {
    if ($ts.TotalMinutes -ge 1) {
        return "{0}m {1:D2}.{2:D2}s" -f [int]$ts.TotalMinutes, $ts.Seconds, [int]($ts.Milliseconds / 10)
    }
    return "{0:F2}s" -f $ts.TotalSeconds
}

$Target = "JetmyV2.exe"
$SourcePatterns = @(
    "main.cpp",
    "runtime/*.cpp",
    "payloads/visual/*.cpp",
    "payloads/sound/*.cpp"
)

# ------------------------------------------------------------------------------
# 1. Locate g++ (Prefers 32-bit i686 if present, falls back to 64-bit x86_64)
# ------------------------------------------------------------------------------
function Find-Compiler {
    $knownPaths = @(
        # 32-bit locations
        "C:\msys64\mingw32\bin\g++.exe",
        "C:\mingw32\bin\g++.exe",
        "C:\MinGW\bin\g++.exe",
        "C:\TDM-GCC-32\bin\g++.exe",
        # 64-bit locations (WinLibs WinGet, MSYS2, TDM)
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.*\mingw64\bin\g++.exe",
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\mingw64\bin\g++.exe",
        "C:\mingw64\bin\g++.exe",
        "C:\TDM-GCC-64\bin\g++.exe"
    )

    foreach ($pattern in $knownPaths) {
        $found = Resolve-Path $pattern -ErrorAction SilentlyContinue
        if ($found) { return $found[0].Path }
    }

    $cmd = Get-Command "g++.exe" -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    return $null
}

$gppPath = Find-Compiler

if (-not $gppPath) {
    Write-Host "[!] ERROR: g++ was not found in PATH or standard installation paths." -ForegroundColor Red
    exit 1
}

# ------------------------------------------------------------------------------
# 2. Terminate Running Instance & Remove Old Binary
# ------------------------------------------------------------------------------
$procName = [System.IO.Path]::GetFileNameWithoutExtension($Target)
$running = Get-Process -Name $procName -ErrorAction SilentlyContinue
if ($running) {
    Write-Host "[*] Terminating running $Target instance..." -ForegroundColor Yellow
    $running | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 200
}

if (Test-Path $Target) {
    try {
        Remove-Item -Path $Target -Force -ErrorAction Stop
    } catch {
        Write-Host "[!] ERROR: Cannot remove '$Target'. File is locked." -ForegroundColor Red
        exit 1
    }
}

# ------------------------------------------------------------------------------
# 3. Collect Source Files
# ------------------------------------------------------------------------------
if (-not (Test-Path "main.cpp")) {
    Write-Host "[!] ERROR: Entry point 'main.cpp' not found in current folder." -ForegroundColor Red
    exit 1
}

$Sources = [System.Collections.Generic.List[string]]::new()
$Sources.Add("main.cpp")

foreach ($pattern in $SourcePatterns) {
    if ($pattern -eq "main.cpp") { continue }
    $files = Get-ChildItem -Path $pattern -File -ErrorAction SilentlyContinue
    if ($files) {
        foreach ($file in $files) {
            $rel = Resolve-Path -Path $file.FullName -Relative
            if (-not $Sources.Contains($rel)) {
                $Sources.Add($rel)
            }
        }
    }
}

# ------------------------------------------------------------------------------
# 4. Neutralize default-manifest.o (Fixes MinGW Space in Path Bug)
# ------------------------------------------------------------------------------
$binDir = Split-Path -Parent $gppPath
$rootDir = Split-Path -Parent $binDir
$env:PATH = "$binDir;" + $env:PATH
$resObj = "resources.o"

if ($gppPath -match ' ') {
    $manifestFiles = Get-ChildItem -Path $rootDir -Filter "default-manifest.o" -Recurse -File -ErrorAction SilentlyContinue
    foreach ($mFile in $manifestFiles) {
        try {
            Rename-Item -Path $mFile.FullName -NewName "default-manifest.o.disabled" -Force -ErrorAction SilentlyContinue
            Write-Host "[*] Disabled unquoted $($mFile.Name) (fixes path-with-spaces bug)" -ForegroundColor Yellow
        } catch {}
    }
}

try {
    # --------------------------------------------------------------------------
    # 5. Architecture & Windows Subsystem Version Setup
    # --------------------------------------------------------------------------
    $machine = (& $gppPath -dumpmachine).Trim()

    if ($machine -match "i686|i386") {
        Write-Host "[*] Architecture: x86 (32-bit) -> Universally executable (Win 7-11, x86/x64/ARM64)" -ForegroundColor Cyan
        $subsystemArg = "-Wl,--subsystem,windows:5.1"
        $windresTarget = "pe-i386"
    } else {
        Write-Host "[*] Architecture: x86_64 (64-bit) -> Executable on Win 7, 10, 11 (x64 and Win11 ARM64)" -ForegroundColor Cyan
        $subsystemArg = "-Wl,--subsystem,windows:6.1"
        $windresTarget = "pe-x86-64"
    }

    # --------------------------------------------------------------------------
    # 6. Compile Resource File (.rc) if Present
    # --------------------------------------------------------------------------
    $rcFile = $null
    if (Test-Path "version.rc") {
        $rcFile = "version.rc"
    } elseif (Test-Path "resources.rc") {
        $rcFile = "resources.rc"
    }

    if ($rcFile) {
        $windresPath = Join-Path $binDir "windres.exe"
        if (Test-Path $windresPath) {
            Write-Host "[*] Compiling $rcFile -> $resObj..." -ForegroundColor Cyan
            & $windresPath "-F" $windresTarget "-i" $rcFile "-o" $resObj "--input-format=rc" "--output-format=coff"
            if ($LASTEXITCODE -ne 0) {
                Write-Host "[!] ERROR: windres failed to compile $rcFile" -ForegroundColor Red
                exit $LASTEXITCODE
            }
            $Sources.Add($resObj)
        }
    }

    # --------------------------------------------------------------------------
    # 7. Compile Main Project
    # --------------------------------------------------------------------------
    Write-Host "[*] Compiling $($Sources.Count) source(s) -> $Target..." -ForegroundColor Cyan

    $CompilerArgs = @(
        # Source & Resource files
        $Sources

        # Output Binary
        "-o", $Target

        # C++ Standard & Includes
        "-std=c++17"
        "-I."
        "-pthread"

        # Code Warnings
        "-Wall"
        "-Wextra"
        "-Wno-unused-parameter"

        # === OPTIMIZATIONS ===
        "-O3"
        "-ffast-math"
        "-funroll-loops"
        "-mtune=generic"
        "-flto=auto"

        # === SIZE REDUCTION & CLEAN BINARY ===
        "-s"
        "-fno-ident"
        "-fno-rtti"
        "-fomit-frame-pointer"
        "-Wl,--build-id=none"
        "-ffunction-sections"
        "-fdata-sections"
        "-Wl,--gc-sections"

        # === OS COMPATIBILITY (Windows 7+ Subsystem) ===
        $subsystemArg
        "-mwindows"

        # === STANDALONE ZERO-DEPENDENCY LINKING ===
        "-static"
        "-static-libgcc"
        "-static-libstdc++"

        # === LINKED LIBRARIES ===
        "-lgdi32"
        "-lgdiplus"
        "-luser32"
        "-lwinmm"
        "-lshell32"
        "-lmsimg32"
        "-ldwmapi"
    )

    & $gppPath $CompilerArgs
    $compExitCode = $LASTEXITCODE

    if ($compExitCode -ne 0) {
        $BuildTimer.Stop()
        Write-Host ""
        Write-Host "[!] ERROR: Compilation failed with exit code $compExitCode after $(Format-Elapsed $BuildTimer.Elapsed)." -ForegroundColor Red
        exit $compExitCode
    }

    # --------------------------------------------------------------------------
    # 8. Success Output
    # --------------------------------------------------------------------------
    $BuildTimer.Stop()
    $elapsedStr = Format-Elapsed $BuildTimer.Elapsed

    Write-Host ""
    Write-Host "[+] Build successful in $elapsedStr!" -ForegroundColor Green
    $fileInfo = Get-Item $Target
    $sizeKb = [Math]::Round($fileInfo.Length / 1KB, 2)
    Write-Host "[+] Output : $($fileInfo.FullName)" -ForegroundColor Gray
    Write-Host "[+] Size   : $sizeKb KB ($($fileInfo.Length) bytes)" -ForegroundColor Gray
}
finally {
    if (Test-Path $resObj) {
        Remove-Item $resObj -Force -ErrorAction SilentlyContinue
    }
}