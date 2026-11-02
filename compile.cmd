@echo off
setlocal enabledelayedexpansion

:: ========================================================
:: CONFIGURATION
:: ========================================================
set "TARGET=JetmyV2.exe"
set "SOURCE=main.cpp"
set "SOURCES=%SOURCE%"
for %%F in (runtime\*.cpp payloads\visual\*.cpp payloads\sound\*.cpp) do set SOURCES=!SOURCES! "%%F"

:: ========================================================
:: 1. ENVIRONMENT & FILE CHECKS
:: ========================================================
where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [!] ERROR: g++ was not found in your PATH.
    echo     Please install MinGW-w64 and add its 'bin' folder to PATH.
    goto :FAIL
)

if not exist "%SOURCE%" (
    echo [!] ERROR: Source file "%SOURCE%" not found in the current folder.
    goto :FAIL
)

:: ========================================================
:: 2. TERMINATE RUNNING INSTANCE & CLEAN OLD BINARY
:: ========================================================
:: Prevents "Permission denied" errors if the app is still open
taskkill /f /im "%TARGET%" >nul 2>&1

if exist "%TARGET%" (
    del /f /q "%TARGET%" 2>nul
    if exist "%TARGET%" (
        echo [!] ERROR: Could not delete "%TARGET%". The file is locked or in use.
        goto :FAIL
    )
)

:: ========================================================
:: 3. COMPILE
:: ========================================================
echo [*] Compiling %SOURCE% -^> %TARGET%...

:: Note: Use -O2 or -O3 for smoother rendering / framerates.
:: Use -Os if you prioritize the smallest possible file size.
g++ %SOURCES% -o "%TARGET%" ^
    -std=c++17 ^
    -pthread ^
    -O3 ^
    -flto ^
    -s ^
    -static ^
    -static-libgcc ^
    -static-libstdc++ ^
    -ffunction-sections ^
    -fdata-sections ^
    -Wl,--gc-sections ^
    -lgdi32 ^
    -luser32 ^
    -lwinmm ^
    -lshell32 ^
    -mwindows

if %ERRORLEVEL% neq 0 (
    echo.
    echo [!] ERROR: Compilation failed with exit code %ERRORLEVEL%.
    goto :FAIL
)

:: ========================================================
:: 4. SUCCESS & STATS
:: ========================================================
echo.
echo [+] Build successful!
for %%I in ("%TARGET%") do (
    set /a SIZE_KB=%%~zI / 1024
    echo [+] Output: %%~fI
    echo [+] Size  : !SIZE_KB! KB (%%~zI bytes)
)
echo.
exit /b 0

:FAIL
echo.
pause
exit /b 1
