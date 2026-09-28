/*
 *        .þÛÛþ þ    þ þÛÛþ.     þ    þ þÛÛÛþ.  þÛÛÛþ .þÛÛþ. þ    þ
 *       .þ   Û Ûþ.  Û Û   þ.    Û    Û Û    þ  Û.    Û.   Û Ûþ.  Û
 *       Û    Û Û Û  Û Û    Û    Û   þ. Û.   Û  Û     Û    Û Û Û  Û
 *     .þþÛÛÛÛþ Û  Û Û þÛÛÛÛþþ.  þþÛÛ.  þþÛÛþ.  þÛ    Û    Û Û  Û Û
 *    .Û      Û Û  .þÛ Û      Û. Û   Û  Û    Û  Û.    þ.   Û Û  .þÛ
 *    þ.      þ þ    þ þ      .þ þ   .þ þ    .þ þÛÛÛþ .þÛÛþ. þ    þ
 *
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz> 
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

/*
  Utility
*/
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <SDL3/SDL.h>

#include "portability.h"
#include "platform.h"

#include "ini.h"
#include "utils.h"
#include "types.h"

#include "berusky.h"
#include "berusky_gui.h"
#include "main.h"
#include "editor.h"
#include "test_script.h"


FHANDLE log_file;

void log_close(void)
{
  if(log_file) {
    file_close(log_file);
    log_file = FHANDLE();
  }
}

void log_open(const char *p_file)
{
  log_close();

  if(p_file) {
    log_file = file_open(NO_DIR,p_file,"a",FALSE);
    if(!log_file)
      berror("Unable to open log file '%s', logging is disabled.",p_file);
    bprintf("------------- start ----------------");
  }
}

void log_open_ini(const char *p_ini_file)
{
  char logfile[MAX_FILENAME];
  #define INI_LOGFILE "logfile"
  #define INI_LOG     "log"

  if(ini_read_bool_file(p_ini_file, INI_LOG, FALSE)) {
    // The default log lives in the user data directory
    ini_read_string_file(p_ini_file, INI_LOGFILE, logfile, sizeof(logfile), user_file_get("berusky.log"));
    log_open(logfile);
  }
}

void log_flush(void)
{
  file_flush(log_file);
}

time_t game_clock(void)
{
  if(test_script_active())
    return((time_t)(test_script_ticks() / 30));
  return(time(NULL));
}

// A fatal error. Show it to the user (SDL message box / stderr) and quit.
void berror_message(const char *p_text)
{
  bprintf("Error: %s", p_text);
  log_close();

  platform_message(true, GAME_TITLE, p_text);
  exit(255);
}

// -------------------------------------------------------
//   User (writable) data locations
// -------------------------------------------------------

// <user data dir>/<name>
const char * user_file_get(const char *p_name)
{
  static char path[MAX_FILENAME];
  snprintf(path, sizeof(path), "%s%s", platform_user_dir(), p_name);
  return(path);
}

const char * user_dir_levels(void)
{
  static char path[MAX_FILENAME];
  snprintf(path, sizeof(path), "%s%s", platform_user_dir(), USER_LEVELS_DIR);
  return(path);
}

const char * user_dir_profiles(void)
{
  static char path[MAX_FILENAME];
  snprintf(path, sizeof(path), "%s%s", platform_user_dir(), USER_PROFILES_DIR);
  return(path);
}

// -------------------------------------------------------
//   Game directories
// -------------------------------------------------------

// Read a directory from the config file. It overrides the default one.
static void dir_config_read(const char *p_ini, const char *p_key, char *p_dir,
                            int max, const char *p_default)
{
  ini_read_string_file(p_ini, p_key, p_dir, max, p_default);

  // An empty value means "use the default"
  if(!p_dir[0])
    strncpy(p_dir, p_default, max-1);

  // '~' -> home directory
  if(p_dir[0] == '~') {
    char tmp[MAX_FILENAME];
    return_path(p_dir, "", tmp, MAX_FILENAME);
    // return_path() appends a separator, we don't want it here
    size_t len = strlen(tmp);
    if(len > 0 && tmp[len-1] == '/')
      tmp[len-1] = '\0';
    strncpy(p_dir, tmp, max-1);
  }
  p_dir[max-1] = '\0';
}

void dir_list::load(const char *p_ini)
{
  #define INI_LEVEL       "level_data"
  #define INI_GAME        "game_data"
  #define INI_GRAPHICS    "graphics_data"
  #define INI_LEVEL_USER  "level_data_user"
  #define INI_BINARY      "game_binary"
  #define INI_TMP         "tmp_data"

  char def[MAX_FILENAME];
  const char *p_root = platform_asset_root();

  // Read-only data. Defaults come from the platform asset root, the config
  // file may point elsewhere.
  snprintf(def, sizeof(def), "%sLevels", p_root);
  dir_config_read(p_ini, INI_LEVEL, levels, sizeof(levels), def);

  snprintf(def, sizeof(def), "%sGameData", p_root);
  dir_config_read(p_ini, INI_GAME, gamedata, sizeof(gamedata), def);

  snprintf(def, sizeof(def), "%sGraphics", p_root);
  dir_config_read(p_ini, INI_GRAPHICS, graphics, sizeof(graphics), def);

  // Writable data - user directory
  dir_config_read(p_ini, INI_LEVEL_USER, levels_user, sizeof(levels_user), user_dir_levels());
  dir_config_read(p_ini, INI_TMP, tmp, sizeof(tmp), user_file_get("Tmp"));
  dir_config_read(p_ini, INI_BINARY, game_binary, sizeof(game_binary), platform_executable());

  dir_create(levels_user);
  dir_create(tmp);

  bprintf("level_data: %s",levels);
  bprintf("game_data: %s",gamedata);
  bprintf("graphics_data: %s",graphics);
  bprintf("level_data_user: %s",levels_user);
  bprintf("tmp_data: %s",tmp);
  bprintf("game_binary: %s",game_binary);
}

// -------------------------------------------------------
//   Config file values
// -------------------------------------------------------

#define INI_FULLSCREEN "fullscreen"
bool get_fullscreen(const char *p_ini_file)
{
  return(ini_read_int_file(p_ini_file, INI_FULLSCREEN, FALSE));
}

bool set_fullscreen(const char *p_ini_file, bool state)
{
  char tmp[100];
  return(ini_write_string(p_ini_file, INI_FULLSCREEN, my_itoa(10, tmp, state ? 1 : 0)));
}

/* The keys of the old double-size mode (disable_double_size,
   startup_doublesize_question) are not read any more: the renderer draws at
   the display's resolution. They are left in old config files untouched. */

bool get_menu_background_photo(const char *p_ini_file)
{
  char value[100];
  ini_read_string_file(p_ini_file, INI_MENU_BACKGROUND, value, sizeof(value), "photo");
  return(!is_token(value, "black"));
}

int  get_colors(const char *p_ini_file, int default_color_depth)
{
  #define INI_COLOR "color_depth"
  return(ini_read_int_file(p_ini_file, INI_COLOR, default_color_depth));
}

char * my_itoa(int base, char *buf, int d)
{
  char *p = buf;
  char *p1, *p2;
  unsigned long ud = d;
  int divisor = 10;

/* If %d is specified and D is minus, put `-' in the head. */
  if (base == 'd' && d < 0) {
    *p++ = '-';
    buf++;
    ud = -d;
  }
  else if (base == 'x')
    divisor = 16;

  /* Divide UD by DIVISOR until UD == 0. */
  do {
    int remainder = ud % divisor;

    *p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
  }
  while (ud /= divisor);

  /* Terminate BUF. */
  *p = 0;

  /* Reverse BUF. */
  p1 = buf;
  p2 = p - 1;
  while (p1 < p2) {
    char tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
    p1++;
    p2--;
  }
  return(buf);
}

// -------------------------------------------------------
//   Paths & files
// -------------------------------------------------------

/* Create a path. '/' is used as the separator on all platforms. */
char * return_path(const char *p_dir, const char *p_file, char *p_buffer, int max_lenght)
{
  if(p_dir) {
    if(p_dir[0] == '~') {
      dir_home_get(p_buffer,max_lenght);
      strncat(p_buffer,p_dir+1,max_lenght-strlen(p_buffer)-1);
    } else {
      strncpy(p_buffer,p_dir,max_lenght-1);
      p_buffer[max_lenght-1] = '\0';
    }
    // don't produce "//" for empty directory parts
    size_t len = strlen(p_buffer);
    if(len > 0)
      strncat(p_buffer,"/",max_lenght-strlen(p_buffer)-1);
    strncat(p_buffer,p_file,max_lenght-strlen(p_buffer)-1);
  } else {
    if(p_file[0] == '~') {
      dir_home_get(p_buffer,max_lenght);
      strncat(p_buffer,p_file+1,max_lenght-strlen(p_buffer)-1);
    } else {
      strncpy(p_buffer,p_file,max_lenght-1);
      p_buffer[max_lenght-1] = '\0';
    }
  }
  return(p_buffer);
}

struct file_impl {

  bool          reading;

  // Reading: whole file in memory
  char         *p_data;
  size_t        size;
  size_t        pos;
  bool          eof;     // set by a read that ran into the end of data (like feof())

  // Writing
  SDL_IOStream *p_io;

};

/* Open a file */
FHANDLE file_open(const char * p_dir, const char * p_file, const char *p_mode, bool safe)
{
  char filename[MAX_FILENAME];
  return_path(p_dir, p_file, filename, MAX_FILENAME);

  struct file_impl *p_f = (struct file_impl *)mmalloc(sizeof(struct file_impl));

  bool writing = strchr(p_mode,'w') || strchr(p_mode,'a') || strchr(p_mode,'+');
  p_f->reading = !writing;

  if(p_f->reading) {
    p_f->p_data = (char *)SDL_LoadFile(filename, &p_f->size);
    if(p_f->p_data && !strchr(p_mode,'b')) {
      // Text mode - drop CR of CRLF the way the C library does on Windows
      size_t in, out;
      for(in = out = 0; in < p_f->size; in++) {
        if(p_f->p_data[in] == '\r' && in+1 < p_f->size && p_f->p_data[in+1] == '\n')
          continue;
        p_f->p_data[out++] = p_f->p_data[in];
      }
      p_f->size = out;
    }
    if(!p_f->p_data) {
      free(p_f);
      p_f = NULL;
    }
  } else {
    p_f->p_io = SDL_IOFromFile(filename, p_mode);
    if(!p_f->p_io) {
      free(p_f);
      p_f = NULL;
    }
  }

  if(!p_f && safe) {
    berror("Unable to open %s!\nError: %s", filename, SDL_GetError());
  }

  return(FHANDLE(p_f));
}

void file_close(FHANDLE f)
{
  if(f) {
    if(f.f->reading)
      SDL_free(f.f->p_data);
    else
      SDL_CloseIO(f.f->p_io);
    free(f.f);
  }
}

// fgets()
char * file_gets(char *p_buffer, int max_lenght, FHANDLE f)
{
  struct file_impl *p_f = f.f;

  if(!p_f || !p_f->reading || max_lenght < 2)
    return(NULL);

  if(p_f->pos >= p_f->size) {
    p_f->eof = true;
    return(NULL);
  }

  int i = 0;
  while(i < max_lenght-1 && p_f->pos < p_f->size) {
    char c = p_f->p_data[p_f->pos++];
    p_buffer[i++] = c;
    if(c == '\n')
      break;
  }
  p_buffer[i] = '\0';

  // like the C library: reading the last line w/o a newline hits the EOF
  if(p_buffer[i-1] != '\n' && p_f->pos >= p_f->size)
    p_f->eof = true;

  return(p_buffer);
}

size_t file_read(void *p_buffer, size_t bytes, FHANDLE f)
{
  struct file_impl *p_f = f.f;

  if(!p_f || !p_f->reading)
    return(0);

  size_t left = p_f->size - p_f->pos;
  if(bytes > left) {
    bytes = left;
    p_f->eof = true;
  }
  memcpy(p_buffer, p_f->p_data + p_f->pos, bytes);
  p_f->pos += bytes;
  return(bytes);
}

size_t file_write(const void *p_buffer, size_t bytes, FHANDLE f)
{
  struct file_impl *p_f = f.f;

  if(!p_f || p_f->reading)
    return(0);

  return(SDL_WriteIO(p_f->p_io, p_buffer, bytes));
}

int file_printf(FHANDLE f, const char *p_format, ...)
{
  struct file_impl *p_f = f.f;

  if(!p_f || p_f->reading)
    return(0);

  va_list arguments;
  va_start(arguments, p_format);
  char text[4000];
  int len = vsnprintf(text, sizeof(text), p_format, arguments);
  va_end(arguments);

  if(len > (int)sizeof(text)-1)
    len = sizeof(text)-1;
  if(len > 0)
    SDL_WriteIO(p_f->p_io, text, len);
  return(len);
}

bool file_seek(FHANDLE f, long offset)
{
  struct file_impl *p_f = f.f;

  if(!p_f || !p_f->reading || offset < 0 || (size_t)offset > p_f->size)
    return(false);

  p_f->pos = offset;
  p_f->eof = false;
  return(true);
}

void file_rewind(FHANDLE f)
{
  file_seek(f, 0);
}

long file_tell(FHANDLE f)
{
  struct file_impl *p_f = f.f;

  if(!p_f)
    return(-1);
  return(p_f->reading ? (long)p_f->pos : (long)SDL_TellIO(p_f->p_io));
}

bool file_eof(FHANDLE f)
{
  return(f.f ? f.f->eof : true);
}

void file_flush(FHANDLE f)
{
  if(f.f && !f.f->reading)
    SDL_FlushIO(f.f->p_io);
}

/* load file into memory */
int file_load(const char * p_dir, const char * p_file, char * p_mem, t_off max_lenght, t_off start_address, bool safe)
{
  return(file_load(file_open(p_dir, p_file, "rb", safe), p_mem, max_lenght, start_address));
}

int file_load_text(const char * p_dir, const char * p_file, char * p_mem, t_off max_lenght, t_off start_address, bool safe)
{
  return(file_load_text(file_open(p_dir, p_file, "r", safe), p_mem, max_lenght, start_address));
}

/* load file into memory */
int file_load(FHANDLE f, char * p_mem, t_off max_lenght, t_off start_address)
{
  dword loaded = 0;

  if(f) {
    if(start_address)
      file_seek(f, start_address);
    loaded = (dword)file_read(p_mem, max_lenght, f);
    file_close(f);
  }

  return (loaded);
}

/* load file into memory */
int file_load_text(FHANDLE f, char * p_mem, t_off max_lenght, t_off start_address)
{
  dword loaded = 0;

  if(f) {
    if(start_address)
      file_seek(f, start_address);
    loaded = (dword)file_read(p_mem, max_lenght, f);
    p_mem[loaded] = '\0';
    file_close(f);
  }

  return (loaded);
}

/* load file into memory */
void * file_load(const char * p_dir, const char * p_file, t_off *p_lenght, t_off start_address, bool safe)
{
  FHANDLE f = file_open(p_dir, p_file, "rb", safe);

  if(!f)
    return(NULL);

  size_t to_load = f.f->size > start_address ? f.f->size - start_address : 0;

  // +1 - zero terminated for convenience
  void *p_mem = mmalloc((int)to_load + 1);
  if(start_address)
    file_seek(f, start_address);
  *p_lenght = (t_off)file_read(p_mem, to_load, f);
  file_close(f);

  return (p_mem);
}

/* save file from memory */
bool file_save(const char * p_dir, const char * p_file, void *p_buffer, t_off lenght, const char *p_mode)
{
  FHANDLE f = file_open(p_dir, p_file, p_mode);
  size_t wrt = file_write(p_buffer, lenght, f);
  file_close(f);
  return(wrt == lenght);
}

bool file_copy(const char *p_src, const char *p_src_dir, const char *p_dest, const char *p_dest_dir, bool safe)
{
  t_off  len = 0;
  void  *p_data = file_load(p_src_dir, p_src, &len, 0, safe);
  if(!p_data)
    return(FALSE);

  FHANDLE dst = file_open(p_dest_dir, p_dest, "wb", safe);
  if(!dst) {
    free(p_data);
    return(FALSE);
  }

  bool ret = (file_write(p_data, len, dst) == len);

  file_close(dst);
  free(p_data);

  return(ret);
}

bool file_exists(const char * p_dir, const char * p_file)
{
  // Open instead of stat() - it works for packaged assets, too
  FHANDLE f = file_open(p_dir, p_file, "rb", FALSE);
  if(!(f)) {
    return(FALSE);
  }
  else {
    file_close(f);
    return(TRUE);
  }
}

int file_size_get(FHANDLE f)
{
  return(f.f ? (int)f.f->size + 1 : 0);
}

int file_size_get(const char * p_dir, const char * p_file)
{
  FHANDLE f = file_open(p_dir, p_file, "rb");
  int size = file_size_get(f);
  file_close(f);
  return(size);
}

void print_errno(bool new_line)
{
  if(new_line) {
    bprintf("\nError: %s",SDL_GetError());
  } else {
    bprintf("%s",SDL_GetError());
  }
}

char * dir_home_get(char *p_dir, int max)
{
  assert(p_dir);

  if(!platform_home_dir(p_dir, max)) {
    // a homeless user?
    assert(max >= 1);
    p_dir[0] = '\0';
  }
  return(p_dir);
}

bool dir_create(const char *p_dir)
{
  assert(p_dir);

  char tmp_dir[MAX_FILENAME];
  return_path(p_dir, "", tmp_dir, MAX_FILENAME);

  // return_path() adds a separator behind the directory
  size_t len = strlen(tmp_dir);
  if(len > 1 && tmp_dir[len-1] == '/')
    tmp_dir[len-1] = '\0';

  // Check the dir
  bprintfnl("Checking %s...",tmp_dir);
  if(platform_dir_exists(tmp_dir)) {
    bprintf("ok");
    return(TRUE);
  }

  bprintfnl("\nmissing, try to create it...");
  if(platform_dir_create(tmp_dir)) {
    bprintf("ok");
    return(TRUE);
  }
  print_errno(TRUE);
  return(FALSE);
}

// Sorted list of files in the directory (writable data only - profiles).
// Files are matched by a glob mask ('*' and '?').
static int file_list_compare(const void *p_a, const void *p_b)
{
  return(strcmp(((const DIRECTORY_ENTRY *)p_a)->name, ((const DIRECTORY_ENTRY *)p_b)->name));
}

int file_list_get(const char *p_dir, const char *p_mask, DIRECTORY_ENTRY **p_list)
{
  char tmp[MAX_FILENAME];
  return_path(p_dir, "", tmp, MAX_FILENAME);

  int c = 0;
  char **p_files = SDL_GlobDirectory(tmp, p_mask, SDL_GLOB_CASEINSENSITIVE, &c);
  if(!p_files || c <= 0) {
    SDL_free(p_files);
    return(0);
  }

  *p_list = (DIRECTORY_ENTRY *)mmalloc(sizeof(DIRECTORY_ENTRY)*c);
  for(int i = 0; i < c; i++) {
    strncpy((*p_list)[i].name, p_files[i], MAX_FILENAME-1);
  }
  SDL_free(p_files);

  qsort(*p_list, c, sizeof(DIRECTORY_ENTRY), file_list_compare);
  return(c);
}

/* Loading routines
*/
bool graphics_logos_load(DIR_LIST *p_dir)
{
  int i = 0;

  p_grf->graphics_dir_set(p_dir->graphics_get());

  sprite::color_key_set(COLOR_KEY_GAME);
  i  += p_grf->sprite_insert("logo.spr", FIRST_LOGO);

  return(i);
}

void graphics_logos_free(void)
{
  p_grf->sprite_delete(FIRST_LOGO, 1);
}

bool graphics_game_load(DIR_LIST *p_dir)
{
  int i;

  p_grf->graphics_dir_set(p_dir->graphics_get());

  bprintf(_("Graphics dir '%s'"),p_dir->graphics_get());
  bprintf(_("Loading game graphics..."));

  sprite::color_key_set(COLOR_KEY_GAME);

  i  = p_grf->sprite_insert("global1.spr", FIRST_GLOBAL_LEVEL);
  i += p_grf->sprite_insert("global2.spr", FIRST_GLOBAL_LEVEL + ROT_SHIFT);
  i += p_grf->sprite_insert("global3.spr", FIRST_GLOBAL_LEVEL + 2 * ROT_SHIFT);
  i += p_grf->sprite_insert("global4.spr", FIRST_GLOBAL_LEVEL + 3 * ROT_SHIFT);

  i += p_grf->sprite_insert("klasik1.spr", FIRST_CLASSIC_LEVEL);
  i += p_grf->sprite_insert("klasik2.spr", FIRST_CLASSIC_LEVEL + ROT_SHIFT);
  i += p_grf->sprite_insert("klasik3.spr", FIRST_CLASSIC_LEVEL + 2 * ROT_SHIFT);
  i += p_grf->sprite_insert("klasik4.spr", FIRST_CLASSIC_LEVEL + 3 * ROT_SHIFT);

  i += p_grf->sprite_insert("kyber1.spr", FIRST_CYBER_LEVEL);
  i += p_grf->sprite_insert("kyber2.spr", FIRST_CYBER_LEVEL + ROT_SHIFT);
  i += p_grf->sprite_insert("kyber3.spr", FIRST_CYBER_LEVEL + 2 * ROT_SHIFT);
  i += p_grf->sprite_insert("kyber4.spr", FIRST_CYBER_LEVEL + 3 * ROT_SHIFT);
  
  i += p_grf->sprite_insert("herni1.spr",  FIRST_OTHER);
  i += p_grf->sprite_insert("herni2.spr",  FIRST_OTHER + ROT_SHIFT);
  
  i += p_grf->sprite_insert("game_cur.spr", FIRST_CURSOR);
  
  i += p_grf->sprite_insert("hraci1.spr", FIRST_PLAYER);
  i += p_grf->sprite_insert("hraci2.spr", FIRST_PLAYER + ROT_SHIFT);
  i += p_grf->sprite_insert("hraci3.spr", FIRST_PLAYER + 2 * ROT_SHIFT);
  i += p_grf->sprite_insert("hraci4.spr", FIRST_PLAYER + 3 * ROT_SHIFT);

  // Genuine 2x artwork (Berusky 1.7). They used to be loaded in the
  // double-size mode only; now they are just assets with density 2.
  {
    i += p_grf->sprite_insert("box_bright1.spr", FIRST_BOX_BRIGHT);
    i += p_grf->sprite_insert("box_dark1.spr", FIRST_BOX_DARK);
    i += p_grf->sprite_insert("box_paper1.spr", FIRST_BOX_PAPER);
    i += p_grf->sprite_insert("box_snow1.spr", FIRST_BOX_SNOW);
    i += p_grf->sprite_insert("light_box1.spr", FIRST_LIGHT_BOX);
    i += p_grf->sprite_insert("tnt_bright1.spr", FIRST_TNT_BRIGHT);
    i += p_grf->sprite_insert("tnt_dark1.spr", FIRST_TNT_DARK);
    i += p_grf->sprite_insert("tnt_paper1.spr", FIRST_TNT_PAPER);
    i += p_grf->sprite_insert("tnt_snow1.spr", FIRST_TNT_SNOW);
    i += p_grf->sprite_insert("tnt_swamp1.spr", FIRST_TNT_SWAMP);
    i += p_grf->sprite_insert("wall_iron_blue1.spr", FIRST_WALL_IRON_BLUE);
    i += p_grf->sprite_insert("wall_iron_brown1.spr", FIRST_WALL_IRON_BROWN);
    i += p_grf->sprite_insert("wall_iron_dark1.spr", FIRST_WALL_IRON_DARK);
    i += p_grf->sprite_insert("wall_iron_gray1.spr", FIRST_WALL_IRON_GRAY);
    i += p_grf->sprite_insert("wall_machine1.spr", FIRST_WALL_MACHINE);
    i += p_grf->sprite_insert("wall_repro1.spr", FIRST_WALL_REPRO);
    i += p_grf->sprite_insert("wall_snow1.spr", FIRST_WALL_SNOW);
    i += p_grf->sprite_insert("wall_swamp1.spr", FIRST_WALL_SWAMP);
    i += p_grf->sprite_insert("wall_wood1.spr", FIRST_WALL_WOOD);
  
    i += p_grf->sprite_insert("floor_danger1.spr", FIRST_FLOOR_DANGER_SRC);
    i += p_grf->sprite_insert("floor_elevators1.spr", FIRST_FLOOR_ELEVATORS_SRC);
    i += p_grf->sprite_insert("floor_gray1.spr", FIRST_FLOOR_GRAY_SRC);
    
    i += p_grf->sprite_insert("floor_iron_1.spr", FIRST_FLOOR_IRON);
    i += p_grf->sprite_insert("floor_iron_2.spr", FIRST_FLOOR_IRON+5);
    i += p_grf->sprite_insert("floor_iron_3.spr", FIRST_FLOOR_IRON+10);
    i += p_grf->sprite_insert("floor_iron_4.spr", FIRST_FLOOR_IRON+15);
    i += p_grf->sprite_insert("floor_iron_5.spr", FIRST_FLOOR_IRON+20);
    i += p_grf->sprite_insert("floor_snow.spr",   FIRST_FLOOR_SNOW);
    
    // Generate rest of the items
    graphics_generate();
  }

  if(!i) {
    berror(_("Unable to load data, exiting..."));    
  }
  bprintf(_("%d sprites loaded..."), i);

  return(i > 0);
}

void graphics_game_free(void)
{
  p_grf->sprite_delete(FIRST_GLOBAL_LEVEL, GLOBAL_SPRITES);
  p_grf->sprite_delete(FIRST_GLOBAL_LEVEL + ROT_SHIFT, GLOBAL_SPRITES);
  p_grf->sprite_delete(FIRST_GLOBAL_LEVEL + 2 * ROT_SHIFT, GLOBAL_SPRITES);
  p_grf->sprite_delete(FIRST_GLOBAL_LEVEL + 3 * ROT_SHIFT, GLOBAL_SPRITES);

  p_grf->sprite_delete(FIRST_CLASSIC_LEVEL, CLASSIC_SPRITES);
  p_grf->sprite_delete(FIRST_CLASSIC_LEVEL + ROT_SHIFT, CLASSIC_SPRITES);
  p_grf->sprite_delete(FIRST_CLASSIC_LEVEL + 2 * ROT_SHIFT, CLASSIC_SPRITES);
  p_grf->sprite_delete(FIRST_CLASSIC_LEVEL + 3 * ROT_SHIFT, CLASSIC_SPRITES);

  p_grf->sprite_delete(FIRST_CYBER_LEVEL, CYBER_SPRITES);
  p_grf->sprite_delete(FIRST_CYBER_LEVEL + ROT_SHIFT,CYBER_SPRITES);
  p_grf->sprite_delete(FIRST_CYBER_LEVEL + 2 * ROT_SHIFT, CYBER_SPRITES);
  p_grf->sprite_delete(FIRST_CYBER_LEVEL + 3 * ROT_SHIFT, CYBER_SPRITES);
  
  p_grf->sprite_delete(FIRST_OTHER, GAME_SPRITES);
  p_grf->sprite_delete(FIRST_OTHER + ROT_SHIFT, GAME_SPRITES);

  p_grf->sprite_delete(FIRST_CURSOR, CURSOR_SPRITES);
  
  p_grf->sprite_delete(FIRST_PLAYER, PLAYER_SPRITES);
  p_grf->sprite_delete(FIRST_PLAYER + ROT_SHIFT, PLAYER_SPRITES);
  p_grf->sprite_delete(FIRST_PLAYER + 2 * ROT_SHIFT, PLAYER_SPRITES);
  p_grf->sprite_delete(FIRST_PLAYER + 3 * ROT_SHIFT, PLAYER_SPRITES);  
}

bool graphics_menu_load(DIR_LIST *p_dir)
{
  int i = 0;
  
  p_grf->graphics_dir_set(p_dir->graphics_get());

  bprintf(_("Graphics dir '%s'"),p_dir->graphics_get());
  bprintf(_("Loading menu graphics..."));

  sprite::color_key_set(COLOR_KEY_BLACK);
  i   = p_grf->sprite_insert("menu1.spr", MENU_SPRIT_ROCK);
  i  += p_grf->sprite_insert("menu2.spr", MENU_SPRIT_LOGO);
  i  += p_grf->sprite_insert("menu3.spr", MENU_SPRIT_BACK);
  i  += p_grf->sprite_insert("menu6.spr", MENU_SPRIT_WALL);
  
  sprite::color_key_set(COLOR_KEY_MENU);
  i  += p_grf->sprite_insert("menu_back1.spr", MENU_SPRIT_BACK1);
  i  += p_grf->sprite_insert("menu_back2.spr", MENU_SPRIT_BACK2);
  i  += p_grf->sprite_insert("menu_back3.spr", MENU_SPRIT_BACK3);

  sprite::color_key_set(COLOR_KEY_MENU);
  i  += p_grf->sprite_insert("menu4.spr", MENU_SPRIT_ARROWS);
  i  += p_grf->sprite_insert("controls.spr", MENU_CHECKBOX_CHECKED);
  i  += p_grf->sprite_insert("slidebar.spr", MENU_SLIDEBAR);
  i  += p_grf->sprite_insert("slider.spr", MENU_SLIDER);

  sprite::color_key_set(COLOR_KEY_GAME);
  i  += p_grf->sprite_insert("menu5.spr", MENU_SPRIT_LOGO_SMALL_1);

  sprite::color_key_set(COLOR_KEY_BLACK_FULL);
  i  += p_grf->sprite_insert("back1.spr", MENU_SPRIT_START);
  sprite::color_key_set(COLOR_KEY_BLACK);
  i  += p_grf->sprite_insert("back2.spr", MENU_SPRIT_START+1);
  i  += p_grf->sprite_insert("back3.spr", MENU_SPRIT_START+2);
  i  += p_grf->sprite_insert("back4.spr", MENU_SPRIT_START+3);
  
  sprite::color_key_set(COLOR_KEY_GAME);
  i  += p_grf->sprite_insert("mask1.spr",  EDITOR_MARK_BLACK);
  i  += p_grf->sprite_insert("mask2.spr",  EDITOR_MARK_RED);
  i  += p_grf->sprite_insert("mask3.spr",  EDITOR_MARK_YELLOW);
  
  sprite::color_key_set(COLOR_KEY_MENU);
  
  int j;
  for(j = 0; j < FONT_NUM; j++) {
    if(!p_font->load(j, FIRST_FONT + j*FONT_STEP, FONT_SPRITES))
      bprintf(_("Unable to load font %d!"),j);
  }

  if(!i) {
    berror(_("Unable to load data, exiting..."));    
  }
  bprintf(_("%d sprites loaded..."), i);

  return((bool)i);
}

void graphics_menu_free(void)
{
  p_grf->sprite_delete(MENU_SPRIT_ROCK);
  p_grf->sprite_delete(MENU_SPRIT_LOGO);
  p_grf->sprite_delete(MENU_SPRIT_BACK);

  int i;
  for(i = 0; i < FONT_NUM; i++) {
    p_font->free(i);
  }
}

int  background_num(DIR_LIST *p_dir)
{
  bprintf(_("Graphics dir '%s'"),p_dir->graphics_get());
  bprintf(_("Checking backgrounds..."));

  int j;

  for(j = 0; j < 100; j++) {
    char file[MAX_FILENAME];
    sprintf(file, BACKGROUND_NAME, j+1);
    if(!file_exists(p_dir->graphics_get(),file))
      break;
  }  

  bprintf(_("%d backgrounds..."), j);

  return(j);
}

/*
  // ####
  // ##@@         
  CHANGE_FLOOR(x,y,0);
  //   ##        
  //   @@         
  CHANGE_FLOOR(x,y,3);
  //
  // ##@@         
  CHANGE_FLOOR(x,y,2);
  // ##
  //   @@
  CHANGE_FLOOR(x,y,1);
  // 
  //   @@
  CHANGE_FLOOR(x,y,4); 
*/
// TODO -> randomize the shadow
void graphics_generate_floor(spr_handle spr, int type)
{
  // The shade is drawn in pixels of the (2x, 40x40) floor artwork
  SURFACE *p_surf = (p_grf->sprite_get(spr))->surf_get();
  tcolor color = p_surf->color_map(30, 30, 30);
  tpos   width = p_surf->pixel_width_get();
  tpos   height = p_surf->pixel_height_get();
  
  switch(type) {
    case 0:
      p_surf->blend(0, 0, 12, height, color, BLEND_SUB);
      p_surf->blend(12, 0, width-12, 14, color, BLEND_SUB);
      break;
    case 1:
      p_surf->blend(0, 0, 12, 14, color, BLEND_SUB);
      break;
    case 2:
      p_surf->blend(0, 0, 12, height, color, BLEND_SUB);
      break;
    case 3:
      p_surf->blend(0, 0, width, 14, color, BLEND_SUB);
      break;
    case 4: // no action
      break;
    default:
      break;  
  }
}

// Regenerate rest of graphics
void graphics_generate(void)
{
  // Create black sprite for blending
  p_grf->sprite_copy(SPRITE_BLACK, FIRST_CLASSIC_LEVEL+57, TRUE);
  (p_grf->sprite_get(SPRITE_BLACK))->surf_get()->alpha_mod_set(150);

  int i;

  // Generate floor graphics
  for(i = 1; i < 5; i++) {
    p_grf->sprite_copy(FIRST_FLOOR_IRON+i, FIRST_FLOOR_IRON, TRUE);
    graphics_generate_floor(FIRST_FLOOR_IRON+i, i);
  }
  graphics_generate_floor(FIRST_FLOOR_IRON, 0);

  for(i = 1; i < 5; i++) {
    p_grf->sprite_copy(FIRST_FLOOR_IRON+5+i, FIRST_FLOOR_IRON+5, TRUE);
    graphics_generate_floor(FIRST_FLOOR_IRON+5+i, i);
  }
  graphics_generate_floor(FIRST_FLOOR_IRON+5, 0);

  for(i = 1; i < 5; i++) {
    p_grf->sprite_copy(FIRST_FLOOR_IRON+10+i, FIRST_FLOOR_IRON+10, TRUE);
    graphics_generate_floor(FIRST_FLOOR_IRON+10+i, i);
  }
  graphics_generate_floor(FIRST_FLOOR_IRON+10, 0);

  for(i = 1; i < 5; i++) {
    p_grf->sprite_copy(FIRST_FLOOR_IRON+15+i, FIRST_FLOOR_IRON+15, TRUE);
    graphics_generate_floor(FIRST_FLOOR_IRON+15+i, i);
  }
  graphics_generate_floor(FIRST_FLOOR_IRON+15, 0);

  for(i = 1; i < 5; i++) {
    p_grf->sprite_copy(FIRST_FLOOR_IRON+20+i, FIRST_FLOOR_IRON+20, TRUE);
    graphics_generate_floor(FIRST_FLOOR_IRON+20+i, i);
  }
  graphics_generate_floor(FIRST_FLOOR_IRON+20, 0);

  for(i = 1; i < 5; i++) {
    p_grf->sprite_copy(FIRST_FLOOR_SNOW+i, FIRST_FLOOR_SNOW, TRUE);
    graphics_generate_floor(FIRST_FLOOR_SNOW+i, i);
  }
  graphics_generate_floor(FIRST_FLOOR_SNOW, 0);

  for(i = 0; i < 5; i++) {
    // 4 sprites
    p_grf->sprite_copy(FIRST_FLOOR_DANGER+i, FIRST_FLOOR_DANGER_SRC, TRUE);
    graphics_generate_floor(FIRST_FLOOR_DANGER+i, i);
    p_grf->sprite_copy(FIRST_FLOOR_DANGER+5+i, FIRST_FLOOR_DANGER_SRC+1, TRUE);
    graphics_generate_floor(FIRST_FLOOR_DANGER+5+i, i);
    p_grf->sprite_copy(FIRST_FLOOR_DANGER+10+i, FIRST_FLOOR_DANGER_SRC+2, TRUE);
    graphics_generate_floor(FIRST_FLOOR_DANGER+10+i, i);
    p_grf->sprite_copy(FIRST_FLOOR_DANGER+15+i, FIRST_FLOOR_DANGER_SRC+3, TRUE);
    graphics_generate_floor(FIRST_FLOOR_DANGER+15+i, i);
  
    // 3 sprites
    p_grf->sprite_copy(FIRST_FLOOR_ELEVATORS+i, FIRST_FLOOR_ELEVATORS_SRC, TRUE);
    graphics_generate_floor(FIRST_FLOOR_ELEVATORS+i, i);
    p_grf->sprite_copy(FIRST_FLOOR_ELEVATORS+5+i, FIRST_FLOOR_ELEVATORS_SRC+1, TRUE);
    graphics_generate_floor(FIRST_FLOOR_ELEVATORS+5+i, i);
    p_grf->sprite_copy(FIRST_FLOOR_ELEVATORS+10+i, FIRST_FLOOR_ELEVATORS_SRC+2, TRUE);
    graphics_generate_floor(FIRST_FLOOR_ELEVATORS+10+i, i);
  
    // 2 sprites
    p_grf->sprite_copy(FIRST_FLOOR_GRAY+i, FIRST_FLOOR_GRAY_SRC, TRUE);
    graphics_generate_floor(FIRST_FLOOR_GRAY+i, i);
    p_grf->sprite_copy(FIRST_FLOOR_GRAY+5+i, FIRST_FLOOR_GRAY_SRC+1, TRUE);
    graphics_generate_floor(FIRST_FLOOR_GRAY+5+i, i);
  }
}

// Used when the configuration template isn't shipped with the game data
static const char default_config[] =
  "# Configuration for berusky game\n"
  "\n"
  "# Graphics settings\n"
  "# fullscreen = 1 - borderless fullscreen (desktop resolution), the game is scaled\n"
  "# keeping its aspect ratio\n"
  "fullscreen = 0\n"
  "\n"
  "# Logging\n"
  "log = 0\n";

/* It creates the user data directories (see platform.h) and the default
 * configuration file there.
 */
void user_directory_create(void)
{
  dir_create(platform_user_dir());
  dir_create(user_dir_levels());
  dir_create(user_dir_profiles());

  const char *p_ini = user_file_get(INI_FILE_NAME);

  bprintfnl(_("Checking %s..."), p_ini);
  if(!file_exists(NULL, p_ini)) {
    bprintfnl(_("missing, creating it..."));

    // Prefer the template shipped with the game data
    bool ret = file_copy(INI_FILE_NAME, platform_asset_root()[0] ? platform_asset_root() : NULL,
                         p_ini, NULL, FALSE);
    if(!ret) {
      ret = file_save(NULL, p_ini, (void *)default_config, (t_off)strlen(default_config), "wb");
    }
    if(ret) {
      bprintf(_("ok"));
    } else {
      print_errno(TRUE);
      bprintf(_("failed"));
    }
  } else {
    bprintf(_("ok"));
  }
  bprintf(" ");
}
