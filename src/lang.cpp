/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <string>
#include <unordered_map>

#include "berusky.h"
#include "lang_table.h"
#include "test_script.h"

static LANGUAGE language = LANGUAGE_EN;

static const char *codes[LANGUAGE_NUM] = { "en", "cs" };
static const char *names[LANGUAGE_NUM] = { "english", "čeština" };

void lang_set(LANGUAGE lang)
{
  if(lang >= 0 && lang < LANGUAGE_NUM)
    language = lang;
}

LANGUAGE lang_get(void)
{
  return(language);
}

const char *lang_code(LANGUAGE lang)
{
  return(lang >= 0 && lang < LANGUAGE_NUM ? codes[lang] : codes[LANGUAGE_EN]);
}

const char *lang_name(LANGUAGE lang)
{
  return(lang >= 0 && lang < LANGUAGE_NUM ? names[lang] : names[LANGUAGE_EN]);
}

bool lang_from_code(const char *p_code, LANGUAGE *p_lang)
{
  static const struct { const char *p_code; LANGUAGE lang; } aliases[] = {
    { "en", LANGUAGE_EN }, { "english", LANGUAGE_EN },
    { "cs", LANGUAGE_CS }, { "cz", LANGUAGE_CS }, { "czech", LANGUAGE_CS },
    { "cestina", LANGUAGE_CS }, { "čeština", LANGUAGE_CS },
  };

  if(!p_code)
    return(FALSE);

  for(unsigned i = 0; i < sizeof(aliases)/sizeof(aliases[0]); i++) {
    if(!SDL_strcasecmp(p_code, aliases[i].p_code)) {
      *p_lang = aliases[i].lang;
      return(TRUE);
    }
  }
  return(FALSE);
}

LANGUAGE lang_system(void)
{
  LANGUAGE lang = LANGUAGE_EN;
  int      count = 0;

  SDL_Locale **p_locales = SDL_GetPreferredLocales(&count);
  if(p_locales) {
    // The first language the user prefers that the game has
    for(int i = 0; i < count; i++) {
      if(!p_locales[i] || !p_locales[i]->language)
        continue;
      if(!SDL_strcasecmp(p_locales[i]->language, "cs")) {
        lang = LANGUAGE_CS;
        break;
      }
      if(!SDL_strcasecmp(p_locales[i]->language, "en"))
        break;
    }
    SDL_free(p_locales);
  }
  return(lang);
}

#define INI_LANGUAGE "language"

LANGUAGE lang_config_load(const char *p_ini_file)
{
  char     value[100];
  LANGUAGE lang;

  // BERUSKY_LANGUAGE overrides the config file (tests, a quick try)
  const char *p_env = SDL_getenv("BERUSKY_LANGUAGE");
  if(p_env && p_env[0])
    snprintf(value, sizeof(value), "%s", p_env);
  else
    ini_read_string_file(p_ini_file, INI_LANGUAGE, value, sizeof(value), "auto");

  if(!lang_from_code(value, &lang)) {
    if(!is_token(value, "auto"))
      bprintf("Unknown language '%s', using auto", value);
    lang = test_script_active() ? LANGUAGE_EN : lang_system();
  }

  lang_set(lang);
  bprintf("Language: %s", lang_code(lang));
  return(lang);
}

bool lang_config_save(const char *p_ini_file, LANGUAGE lang)
{
  return(ini_write_string(p_ini_file, INI_LANGUAGE, lang_code(lang)));
}

/*
 * Translation tables
 */
typedef std::unordered_map<std::string, const char *> TRANSLATION_MAP;

static const LANG_TABLE *lang_table_get(LANGUAGE lang)
{
  switch(lang) {
    case LANGUAGE_CS:
      return(&lang_table_cs);
    default:
      return(NULL);
  }
}

static TRANSLATION_MAP * translation_map(LANGUAGE lang)
{
  static TRANSLATION_MAP *p_maps[LANGUAGE_NUM] = { NULL };

  const LANG_TABLE *p_table = lang_table_get(lang);
  if(!p_table)
    return(NULL);

  if(!p_maps[lang]) {
    p_maps[lang] = new TRANSLATION_MAP;
    for(int i = 0; i < p_table->num; i++)
      (*p_maps[lang])[p_table->p_entries[i].p_en] = p_table->p_entries[i].p_text;
  }
  return(p_maps[lang]);
}

char *lang_translate(const char *p_text)
{
  if(!p_text || !p_text[0] || language == LANGUAGE_EN)
    return((char *)p_text);

  TRANSLATION_MAP *p_map = translation_map(language);
  if(p_map) {
    TRANSLATION_MAP::const_iterator it = p_map->find(p_text);
    if(it != p_map->end())
      return((char *)it->second);
  }
  return((char *)p_text);
}

int lang_table_size(LANGUAGE lang)
{
  const LANG_TABLE *p_table = lang_table_get(lang);
  return(p_table ? p_table->num : 0);
}

bool lang_table_entry(LANGUAGE lang, int i, const char **p_en, const char **p_text)
{
  const LANG_TABLE *p_table = lang_table_get(lang);
  if(!p_table || i < 0 || i >= p_table->num)
    return(FALSE);
  *p_en = p_table->p_entries[i].p_en;
  *p_text = p_table->p_entries[i].p_text;
  return(TRUE);
}

const char *lang_data_file(const char *p_dir, const char *p_file, char *p_buffer, size_t size)
{
  if(language != LANGUAGE_EN) {
    snprintf(p_buffer, size, "%s/%s", lang_code(language), p_file);
    if(file_exists(p_dir, p_buffer))
      return(p_buffer);
  }
  snprintf(p_buffer, size, "%s", p_file);
  return(p_buffer);
}

/*
 * UTF-8
 */
unsigned utf8_next(const char **pp_text)
{
  const unsigned char *p = (const unsigned char *)*pp_text;
  unsigned ch = p[0];

  if(!ch)
    return(0);

  int      extra = 0;
  unsigned min = 0;

  if(ch < 0x80) {
    *pp_text += 1;
    return(ch);
  } else if((ch & 0xe0) == 0xc0) {
    extra = 1; ch &= 0x1f; min = 0x80;
  } else if((ch & 0xf0) == 0xe0) {
    extra = 2; ch &= 0x0f; min = 0x800;
  } else if((ch & 0xf8) == 0xf0) {
    extra = 3; ch &= 0x07; min = 0x10000;
  } else {
    *pp_text += 1;
    return(p[0]);
  }

  for(int i = 1; i <= extra; i++) {
    if((p[i] & 0xc0) != 0x80) {
      // broken sequence - take the byte alone
      *pp_text += 1;
      return(p[0]);
    }
    ch = (ch << 6) | (p[i] & 0x3f);
  }

  if(ch < min) {
    *pp_text += 1;
    return(p[0]);
  }

  *pp_text += 1 + extra;
  return(ch);
}

int glyph_decompose(unsigned ch, ACCENT *p_accent)
{
  // The letters of Czech. The font has only capitals, lower case letters
  // are drawn with them too (as all the other texts).
  static const struct { unsigned ch; char base; ACCENT accent; } table[] = {
    { 0x00e1, 'a', ACCENT_ACUTE }, { 0x00c1, 'A', ACCENT_ACUTE },   // á Á
    { 0x010d, 'c', ACCENT_CARON }, { 0x010c, 'C', ACCENT_CARON },   // č Č
    { 0x010f, 'd', ACCENT_CARON }, { 0x010e, 'D', ACCENT_CARON },   // ď Ď
    { 0x00e9, 'e', ACCENT_ACUTE }, { 0x00c9, 'E', ACCENT_ACUTE },   // é É
    { 0x011b, 'e', ACCENT_CARON }, { 0x011a, 'E', ACCENT_CARON },   // ě Ě
    { 0x00ed, 'i', ACCENT_ACUTE }, { 0x00cd, 'I', ACCENT_ACUTE },   // í Í
    { 0x0148, 'n', ACCENT_CARON }, { 0x0147, 'N', ACCENT_CARON },   // ň Ň
    { 0x00f3, 'o', ACCENT_ACUTE }, { 0x00d3, 'O', ACCENT_ACUTE },   // ó Ó
    { 0x0159, 'r', ACCENT_CARON }, { 0x0158, 'R', ACCENT_CARON },   // ř Ř
    { 0x0161, 's', ACCENT_CARON }, { 0x0160, 'S', ACCENT_CARON },   // š Š
    { 0x0165, 't', ACCENT_CARON }, { 0x0164, 'T', ACCENT_CARON },   // ť Ť
    { 0x00fa, 'u', ACCENT_ACUTE }, { 0x00da, 'U', ACCENT_ACUTE },   // ú Ú
    { 0x016f, 'u', ACCENT_RING  }, { 0x016e, 'U', ACCENT_RING  },   // ů Ů
    { 0x00fd, 'y', ACCENT_ACUTE }, { 0x00dd, 'Y', ACCENT_ACUTE },   // ý Ý
    { 0x017e, 'z', ACCENT_CARON }, { 0x017d, 'Z', ACCENT_CARON },   // ž Ž
  };

  *p_accent = ACCENT_NONE;

  if(ch < 0x80)
    return((int)ch);

  for(unsigned i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
    if(table[i].ch == ch) {
      *p_accent = table[i].accent;
      return(table[i].base);
    }
  }

  return('?');
}
