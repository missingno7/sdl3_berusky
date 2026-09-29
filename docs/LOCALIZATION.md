# Languages: English and Czech

The game can be played in **English** (the texts of the 1.x version) or
**Czech** – the texts of the original DOS game (1999), not a translation of
the English ones back. The language is chosen in the settings menu or in the
config file; English stays the default text of the code.

## 1. Architecture

One mechanism for everything, no per-language branches in the game code:

* **Short texts in the code** are English and marked `_("...")`.
  `_()` (`lang.h`) looks the text up in the table of the selected language
  (`src/lang_cs.cpp`, a `std::unordered_map` built on first use) and returns
  the English text when the language is English or has no translation. The
  result is a static string – it may be kept (menus keep the pointer for
  redrawing the highlighted item), never modified.
* Tables that are initialized before the language is known (item names of
  the editor, the difficulty names) are marked `N_("...")` and translated
  with `_()` where they're used. Menu labels are no longer `static` locals,
  so switching the language shows at once.
* **Long texts are data files**: `lang_data_file()` returns
  `GameData/<lang>/<file>` when it exists, the English `GameData/<file>`
  otherwise. Czech has `cs/hints.dat` (level texts), `cs/end0-4.dat` (episode
  endings), `cs/credits.dat`. They go through the normal file layer, so they
  work from APK assets on Android.
* `lang_config_load()` reads `language = en | cs | auto` (default `auto` =
  Czech when the system prefers Czech – `SDL_GetPreferredLocales()` –
  otherwise English; a running test script uses English so the pictures
  don't depend on the machine). `BERUSKY_LANGUAGE=cs` overrides the file.
  The editor (a separate process) reads the same setting.

### Why not gettext

The code base had `_()` markers and a `po/` directory, but gettext was never
initialized, `po/` had no translation at all (empty `LINGUAS`), and the SDL3
port builds with CMake on Windows, Linux and Android where libintl is an
extra dependency with locale-dependent behavior (Android has no usable
system gettext). A 250-entry table compiled into the game is simpler, needs
no locale, and is checked automatically. The dead `po/` directory and
`setup-gettext` were removed (and their lines in the unmaintained autotools
files); the `_()` markers were kept, so the sources read as before.

Log messages (`bprintf`) are developer output and stay English (their `_()`
markers were removed); every remaining `_()` is a user-visible text and must
have a translation – `tests/check_translations.py` (CTest `translations`)
fails otherwise, for a stale entry, a duplicate, different `printf`
conversions (a crash risk) or a letter the font can't draw.

## 2. Czech letters in the renderer

The 1.x UI font (`font0-2.png`) has capitals only – English is shown in
capitals too. It still contains the three accent glyphs of the DOS font
(caron, acute, ring: the three `^` lines of `font*.tab`). The DOS game drew
Czech the same way – letter + accent glyph over it (`$a` → á, `^c` → č,
`#u` → ů in its texts). So the port needs no new artwork:

* `font::print()` decodes UTF-8 (`utf8_next()`), `glyph_decompose()` maps
  á č ď é ě í ň ó ř š ť ú ů ý ž and capitals to base letter + accent, the
  accent sprite is centered over the letter, 1 unit higher. The width is
  the letter's width, so centering and hit rectangles stay right.
* Why 1 unit: capitals occupy rows 2–18 of the 20 unit glyph cell and
  multi-line texts are 20 units apart, so rows −1…1 are the only free band –
  higher accents land on the previous line's letters, lower ones on the
  letter.
* It's part of the resolution independent scene like any other sprite, so
  it scales with the renderer (and HD packs of the font).
* Characters the font can't draw at all become `?`.

## 3. Where the Czech texts come from

| Texts | Source | Notes |
|-------|--------|-------|
| menu labels | DOS menu pictures `DATA_BMP/MENU/MENU_*.BMP` | Nová hra, Nastavení, Nápověda, Konec, Obtížnost, Trénink … Nemožná, Ovládání, Pravidla, Autoři, Zpět, Zvuky, Hudba, Restart levelu, Uložit hru, Nahrát hru |
| controls help, rules | `BERUSKY.C` (`prvni_napoveda`, `pravidla_1-4`) | the DOS lines; double spaces of its justified proportional font removed |
| level screen, messages | `BERUSKY.C` | "obtížnost:", "Úroveň č.", "start levelu", "celkem kroků", "Ukládám úroveň…", editor messages, the command line help (plain ASCII like the DOS console) |
| level texts | `DOCLEP/TEXTY.TXT` (CP852) | 120 texts, same keys as `hints.dat` |
| episode endings | `DOCLEP/EPIZODA1-5.TXT` | |
| credits | `DOCLEP/CREDIT.TXB` + the 1.x additions | including its joke "jazikova koroktúra" |

The `.TXT` files are the CP852 sources of the shipped `.TXB` files (they
convert byte-exactly with the rules of the DOS `L2BER` tool). They keep
upper case accented letters, which `L2BER` wrote as lower case. Converted by
`tools/import_dos_assets.py` to UTF-8 (CRLF like the other data files); the
credits get 4 leading empty lines instead of 23 (the DOS scroller started at
the bottom of its window, the port's at the top).

**New Czech texts** (no DOS original – things 1.x added): profiles, the
level map ("Tip k úrovni", "vybrat poslední"), the level end screen, the
settings items of 1.x (full screen, scaling, photos, filter, language,
volume), the level editor of 1.x (the DOS editor was different; its
messages were reused where they match: "Uložit jako:", "%s již existuje.
přepsat?", "nemohu zapsat úroveň", the `(A/N)` answer), error dialogs.
Where a DOS label didn't fit the 1.x layout a shorter wording was chosen
("minulá" / "další" for the rules pages, "Tip k úrovni").

## 4. Adding or changing texts

* New text in the code: `_("english")` (or `N_()` in a static table), then
  `python tests/check_translations.py --missing` prints the entries to add
  to `src/lang_cs.cpp`.
* Another language: a `LANGUAGE_xx` value and names in `lang.h/.cpp`, a
  table like `lang_cs.cpp`, `GameData/xx/` files, the letters in
  `glyph_decompose()` if it needs accents the font has.

## 5. Known limitations

* The font has capitals only (both languages); profile names can be typed
  in ASCII only (as before).
* Some English lines of the level texts and episode endings are wider than
  the 640 unit composition with this font (already so before this work);
  they are cut at the right edge. Centered ones used to abort the game on an
  `assert` (every English episode ending did) – that is fixed. The Czech
  originals fit.
* `file_id.diz`, printed to the console on exit, is English.
* The window title is "Berušky" in Czech.
