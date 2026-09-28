# Berusky – SDL3 port

Progress document: what was inspected, what was migrated, what is left.

## 1. Build and run

Requirements: a C++14 compiler and CMake >= 3.16. SDL3 and SDL3_image are used
when installed; otherwise CMake downloads and builds them
(`BERUSKY_FETCH_SDL=ON`, needs `git`).

```
cmake -S . -B build -G Ninja
cmake --build build
build/berusky                    # the game (menu)
build/berusky -u level.lv3       # run a user level
build/berusky -e [level.lv3]     # level editor
```

CMake options: `BERUSKY_ENABLE_EDITOR` (ON), `BERUSKY_ENABLE_ASSERTS` (ON – the
code base uses `assert()` for sanity checks, release builds keep them),
`BERUSKY_FETCH_SDL`, `BERUSKY_STATIC_SDL`.

Game data (`Graphics`, `GameData`, `Levels`) is found automatically:
`$BERUSKY_DATA`, `<exe>/data`, `<exe>/../data`, `<exe>/../../data`,
`<exe>/../share/berusky`, and finally the `data/` directory of the source tree
the executable was built from. Everything can be overridden in the config file.

Configuration, profiles, user levels and the log live in the per-user data
directory (`SDL_GetPrefPath("Anakreon", "Berusky")`):

| Platform | Location |
|----------|----------|
| Windows  | `%APPDATA%\Anakreon\Berusky\` |
| Linux    | `~/.local/share/Anakreon/Berusky/` |
| macOS    | `~/Library/Application Support/Anakreon/Berusky/` |

`berusky.ini` is created there on the first start from `data/berusky.ini`.
The old `~/.berusky` directory and `/usr/share/berusky` are no longer used
(nothing is migrated automatically).

## 2. Architecture

```
game / core code (unchanged game logic, animation, levels, menus, sprite store)
   |                         |                         |
   | software renderer       | key sets -> LEVEL_EVENT | files / paths
   v                         v                         v
2d_graph.*  ->  video.*   input.* <- input_sdl.*   utils.* -> platform.*
(SDL_Surface     (SDL3 window,   (neutral keys,      (SDL_IOStream,
 framebuffer)     renderer,       pointer in          pref path, asset
                  texture)        logical coords)     root, dialogs)
```

* **Rendering** – replaced by a resolution-independent renderer, see
  **`docs/RENDERER.md`**. The game draws in logical units (a 640x480 unit
  composition, 20 unit cells) into canvases that record a display list;
  `SDL_Renderer` composites the sprites (each asset at its native pixel
  density) into a render target at the output's resolution, which is placed in
  the window keeping the aspect ratio. The fixed 640x480 / 1280x900
  framebuffer and the double-size mode are gone.
* **Coordinates** – `RENDER_LAYOUT` (`render_layout.h`) converts window
  coordinates (HiDPI, letterboxing) to logical ones for mouse and touch
  (`video_backend::event_to_logical`).
* **Input** – `input_sdl.cpp` is the only place that reads SDL events. It
  produces *neutral key codes* (`K_xxx` in `input.h`, ASCII for printable
  keys) and pointer events in logical coordinates. `input::key_input()` keeps
  its own key state; the existing key sets (`game_keys`, `menu_keys`, ...)
  translate keys into the game events (`LEVEL_EVENT`), which is the "game
  action" layer. Keyboard, gamepad and touch controls all just call
  `key_input()` with neutral keys. `KEY_CLEAR_AFTER_PRESS` / held-key
  semantics are unchanged (a one-shot key also survives a tap shorter than one
  tick).
* **Touch** – `touch_controls.*`: on-screen D-pad, next player, player 1-5,
  restart, menu, drawn as an overlay in window pixels (`video.cpp`) while a
  level is played; a finger on a control = a neutral key press. Any other
  finger is converted window -> logical game coordinates and drives the
  existing mouse/menu system. On by default on Android/iOS
  (`touch_controls = yes|no|auto`).
* **Files** – `platform.*` knows the read-only asset root and the writable user
  directory. `utils.cpp` implements the `file_*` API on top of SDL (`SDL_LoadFile`
  for reading → works for packaged Android assets, `SDL_IOStream` for writing).
  There is no `FILE*`, `chdir`, `getcwd`, `HOME`, `scandir`, `fork`, or
  `#ifdef LINUX/WINDOWS` in the game code any more.
* **Timing** – unchanged: fixed 30 Hz game tick (`GAME_FPS`), the same
  catch-up loop as before. Presentation is decoupled from the tick in
  `video.*`, so a higher refresh rate with interpolation can be added later.

## 3. Migration status

| Area | Status |
|------|--------|
| Build system (CMake, `berusky_core` + `berusky`) | done |
| SDL 1.2 -> SDL3 (video, surfaces, blitting, color key, alpha) | done |
| SDL_image -> SDL3_image (PNG loaded through `IMG_Load_IO`) | done |
| Software framebuffer + texture presentation | done |
| Logical resolution / resizable window / fullscreen / HiDPI | done |
| Keyboard, mouse, wheel, window events | done |
| Touch (controls + finger -> logical pointer) | done, tested with injected finger events only |
| Gamepad | buttons mapped (d-pad, A/B/X/Y, shoulders, start), untested with hardware |
| GTK / GDK | removed (was only `gtk_parse_args` and a commented dialog) |
| Error dialogs | SDL message box (`platform_message`) |
| Files / paths / config / profiles / user levels | done (`platform.*`, `utils.*`) |
| 64-bit correctness (`INT_TO_POINTER`, event params, packed structs) | done |
| Editor | builds and runs (`BERUSKY_ENABLE_EDITOR`, ON by default) |
| Game-only build (`-DBERUSKY_ENABLE_EDITOR=OFF`) | builds, tests pass |
| MSVC and MinGW GCC builds | both build, both produce identical pixels in all regression scenarios |
| Android project skeleton (`android/`) | written from the SDL3 template, **never built** (no SDK/NDK here) |
| Android touch controls | same code as desktop touch, **untested on a device** |

## 4. Remaining SDL 1.2 APIs

None. The only matches for `SDL_HWSURFACE`, `SDL_SetVideoMode`,
`SDL_UpdateRect`, `SDL_GetKeyState`, `SDL_DisplayFormat`, `SDL_FreeSurface` in
`src/` are comments that explain what replaced them. `SDLK_*` appears only in
the SDL3 keycode translation table (`input_sdl.cpp`).

## 5. Remaining desktop-only assumptions

* The editor and "run level" spawn this executable (`SDL_CreateProcess`); the
  menu hides the editor entry when the editor is not built or the platform
  cannot start processes.
* Directory listing (`SDL_GlobDirectory`) is used only for the profile directory
  (user-writable data); it is not used for bundled assets.
* Text input (profile name, editor prompts) uses key presses like the
  original, so it types lower case ASCII only. On-screen keyboards on Android
  need `SDL_StartTextInput` / `SDL_EVENT_TEXT_INPUT` support.
* Windows builds still use the console subsystem (log goes to the console).
* The original build never initialized gettext, so `_()` is the identity;
  the `po/` translations are not used.
* The autotools files (`configure.in`, `Makefile.am`, ...) are still in the tree
  but describe the old SDL 1.2/GTK build and no longer work.

## 6. Remaining Android blockers

* `android/` was written but never compiled or run (no SDK/NDK available):
  Gradle project from the SDL3 template, `libmain.so` built by the top-level
  CMake, assets copied from `data/`. Expect first-build fixes.
* Text input (profile names) needs the SDL text-input path
  (`SDL_StartTextInput`, `SDL_EVENT_TEXT_INPUT`).
* Packaged assets are read through `SDL_LoadFile` / `SDL_IOFromFile` - needs to
  be verified on a device (the desktop path is what was tested).
* Lifecycle events (`SDL_EVENT_WILL_ENTER_BACKGROUND`, `TERMINATING`) are not
  handled specially; the game just keeps running its loop.
* Small / portrait screens get the composition letterboxed (unit tested in
  `tests/layout_test.cpp`, not seen on a device).

## 7. Known behavior differences from the original

* The scene is rendered at the window's resolution (fit, pixelart filter by
  default; integer scaling and other filters are settings, see
  `docs/RENDERER.md`). There is no double-size mode and no start-up question.
* Fullscreen is borderless desktop fullscreen instead of a video mode switch.
* Everything drawn is presented on the next flip (the old code only showed the
  rectangles marked dirty).
* `menu_dialog_error` (was an empty stub after the GTK removal) now shows an
  SDL message box.
* One-shot keys (`KEY_CLEAR_AFTER_PRESS`) keep a pending press until the tick
  consumes it, so a tap shorter than one tick is not lost.
* User levels default to the user data directory; the editor saves bare file
  names there.
* Config file is only read from the user data directory.
* Sound: the original code base has **no** audio engine. The settings menu
  has sound/music check boxes but their handlers are commented out, and the
  level format has an unused `music` byte. Nothing was changed or added; audio is
  an unfinished feature that is independent of this port.
* The default `berusky.ini` template no longer contains the old
  `/usr/share/berusky` paths.
* Two latent bugs of the original were fixed because they break 64-bit or make
  the output non-deterministic: pointer-carrying event parameters were read as
  `int` (profile create/select), and `surface::scale()` read uninitialized
  colors for transparent neighbours.

## 8. Regression checks

`python tests/run_tests.py` replays scripted sessions (`tests/scripts/*.txt`,
see `src/test_script.h`) in an isolated user directory
(`BERUSKY_USER_DIR`), saves scene / window screenshots and layout dumps and compares them
with `tests/expected/*.sha256`. Real input devices are ignored while a script
runs and the level clock is tick based, so the pictures are reproducible.

Covered: startup, menus + hover, level select, level start, movement
(keyboard), switching players, pause menu, settings, help, level completion +
profile file, profile creation, window presentation, command-line user level
(`-u`), the editor, touch input and controls, and the renderer scenarios listed
in `docs/RENDERER.md` (resolutions, widescreen, filters, pixel densities, HD
packs, resizing). Tests use SDL's software renderer (`--renderer` overrides).

`python tests/run_tests.py --exe build-gcc/berusky.exe` checks another
compiler; MSVC and MinGW GCC currently give identical pixels for all scenarios.

**Caveat:** there is no SDL 1.2 build of the original to compare with (SDL 1.2
and GTK2 cannot be built in this environment). The expected hashes are
baselines of the SDL3 port, produced after the screenshots were checked by eye.
A pixel comparison with the original still has to be done on a machine with
the old libraries: run the old binary and the port on the same level and
compare the framebuffers.

Environment variables used by these hooks and by the portable mode:
`BERUSKY_TEST_SCRIPT`, `BERUSKY_TEST_OUT`, `BERUSKY_USER_DIR`,
`BERUSKY_TOUCH_CONTROLS`, `BERUSKY_DATA`.
