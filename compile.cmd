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
# PowerShell Engine
# ==============================================================================
$ErrorActionPreference = "Stop"

$Target = "JetmyV2.exe"
$SourcePatterns = @(
    "main.cpp",
    "runtime/*.cpp",
    "payloads/visual/*.cpp",
    "payloads/sound/*.cpp"
)

# ------------------------------------------------------------------------------
# 1. Locate g++ (PATH or common WinGet / MSYS2 paths)
# ------------------------------------------------------------------------------
function Find-Compiler {
    $cmd = Get-Command "g++.exe" -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $knownPaths = @(
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.*\mingw64\bin\g++.exe",
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\mingw64\bin\g++.exe",
        "C:\TDM-GCC-64\bin\g++.exe",
        "C:\MinGW\bin\g++.exe",
        "C:\mingw64\bin\g++.exe"
    )
    foreach ($pattern in $knownPaths) {
        $found = Resolve-Path $pattern -ErrorAction SilentlyContinue
        if ($found) { return $found[0].Path }
    }
    return $null
}

$gppPath = Find-Compiler

if (-not $gppPath) {
    Write-Host "[!] ERROR: g++ was not found in PATH or standard installation paths." -ForegroundColor Red
    Write-Host "    Install it via WinGet by running:" -ForegroundColor Yellow
    Write-Host "    winget install BrechtSanders.WinLibs.POSIX.UCRT" -ForegroundColor White
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
    Start-Sleep -Milliseconds 250
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
            $Sources.Add($rel)
        }
    }
}

# ------------------------------------------------------------------------------
# 4. Resolve Path with Spaces (Auto-Fix for default-manifest.o)
# ------------------------------------------------------------------------------
$binDir = Split-Path -Parent $gppPath
$rootDir = Split-Path -Parent $binDir
$substDrive = $null
$resObj = "resources.o"

try {
    if ($gppPath -match ' ') {
        $shortBin = $null
        try {
            $fso = New-Object -ComObject Scripting.FileSystemObject
            $shortBin = $fso.GetFolder($binDir).ShortPath
        } catch {}

        if ($shortBin -and ($shortBin -notmatch ' ') -and (Test-Path "$shortBin\g++.exe")) {
            $gppPath = "$shortBin\g++.exe"
            $binDir = $shortBin
            $env:PATH = "$shortBin;" + $env:PATH
        } else {
            $usedDrives = [System.IO.DriveInfo]::GetDrives() | ForEach-Object { $_.Name.Substring(0, 1).ToUpper() }
            $availableDrive = ([char[]](65..90) | Where-Object { $_ -notin $usedDrives } | Select-Object -Last 1)

            if ($availableDrive) {
                $substDrive = "${availableDrive}:"
                subst $substDrive "$rootDir" > $null
                $gppPath = "$substDrive\bin\g++.exe"
                $binDir = "$substDrive\bin"
                $env:PATH = "$substDrive\bin;" + $env:PATH
            }
        }
    } else {
        $env:PATH = "$binDir;" + $env:PATH
    }

    # --------------------------------------------------------------------------
    # 5. Compile Resource File (.rc) if Present
    # --------------------------------------------------------------------------
    if (Test-Path "resources.rc") {
        $windresPath = Join-Path $binDir "windres.exe"
        if (Test-Path $windresPath) {
            Write-Host "[*] Compiling resources.rc..." -ForegroundColor Cyan
            & $windresPath "-i" "resources.rc" "-o" $resObj "--input-format=rc" "--output-format=coff"
            if ($LASTEXITCODE -ne 0) {
                Write-Host "[!] ERROR: windres failed to compile resources.rc" -ForegroundColor Red
                exit $LASTEXITCODE
            }
            $Sources.Add($resObj)
        }
    }

    # --------------------------------------------------------------------------
    # 6. Target Architecture & OS Subsystem Detection
    # --------------------------------------------------------------------------
    # Detect 32-bit vs 64-bit target from compiler
    $machine = & $gppPath -dumpmachine
    if ($machine -match "x86_64") {
        # 64-bit: Windows XP Pro x64 / Server 2003 is NT 5.2. Runs on XP64 -> Win11
        $subsystemArg = "-Wl,--subsystem,windows:5.2"
    } else {
        # 32-bit: Windows XP 32-bit is NT 5.1. Runs on XP -> Win11
        $subsystemArg = "-Wl,--subsystem,windows:5.1"
    }

    # --------------------------------------------------------------------------
    # 7. Compile Main Project
    # --------------------------------------------------------------------------
    Write-Host "[*] Compiling main.cpp -> $Target..." -ForegroundColor Cyan

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

        # === SPEED OPTIMIZATION ===
        "-O3"                   # Aggressive vectorization & loop optimization
        "-ffast-math"           # Maximum floating-point & sinusoidal speed
        "-funroll-loops"        # Unrolls render/audio loops for instruction parallelism
        "-mtune=generic"        # Fast on modern CPUs while remaining 100% compatible with older CPUs

        # === ANTI-DECOMPILATION & STRIPPING ===
        "-s"                    # Strips all symbols and debug data
        "-fno-ident"            # Removes GCC compiler signatures & metadata from .comment section
        "-fno-rtti"             # Strips C++ runtime type inspection & class names
        "-fomit-frame-pointer"  # Reuses RBP/EBP as register, breaking decompiler stack reconstructions
        "-flto"                 # Whole-program optimization: inlines and merges cross-file functions
        "-Wl,--build-id=none"   # Eliminates GNU build hash headers
        "-ffunction-sections"   # Isolates functions for tree-shaking
        "-fdata-sections"       # Isolates data items for tree-shaking
        "-Wl,--gc-sections"     # Discards all unreferenced dead code and strings

        # === OS COMPATIBILITY (XP, Vista, 7, 8, 10, 11) ===
        $subsystemArg           # Sets PE header major/minor subsystem version to 5.x
        "-mwindows"             # GUI subsystem (no black console prompt)

        # === STANDALONE PORTABILITY ===
        "-static"               # Avoids dynamic library dependencies
        "-static-libgcc"        # Embeds GCC C runtime
        "-static-libstdc++"     # Embeds C++ standard library runtime

        # === LINKED LIBRARIES ===
        "-lgdi32"               # Windows GDI (BitBlt, StretchBlt, DCs)
        "-luser32"              # Windows UI (GetDC, message loops)
        "-lwinmm"               # Audio / WaveOut playback
        "-lshell32"             # Shell API
        "-lmsimg32"             # AlphaBlend, TransparentBlt, GradientFill
        "-ldwmapi"              # Desktop Window Manager effects
    )

    & $gppPath $CompilerArgs
    $compExitCode = $LASTEXITCODE

    if ($compExitCode -ne 0) {
        Write-Host ""
        Write-Host "[!] ERROR: Compilation failed with exit code $compExitCode." -ForegroundColor Red
        exit $compExitCode
    }

    # --------------------------------------------------------------------------
    # 8. Success Output
    # --------------------------------------------------------------------------
    Write-Host ""
    Write-Host "[+] Build successful!" -ForegroundColor Green
    $fileInfo = Get-Item $Target
    $sizeKb = [Math]::Round($fileInfo.Length / 1KB, 2)
    Write-Host "[+] Output : $($fileInfo.FullName)" -ForegroundColor Gray
    Write-Host "[+] Size   : $sizeKb KB ($($fileInfo.Length) bytes)" -ForegroundColor Gray
}
finally {
    # Clean up temporary resource object
    if (Test-Path $resObj) {
        Remove-Item $resObj -Force -ErrorAction SilentlyContinue
    }
    # Clean up virtual drive if mounted
    if ($substDrive) {
        subst $substDrive /d > $null 2>&1
    }
}