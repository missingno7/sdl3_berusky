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
 * Translation tables of lang.cpp: English text -> text of the language.
 */
#ifndef __LANG_TABLE_H__
#define __LANG_TABLE_H__

typedef struct {
  const char *p_en;
  const char *p_text;
} LANG_ENTRY;

typedef struct {
  const LANG_ENTRY *p_entries;
  int               num;
} LANG_TABLE;

extern const LANG_TABLE lang_table_cs;

#endif // __LANG_TABLE_H__
