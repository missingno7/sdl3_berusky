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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "portability.h"

#define  LOG_ENABLED 1

// Maximal length of a path (assets, user data, ...)
#define  MAX_FILENAME 1024

#ifndef  FALSE
#define  FALSE (1!=1)
#endif

#ifndef  TRUE
#define  TRUE  (1==1)
#endif

#ifndef  ERROR
#define  ERROR (-1)
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define BOOL_UNDEFINED            (-1)
#define ERROR                     (-1)

// Integers are carried inside void* event parameters. Go through intptr_t so
// the conversion is well defined (and lossless) on 32 and 64 bit targets.
#define POINTER_TO_INT(pointer) ((int)(intptr_t)(pointer))
#define INT_TO_POINTER(integer) (reinterpret_cast<void *>((intptr_t)(integer)))

// -------------------------------------------------------
// file interfaces
// -------------------------------------------------------
#define NO_DIR (NULL)

// A file handle. It's a small copyable handle (like FILE *), the file is
// released by file_close().
//
//  - files opened for reading ("r", "rb") are loaded into memory at once
//    (SDL_LoadFile), so they work the same for regular files and for
//    packaged Android assets, and reading is trivially fast,
//  - files opened for writing ("w", "wb", "a") are written through an
//    SDL_IOStream.
//
// The file_* functions below mimic the stdio ones the game used before.
struct file_impl;

typedef class fhandle {

public:

  struct file_impl *f;

public:

  fhandle(void) : f(NULL) {};
  fhandle(struct file_impl *f_in) : f(f_in) {};

  operator bool(void) const
  {
    return(f != NULL);
  }

} FHANDLE;

typedef unsigned int t_off;

char *    return_path(const char *p_dir, const char *p_file, char *p_buffer, int max_lenght);

FHANDLE   file_open(const char *p_dir, const char *p_file, const char *p_mode, bool safe = TRUE);
void      file_close(FHANDLE f);

// stdio-like operations on FHANDLE
char *    file_gets(char *p_buffer, int max_lenght, FHANDLE f);      // fgets()
size_t    file_read(void *p_buffer, size_t bytes, FHANDLE f);        // fread(buf, 1, bytes, f)
size_t    file_write(const void *p_buffer, size_t bytes, FHANDLE f); // fwrite(buf, 1, bytes, f)
int       file_printf(FHANDLE f, const char *p_format, ...);         // fprintf()
bool      file_seek(FHANDLE f, long offset);                         // fseek(f, offset, SEEK_SET)
void      file_rewind(FHANDLE f);
long      file_tell(FHANDLE f);
bool      file_eof(FHANDLE f);
void      file_flush(FHANDLE f);

int    		file_load(const char *p_dir, const char *p_file, char *p_mem, t_off max_lenght, t_off start_address = 0, bool safe = TRUE);
int    		file_load_text(const char * p_dir, const char * p_file, char * p_mem, t_off max_lenght, t_off start_address = 0, bool safe = TRUE);

int    	  file_load(FHANDLE f, char * p_mem, t_off max_lenght, t_off start_address = 0);
int       file_load_text(FHANDLE f, char * p_mem, t_off max_lenght, t_off start_address = 0);

void *    file_load(const char *p_dir, const char *p_file, t_off *p_lenght, t_off start_address = 0, bool safe = TRUE);

bool      file_save(const char *p_dir, const char *p_file, void *p_buffer, t_off lenght, const char *p_mode = "wb");

bool      file_exists(const char *p_dir, const char *p_file);

int       file_size_get(const char * p_dir, const char * p_file);
int       file_size_get(FHANDLE f);

bool      file_copy(const char *p_src, const char *p_src_dir, const char *p_dest, const char *p_dest_dir, bool safe = TRUE);

void      print_errno(bool new_line = FALSE);

bool      dir_create(const char *p_dir);
char   *  dir_home_get(char *p_dir, int max);

// User (writable) data - in the platform user directory (platform.h).
// The returned strings are valid until the next call of the same function.
#define USER_LEVELS_DIR    "User"
#define USER_PROFILES_DIR  "Profiles"

const char * user_file_get(const char *p_name);
const char * user_dir_levels(void);
const char * user_dir_profiles(void);

// -------------------------------------------------------
// log file management
// -------------------------------------------------------

extern FHANDLE log_file;

void log_open(const char *p_file);
void log_open_ini(const char *p_ini_file);
void log_close(void);
void log_flush(void);

// -------------------------------------------------------
// memory allocation
// -------------------------------------------------------

inline void * mmalloc(int size)
{
   void *p_tmp = malloc(size);

   if(!p_tmp) {
     fprintf(stderr,"Out of memory! file: %s line: %d\n",__FILE__,__LINE__);
     assert(0);
     exit(0);
   } else {
     memset(p_tmp,0,size);
     return(p_tmp);
   }
}

inline void * mmemcpy(void *p_src, int size)
{
  void *p_tmp = mmalloc(size);
  memcpy(p_tmp, p_src, size);
  return(p_tmp);
}

inline void * rrealloc(void *p_mem, int size)
{
   void *p_tmp = realloc(p_mem,size);

   if(!p_tmp) {
     fprintf(stderr,"Out of memory! file: %s line: %d\n",__FILE__,__LINE__);
     assert(0);
     exit(0);
   } else {
     return(p_tmp);
   }
}

inline void xfree(void **p_mem)
{
   if(p_mem && *p_mem) {
     free(*p_mem);
     *p_mem = NULL;     
   }
}

#define ffree(ptr) { if(ptr) { free(ptr); ptr = NULL; }}

// -------------------------------------------------------
// logging - helper function
// -------------------------------------------------------
#ifdef  LOG_ENABLED

// Fatal error: reported to the log and to the user (message box), then exit.
void berror_message(const char *p_text);

inline void berror(const char *p_text,...)
{
  char      text[2000];
  va_list   arguments;

  va_start(arguments,p_text);
  vsnprintf(text,2000,p_text,arguments);
  va_end(arguments);

  berror_message(text);
}

inline void bprintf(const char *p_text,...)
{
  char      text[2000];
  va_list   arguments;  

  va_start(arguments,p_text);
  vsnprintf(text,2000,p_text,arguments);
  va_end(arguments);

  fprintf(stderr,"%s\n",text);

  if(log_file) {
    file_printf(log_file,"%s\n",text);
  }
}

inline void bprintfnl(const char *p_text,...)
{
  char      text[2000];
  va_list   arguments;  

  va_start(arguments,p_text);
  vsnprintf(text,2000,p_text,arguments);
  va_end(arguments);

  fprintf(stderr,"%s",text);

  if(log_file) {
    file_printf(log_file,"%s",text);
  }
}

inline int fgets_correction(char * p_kor)
{
  int delka = strlen(p_kor);
  if (p_kor[delka - 1] == '\n') {
    p_kor[delka - 1] = 0;
    return (delka - 1);
  }
  else {
    return (delka);
  }
}

#else

#define berror(p_text,...)
#define bprintf(p_text,...)
#define fgets_correction(p_kor)

#endif

// -------------------------------------------------------
// configuration management
// -------------------------------------------------------

typedef class dir_list {

  char levels[MAX_FILENAME];        // read-only: bundled levels
  char levels_user[MAX_FILENAME];   // writable:  user levels
  char gamedata[MAX_FILENAME];      // read-only: game data
  char graphics[MAX_FILENAME];      // read-only: sprites
  char tmp[MAX_FILENAME];           // writable:  temporary files (editor)
  char game_binary[MAX_FILENAME];   // this executable (editor <-> game)

public:
  
  dir_list(void)
  {
    levels[0] = '\0';
    levels_user[0] = '\0';
    gamedata[0] = '\0';
    graphics[0] = '\0';
    tmp[0] = '\0';
    game_binary[0] = '\0';
  }

  ~dir_list(void) {};

public:

  // Directories come from platform.h (assets / user data), the
  // configuration file may override them.
  void load(const char *p_ini);

  char * levels_get(void)
  {
    return(levels);
  }

  char * levels_user_get(void)
  {
    return(levels_user);
  }

  char * gamedata_get(void)
  {
    return(gamedata);
  }

  char * graphics_get(void)
  {
    return(graphics);
  }

  char * tmp_get(void)
  {
    return(tmp);
  }

  char * game_binary_get(void)
  {
    return(game_binary);
  }

} DIR_LIST;

bool get_fullscreen(const char *p_ini_file);
bool set_fullscreen(const char *p_ini_file, bool state);

bool get_doublesize(const char *p_ini_file);
bool set_doublesize(const char *p_ini_file, bool state);

bool get_doublesize_question(const char *p_ini_file);
bool set_doublesize_question(const char *p_ini_file, bool state);

int  get_colors(const char *p_ini_file, int default_color_depth);

// -------------------------------------------------------
// the rest
// -------------------------------------------------------

inline char * get_tail(char * p_str)
{
  char *p_act = p_str, *p_last = NULL;
  while((p_act = strchr(p_act, '.'))) {
    p_last = p_act;
    p_act++;
  }  
  return (p_last ? p_str : NULL);
}

inline char * change_tail(char * p_str, const char * p_end)
{
  char *p_act = p_str, *p_last = NULL;
  while((p_act = strchr(p_act, '.'))) {
    p_last = p_act;
    p_act++;
  }  
  if (p_last)
    *p_last = 0;
  return (p_end ? (char *) strcat(p_str, p_end) : p_str);
}

char * my_itoa(int base, char *buf, int d);

typedef struct _DIRECTORY_ENTRY {

  char name[MAX_FILENAME];

} DIRECTORY_ENTRY;

int file_list_get(const char *p_dir, const char *p_mask, DIRECTORY_ENTRY **p_list);

void  user_directory_create(void);

// Wall clock in seconds (level time). A test script replaces it by the game
// ticks, so the pictures don't depend on how fast the machine is.
time_t game_clock(void);

// The configuration file (in the user data directory)
const char *config_file(bool configure = FALSE);

/* Data loading
*/
bool  graphics_game_load(DIR_LIST *p_dir);
void  graphics_game_free(void);

bool  graphics_menu_load(DIR_LIST *p_dir);
void  graphics_menu_free(void);

bool  graphics_logos_load(DIR_LIST *p_dir);
void  graphics_logos_free(void);

void  graphics_generate(void);

#endif // __UTILS_H__
