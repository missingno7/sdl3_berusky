# Berusky – renderer modernization

This document tracks the move from the fixed-resolution software framebuffer
(640x480 / 1280x900 "double size") to a resolution-independent renderer.
`PORTING_SDL3.md` covers the SDL 1.2 -> SDL3 port that preceded it.

## 1. Audit (stage 1)

### 1.1 Old architecture

```
game / menus / editor  --(pixel coordinates of the framebuffer)-->  graph_2d
graph_2d: SDL_Surface framebuffer, 640x480 or 1280x900, CPU blits
video.cpp: SDL_UpdateTexture(dirty rects) -> SDL_SetRenderLogicalPresentation
           -> the finished low-resolution frame is stretched to the window
```

One global switch, `berusky_config::double_size` (`DOUBLE_SIZE`), selected
between two complete "worlds":

| | original | double size |
|---|---|---|
| framebuffer | 640x480 | 1280x900 (not 4:3) |
| level cell | 20 px | 40 px |
| level area | 640x420 at y=40 | 1280x840 at y=40 |
| top / bottom panel | 40 / 20 px | 40 / 20 px (same pixels, i.e. half the relative size) |
| fonts, logos, UI icons | 1x | 1x (unscaled: small text on a big screen) |
| 1x level sprites | as is | enlarged 2x at load time (`surface::scale`, destructive) |
| genuine 2x sprites (`new_gfx`) | **not loaded** | loaded |
| menu background | black / 1x art | 2x photos (`menu_back*`) |
| move animation step | 20 px | 40 px (`animation.cpp`: `dx *= 2`) |
| menu layouts | set A | set B (~150 `DOUBLE_SIZE ?` expressions) |
| editor | – | always double size (its only layout) |

### 1.2 Classification of every dependency

Legend: **G** gameplay, **L** logical layout, **A** asset density,
**R** render resolution / presentation, **C** legacy compatibility.

| Location | What | Class | Resolution |
|---|---|---|---|
| `2d_graph.cpp` `sprite_insert` (`DOUBLE_SIZE`, `SCALE_FACTOR`, `surface::scale`) | 1x sheets destructively enlarged to 2x when the scale field is 1 | A | per-asset native density, no load-time scaling |
| `2d_graph.cpp` `surface::scale` | the 2x "interpolate" enlarger | A | kept as an optional CPU asset scaler (`legacy2x`) |
| `utils.cpp` `graphics_game_load` (`new_gfx`) | genuine 2x sheets (boxes, TNT, walls, floors) only loaded in double size; `graphics_generate()` too | A | always loaded (they are just 2x assets) |
| `utils.cpp` `graphics_generate_floor` | shading drawn in pixels of the 40x40 art | A | CPU pixel operation on the source image (all variants) |
| `utils.cpp` `SDL_SetSurfaceAlphaMod(SPRITE_BLACK)` | raw SDL surface access | A | asset alpha modulation |
| `animation.cpp:142` | move animation distance doubled | L | removed: animations move in logical units (20 per cell) |
| `berusky.cpp` `original_size_set` / `double_size_set` | framebuffer size, level area, cell size | R + L | layout profiles (game 640x480 units, editor 1280x900 units); render size comes from the output |
| `berusky.cpp` `game_config_load`, `game_screen_set` | reads `disable_double_size`, `startup_doublesize_question` | C | ignored (documented) |
| `defines.h` `GAME_RESOLUTION_*`, `LEVEL_RESOLUTION_*`, `EDITOR_RESOLUTION_*`, `CELL_SIZE_*` | pixel sizes of the framebuffer | L | logical composition sizes of the active layout profile |
| `defines.h` `SCREEN_TOP_PANNEL_DX` | panel width per mode | L | one value |
| `berusky_gui.h` `ITEM_SIZE`, `TEXT_SHIFT_*` | level select grid | L | one value (original layout) |
| `berusky_gui.cpp` menus (logo position, item spacing, profile list, help screens, level select, end screens, level name) | two layouts | L | original (640x480) layout kept |
| `berusky_gui.cpp` `menu_background_get()` | photo background only in double size | A + L | photos (2x assets) behind the original layout, `menu_background = yes|no` |
| `berusky_gui.cpp` `menu_double_size_question`, settings "High resolution mode", "Ask on start up" | user choice of the render mode | C | removed; settings menu gets scaling options |
| `berusky_gui.cpp` `level_name_print` (`NAME_MARGIN`), level passwords | layout | L | original values |
| `main.cpp` `DOUBLE_SIZE_QUESTION` | first start question | C | removed |
| `main.cpp` `run_editor` -> `editor_config_load` | editor forced to double size | L | editor layout profile (1280x900 units, 40 unit cells) |
| `graphics.cpp` `screen_editor` selection area | editor | L | always on (editor has one layout) |
| `graphics.h` `IS_ON_SCREEN`, `screen::cell_x` | level cell -> screen position | L | logical units, renderer does the rest |
| `level.h` `ITEM_REPOSITORY::draw` (`CELL_SIZE`) | sub-item offsets | L | logical units |
| `level_game.cpp` panels (`CELL_SIZE_X`, `LEVEL_RESOLUTION_Y`) | panel positions | L | logical units |
| `editor.cpp` / `editor.h` `EDITOR_ITEM_SIZE`, `SIDE_MENU_*`, `EDITOR_*` | editor layout | L | editor profile units |
| `video.cpp` fixed logical presentation, `scale_mode = integer|fit|smooth` | geometry and filtering in one switch | R | `presentation`, `render_resolution`, `asset_scaler`, `presentation_filter` |
| `berusky_gui.cpp:2105` `new SURFACE(screen)` | reads the framebuffer back (save/restore under the level name) | L | canvas snapshot (display-list copy) |
| `test_script.cpp` `shot` | saves the framebuffer | R | reads back the scene render target |

No dependency is **G**: the game logic works on the 32x21 cell grid and never
reads pixel sizes. The move animation offset (`grid_diff`) is purely visual.

### 1.3 Sprite sheet metadata (`tests/tools/spr_audit.py`)

82 sheets. The optional 5th number of an `s` line is the double-size flag:

* `1` (25 sheets): original 1x artwork, enlarged in double size. Level
  sprites (`global*`, `klasik*`, `kyber*`, `hraci*`), backgrounds
  (`background*`, 640x420), `game_cur`, `menu6`.
* `0` (30 sheets): drawn for double size, **genuine 2x** artwork: boxes, TNT,
  walls, floors, `light_box` (40x40 per cell), `menu_back1`/`menu_back2`
  (1280x960 / 1280x1024 photos for a 640x480 composition).
* missing (27 sheets): drawn unscaled in both modes: fonts (16x20), logos,
  panel icons (`herni*`), menu art, masks, controls, end screens – 1x UI art.

Every sheet uses one value for all its sprites (no mixed sheets). One
inconsistency: `menu_back3.spr` (1280x960, same kind as `menu_back1`) had no
flag; it was fixed to `s 0 0 1280 960 0` (in the old code 0 and "missing" meant
the same, so the change is inert there).

Derived model: **density** = 1 for `1`/missing, 2 for `0`. Sheets with a flag
are **cell art** (they live in level-cell space and follow the editor's 40 unit
cells), the others are **UI art**. New `m density N` / `m cell 0|1` lines can
state both explicitly (older versions ignore unknown lines).

## 2. New architecture

```
GAME LOGIC (unchanged)            32 x 21 cells, 30 Hz ticks
      |
LOGICAL SCENE                     canvases in logical units (scene.h)
  game layout: 640x480 units, cell = 20      editor layout: 1280x900, cell = 40
      |  every draw is recorded: fill / image(region, src, dst) - a retained
      |  display list; hidden operations are compacted away
      v
SCENE RENDERER (scene.cpp)        SDL_Renderer render target at render_scale
      |  per operation: best variant of the asset for the render scale,
      |  optional CPU pre-scale (cached texture), GPU filter, edges snapped
      v
VIDEO BACKEND (video.cpp)         window, HiDPI, fullscreen, output size,
      |                           viewport placement, presentation filter,
      |                           touch overlay, diagnostics overlay (F12)
      v
SDL3 window / any display
```

| File | Role |
|---|---|
| `render_layout.*` | `RENDER_SETTINGS` (config keys) and `RENDER_LAYOUT`: output size, pixel density, content viewport, view scale, render scale, render target size, window <-> logical conversion. The only place with layout policy. |
| `image_asset.*` | `image_asset`: source pixels, native density, HD variants, cell zoom. `IMAGE_REGION`: the rectangle of one sprite, drawn from its own texture so filtering never bleeds into neighbour sprites of a sheet. CPU scalers. |
| `scene.*` | `canvas` (display list, compaction, canvas-to-canvas copies), `scene_renderer` (rasterization, texture cache, replay, read-back). |
| `video.*` | `video_backend`: SDL window / renderer, presentation, overlays, coordinates, capture, benchmark. |
| `2d_graph.*` | The game-facing API of the original (`surface`, `sprite`, `sprite_store`, `graph_2d`, fonts), now in logical units on top of the above. |

### 2.1 Why GPU compositing with a retained display list

The old model is "draw once, repaint what changed" into a persistent CPU
framebuffer; every menu, the level screen and the editor rely on it.
Two options were evaluated:

* **CPU framebuffer at render resolution**: keeps the blitter, but every asset
  needs CPU-scaled copies per render scale, a 4K frame is 25 MB of CPU pixels
  to upload per change, `SDL_SCALEMODE_PIXELART` (a GPU sampling mode) can't be
  used, and a resize needs the game to repaint everything it ever drew.
* **GPU compositing** (chosen): assets become textures once, filtering is free,
  PIXELART / linear / nearest are sampler states, Android's GLES2 supports it.

The incremental model is kept by recording what the game draws, in logical
units, instead of pixels. The screen canvas is rasterized into a persistent
render target as operations arrive (no per-frame redraw, no uploads). When the
render target changes (resize, HiDPI, filter, device reset) the display list is
replayed at the new resolution. The game code never notices.

* An operation fully covered by newer opaque ones is dropped (amortized
  coverage pass, 1 bit per logical unit; an opaque full-canvas fill clears the
  list). The list stays proportional to what is visible: 700-800 operations
  for a level.
* Drawing one canvas into another (the level background with static items into
  the screen, the saved screen part under the level name) copies and clips its
  operations, so lists refer to images only and a replay never depends on the
  current content of another canvas. The only framebuffer read-back of the game
  (`new SURFACE(screen, rect)`) became such a copy.
* Measured with `tests/bench/level_bench.txt` (full re-render + present of a
  level): Direct3D 11 1.2-2.5 ms from 640x480 up to 2880x2160; SDL software
  renderer 1.1 ms at 640x480 and 6.8 ms at 2880x2160 (18.8 ms with the CPU
  scale2x path at that size). Normal frames only draw what changed.

Kept on the CPU: color keys (converted to premultiplied alpha per sprite region
when a texture is made) and the shading of the generated floors
(`surface::blend`, applied to every variant; the base image uses the same
`rand()` sequence as before).

### 2.2 Per-asset native resolution

`surface::load` never rescales. An asset knows:

* **source size** in pixels (base variant);
* **density**: source pixels per logical unit (1 or 2 in the shipped data, any
  number for HD packs);
* **cell zoom**: logical units per native unit (1 in the game; 2 for level art
  in the editor's 40 unit cells);
* **HD variants**: `name@Nx.png` (N = 2, 3, 4, 6, 8, above the base density)
  from the `hd_pack` directory (config key, or `BERUSKY_HD_PACK`) or next to
  the base file. They must be exactly N/base times its size; alpha is honoured.

A `.spr` rectangle (pixels) becomes a logical rectangle (pixels / density *
zoom): `s 0 0 20 20 1` (1x) and `s 0 0 40 40 0` (2x) are both one 20x20 cell.

At draw time the renderer takes the lowest-density variant that needs no
magnification at the current render scale, otherwise the highest one: at
render scale 3 with 1x and 2x available the 2x one is scaled by 1.5; a 4x HD
variant would be reduced to 3x.

`.spr` metadata (backward compatible, older readers ignore `m` lines):

```
s x y w h [scale]    legacy scale field: 1 = 1x art, 0 = genuine 2x art,
                     none = 1x UI art (section 1.3)
m density N          explicit native density
m cell 0|1           level cell art (follows the cell zoom) / UI art
```

### 2.3 Scaling: asset scaler vs. presentation filter

| Setting | Values | Applies to |
|---|---|---|
| `asset_scaler` | `nearest`, `linear`, `pixelart` (default), `scale2x`, `legacy2x`, `xbrz` | each asset, from its own pixels to the render resolution |
| `presentation_filter` | `nearest`, `linear`, `pixelart` (default) | the finished scene -> window, only when their sizes differ |
| `render_resolution` | `native` (default), `integer`, a number | the scene's resolution |
| `presentation` | `fit` (default), `integer` | the viewport geometry |

`nearest` / `linear` / `pixelart` are `SDL_SCALEMODE_*` of the sprite textures.
`scale2x` (EPX / AdvMAME2x / 3x; 4x = 2x twice) and `legacy2x` (the 1.7
double-size enlarger, including its averaging quirk) are CPU scalers: a sprite
is pre-scaled once by an integer factor (the needed magnification rounded up,
capped by the scaler), cached as a texture per (region, variant, factor) and
drawn with linear filtering for the remaining fraction. **xBRZ** plugs into
the same table (`cpu_scalers[]` in `image_asset.cpp`, factors 2..6); until it
is built in, `xbrz` falls back to `pixelart`.

Caches: textures are keyed by region + variant + factor and carry the asset's
version. They are dropped on a scaler change, a render device reset and when
the asset is released. The render target is recreated when its size changes.
Source images are never modified by scaling.

The settings menu toggles integer scaling and the menu photos and cycles the
asset scaler at run time.

### 2.4 Layout, widescreen, coordinates

* The game is one composition of 640x480 units (the original layout: the level
  is 640x420 at y = 40, cells of 20). The editor: 1280x900 units, cells of 40.
* `RENDER_LAYOUT` fits it into the output, never distorted, centered, black
  elsewhere (16:9, 16:10, 21:9, 32:9, portrait). Level size and format are
  untouched.
* Window coordinates -> output pixels (pixel density) -> logical units for the
  mouse and touch (`video_backend::event_to_logical`, `window_to_logical`).
  Touch controls stay in output pixels, drawn after the scene; on a wide phone
  screen they sit in the free space beside the composition.
* Diagnostics overlay (`debug_overlay = yes`, F12): window / output size,
  density, composition, viewport, render target, render scale, scaler, filter,
  drawn images by native density, textures, replay time, present rate.

### 2.5 Menus

Layouts are the original 640x480 ones; all `DOUBLE_SIZE` alternatives are
gone. The 2x photo backgrounds of the old high-resolution mode are drawn behind
the menus that are black in that layout (`menu_background = photo`; `black`
restores the original look). Help, hint and level select screens keep their
own 1x artwork. The level passwords next to the level icons and the "light
box" rules line existed only in the 1280x900 layout (no room in 640x480) and
are not shown.

## 3. Remaining DOUBLE_SIZE dependencies

None in the code. What remains of the concept:

* The legacy scale field of `.spr` files is read as density / cell-art metadata.
* `disable_double_size` and `startup_doublesize_question` are no longer read
  (left untouched in old config files). `scale_mode` of the first SDL3 port is
  still understood when the new keys are missing.
* The editor's 40 unit cells are a layout of the editor composition, applied as
  a cell zoom of level art; no pixels are scaled.

## 4. Test coverage

`python tests/run_tests.py` uses SDL's software renderer for machine
independent pixels; MSVC and MinGW GCC builds give identical results.

| Script | Covers |
|---|---|
| 01-04, 07, 11 | start-up, menus, level select, play (moves, animation, player switch, pause), completion, profiles at render scale 1. Level screens are pixel-identical to the old 640x480 framebuffer. |
| 05_hires | the same layout at 1x, 2x, 3x (`# same:` asserts identical display lists) |
| 06_window | fit / integer presentation, small window (render scale < 1), fullscreen |
| 08_menus | settings: fullscreen, integer scaling, menu photos, filter cycling at run time |
| 09_editor, 18_editor_run | editor at 1280x900 and 1600x1125, placing items, "run level" in a child process |
| 10_user_level | `-u level` |
| 12_touch, 22_touch_hud | touch menus, swipes (one step, queued steps, walk on, turn), MENU button, a tap / click on a bug in the top panel |
| 13_widescreen | 16:9, 21:9 and tall windows; identical layout |
| 14_filters | nearest / linear / pixelart / scale2x / legacy2x / xbrz fallback; integer render resolution presented with linear and nearest |
| 15_density | mixed 1x + genuine 2x art (`tests/levels/mixed_density.lv3`; no shipped level uses the 2x art) at 1x, 2x, 4x; diagnostics overlay |
| 16_hdpack | an `@4x` HD variant (`tests/hdpack`) preferred at render scale 3 |
| 17_resize | resizing during a move animation |

`berusky_layout_test` (CTest) checks the layout math for HiDPI, phone landscape
densities, ultrawide, portrait, integer modes, texture limits and coordinate
round trips. `tests/bench/` holds performance scripts
(`python tests/run_tests.py --renderer "" --scripts tests/bench`).

Gameplay regression: the level scenarios reproduce the old pixels at scale 1
after every move; timing is untouched (30 Hz ticks, animations in logical
units). Layout regression: `layoutshot` display lists compared across
resolutions. Visual regression: hashes per render configuration.

## 5. Remaining blockers

* **Android** builds and runs on the emulator (Android 15, 2400x1080, density
  2.625, GLES2): the scene is rendered at 1440x1080, assets come from the APK,
  touch controls and Home -> return work. Found and fixed there: SDL rotated
  the resizable window to portrait (landscape hint + fullscreen on mobile), the
  one present after resuming came before the new surface was ready (a short
  repaint burst), taps shorter than a tick were lost (minimum hold). Walking
  is by swipes, only RESET / MENU are buttons (top right).
* **xBRZ** is not built in (license and size); the slot exists.
* Only SDL's software renderer is pixel-tested. GPU backends were checked
  visually (Direct3D 11: nearest / linear / pixelart / scale2x) and benchmarked.
* The scene is presented when the game draws (30 Hz); rendering between ticks
  with interpolation is a separate feature.
* Widescreen side areas are black; extended artwork, HUD or touch layout there
  is future work.
