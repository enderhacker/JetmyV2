# JetmyV2

**JetmyV2 is a harmless visual malware simulator for Windows.** It makes your
desktop look chaotic with screen distortions, animations and electronic sounds,
without actually damaging your system. Effects are drawn in a temporary overlay,
not applied to your files or desktop settings.

Includes **32 visual effects** and **15 sounds**, including original bytebeat songs.
The show ends with a **10-second simulated blue-screen finale**.
The finale plays one continuous error tone at a bounded volume.
See [payloads.md](payloads.md) for the full list and short descriptions.

## Run

- **`JetmyV2.exe`** — starts a 100-second show, adding a random visual every
  10 seconds with looping audio, then stops all payloads and displays the finale
  for 10 seconds before exiting (110 seconds total).
- **`JetmyV2.exe --menu`** — opens manual controls with everything off. Toggle
  effects individually or combine them; use **Stop all** to clear them.
  **Blue screen (10s)** stops all payloads, previews the finale, then returns to
  the menu with everything off.
- **Exit anytime:** `Ctrl+Shift+Q` or `Ctrl+Shift+Esc`.

**Warning:** contains flashing visuals and electronic sounds. Avoid if you have
photosensitivity, and only run it on a computer with its user's permission.
Popups are overlays, not real dialogs or changes to other apps.
The finale shows a centered 4:3 crash-screen simulation over a black background,
hides the cursor over its window, and consumes clicks and ordinary keys while
focused. It does **not** globally lock input, crash Windows, write a memory dump,
or restart the computer. Escape and the emergency shortcuts can dismiss it early;
Windows system shortcuts remain available.

## Build

Install MinGW-w64 with `g++` on your PATH, then run `compile.cmd` from the
repository folder. It builds `JetmyV2.exe`, stopping any running copy first.
