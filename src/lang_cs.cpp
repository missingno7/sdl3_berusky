/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
/*
 * Czech texts (UTF-8).
 *
 * Wherever the DOS game (1999) had the text, its wording is used - from its
 * menu pictures (DATA_BMP/MENU) and from BERUSKY.C; those entries are
 * marked "DOS". The rest is new (things the DOS game didn't have).
 * tests/check_translations.py checks this table against the sources.
 */
#include "lang_table.h"

static const LANG_ENTRY entries[] = {

  // ---------------------------------------------------------------------
  // Main menu
  // ---------------------------------------------------------------------
  { "play",                       "Nová hra" },                   // DOS
  { "change profile",             "Změnit profil" },
  { "settings",                   "Nastavení" },                  // DOS
  { "help",                       "Nápověda" },                   // DOS
  { "editor",                     "Editor" },                     // DOS
  { "quit",                       "Konec" },                      // DOS
  { "berusky version %s (C) Anakreon 1997-2012\n",
    "Berušky verze %s (C) Anakreon 1997-2012\n" },
  { "distributed under GPLv2\n",  "Chráněno GNU licencí (GPLv2)\n" },   // DOS
  { "Selected profile: %s",       "Vybraný profil: %s" },
  { "Berusky",                    "Berušky" },

  // ---------------------------------------------------------------------
  // Difficulty (level sets)
  // ---------------------------------------------------------------------
  { "Choose your level map:",     "Obtížnost:" },                 // DOS
  { "training",                   "Trénink" },                    // DOS
  { "easy",                       "Lehká" },                      // DOS
  { "intermediate",               "Střední" },                    // DOS
  { "advanced",                   "Těžká" },                      // DOS
  { "impossible",                 "Nemožná" },                    // DOS
  { "user levels",                "Uživatelské úrovně" },
  { "user set",                   "uživatelská sada" },
  { "back",                       "Zpět" },                       // DOS

  // ---------------------------------------------------------------------
  // Profiles
  // ---------------------------------------------------------------------
  { "Current profile is: %s",     "Aktuální profil: %s" },
  { "Create a new player profile:\n", "Vytvořit nový profil hráče:\n" },
  { "create",                     "vytvořit" },
  { "Choose saved profile:\n",    "Vybrat uložený profil:\n" },

  // ---------------------------------------------------------------------
  // Help menu
  // ---------------------------------------------------------------------
  { "level hint",                 "Tip k úrovni" },
  { "game controls",              "Ovládání" },                   // DOS
  { "game rulez",                 "Pravidla" },                   // DOS
  { "authors",                    "Autoři" },                     // DOS

  // ---------------------------------------------------------------------
  // Settings
  // ---------------------------------------------------------------------
  { "fulscreen",                  "Celá obrazovka" },
  { "integer scaling",            "Celočíselné zvětšení" },
  { "menu photos",                "Fotky v menu" },
  { "graphics filter: %s",        "Grafický filtr: %s" },
  { "sound",                      "Zvuky" },                      // DOS
  { "music",                      "Hudba" },                      // DOS
  { "language: %s",               "Jazyk: %s" },

  // ---------------------------------------------------------------------
  // Rules (DOS: pravidla_1 - pravidla_4)
  // ---------------------------------------------------------------------
  { "Basic rules and game elements", "Základní pravidla a herní prvky" },
  { "In order to leave each level it is\n"
    "necessary to own five keys and also\n"
    "to have a free way to the exit.\n"
    "\n"
    "You will be meeting miscellaneous game\n"
    "elements while completing individual\n"
    "missions, we will try to explain their\n"
    "meaning now.\n",
    "K opuštění každé úrovně je třeba\n"
    "vlastnit pět klíčů a navíc mít volnou\n"
    "cestu k východu. Při plnění jednotlivých\n"
    "misí se budete setkávat s rozličnými\n"
    "herními prvky, jejichž význam se vám\n"
    "nyní pokusíme ve stručnosti přiblížit.\n" },
  { "box - it is possible to push it.",
    "Bedny - lze je tlačit před sebou." },
  { "explosive - can destroy the boxes.",
    "výbušnina - dá se s ní zničit bedna." },
  { "Active game elements",       "Aktivní prvky ve hře" },
  { "key - you need five of them.",
    "klíč - potřebujete jich rovných pět." },
  { "exit - a gate to next level.",
    "východ - brána do další úrovně." },
  { "stone - can be broken by a pickax.",
    "kámen - je možné jej rozbít krompáčem." },
  { "pickax - a tool for stone crushing.",
    "krompáč - nástroj pro eliminaci kamenů." },
  { "color key - used to unlock color door,\n"
    "only a bug with identical color can\n"
    "pick them up",
    "barevné klíče - slouží pro odemykání\n"
    "barevných dveří, vzít jej může pouze\n"
    "beruška identické barvy." },
  { "color door - can be opened by the\n"
    "respective color key only",
    "Barevné dveře - otevřít je lze výhrad-\n"
    "ně klíčem příslušné barvy." },
  { "color gate-way - only a bug with\n"
    "identical color is allowed to go\n"
    "through. Boxes cannot be pushed\n"
    "through.",
    "Barevné průchody - propustí přes své\n"
    "věřeje jen berušku stejné barvy. Průcho-\n"
    "dem nelze protlačit bednu." },
  { "one-pass door - can be used only once,\n"
    "then it is closed off and there's no\n"
    "way to open it\n",
    "Jednoprůchodové dveře - lze jimi projít\n"
    "maximálně jednou, potom se navždy uza-\n"
    "vřou a již není cesty, jak je otevřít.\n" },
  { "Other elements not listed here are just\n"
    "walls, which have no interesting\n"
    "properties. They cannot be push away nor\n"
    "it is possible to break them anywise.\n",
    "Ostatní prvky, tedy ty, které zde nejsou\n"
    "uvedeny, jsou pouhými zdmi, které nemají\n"
    "žádné zajímavé vlastnosti. Nelze je od-\n"
    "tlačit a ani je není možno žádným způ-\n"
    "sobem rozbít.\n" },
  { "previous",                   "minulá" },
  { "next",                       "další" },

  // ---------------------------------------------------------------------
  // Controls (DOS: prvni_napoveda, druha_napoveda)
  // ---------------------------------------------------------------------
  { "Game Controls",              "Ovládání berušek" },
  { "Up to five bugs are available,\n"
    "which can be controlled by these keys:",
    "K dispozici můžete mít až pět berušek,\n"
    "které lze ovládat těmito klávesami:" },
  { "arrows",                     "šipky" },
  { "SHIFT+arrows",               "SHIFT+šipky" },
  { ". . . . . move the bug",     ". . . . . pohyb berušky" },
  { ". . quick bug movement",     ". . běh" },
  { ". . . . . . . switch among the bugs",
    ". . . . . . . přepínání mezi beruškami" },
  { ". . . . . . . . change the music", ". . . . . . . . změna hudby" },
  { ". . . . . . quit quickly",   ". . . . . . Rychlé ukončení" },
  { ". . . . . . . . Help",       ". . . . . . . . Nápověda" },
  { ". . . . . . . . Save level", ". . . . . . . . Uložení pozice" },
  { ". . . . . . . . Load level", ". . . . . . . . Nahrání pozice" },

  // ---------------------------------------------------------------------
  // Level hint, level map
  // ---------------------------------------------------------------------
  { "Level hint:",                "Tip k úrovni:" },
  { "\nSorry dude, no hint available for this\nlevel.",
    "\nPromiň, kámo, k této úrovni\nžádný tip nemáme." },
  { "play level",                 "start levelu" },               // DOS
  { "select last",                "vybrat poslední" },
  { "Level: %d - %s",             "Úroveň č. %d - %s" },          // DOS

  // ---------------------------------------------------------------------
  // End of a level
  // ---------------------------------------------------------------------
  { "your bugs have survived!",   "vaše berušky přežily!" },
  { "difficulty %s",              "obtížnost: %s" },              // DOS
  { "they made %d steps",         "celkem kroků: %d" },           // DOS
  { "and %s.",                    "čas: %s" },
  { "your bugs have given it up!", "vaše berušky to vzdaly!" },
  { "and spent %s",               "čas: %s" },
  { "play next level",            "další úroveň" },
  { "back to menu",               "zpět do menu" },
  { "custom level %s.",           "vlastní úroveň %s." },
  { "it took %d steps",           "celkem kroků: %d" },           // DOS

  // ---------------------------------------------------------------------
  // In-game menu (DOS: dilema)
  // ---------------------------------------------------------------------
  { "return to game (ESC)",       "Zpět do hry (ESC)" },          // DOS "Zpět"
  { "restart level (CTRL+R)",     "Restart levelu (CTRL+R)" },    // DOS
  { "save game (F2)",             "Uložit hru (F2)" },            // DOS
  { "load game (F3)",             "Nahrát hru (F3)" },            // DOS
  { "level hint (CTRL+F1)",       "Tip k úrovni (CTRL+F1)" },
  { "help (F1)",                  "Nápověda (F1)" },              // DOS
  { "quit (CTRL+X)",              "Konec (CTRL+X)" },             // DOS
  { "Level saved...",             "Ukládám úroveň..." },          // DOS
  { "Level loaded...",            "Nahrávám úroveň..." },         // DOS
  { "steps: %d",                  "celkem kroků: %d" },           // DOS

  // ---------------------------------------------------------------------
  // Errors (message boxes)
  // ---------------------------------------------------------------------
  { "Can't load surface %s",      "Nemohu načíst obrázek %s" },
  { "Unable to init SDL video: %s", "Nemohu spustit grafiku SDL: %s" },
  { "Unable to load levelset %d", "Nemohu načíst sadu úrovní %d" },
  { "Unable to load level %s",    "Nemohu načíst úroveň %s" },    // DOS
  { "Unable to load level %d from set %d", "Nemohu načíst úroveň %d ze sady %d" },
  { "This build doesn't contain the level editor.", "Tato verze nemá editor úrovní." },
  { "Can't find any configuration file!", "Nemohu najít konfigurační soubor!" },
  { "Failed, exiting...",         "Chyba, končím..." },
  { "Unable to open %s!\nError: %s", "Nemohu otevřít soubor %s!\nChyba: %s" },  // DOS
  { "Unable to load data, exiting...", "Nemohu načíst data, končím..." },
  { "Unable to create the window: %s", "Nemohu vytvořit okno: %s" },
  { "Unable to create the renderer: %s", "Nemohu spustit vykreslování: %s" },

  // ---------------------------------------------------------------------
  // Command line (console - plain ASCII like the DOS game printed it)
  // ---------------------------------------------------------------------
  { "This is free software; see the source for copying conditions.\n",
    "Blizsi informace ziskate v souboru copying.\n" },            // DOS
  { "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
    "Berusky jsou ABSOLUTNE BEZ ZARUKY.\n" },                     // DOS
  { "Built %s, %s\n\n",           "Sestaveno %s, %s\n\n" },
  { "Bad command line argument(s)!\n\n", "Chybne parametry!\n\n" },
  { "Using: berusky [-e [level.lv3]] [-u level.lv3]\n\n",
    "Pouziti: berusky [-e [level.lv3]] [-u level.lv3]\n\n" },
  { "  -e [level.lv3]       -run level editor\n",
    "  -e [level.lv3]       -editor urovni (jmeno urovne zadavat nemusite)\n" },  // DOS
  { "  -u  level.lv3        -run level\n",
    "  -u  level.lv3        -spusteni uzivatelske urovne\n" },    // DOS

  // ---------------------------------------------------------------------
  // Level editor
  // ---------------------------------------------------------------------
  { "Item: %s\nVariation: %d\nRotation: %s", "Prvek: %s\nVarianta: %d\nRotace: %s" },
  { "selection %d,%d - %d,%d",    "výběr %d,%d - %d,%d" },
  { "level cursor %d x %d",       "kurzor %d x %d" },
  { "No selection",               "Nic není vybráno" },
  { "help (f1)",                  "nápověda (f1)" },
  { "run level (f9)",             "spustit úroveň (f9)" },
  { "undo (ctrl+u)",              "zpět (ctrl+u)" },
  { "shade floor (ctrl+s)",       "stínovat podlahu (ctrl+s)" },
  { "change background (b)",      "změnit pozadí (b)" },
  { "floor",                      "podlaha" },
  { "items",                      "prvky" },
  { "players",                    "berušky" },
  { "all",                        "vše" },
  { "edited layer: %s",           "vrstva: %s" },
  { "Grid:",                      "Mřížka:" },
  { "Floor:",                     "Podlaha:" },
  { "Items:",                     "Prvky:" },
  { "Players:",                   "Berušky:" },
  { "off",                        "vyp" },
  { "on",                         "zap" },
  { "erase all data?",            "smazat všechna data?" },
  { "new level",                  "nová úroveň" },
  { "level is modified. erase all data?", "úroveň je změněná. smazat všechna data?" },
  { "Level to load:",             "Nahrát úroveň:" },             // DOS "Nahrát:"
  { "save to:",                   "Uložit jako:" },               // DOS
  { "file %s exists! overwrite?:", "%s již existuje. přepsat?" }, // DOS
  { "Unable to save level %s",    "nemohu zapsat úroveň %s" },    // DOS
  { "Saved as %s",                "Uloženo jako %s" },
  { "Keyboard control:",          "Ovládání klávesnicí:" },
  { "- Help",                     "- Nápověda" },
  { "- Quit",                     "- Konec" },
  { "- New level",                "- Nová úroveň" },
  { "- Save level",               "- Uložit úroveň" },
  { "- Save level as",            "- Uložit úroveň jako" },
  { "- Load level",               "- Nahrát úroveň" },
  { "- Run level",                "- Spustit úroveň" },
  { "- Pick item from cursor",    "- Vzít prvek pod kurzorem" },
  { "- Rotate item",              "- Otočit prvek" },
  { "- Undo",                     "- Zpět" },
  { "- Shade level",              "- Stínovat úroveň" },
  { "- Background",               "- Pozadí" },
  { "- Select floor layer",       "- Vrstva podlahy" },
  { "- Select items layer",       "- Vrstva prvků" },
  { "- Select players layer",     "- Vrstva berušek" },
  { "- Select all layer",         "- Všechny vrstvy" },
  { "- on/off background",        "- Zap/vyp pozadí" },
  { "- on/off floor layer",       "- Zap/vyp podlahu" },
  { "- on/off items layer",       "- Zap/vyp prvky" },
  { "- on/off players layer",     "- Zap/vyp berušky" },
  { "Mouse control:",             "Ovládání myší:" },
  { "first",                      "levé" },
  { "- insert selected item",     "- vložit vybraný prvek" },
  { "third",                      "pravé" },
  { "- clear selected cell",      "- smazat políčko" },
  { "R+wheel",                    "R+kolo" },
  { "- in place rotation",        "- otočit na místě" },
  { "V+wheel",                    "V+kolo" },
  { "- in place variation",       "- změnit variantu na místě" },
  { "F+first",                    "F+levé" },
  { "- fill rect with item",      "- vyplnit obdélník prvkem" },
  { "D+first",                    "D+levé" },
  { "- draw rect with item",      "- obdélník z prvku" },
  { "F+third",                    "F+pravé" },
  { "- clear solid rect",         "- smazat plný obdélník" },
  { "D+third",                    "D+pravé" },
  { "- clear empty rect",         "- smazat obvod obdélníku" },
  { "item panel mouse control:",  "panel prvků a myš:" },
  { "wheel",                      "kolečko" },
  { "- scroll by one",            "- posun o jeden" },
  { "- scroll page up",           "- stránka nahoru" },
  { "- scroll page down",         "- stránka dolů" },
  { "- first item",               "- první prvek" },
  { "- last item",                "- poslední prvek" },
  { "Screen control:",            "Ovládání obrazovky:" },
  { "- Full screen mode",         "- Celá obrazovka" },
  { "Arrows",                     "Šipky" },
  { "- Move screen",              "- Posun obrazovky" },
  { "Inserted item %d at %dx%d - %dx%d layer %d",
    "Vložen prvek %d na %dx%d - %dx%d, vrstva %d" },
  { "Inserted item %d at %dx%d layer %d",
    "Vložen prvek %d na %dx%d, vrstva %d" },
  { "[Modify] Rotating item at %dx%d layer %d",
    "[Změna] Otáčím prvek na %dx%d, vrstva %d" },
  { "[Set] Rotating item at %dx%d layer %d",
    "[Nastavení] Otáčím prvek na %dx%d, vrstva %d" },
  { "Can't rotate item at %dx%d layer %d",
    "Prvek na %dx%d, vrstva %d, nelze otočit" },
  { "[Modify] Variating item at %dx%d layer %d",
    "[Změna] Varianta prvku na %dx%d, vrstva %d" },
  { "[Set] Variating item at %dx%d layer %d",
    "[Nastavení] Varianta prvku na %dx%d, vrstva %d" },
  { "Erased items from %dx%d - %dx%d layer %d",
    "Smazány prvky %dx%d - %dx%d, vrstva %d" },
  { "Erased item at %dx%d layer %d",
    "Smazán prvek na %dx%d, vrstva %d" },
  { "Can't rotate item %d",       "Prvek %d nelze otočit" },
  { "Picking up item %d variant %d", "Beru prvek %d, varianta %d" },
  { "level is not saved. exit anyway?", "úroveň není uložená. přesto skončit?" },
  { "Run level...",               "Spouštím úroveň..." },
  { "Shading...",                 "Stínuji..." },
  { "background %d (from %d)",    "pozadí %d (z %d)" },
  { "Level %s %s",                "Úroveň %s %s" },
  { "(unsaved)",                  "(neuloženo)" },
  { "(saved)",                    "(uloženo)" },
  { "undo",                       "zpět" },
  { "no more undo",               "nelze vrátit" },
  { "y",                          "a" },                          // DOS "(A/N)"
  { "n",                          "n" },

  // Items (editor)
  { "Floor",                      "Podlaha" },
  { "Player 1",                   "Beruška 1" },
  { "Player 2",                   "Beruška 2" },
  { "Player 3",                   "Beruška 3" },
  { "Player 4",                   "Beruška 4" },
  { "Player 5",                   "Beruška 5" },
  { "Box",                        "Bedna" },
  { "Explosive",                  "Výbušnina" },
  { "Wall",                       "Zeď" },
  { "Exit",                       "Východ" },
  { "Stone",                      "Kámen" },
  { "Key (to exit)",              "Klíč (k východu)" },
  { "Pickax",                     "Krompáč" },
  { "Color Key (for player 1)",   "Barevný klíč (pro berušku 1)" },
  { "Color Key (for player 2)",   "Barevný klíč (pro berušku 2)" },
  { "Color Key (for player 3)",   "Barevný klíč (pro berušku 3)" },
  { "Color Key (for player 4)",   "Barevný klíč (pro berušku 4)" },
  { "Color Key (for player 5)",   "Barevný klíč (pro berušku 5)" },
  { "Color door (for player 1)",  "Barevné dveře (pro berušku 1)" },
  { "Color door (for player 2)",  "Barevné dveře (pro berušku 2)" },
  { "Color door (for player 3)",  "Barevné dveře (pro berušku 3)" },
  { "Color door (for player 4)",  "Barevné dveře (pro berušku 4)" },
  { "Color door (for player 5)",  "Barevné dveře (pro berušku 5)" },
  { "Color gate-way (for player 1)", "Barevný průchod (pro berušku 1)" },
  { "Color gate-way (for player 2)", "Barevný průchod (pro berušku 2)" },
  { "Color gate-way (for player 3)", "Barevný průchod (pro berušku 3)" },
  { "Color gate-way (for player 4)", "Barevný průchod (pro berušku 4)" },
  { "Color gate-way (for player 5)", "Barevný průchod (pro berušku 5)" },
  { "One-pass door",              "Jednoprůchodové dveře" },
  { "Jamb (Wall)",                "Zárubeň (zeď)" },
  { "Light box",                  "Lehká bedna" },
  { "0 DG.",                      "0 st." },
  { "90 DG.",                     "90 st." },
  { "180 DG.",                    "180 st." },
  { "270 DG.",                    "270 st." },
};

const LANG_TABLE lang_table_cs = { entries, (int)(sizeof(entries)/sizeof(entries[0])) };
