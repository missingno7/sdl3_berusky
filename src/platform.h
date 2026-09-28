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
 * Platform layer.
 *
 * Everything the game needs from the operating system that is not graphics
 * or input lives here, so that Windows / Linux / macOS / Android differ only
 * in this one place:
 *
 *   - read-only game assets   (Graphics, GameData, Levels, ...)
 *   - user-writable data      (configuration, profiles, user levels, log)
 *   - error / message dialogs
 *   - starting another process (editor <-> game)
 *
 * The game never assumes that assets are files next to the executable or in
 * the current working directory.  Asset paths are always built from
 * platform_asset_root().  On Android that root is empty and SDL_IOFromFile()
 * resolves relative names against the APK assets.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stddef.h>

/* Initialize paths. argv0 may be NULL (e.g. on Android). */
bool         platform_init(const char *argv0);
void         platform_shutdown(void);

/* Read-only data root. Either "" (relative to packaged assets) or a directory
 * ending with '/'. */
const char * platform_asset_root(void);

/* User-writable application data directory, always ends with '/'.
 * Created on demand (SDL_GetPrefPath). */
const char * platform_user_dir(void);

/* Full path to the running executable (used by the editor to run the game
 * and the game to run the editor). May be empty on platforms that can't
 * launch another process. */
const char * platform_executable(void);

/* Home directory expansion for '~' - returns false when there is no home */
bool         platform_home_dir(char *p_dir, size_t max);

/* Directories */
bool         platform_dir_create(const char *p_dir);
bool         platform_dir_exists(const char *p_dir);

/* Modal message. Uses SDL message box, falls back to stderr. */
void         platform_message(bool error, const char *p_title, const char *p_text);

/* Run a program and wait for it. p_args is a NULL terminated argv array.
 * Returns false when the program can't be started. */
// Runs a program and waits for it. idle() is called while waiting, so the
// caller's window keeps responding (repaints, the OS doesn't flag it as hung).
typedef void (*PLATFORM_IDLE)(void);
bool         platform_run_and_wait(const char * const *p_args, PLATFORM_IDLE idle = NULL);
bool         platform_can_run_processes(void);

#endif // __PLATFORM_H__
