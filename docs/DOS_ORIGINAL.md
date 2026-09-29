# The DOS original (`original/`) – investigation

`original/` (not tracked by git, read-only reference) holds the shipped DOS
game and the source archive Komat published in November 1999. This document
records what is in it, how it relates to this code base and which facts were
used to restore sound, music and the Czech texts. The implementation is
described in [AUDIO.md](AUDIO.md) and [LOCALIZATION.md](LOCALIZATION.md).

Everything below was checked against the **shipped executable** (the
behavioral oracle), not only the source. `BERUSKY.EXE` is packed with UPX
0.82 (djgpp2/coff); a copy was unpacked with UPX 3.05 and disassembled
(the tree under `original/` was not modified).

## 1. Contents

| Path | What it is |
|------|------------|
| `game/BERUSKY.EXE` | the shipped game, v0.1.1 (DJGPP, UPX packed), linked with MIDAS 1.1 |
| `game/BERUSKY.DAT` | "LEP" archive: 563 files – sprites, fonts, palettes, 120 levels, texts, demos |
| `game/MOD/X_000.XM … X_029.XM` | 30 FastTracker II modules (12.6 MB) – **the music** |
| `game/SMP/SMP_000.RAW … SMP_019.RAW` | 20 raw sound effects (694 KB) – **the sounds** |
| `game/BERUSKY.CFG` | MIDAS config + volumes (written by SETUP.EXE, edited in the game) |
| `game/SETUP.EXE` | MIDAS sound card setup |
| `game/DOC/*.TXT` | Czech manual/FAQ (`BERUSKY.TXT` CP852, `WBERUSKY.TXT` CP1250), GPL, contest |
| `src/BERUSKY/*.C, *.H` | game source: `BERUSKY.C` (game, menus, editor), `MENU.C`, `ANIMACE.C`, `SOUND.C`, `KONVER.C` |
| `src/BERUSKY/DOCLEP/` | text sources: `TEXTY.TXT` (level texts), `EPIZODA1-5.TXT`, level set lists, `CREDIT.TXB` |
| `src/BERUSKY/DATA/`, `DATA_BMP/` | archive inputs; `DATA_BMP/MENU/*.BMP` are the menu graphics with the Czech labels |
| `src/UTIL/` | tools: `L2BER` (text converter), `RW` (WAV→RAW), `LEP` (archiver), `SETUP`, `KONVLEV`, `BK` |
| `src/LIB/LIBMIDAS.A`, `src/INCLUDE/MIDASDLL.H` | MIDAS Digital Audio System 1.1 (with symbols) |
| `src/BERUSKY/MOD`, `SMP`, `HOTOVO/` | only README files: "take the music/samples from the binary distribution" |

### Relationship to this code base

This port is Berusky 1.7 (Martin "Komat" Stránský's 2006–2012 C++ rewrite,
translated to English by Radek Biba) ported to SDL3. It is **not** a
descendant of this source tree: the game logic, menus and renderer were
rewritten. Common ground:

* The **levels** are the same: all 120 `data/Levels/*.lv3` are byte-identical
  to the ones in `BERUSKY.DAT`, and the level sets (`s0-s4.dat`) list the same
  levels and passwords as `s_tren/lehky/stred/tezky/nemoz.txt`.
* The **English texts** are translations of the DOS Czech texts: `hints.dat`
  ↔ `TEXTY.TXT` (same 120 `~set_level` keys), `end0-4.dat` ↔ `EPIZODA1-5.TXT`,
  the help/rules screens ↔ `prvni_napoveda`, `pravidla_1-4` in `BERUSKY.C`.
* The **UI font** of 1.7 still contains the three DOS accent glyphs (caron,
  acute, ring – glyphs 53–55 of `font*.png`) although nothing used them.
* Sound was never implemented in 1.x: the settings menu had commented-out
  sound/music check boxes, `events.h` had unused `SN_PLAY_SAMPLE` /
  `SN_PLAY_MUSIC` events, `berusky.h` an unused `berusky_sound` class, and the
  README said "N – change the music (not implemented yet)".

Classification of differences (see also section 7):

* deliberate modern improvements – kept: renderer, resolution independence,
  profiles + level map instead of passwords, touch/gamepad, portable files,
  editor as a separate process, save/load/restart without limits;
* later upstream changes – kept: the 1.x menu layout, new light box item,
  new level sets (`s5.dat` user levels), the rewritten editor;
* accidentally lost – restored: **sound effects, music, the Czech texts**, and
  the per-episode ending (the port always showed the first episode's ending,
  see section 6).

## 2. Original audio architecture (`SOUND.C`, `SOUND.H`)

MIDAS 1.1 (Sahara Surfers) does all mixing. `zapni_midas()` reads
`berusky.cfg` (`buffer[0] == 4` = "No Sound", volumes at offset 28/32),
starts MIDAS, installs the display-refresh timer (`tik_30`, the animation
clock `kresli`), opens the module channels + **8 auto effect channels** and
loads the 20 samples (`smp\smp_%.3d.raw`, 16-bit signed mono, not looped).

* **Sound effects**: `hraj_sampl(id, length, priority)` queues a request in
  `tbas[50]`; `updatuj_samply()` (called from the game/menu loops) starts it
  with `MIDASplaySample(sample, MIDAS_CHANNEL_AUTO, prio, 20050 Hz, vol_sound,
  middle)` and stops it again when `length` ticks of the refresh timer
  (60 Hz in the 640x480 VESA mode, `FPS` = one second) have passed.
* **Channel allocation**: disassembly of `fxPlaySample` in the shipped EXE
  (identical to `LIBMIDAS.A/midasfx.o`) shows the auto channels are used
  **round-robin**; the priority argument is stored but never compared. A 9th
  simultaneous sound replaces the oldest one.
* **Music**: `hraj_modul(n)` loads `mod\x_%.3d.xm`, reallocates the channels
  (`uprav_kanaly` – this stops every playing effect) and plays it with
  `MIDASplayModule(module, TRUE)` – every module **loops** (the `loop` argument
  of `hraj_modul` is ignored). `stop_modul()` stops and frees it. With music
  off nothing is loaded. A missing file ends the program (`MIDASerror` →
  `exit`).
* **Volumes**: 0–64 (MIDAS scale). Defaults 55 (sound) / 15 (music) – in the
  source, in the EXE's data (`vol_music = 15, vol_sound = 55`) and in the
  shipped `SETUP.EXE`. The settings menu (`menu_nastaveni`) has two sliders;
  moving the sound slider plays sample 18 at the new volume, the music slider
  changes the volume live; Enter saves both to `berusky.cfg`, Esc cancels.
* Shutdown: `vypni_midas()` (`atexit`, error paths).

## 3. Sound effects – complete map

All 25 `hraj_sampl` call sites of the EXE were compared with the source
(ids, lengths, priorities and conditions are identical, including the typo
that passes 36 as the priority of sample 3). Lengths are in 60 Hz ticks.

| # | DOS name | Sound | Triggered by | Cut after | Prio |
|---|----------|-------|--------------|-----------|------|
| 0 | MODRY_BLESK | blue flash | a *cyber* (variation ≠ 0) one-pass door or color passage closes behind a bug – when the step ends (`soucastny_pole` → `pridej_zmenu` → `proved_zmenu`) | 1 s | 2 |
| 1 | ZELENY_BLESK | green flash | **never used** | – | – |
| 2 | MODRY_KAMEN | blue stone | stone of variation ≠ 0 broken with a pickax | 36 | 2 |
| 3 | ZELEZNY_KAMEN | iron stone | stone of variation 0 broken | 36 | 36 |
| 4 | KROKY_BLATO | steps, mud | every step onto floor variation < 5 | the step (20 / 10 fast) | 0 |
| 5 | KROKY_MRAMOR | steps, marble | every step onto floor variation ≥ 5 | the step | 0 |
| 6 | KROKY_POZADI | steps, background | every step onto a cell without floor | the step | 0 |
| 7 | MENU_SKOK | menu jump | the highlighted menu item changes (mouse or keys) | 18 (0.3 s); 1 s in settings | 1 |
| 8 | MENU_KLIK | menu click | a menu item is chosen (also Esc) | 18; 1 s in settings | 1 |
| 9 | NARAZ | bump | **never used** | – | – |
| 10 | ODEMYK | unlock | a color door is unlocked with a key | 1 s | 1 |
| 11 | OTEVRENI_EXITU | exit opens | the fifth key is taken – after the step ends | 1 s | 2 |
| 12 | OTEVRENI_DVERI | door opens | a *classic* color door is unlocked (with 10) | 1 s | 2 |
| 13 | POSUN | push | a box (or explosive) is pushed – with the step sound | the step | 1 |
| 14 | SEBRANI | pick up | a key, a pickax, a color key is taken | 1 s | 3 |
| 15 | VYBUCH | explosion | an explosive pushed into a box | 48 | 3 |
| 16 | ZAVRENI | closing | a *classic* one-pass door or color passage closes behind a bug (end of the step); a bug enters a closed *classic* color passage | 1 s | 2 |
| 17 | SAMPL_OK | level done | the bug walks into the open exit (the music is stopped first); the game then waits 30 DOS ticks (1.65 s) | 2 s | 3 |
| 18 | PREPNI | switch | switching bugs (Tab, not during a move); preview of the sound volume slider | 1 s | 1 |
| 19 | LISTA | bar | **never used** | – | – |

The same sample may play several times at once. Nothing loops. The steps
use the floor of the *target* cell. The intro typewriter text randomly
clicks samples 7/8 per letter (the port has no such intro).

## 4. Music – complete map

Module numbers come from the tables in the EXE's data section, which match
the source exactly:

| Situation | Tracks | Selection |
|-----------|--------|-----------|
| intro ("uvítání" typewriter animation after the Anakreon logo) | 26 | fixed |
| main menu, level intro screens, after every level | 4, 5, 6, 7, 8 | shuffle bag |
| a level starts (every level, also the next one) | 0,1,2,4,3,5,6 … 25 (26 tracks) | shuffle bag |
| `N` in a level | next level track | same bag |
| credits from the main menu | 0, 1, 2, 3 | shuffle bag |
| episode end, level sets 1–5 | 27, 28, 27, 28, 29 | fixed |

* **Shuffle bag** (`hrali[]` per category): a random entry; if it was
  already played, the next unplayed one; when all were played, the bag is
  emptied and – a quirk of the loop – the **first entry** of the list is
  played. (Low-memory machines used a 22-track list without 9, 10, 13, 15.)
* The level header's **`music` byte is never read**. It is 0 in 99 levels
  and 4 in 21 – the value of the editor template `level1.dat`, copied into
  every level made from it.
* Behavior: restart and quick save/load keep the music; the in-game menu, help
  and settings keep it (credits opened from the game too); level exit stops
  it; leaving a level (solved or not) starts a new menu track; the editor
  stops the music and runs without sound; `berusky u level.lv3` plays level
  music and stops at the end.

## 5. Texts

* **Encoding**: the game's texts use their own ASCII markup made by `L2BER`
  from CP852 sources: `$x` acute, `^x` caron, `#u` ring, drawn as an accent
  glyph over the letter. `L2BER` writes upper case accented letters as lower
  case (`Č` → `^c`), so the DOS game showed e.g. "čtvrtá" where the source has
  "Čtvrtá". The CP852 `.TXT` sources in `DOCLEP/` convert **byte-exactly**
  (verified) to the shipped `texty.txb` and `epizoda1-5.txb`, so they are the
  authoritative text.
* **Menus** are pictures (`DATA_BMP/MENU/MENU_*.BMP`): Nová hra, Heslo,
  Nápověda, Nastavení, Editor, Konec; Obtížnost: Trénink, Lehká, Střední,
  Těžká, Nemožná, Zpět; Nápověda: Ovládání, Pravidla, Autoři, Zpět; Opravdu
  skončit? Ano, Ne; in game: Restart levelu, Nahrát hru, Uložit hru, Konec,
  Zpět; Nastavení: Zvuky, Hudba, Zpět; editor: Uložit jako, Nahrát, Ok.
* **In-code texts** (`BERUSKY.C`): controls help, rules pages 1–4, level
  intro screen ("obtížnost:", "Úroveň č.", "Heslo:", "Hlavní menu", "start
  levelu"), save/load messages, editor messages, command line help (plain
  ASCII Czech), logo texts.
* `CREDIT.TXB` (credits, no `.TXT` source), `FILE_ID.DIZ` (printed on exit).
* Czech letters used: á č ď é ě í ň ó ř š ť ú ů ý ž and capitals.

## 6. Findings in the current port

* `GC_MENU_END_LEVEL_SET` did not pass the level set to the end screen, so
  every episode showed the training ending – fixed (the outro music depends
  on it too).
* The UI font has only capitals; Czech is shown the same way the DOS game did
  it – base glyph + accent glyph – which needs no new artwork.
* The in-game step counter ("steps: %d") was not translatable.
* English hint and ending lines are wider than the 640 unit composition with
  the 1.7 capitals font (pre-existing); the Czech originals fit.

## 7. What is not restored (and why)

* The DOS intro animation (typewriter "uvádí logickou hru Berušky" with
  track 26) – the 1.x start-up replaced it with the logo and loading bar;
  track 26 is shipped but has no trigger (see AUDIO.md).
* Demo recording/playback (`D`), passwords menu, screenshot key, "Opravdu
  skončit?" dialog, the limited number of quick saves/loads per level
  (8/8/6/4/4 by difficulty) – upstream 1.x design.
* The low-memory track list, MIDAS sound card setup, `berusky.cfg` binary
  format – DOS specific.
* Test runs from the editor: DOS ran the editor with sound off; the port's
  editor starts `berusky -u`, which has sound.

## 8. Provenance of the runtime assets taken from `original/`

`tools/import_dos_assets.py` copies/converts them reproducibly and
`--check` verifies the result:

| Runtime file | Source | Transformation |
|--------------|--------|----------------|
| `data/Music/x_000.xm … x_029.xm` | `game/MOD/X_0nn.XM` | none (lower case name) |
| `data/Sound/smp_000.raw … smp_019.raw` | `game/SMP/SMP_0nn.RAW` | none (lower case name) |
| `data/GameData/cs/hints.dat` | `src/BERUSKY/DOCLEP/TEXTY.TXT` | CP852 → UTF-8, LF |
| `data/GameData/cs/end0.dat … end4.dat` | `DOCLEP/EPIZODA1-5.TXT` | CP852 → UTF-8, LF |
| `data/GameData/cs/credits.dat` | `DOCLEP/CREDIT.TXB` + port credits | markup → UTF-8, see file |

SHA-256 sums are in `data/Music/SOURCE.txt` and `data/Sound/SOURCE.txt`. The
DOS release put all of it (graphics, programs, levels, music) under the GNU
GPL (`src/README.TXT`, `game/DOC/COPYING`).
