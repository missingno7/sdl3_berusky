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
