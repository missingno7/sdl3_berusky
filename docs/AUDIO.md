# Sound effects and music

The DOS original (1999) had 20 sound effects and 30 FastTracker II modules,
played by the MIDAS Digital Audio System. The 1.x rewrite this port is based
on never had audio. The port plays the original assets again, with the
behavior of the shipped DOS game (`BERUSKY.EXE`) – see
[DOS_ORIGINAL.md](DOS_ORIGINAL.md) for how it was recovered. Nothing of the
MIDAS / Sound Blaster / DMA code is used.

## 1. Layers

```
game logic (game_logic.cpp)          menus (berusky_gui.cpp, gui.cpp)
  | SN_PLAY_SAMPLE events in the       | SN_ events (keys, menu items),
  | level's event stream (some are     | calls: audio.music_menu(),
  | chained to the end of a step)      | music_level(), music_credits()...
  v                                    v
level_changer.cpp -----------> GAME_AUDIO  (audio.h / audio.cpp)
                                 - sound ids = the DOS sample numbers
                                 - music situations + DOS track lists
                                 - 8 voices, round robin, cut after a length
                                 - on/off, volumes (config file)
                                 - clock = the 30 Hz game tick (audio.tick())
                                 - event log (tests)
                                     |
                                     v
                               AUDIO_BACKEND (audio_backend.h)
                                 - SDL3 + libxmp (audio_sdl.cpp)
                                 - null (tests, no device)
```

* Game logic and menus know sound **ids** and music **situations**, never SDL.
* The audio layer never looks at the wall clock: sounds are cut and delayed
  music starts on game ticks (`audio.tick()` in the main loop). The game's
  30 Hz simulation and the renderer are not affected.
* The SDL backend is the only file with SDL audio calls: one playback device,
  8 audio streams for the effects (16-bit mono, 20050 Hz, converted by SDL),
  one stream for the music that asks for data on SDL's audio thread and gets
  it from **libxmp** (see 4.). Modules are loaded into a new libxmp context
  on the main thread and swapped in under the stream lock, so a track change
  never stalls the audio thread.
* A running test script uses the null backend (no device, nothing depends on
  audio timing); `BERUSKY_TEST_AUDIO=1` plays anyway, `BERUSKY_NO_AUDIO=1`
  silences the game. When no device can be opened the game runs silent.

## 2. Assets

| Directory | Files | Format |
|-----------|-------|--------|
| `data/Sound/` | `smp_000.raw` … `smp_019.raw` | raw PCM, 16-bit signed little endian, mono, played at 20050 Hz (the rate the EXE passes to MIDAS) |
| `data/Music/` | `x_000.xm` … `x_029.xm` | FastTracker II modules (XM 1.04), 10–26 channels, played as they are |

They are the DOS files byte for byte (only the names are lower case); the
DOS game opened them as `smp\smp_%.3d.raw` and `mod\x_%.3d.xm`.
`tools/import_dos_assets.py` copies them from `original/` and
`--check` verifies them; `SOURCE.txt` in each directory lists the SHA-256
sums. The directories can be moved with `sound_data` / `music_data` in the
config file; on Android they are APK assets. Samples 1, 9 and 19 were never
used by the DOS game – they are shipped (and have ids) but nothing plays them.

## 3. Sound effects

Where the port plays each sound (`game_logic.cpp` unless noted). Lengths are
game ticks at 30 Hz – the sound is cut then, like the `do_stop` of
`hraj_sampl()`; the DOS value (60 Hz display ticks) in brackets. "End of
step" = chained to the move animation, like the DOS `pridej_zmenu()`.

| Id | File | Sound | When | Length | Prio |
|----|------|-------|------|--------|------|
| `SOUND_BLUE_FLASH` | smp_000 | MODRY_BLESK | a **cyber** one-pass door or color passage closes behind the bug – end of step (`door_close_sound`) | 30 (FPS) | 2 |
| `SOUND_GREEN_FLASH` | smp_001 | ZELENY_BLESK | – (unused in DOS) | | |
| `SOUND_BLUE_STONE` | smp_002 | MODRY_KAMEN | a stone of variation ≠ 0 broken (`P_STONE`) | 18 (36) | 2 |
| `SOUND_IRON_STONE` | smp_003 | ZELEZNY_KAMEN | a stone of variation 0 broken | 18 (36) | 36 |
| `SOUND_STEPS_MUD` | smp_004 | KROKY_BLATO | a step onto floor variation < 5; also into an opened classic color door (its frame becomes floor 2/3) | the step: 10, fast 5 (20/10) | 0 |
| `SOUND_STEPS_MARBLE` | smp_005 | KROKY_MRAMOR | a step onto floor variation ≥ 5 | the step | 0 |
| `SOUND_STEPS_BACKGROUND` | smp_006 | KROKY_POZADI | a step onto a cell without floor | the step | 0 |
| `SOUND_MENU_MOVE` | smp_007 | MENU_SKOK | another menu item gets highlighted (`gui.cpp`, `SN_MENU_HIGHLIGHT`) | 9 (TPS 18) | 1 |
| `SOUND_MENU_CLICK` | smp_008 | MENU_KLIK | a menu item is clicked (`gui.cpp`); Esc leaves the in-game menu (`input.cpp`) | 9 (18) | 1 |
| `SOUND_BUMP` | smp_009 | NARAZ | – (unused in DOS) | | |
| `SOUND_UNLOCK` | smp_010 | ODEMYK | a color door opened with a key (`unlock_sound`) | 30 | 1 |
| `SOUND_EXIT_OPEN` | smp_011 | OTEVRENI_EXITU | the fifth key taken – end of step | 30 | 2 |
| `SOUND_DOOR_OPEN` | smp_012 | OTEVRENI_DVERI | a **classic** color door opened with a key (with UNLOCK) | 30 | 2 |
| `SOUND_PUSH` | smp_013 | POSUN | a box or explosive pushed (also the light box, which DOS didn't have) – with the steps (`push_sound`) | the step | 1 |
| `SOUND_PICKUP` | smp_014 | SEBRANI | a key, a pickax or a color key taken | 30 | 3 |
| `SOUND_EXPLOSION` | smp_015 | VYBUCH | an explosive pushed into a box | 24 (48) | 3 |
| `SOUND_DOOR_CLOSE` | smp_016 | ZAVRENI | a **classic** one-pass door or color passage closes behind the bug – end of step; the bug enters its closed classic color passage (`passage_sound`) | 30 | 2 |
| `SOUND_LEVEL_DONE` | smp_017 | SAMPL_OK | the bug walks into the open exit; the music stops first (`P_EXIT`) | 60 (2*FPS) | 3 |
| `SOUND_SWITCH` | smp_018 | PREPNI | another bug selected (Tab, 1–5); the sound volume changed in the settings (a preview, like the DOS slider) | 30 | 1 |
| `SOUND_BAR` | smp_019 | LISTA | – (unused in DOS) | | |

* **Voices**: 8, like the MIDAS auto effect channels, taken **round robin**
  – the shipped MIDAS (`fxPlaySample`) does exactly that and ignores the
  priority, so the priorities are only logged. The same sound can play on
  several voices at once. Nothing loops.
* **Starting a module silences all effects** – MIDAS reallocated its
  channels in `hraj_modul()`. That is kept: it is how the level-done sound
  ends when the menu music starts.

## 4. Music

| Situation | Tracks | Choice | Where in the port |
|-----------|--------|--------|-------------------|
| menus | 4, 5, 6, 7, 8 | shuffle bag | main menu, difficulty, profiles, level map, help, settings; after the editor |
| a level starts | 0, 1, 2, 4, 3, 5 … 25 | shuffle bag | `level_run()` – every level (also the next one, also `-u`); **N** = next track |
| after a solved level | menu | shuffle bag | 50 ticks after the level end screen appears (the DOS game waited 30 ticks of its 18.2 Hz clock with the level-done sound) |
| a level given up | menu | shuffle bag | at once |
| credits | 0, 1, 2, 3 | shuffle bag | help → authors from the menus (in a game the level music goes on) |
| end of an episode | 27, 28, 27, 28, 29 | fixed (training … impossible) | the episode end screen, 50 ticks later |
| intro | 26 | fixed | not played – see 7. |

* Every module **loops** (`MIDASplayModule(module, TRUE)`); libxmp is asked
  to play forever (`xmp_play_buffer(..., loop = 0)`), the module's restart
  position is used.
* **Shuffle bag** (`music_bag`): a random track not played yet, the next
  unplayed one if it was; when all were played the bag starts again – and,
  as in the DOS loop, with the **first** track of the list.
* The level header's `music` byte is **not used** (the DOS game never read it;
  it is 4 in the 21 levels made from the editor template).
* Kept going: restart, save/load, the in-game menu, help, hint, settings,
  pause. A menu screen starts menu music unless menu music is playing.
* The editor has no sound; the game stops the music while the editor runs
  and starts a new menu track afterwards (as the DOS menu did).
* A custom level (`-u`) plays level music; after it's solved the end screen
  is silent (the DOS game quit).

## 5. Settings

```
sound = yes            # yes | no
music = yes            # yes | no
sound_volume = 86      # 0 - 100 %
music_volume = 23      # 0 - 100 %
```

The defaults are the DOS defaults 55 and 15 of the MIDAS 0–64 scale (in the
source, in the EXE and in the shipped SETUP.EXE). A config file without the
keys (older versions) gets these defaults; values out of range are clamped.
The music volume is libxmp's master volume, applied before the mix is
clipped (as `MIDASsetMusicVolume()`); the sound volume is the gain of the
effect streams.

Settings menu (the check boxes that were commented out in 1.x are back):
**sound** / **music** switch it at once (music on = the music of the current
situation); `- nn % +` next to them change the volume by 10 (a sound
volume change plays the switch sound, like the DOS slider). Saved at once.

## 6. libxmp

libxmp-lite 4.7.3 (MIT) – only the MOD / S3M / XM / IT player, a few dozen C
files. CMake uses an installed libxmp-lite or libxmp, otherwise it downloads
it like SDL (`BERUSKY_FETCH_LIBXMP`, `BERUSKY_LIBXMP_TAG`) and builds only
the lite static library (PIC for Android). All 30 modules load and play with
it (checked: channels, length, levels). Mixing: linear interpolation, stereo
separation 70 % (libxmp defaults).

## 7. Differences from the DOS game

* **Intro music (track 26)**: DOS played it during the typewriter intro
  ("uvádí logickou hru Berušky…", ~20 s) that 1.x replaced with the logo and
  the loading bar. There is no such scene, so the track isn't played (a
  0.4 s logo would only cut it). The id (`MUSIC_INTRO`) and the file exist.
* The first highlighted menu item: DOS pre-selected the first item without a
  sound; the port has no pre-selected item, the first hover plays it.
* Player switch: the 1.x logic allows switching while a bug moves; the sound
  plays when a bug actually gets selected (DOS: every Tab when no bug was
  moving, even with one bug).
* Menus of 1.x that DOS didn't have (profiles, level map, level end screen)
  use the same menu sounds; the light box pushes like a box.
* Sound after a solved level with music off: cut after 2 s (DOS: when the
  next level loop ran).
* Test runs from the editor (`berusky -u` started by F9) have sound; the DOS
  editor ran with sound off.
* The balance music : effects follows the DOS volumes (0.23 : 0.86); MIDAS'
  channel amplification is not reproduced exactly.
* Low-memory track list, MIDAS sound card setup, `berusky.cfg` – DOS only.

## 8. Tests

* `tests/audio_lang_test.cpp` (CTest `audio_language`): settings defaults,
  old configs, clamping, round trip; track lists vs. the DOS tables; the
  shuffle bag incl. the restart quirk; round-robin voices, cutting, sound
  off, music off/on, delayed menu / outro music, a module start silencing
  the effects, `SN_` events, missing sound and music files.
* `tests/scripts/20_audio.txt` plays `tests/levels/audio_events.lv3`
  (generated by `tests/tools/make_audio_level.py`): every in-game sound of
  the table above, the deferred ones at the end of the step, N, the exit.
  The audio log (`audiolog` command: game tick, sound, voice, length,
  priority, music tracks, cuts) is compared like a screenshot.
* `tests/scripts/21_settings_audio.txt`: the settings items, menu sounds.
* Real output was checked with SDL's disk audio driver
  (`BERUSKY_TEST_AUDIO=1 SDL_AUDIO_DRIVER=disk`): menu music, the clicks,
  steps and the N track change are where the log says.
