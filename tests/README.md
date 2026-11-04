# Startup gate checks

From the repository root with MinGW-w64 `g++` on PATH:

```powershell
.\tests\run_startup_tests.ps1

# Optionally exercise a different architecture/toolchain as well:
.\tests\run_startup_tests.ps1 -Compiler C:\mingw32\bin\g++.exe
```

The script prepends the compiler's `bin` directory to its process PATH, compiles
with assertions enabled in a unique temporary directory, runs the tests, and
removes only that directory. It does not use `compile.cmd`, replace the app, stop
processes, or launch the simulator.

Coverage includes both approvals, rejection at either step, cancellation during
the countdown, missing callbacks, fresh approval on every launch, and independent
three-second waits in both launch modes. Clock tests cover the exact unlock
boundary, countdown text values, timer restarts, and backward clock values.

Tests use a simulated monotonic clock: they do not show dialogs, probe the host,
capture the desktop, display effects, or play audio. Actual native button state,
focus, and keyboard handling require the manual integration checks below.

These checks cover startup only, not payload rendering, streaming, or the finale.

## Manual integration checks

Only perform these when prepared for flashing effects and audio. Save work and
lower the system volume first. Never automate a launch of the app without arguments.

- `JetmyV2.exe --help`: usage only, no confirmations.
- Unknown arguments: usage error, no effects.
- `JetmyV2.exe --menu`: two warning dialogs, each defaulting to **No**, then the
  panel with all effects off. No machine/environment checks precede the dialogs.
- In each dialog, **Yes** starts disabled, a three-second countdown appears, and
  Yes becomes enabled only after three seconds. The second dialog has a fresh wait.
- Clicking or using the Yes keyboard shortcut early must not accept a dialog.
- No, Escape, and the title-bar close button exit immediately, even during the wait.
- Decline either dialog: exit without the panel, capture, overlay, or audio.
- Restart: both dialogs and both waits appear again. Verify emergency exit shortcuts
  after deliberately approving both in an appropriate test environment.
- If the common-controls v6 dialog cannot be created, an error appears and no
  simulator initialization occurs; there is no fallback that bypasses the wait.
