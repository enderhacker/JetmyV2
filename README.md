# JetmyV2

**JetmyV2 is a harmless visual malware simulator for Windows.** It recreates the
appearance of a chaotic desktop with screen distortions, animations, simulated
system messages, and synthesized audio. It is a visual demo, not a tool that
infects or damages your computer.

The project includes **34 visual effects**, **18 sound payloads**, and a separate
**10-second simulated blue-screen finale**. You can run an automatic show or use
the manual control panel to preview effects independently.

**Before running:** this program contains flashing patterns, strong color changes,
moving imagery, and sustained electronic sounds. Avoid it if you have
photosensitivity, start with a comfortable system volume, and only run it with
the computer user's permission. Save your work first to avoid accidental actions
while the desktop is visually obscured.

## Contents

- [What it does—and does not do](#what-it-doesand-does-not-do)
- [Getting started](#getting-started)
- [Automatic show](#automatic-show)
- [Manual control panel](#manual-control-panel)
- [Visual and sound payloads](#visual-and-sound-payloads)
- [Blue-screen finale](#blue-screen-finale)
- [Stopping and cleanup](#stopping-and-cleanup)
- [Building from source](#building-from-source)
- [Troubleshooting and limitations](#troubleshooting-and-limitations)
- [Project layout](#project-layout)

## What it does—and does not do

Normal visual payloads capture the primary desktop into memory and draw modified
versions in a temporary overlay. They do not permanently change the desktop image.
The finale uses a separate presentation window and does not need a desktop capture.

JetmyV2 does **not**:

- Modify your documents, wallpaper, display orientation, or resolution.
- Install itself as a service or configure startup persistence.
- Move the mouse pointer or rewrite text in other applications.
- Open real error dialogs when displaying Fake Message Popups.
- Crash Windows, restart the computer, or create a memory dump.
- Globally block keyboard or mouse input.
- Change your system volume.

Desktop snapshots are processed locally in memory; the simulator does not save
them as recordings or upload them. The modern blue-screen QR code contains a
Microsoft support URL, but JetmyV2 does not open it or make a network request.

“Harmless” describes the lack of destructive behavior, not a guarantee that the
flashing visuals or sounds are suitable for everyone. Outside the focused finale,
visual effects do not block normal desktop input, so clicking through an obscured
desktop can still affect the applications underneath.

## Getting started

Use Windows 10 or Windows 11 for the current build. An audio output device is
needed for sound, but the visual show can continue if audio is unavailable.

From a PowerShell terminal in the executable's folder:

```powershell
# Manual preview: everything starts off. Recommended for a first run.
.\JetmyV2.exe --menu

# Automatic show: starts visuals and audio immediately.
.\JetmyV2.exe

# Display usage without starting effects.
.\JetmyV2.exe --help
```

Double-clicking the executable starts the **automatic show**, not the control
panel. `-h` also displays help. Unknown command-line arguments display a usage
error instead of starting effects.

**Emergency exit:** press **Ctrl+Shift+Q** or **Ctrl+Shift+Esc**.
During the blue-screen finale, **Escape** also exits early.

## Automatic show

A normal launch runs a **100-second effects sequence**, followed by the
**two-second silent fake freeze and 10-second blue-screen finale**: approximately **112 seconds total**, excluding
initialization and cleanup.

The show chooses ten distinct visual payloads in a shuffled order. It starts one
visual immediately, then adds another every ten seconds. Previous visuals stay
active, so the image becomes progressively more layered.

| Time | Visual behavior | Audio behavior |
| --- | --- | --- |
| 0 seconds | Visual #1 starts. | A random sound starts looping. |
| 10 seconds | Add visual #2. | Keep the current sound. |
| 20 seconds | Add visual #3. | Switch to a different random sound. |
| 30 seconds | Add visual #4. | Keep the current sound. |
| 40 seconds | Add visual #5. | Switch to a different random sound. |
| 50 seconds | Add visual #6. | Keep the current sound. |
| 60 seconds | Add visual #7. | Switch to a different random sound. |
| 70 seconds | Add visual #8. | Keep the current sound. |
| 80 seconds | Add visual #9. | Switch to a different random sound. |
| 90 seconds | Add visual #10. | Keep the current sound. |
| 100 seconds | Stop every normal payload; fake-freeze the entire desktop for two seconds. | Stop audio. |
| 102 seconds | Show the blue-screen finale. | Silent. |
| 112 seconds | Close the finale and exit. | Silent. |

Sound changes never intentionally choose the immediately previous sound. A short
crossfade connects selections without an intentional silent stage. A sound may
return later in the sequence. The blue-screen finale is always last; it is not
part of the random visual selection.

Exiting early skips the remaining sequence and the finale.

## Manual control panel

Launch with `--menu` to open a native Windows control panel. All payloads start
**off**, and there is **no automatic timeout or stage progression**.

### Visual controls

- Click a visual's button to turn it on; click again to turn it off.
- Enable just one to inspect it, or enable several to combine their output.
- Re-enabling an effect restarts its animation state.
- Growing Light Trails, paint, icons, and simulated popups draw over the incoming
  image. Light trails remain until the effect is restarted.
  Shattered Screen, Exploding Blocks, and Screen Collapse preserve the
  incoming image as their texture, with black space around the moving pieces.
  A black-backed effect covers earlier layers; effects drawn afterward can still
  appear on top. Ordinary desktop distortions keep their image backgrounds.
- Turning off every visual hides the effects overlay. The idle/audio-only menu
   does not capture or render visual frames. Digital Noise Bursts makes one temporary
   desktop capture when its audio loop is generated, using 16 pixels solely to seed its noise.
- The panel stays clear of the visual overlay so its controls remain usable.

### Audio controls

- Click a sound to start looping it.
- Selecting another sound replaces the current one with a short crossfade.
- Clicking the selected sound again turns audio off.
- Only one normal sound is selected at a time; sounds do not accumulate.
- Audio can run without visuals, and visuals can run without audio.

### Other controls

- **Stop all:** turns off every visual and sound and hides the effects overlay.
- **Blue-screen style:** chooses Automatic, Windows XP, Vista, 7, 10, or 11 (blue).
  Changing the selection alone does not start the finale.
- **Blue screen (10s):** stops all normal payloads, hides the panel, and runs the
  selected finale. After normal completion, the panel returns with everything off;
  previous previews do not resume.
- **Close:** stops playback and closes the simulator.

Both payload lists are sorted alphabetically, and their headings show the total
number of available effects and audio loops.

Resize the panel freely: wide windows show both independently scrolling lists;
narrow windows switch to **Visuals** and **Audio** tabs. Use Tab/Shift+Tab to
navigate and Space to toggle a focused checkbox. Keyboard focus automatically
scrolls the selected row into view. Sorting only changes the menu order, not
visual stacking or playback behavior.

The status area shows the total number of active visuals, selected sound, and
audio-device errors. **Stop all** stops everything regardless of the current tab,
and is disabled when nothing is active.

## Visual and sound payloads

See **[payloads.md](payloads.md)** for the complete list of all **53 payloads**
and a short description of each: 34 normal visuals, 18 sounds, and one finale.

Visuals include desktop warping, rotation, recursive tunnels, glass shattering,
melting, persistent paint trails, pixel sorting, fractals, status icons, and simulated
popups. Their names describe visual effects, not actual changes to Windows.
For example, Rotate Screen transforms the captured image rather than changing
display settings; the messages in Fake Message Popups do not report actual system problems.
Sorted Color Bands smears large overlapping sorted bands with displaced scanlines
and rapid reshuffling. Flashing Inverted Colors alternates inverted and normal output
every 500 ms, starting inverted; this produces full-screen flashing.

Sliding Screen Strips adds counter-phase raster shearing; Zooming Screen Glitch
keeps its own previous overlay frame for expanding XOR/difference feedback;
Flashing Brightness Bands cycles binary luminance contours with brief visible inversions.
These effects contain particularly strong motion and flashing.

Screen Decay gradually shifts the captured image slightly beyond the edges,
adds blocky low-bitrate-style artifacts, stalled tiles and torn strips, then
darkens it with red/orange heat distortion over 35 seconds. This damage is
entirely simulated; no applications, files, or display settings are changed.

The 18 sounds are deliberately abrasive glitch/noise simulations: bytebeat
fractures, buffer-freeze stutters, corrupted alarms, bit-depth collapse, modem
hash, PC-speaker storms, feedback howl, accelerating crackles and a rising
layered rising buzz. Harsh Digital Buzz and Digital Noise Bursts retain the
earlier noise generators; the former melodic/ambient collection is replaced.
See [payloads.md](payloads.md#sound-payloads) for the complete list.

Sounds are synthesized locally rather than played from song recordings. Normal
clips use ten-second loops with smoothed boundaries. A continuously queued WinMM
stream handles playback, and selection changes crossfade over approximately
50 milliseconds. Signal filtering and bounded sample amplitude reduce harsh
edges, but listening volume still depends on your Windows and speaker settings.
Clipping, aliasing, buffer starvation and register patterns are synthesized effects,
not actual device failures or reads of CPU/process memory. The stream remains
8 kHz mono: modem sounds are stylized, and comb resonances sweep approximately
1–3.6 kHz, not the upper human hearing range. Bounded PCM does not guarantee safe
listening volume; start low. Digital Noise Bursts discards sampled pixel values immediately after mixing
them into its local LFSR seed; it does not save or transmit desktop images.

For a payload-by-payload performance audit, measured improvements, off-screen
tests and optional runtime timing traces, see **[performance.md](performance.md)**.
Rendering preserves the existing sampling quality and uses a bounded CPU worker
pool for independent pixel work; stacked effects still run in their original order.

## Blue-screen finale

The finale is a **crash-screen simulation**, not an actual Windows stop error.
Its diagnostic addresses, failed-driver text, progress counter, and references to
restarting or dumping memory are cosmetic. No such operation takes place.

### Appearance and styles

The screen is displayed in a **centered 4:3 area on the primary monitor**, with
black surrounding space. A topmost presentation window also covers other
monitors in black. It does not change monitor resolution or orientation.
Artwork is rendered at the presentation's native pixel dimensions and copied
one-to-one, rather than enlarging a low-resolution bitmap. Fonts use explicit
grayscale antialiasing independently of Windows font-smoothing settings; QR
modules are drawn at whole-pixel sizes to keep their edges crisp.

| Style | Appearance |
| --- | --- |
| Automatic | Detects the host Windows version and chooses the corresponding preset. |
| Windows XP | Classic monospaced white-on-dark-blue STOP diagnostics and memory-dump counter, rendered sharply at the presentation resolution. |
| Windows Vista | Classic diagnostic layout with wider addresses and a memory-dump progress line. |
| Windows 7 | Classic STOP layout with crash-dump collection and initialization messages. |
| Windows 10 | Brighter blue background, Segoe UI text, large sad face, percentage, QR code, and stop-code details. |
| Windows 11 (blue) | Historical Windows 11 blue-screen layout with device-oriented wording and modern support details. |

The Windows 11 option intentionally recreates its **earlier blue design**, not
the newer black crash screen. These are visual recreations, not exact replicas of
every Windows build, localization, or hardware configuration.

### Duration, sound, and input

- The timer uses a ten-second deadline measured after the first frame is drawn.
  Progress is derived from elapsed time rather than counting animation frames.
- Only the progress region is refreshed as the number changes; the blue image is
  not cleared to black between updates.
- A static snapshot across all monitors simulates a two-second freeze before the
  blue screen. Both phases are silent; other applications keep running normally.
- The cursor is hidden while it is over the presentation window.
- Clicks and ordinary keyboard input are consumed while that window has focus.
- Input is **not globally locked**. Windows system shortcuts remain available;
  the simulator cannot cover secure Windows screens such as Ctrl+Alt+Delete.
- Escape or an emergency shortcut ends the finale early and closes the simulator,
  including when the finale was launched from the menu.

After normal completion, automatic mode exits. Manual mode returns to the panel
with all effects and audio off. The presentation window is removed, its cursor
handling ends, and the frozen snapshot is released.

## Stopping and cleanup

Use **Ctrl+Shift+Q** or **Ctrl+Shift+Esc** to exit at any point. Close the manual
panel to stop the application, or use **Stop all** if you want to keep the panel
open without active payloads. Escape is an additional exit during the finale only.

On exit, the program stops audio, removes its windows, releases drawing resources,
and requests a desktop refresh. Since it draws into temporary windows rather than
changing your desktop settings, there are no display settings to restore.

A display-mode change ends the simulator rather than continuing with stale screen
dimensions. If the application becomes unresponsive, use Windows system controls
to open Task Manager and end `JetmyV2.exe`.

## Building from source

### Requirements

- A Windows development environment; Windows 10/11 is recommended for running.
- MinGW-w64 `g++` with C++17 and threading support.
- The compiler's `bin` directory on `PATH`.

Check the compiler, then build from the repository's root folder:

```powershell
g++ --version
.\compile.cmd
```

The script compiles `main.cpp` and the C++ modules in `runtime/`,
`payloads/visual/`, and `payloads/sound/` into **`JetmyV2.exe`**. It enables
optimization, link-time optimization, and static runtime linking. Windows GDI/GDI+,
User32, WinMM, and Shell32 provide drawing, windowing, audio, and argument parsing.
Popup theming functions are loaded dynamically when available.

**Build-script warning:** before compiling, `compile.cmd` forcibly stops a running
`JetmyV2.exe` and deletes the existing executable. If compilation fails after that
point, the old executable is not retained. Build while the simulator is closed.

After rebuilding, start with the manual panel to inspect the current version:

```powershell
.\JetmyV2.exe --menu
```

Legacy-looking XP/Vista/7 presets do **not** guarantee that the executable runs on
those operating systems. Runtime compatibility depends on the compiler and the
Windows APIs used, including capture exclusion. Older Windows execution has not
been established by the availability of those styles.

## Troubleshooting and limitations

| Issue | What to check |
| --- | --- |
| `g++` is not found | Install MinGW-w64, add its `bin` directory to `PATH`, and reopen the terminal. |
| The build cannot replace the executable | Close the simulator and any process holding the file, then rebuild. Check the compiler output for other errors. |
| Changes are missing | Run `compile.cmd` and launch the newly built executable from that same folder. Editing source does not update an existing `.exe`. |
| No sound | Check the selected output device, application mixer, and speaker volume. In the menu, select a sound again after correcting the device problem. |
| Visuals are slow | Try fewer simultaneous effects or a lower desktop resolution. Stacked effects increase CPU and memory work. |
| Screenshots or screen sharing omit effects | Normal preview windows use capture exclusion to avoid capturing themselves recursively. Capture methods may omit or black out excluded windows. |
| Only one monitor has normal effects | Normal desktop effects target the primary monitor. The finale separately covers the virtual desktop and centers its image on the primary monitor. |
| The finale differs from a particular Windows screenshot | Presets represent selected historical designs, retain the requested 4:3 presentation, and do not reproduce every OS revision or screen aspect ratio. |

The rendering loop targets roughly 60 frames per second, but that is not a
guarantee. Many full-screen effects render at native resolution through 1440p;
some expensive effects and larger displays use reduced sampling to bound their
work, which can make fine details appear blockier. Protected content,
secure desktops, remote sessions, and some capture environments may not behave
like a normal local Windows desktop.

## Project layout

| Path | Purpose |
| --- | --- |
| `main.cpp` | Command-line parsing, DPI awareness, entry point, and error reporting. |
| `runtime/app.cpp` | Automatic/manual coordination, desktop capture, effect stacking, overlay ownership, and cleanup. |
| `runtime/menu.cpp` | Native control panel, scrolling payload lists, toggles, and finale style selection. |
| `runtime/schedule.h` | Ten-second stages, ten-visual sequence, and audio-selection timing. |
| `runtime/finale.cpp` | Silent two-second fake freeze, ten-second presentation, input handling, and emergency exits. |
| `runtime/blue_screen.cpp` | Version detection, classic/modern crash-screen artwork, typography, and QR generation. |
| `payloads/visual/` | Individual visual implementations, rendering helpers, registry, and popup styling. |
| `payloads/sound/` | Sound generators, synthesis helpers, registry, loop synthesis, and streaming player. |
| `payloads.md` | Complete payload catalog with quick summaries. |
| `compile.cmd` | Modular release build script. |

Normal visuals are registered in `payloads/visual/registry.cpp`; sounds are
registered in `payloads/sound/registry.cpp`. The blue-screen finale is coordinated
separately so it stops all normal payloads and always runs last in automatic mode.
