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

* **Rendering** – all sprites are `SDL_Surface`s in one pixel format
  (`XRGB8888`, replaces `SDL_DisplayFormat`), the old blitter / fill / color
  key / per-pixel code is untouched. The screen is a normal software
  framebuffer surface owned by `graph_2d`. `graph_2d::flip()` uploads the
  dirty rectangles (the old `SDL_UpdateRects` semantics) to a streaming
  texture and presents it through `SDL_Renderer` (`video.cpp` – the only file
  that knows about window/renderer/texture).
* **Logical resolution** – the framebuffer is 640x480 (or 1280x900 in
  double-size mode). `SDL_SetRenderLogicalPresentation` scales it into any
  window (integer scaling by default, `scale_mode = fit|smooth` in the config),
  handles HiDPI and letterboxing, and converts mouse / touch coordinates back
  to logical coordinates (`video_backend::event_to_logical`).
* **Input** – `input_sdl.cpp` is the only place that reads SDL events. It
  produces *neutral key codes* (`K_xxx` in `input.h`, ASCII for printable
  keys) and pointer events in logical coordinates. `input::key_input()` keeps
  its own key state; the existing key sets (`game_keys`, `menu_keys`, ...)
  translate keys into the game events (`LEVEL_EVENT`), which is the "game
  action" layer. A touch control or a gamepad only needs to call
  `key_input()` with the same neutral keys. Gamepad buttons are mapped
  already (untested with hardware).
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
| Touch | via SDL touch->mouse synthesis + logical coordinate conversion |
| Gamepad | mapped, untested |
| GTK / GDK | removed (was only `gtk_parse_args` and a commented dialog) |
| Error dialogs | SDL message box (`platform_message`) |
| Files / paths / config / profiles / user levels | done (`platform.*`, `utils.*`) |
| 64-bit correctness (`INT_TO_POINTER`, event params, packed structs) | done |
| Editor | builds and runs (optional target) |
| Android project skeleton | **not started** (stage 9) |
| Android touch controls | **not started** (stage 10) |

## 4. Remaining SDL 1.2 APIs

None. `grep -rn "SDL_HWSURFACE\|SDL_SetVideoMode\|SDL_UpdateRect\|SDLK_\|SDL_GetKeyState" src` only finds the SDL3 keycode translation table in `input_sdl.cpp`.

## 5. Remaining desktop-only assumptions

* The editor and "run level" spawn this executable (`SDL_CreateProcess`); the
  menu hides the editor entry when the platform can't start processes.
* Directory listing (`SDL_GlobDirectory`) is used only for the profile directory
  (user-writable data); it is not used for bundled assets.
* Text input (profile name, editor prompts) uses key presses like the
  original, so it types lower case ASCII only. On-screen keyboards on Android
  need `SDL_StartTextInput` / `SDL_EVENT_TEXT_INPUT` support.
* Windows builds still use the console subsystem (log goes to the console).

## 6. Remaining Android blockers

* No Android project yet (`android-project/`, Gradle, `libmain.so` from
  `main.cpp` + `berusky_core`, assets packaged in `assets/`).
* Touch controls (D-pad, switch player, pause, restart) not implemented.
* Text input needs the SDL text-input path.
* Asset enumeration is not needed, but packaged assets are read through
  `SDL_LoadFile` – this needs to be verified on a device.

## 7. Known behavior differences from the original

* Window scaling: integer scaling by default (configurable).
* Fullscreen is borderless desktop fullscreen instead of a video mode switch.
* The whole dirty rectangle set is presented; parts that were not marked dirty
  keep their previous content (same as `SDL_UpdateRects`).
* `menu_dialog_error` (was an empty stub after the GTK removal) now shows an
  SDL message box.
* One-shot keys (`KEY_CLEAR_AFTER_PRESS`) keep a pending press until the tick
  consumes it, so a tap shorter than one tick is not lost.
* User levels default to the user data directory; the editor saves bare file
  names there.
* Config file is only read from the user data directory.
* Sound: the original code base has **no** audio implementation (only an
  unused `music` byte in the level format and `BERUSKY_SOUND` struct). Nothing
  was changed or added.

## 8. Regression checks

See `tests/` (screenshot smoke test) and section "Testing" below.
