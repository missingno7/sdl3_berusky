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
 * Game languages: English and Czech (see docs/LOCALIZATION.md).
 *
 * Texts in the code are English and marked with _("..."). _() looks the
 * text up in the table of the selected language (lang_cs.cpp) and returns
 * the English text itself when the language is English or there's no
 * translation. The returned string is static - it can be stored, it must
 * not be modified. N_("...") only marks a text (for tables initialized
 * before the language is known); translate it with _() where it's used.
 *
 * Longer texts (level hints, episode endings, credits) are data files:
 * lang_data_file() returns "<lang>/<file>" of the data directory when that
 * file exists, the English "<file>" otherwise.
 *
 * All texts are UTF-8; the font draws Czech letters as base letter + accent
 * (2d_graph.cpp).
 */
#ifndef __LANG_H__
#define __LANG_H__

#include <stddef.h>

typedef enum {
  LANGUAGE_EN = 0,
  LANGUAGE_CS,
  LANGUAGE_NUM
} LANGUAGE;

// Selected language
void        lang_set(LANGUAGE lang);
LANGUAGE    lang_get(void);

// "en" / "cs" (config file value, data subdirectory)
const char *lang_code(LANGUAGE lang);
// The language's name in the language itself ("english", "čeština")
const char *lang_name(LANGUAGE lang);

// Config value -> language: en, english, cs, cz, czech, cestina, čeština.
// Returns false for anything else (auto included).
bool        lang_from_code(const char *p_code, LANGUAGE *p_lang);

// Language of the system (Czech when the user prefers Czech, otherwise English)
LANGUAGE    lang_system(void);

// Reads "language = en | cs | auto" from the config file and selects it
// (the environment variable BERUSKY_LANGUAGE overrides it).
// auto (and a missing key) = lang_system(); the regression tests (a running
// test script) use English then, so the pictures don't depend on the machine.
LANGUAGE    lang_config_load(const char *p_ini_file);
bool        lang_config_save(const char *p_ini_file, LANGUAGE lang);

// Translation of an English text, the text itself when there's none
char       *lang_translate(const char *p_text);

// Number of translations of a language (tests)
int         lang_table_size(LANGUAGE lang);
// Translation of i-th table entry (tests): English text / translated text
bool        lang_table_entry(LANGUAGE lang, int i, const char **p_en, const char **p_text);

// Localized variant of a data file: "<lang code>/<file>" when it exists in
// p_dir, otherwise p_file. p_buffer receives the name.
const char *lang_data_file(const char *p_dir, const char *p_file, char *p_buffer, size_t size);

#define _(string)   lang_translate(string)
#define N_(string)  (string)

/*
 * UTF-8 helpers for the text renderer
 */

// Decodes the next character of a UTF-8 string and moves the pointer.
// Invalid bytes are returned one by one (as Latin-1), 0 at the end.
unsigned    utf8_next(const char **pp_text);

typedef enum {
  ACCENT_NONE = 0,
  ACCENT_CARON,       // háček   - č ď ě ň ř š ť ž
  ACCENT_ACUTE,       // čárka   - á é í ó ú ý
  ACCENT_RING,        // kroužek - ů
  ACCENT_NUM
} ACCENT;

// Splits a character into the base letter (ASCII) and an accent the way
// the DOS game drew Czech texts. Characters the font can't show at all
// return '?' (ASCII is returned as it is).
int         glyph_decompose(unsigned ch, ACCENT *p_accent);

#endif // __LANG_H__
