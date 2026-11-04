<# :
@echo off
setlocal
cd /d "%~dp0"
set "SCRIPT_FILE=%~f0"

:: Autodetect if launched by double-clicking in Windows Explorer
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
        $found = Resolve-Path -LiteralPath $pattern -ErrorAction SilentlyContinue
        if ($found) { return $found[0].Path }
        $glob = Resolve-Path $pattern -ErrorAction SilentlyContinue
        if ($glob) { return $glob[0].Path }
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

if (Test-Path -LiteralPath $Target) {
    try {
        Remove-Item -LiteralPath $Target -Force -ErrorAction Stop
    } catch {
        Write-Host "[!] ERROR: Cannot remove '$Target'. File is locked." -ForegroundColor Red
        exit 1
    }
}

# ------------------------------------------------------------------------------
# 3. Collect Source Files (Literal paths to support spaces & brackets)
# ------------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath "main.cpp")) {
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
            $rel = Resolve-Path -LiteralPath $file.FullName -Relative
            if (-not $Sources.Contains($rel)) {
                $Sources.Add($rel)
            }
        }
    }
}

# ------------------------------------------------------------------------------
# 4. Environment & default-manifest.o Neutralization
# ------------------------------------------------------------------------------
$binDir = Split-Path -Parent $gppPath
$rootDir = Split-Path -Parent $binDir
$env:PATH = "$binDir;" + $env:PATH
$resObj = "resources.o"

# Setup MAKE so LTO parallelizes across cores without serial warnings
$makeExe = Join-Path $binDir "mingw32-make.exe"
if (Test-Path -LiteralPath $makeExe) {
    $env:MAKE = $makeExe
} elseif (Get-Command "make.exe" -ErrorAction SilentlyContinue) {
    $env:MAKE = (Get-Command "make.exe").Source
}

# Unconditionally disable default-manifest.o to prevent manifest collisions (.rsrc merge failure)
# and fix GCC's collect2 unquoted path-with-spaces bug.
$manifestFiles = Get-ChildItem -LiteralPath $rootDir -Filter "default-manifest.o" -Recurse -File -ErrorAction SilentlyContinue
foreach ($mFile in $manifestFiles) {
    try {
        Rename-Item -LiteralPath $mFile.FullName -NewName "default-manifest.o.disabled" -Force -ErrorAction SilentlyContinue
    } catch {}
}

try {
    # --------------------------------------------------------------------------
    # 5. Architecture Detection & Platform Specific Optimization
    # --------------------------------------------------------------------------
    $machine = (& $gppPath -dumpmachine).Trim()
    $archOptArgs = @()

    if ($machine -match "i686|i386") {
        Write-Host "[*] Architecture: x86 (32-bit) -> Universally executable (Win 7-11, x86/x64/ARM64)" -ForegroundColor Cyan
        $subsystemArg = "-Wl,--subsystem,windows:5.1"
        $windresTarget = "pe-i386"
        
        # Crucial for zero-lag 32-bit math/GDI: Use SSE2 instead of slow x87 FPU stack
        $archOptArgs += @(
            "-msse2",
            "-mfpmath=sse",
            "-mstackrealign"
        )
    } else {
        Write-Host "[*] Architecture: x86_64 (64-bit) -> Executable on Win 7, 10, 11 (x64 and Win11 ARM64)" -ForegroundColor Cyan
        $subsystemArg = "-Wl,--subsystem,windows:6.1"
        $windresTarget = "pe-x86-64"
    }

    # --------------------------------------------------------------------------
    # 6. Compile Resource File (.rc) if Present
    # --------------------------------------------------------------------------
    $rcFile = $null
    if (Test-Path -LiteralPath "version.rc") {
        $rcFile = "version.rc"
    } elseif (Test-Path -LiteralPath "resources.rc") {
        $rcFile = "resources.rc"
    }

    if ($rcFile) {
        $windresPath = Join-Path $binDir "windres.exe"
        if (Test-Path -LiteralPath $windresPath) {
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

    $cpuCores = [Environment]::ProcessorCount

    $CompilerArgs = @(
        # Source & Resource files
        $Sources

        # Output Binary
        "-o", $Target

        # C++ Standard & Includes
        "-std=c++17"
        "-I."
        "-pthread"
        "-DNDEBUG"

        # Code Warnings
        "-Wall"
        "-Wextra"
        "-Wno-unused-parameter"

        # === HIGH-PERFORMANCE OPTIMIZATIONS (Eliminate Loop / GDI Lag) ===
        "-O3"
        "-ffast-math"
        "-fno-math-errno"
        "-funroll-loops"
        "-fomit-frame-pointer"
        "-falign-functions=16"
        "-falign-loops=16"
        "-mtune=generic"
        "-flto=$cpuCores"

        # Architecture-specific SIMD & realign flags
        $archOptArgs

        # === SIZE REDUCTION & CLEAN PE HEADERS ===
        "-s"
        "-fno-ident"
        "-fno-rtti"
        "-Wl,--build-id=none"
        "-ffunction-sections"
        "-fdata-sections"
        "-Wl,--gc-sections"
        "-Wl,--file-alignment=512"

        # === OS SUBSYSTEM & WINDOWED MODE ===
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
    $fileInfo = Get-Item -LiteralPath $Target
    $sizeKb = [Math]::Round($fileInfo.Length / 1KB, 2)
    Write-Host "[+] Output : $($fileInfo.FullName)" -ForegroundColor Gray
    Write-Host "[+] Size   : $sizeKb KB ($($fileInfo.Length) bytes)" -ForegroundColor Gray
}
finally {
    if (Test-Path -LiteralPath $resObj) {
        Remove-Item -LiteralPath $resObj -Force -ErrorAction SilentlyContinue
    }
}