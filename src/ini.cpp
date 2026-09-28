/* Martin Stransky <stransky@redhat.com>
 *
 * Copyright (C) 2003,2005 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* ini file handling
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ini.h"
#include "utils.h"

#define SPR '='

char *ini_remove_end_of_line(char *p_line)
{
  char *p_start = p_line;

  while (*p_line && *p_line != '\n')
    p_line++;

  if (*p_line == '\n')
    *p_line = '\0';

  return (p_start);
}

char *ini_skip_spaces(char *p_line)
{
  while (*p_line && (*p_line == ' ' || *p_line == '\t'))
    p_line++;
  return (p_line);
}

char *ini_skip_separator(char *p_line)
{
  char *p_tmp = strchr(p_line, SPR);
  if (p_tmp) {
    return (ini_skip_spaces(p_tmp + 1));
  }
  else {
    return (NULL);
  }
}

char *ini_read_param(char *p_line, char *p_param, int max_len)
{
  char *p_start = p_param;
  int i = 0;

  while (*p_line && i < max_len - 1 && *p_line != '\n') {
    *p_param++ = *p_line++;
    i++;
  }
  *p_param = '\0';

  return (p_start);
}


char *ini_read_string(FHANDLE f, const char *p_template, char *p_out,
                      int max_len, const char *p_default)
{
  char line[MAX_TOKEN_LEN];

  file_rewind(f);
  while (file_gets(line, MAX_TOKEN_LEN, f)) {
    int len = is_token(line, p_template);
    char *p_rest;
    if (len && (p_rest = ini_skip_separator(line + len))) {
      return (ini_read_param(p_rest, p_out, max_len));
    }
  }

  return (strcpy(p_out, p_default));
}

char *ini_read_string_file(const char *p_file, const char *p_template, char *p_out,
                           int max_len, const char *p_default)
{
  char line[MAX_TOKEN_LEN];
  FHANDLE f = file_open(NULL, p_file, "r", FALSE);

  if (!f)
    return (strcpy(p_out, p_default));

  while (file_gets(line, MAX_TOKEN_LEN, f)) {
    int len = is_token(line, p_template);
    char *p_rest;
    if (len && (p_rest = ini_skip_separator(line + len))) {
      file_close(f);
      return (ini_read_param(p_rest, p_out, max_len));
    }
  }

  file_close(f);
  return (strcpy(p_out, p_default));
}

int ini_read_int(FHANDLE f, const char *p_template, int dflt)
{
  char line[MAX_TOKEN_LEN];

  file_rewind(f);
  while (file_gets(line, MAX_TOKEN_LEN, f)) {
    int len = is_token(line, p_template);
    char *p_rest;
    if (len && (p_rest = ini_skip_separator(line + len))) {
      return (atoi(ini_remove_end_of_line(p_rest)));
    }
  }
  return (dflt);
}

int ini_read_int_file(const char *p_file, const char *p_template, int dflt)
{
  char line[MAX_TOKEN_LEN];
  FHANDLE f = file_open(NULL, p_file, "r", FALSE);

  if (!f)
    return (dflt);

  while (file_gets(line, MAX_TOKEN_LEN, f)) {
    int len = is_token(line, p_template);
    char *p_rest;
    if (len && (p_rest = ini_skip_separator(line + len))) {
      file_close(f);
      return (atoi(ini_remove_end_of_line(p_rest)));
    }
  }
  file_close(f);
  return (dflt);
}


int ini_read_bool(FHANDLE f, const char *p_template, int dflt)
{
  char line[MAX_TOKEN_LEN];

  ini_read_string(f, p_template, line, MAX_TOKEN_LEN, "");
  if (line[0] == '\0')
    return (dflt);
  else {
    if (is_token(line, TOKEN_FALSE1) || is_token(line, TOKEN_FALSE2))
      return (FALSE);
    else if (is_token(line, TOKEN_TRUE1) || is_token(line, TOKEN_TRUE2))
      return (TRUE);
    else
      return (dflt);
  }
}

int ini_read_bool_file(const char *p_file, const char *p_template, int dflt)
{
  char line[MAX_TOKEN_LEN];
  FHANDLE f = file_open(NULL, p_file, "r", FALSE);

  if (!f)
    return (dflt);

  ini_read_string(f, p_template, line, MAX_TOKEN_LEN, "");
  file_close(f);

  if (line[0] == '\0')
    return (dflt);
  else {
    if (is_token(line, TOKEN_FALSE1) || is_token(line, TOKEN_FALSE2))
      return (FALSE);
    else if (is_token(line, TOKEN_TRUE1) || is_token(line, TOKEN_TRUE2))
      return (TRUE);
    else
      return (dflt);
  }
}


/* Check if the given string is a token
*/
int is_token(char *p_line, const char *p_token)
{
  char *p_start = p_line;

  while (*p_line && *p_token && tolower(*p_line) == tolower(*p_token)) {
    p_line++;
    p_token++;
  }

  return (*p_token ? 0 : (int)(p_line - p_start));
}

/* Reading token (between %) from file
*/
int read_token(FHANDLE f_in, char *p_line, size_t max, char separator)
{
  size_t len;
  char c = 0;
  bool got = false;

  for (len = 0;
       len + 2 < max && (got = (file_read(&c, 1, f_in) == 1)) && c != separator;
       len++, p_line++) {
    *p_line = c;
  }

  if (got && c == separator) {
    *p_line++ = c;
  }
  *p_line = 0;

  return (got && c == separator);
}

/* Set "template = value" in the ini file. The file is small so it's
   rewritten as a whole: the line with the token is replaced (or a new one
   is appended).
*/
bool ini_write_string(const char *p_file,
                      const char *p_template, const char *p_value)
{
  t_off  len = 0;
  char  *p_text = (char *)file_load(NULL, p_file, &len, 0, FALSE);
  if (!p_text)
    return(FALSE);

  // Find the line with the token
  size_t line_start = 0, line_end = 0;
  bool   found = FALSE;
  char   line[MAX_TOKEN_LEN];

  size_t pos = 0;
  while (pos < len) {
    size_t end = pos;
    while (end < len && p_text[end] != '\n')
      end++;
    if (end < len)
      end++;                // include the newline

    size_t copy = end - pos;
    if (copy >= sizeof(line))
      copy = sizeof(line) - 1;
    memcpy(line, p_text + pos, copy);
    line[copy] = '\0';

    if (is_token(line, p_template)) {
      line_start = pos;
      line_end = end;
      found = TRUE;
      break;
    }
    pos = end;
  }

  FHANDLE f = file_open(NULL, p_file, "wb", FALSE);
  if (!f) {
    free(p_text);
    return(FALSE);
  }

  if (found) {
    file_write(p_text, line_start, f);
    file_printf(f, "%s = %s\n", p_template, p_value);
    file_write(p_text + line_end, len - line_end, f);
  }
  else {
    file_write(p_text, len, f);
    file_printf(f, "\n%s = %s\n", p_template, p_value);
  }

  file_close(f);
  free(p_text);

  return(TRUE);
}
