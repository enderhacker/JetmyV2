# JetmyV2

**JetmyV2 is a harmless visual malware simulator for Windows.** It makes your
desktop look chaotic with screen distortions, animations and electronic sounds,
without actually damaging your system. Effects are drawn in a temporary overlay,
not applied to your files or desktop settings.

Includes **28 visual effects** and **15 sounds**, including original bytebeat songs.
See [payloads.md](payloads.md) for the full list and short descriptions.

## Run

- **`JetmyV2.exe`** — starts a 100-second show, adding a random visual every
  10 seconds with looping audio. When it ends, the overlay closes and audio stops.
- **`JetmyV2.exe --menu`** — opens manual controls with everything off. Toggle
  effects individually or combine them; use **Stop all** to clear them.
- **Exit anytime:** `Ctrl+Shift+Q` or `Ctrl+Shift+Esc`.

**Warning:** contains flashing visuals and electronic sounds. Avoid if you have
photosensitivity, and only run it on a computer with its user's permission.

## Build

Install MinGW-w64 with `g++` on your PATH, then run `compile.cmd` from the
repository folder. It builds `JetmyV2.exe`, stopping any running copy first.
