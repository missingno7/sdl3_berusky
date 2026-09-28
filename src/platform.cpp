/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Platform layer implementation - see platform.h */

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

#define PLATFORM_PATH_MAX   1024

#ifndef BERUSKY_ORGANIZATION
#define BERUSKY_ORGANIZATION "Anakreon"
#endif
#ifndef BERUSKY_APPLICATION
#define BERUSKY_APPLICATION  "Berusky"
#endif

static char asset_root[PLATFORM_PATH_MAX] = "";
static char user_dir[PLATFORM_PATH_MAX] = "";
static char executable[PLATFORM_PATH_MAX] = "";

/* We use '/' internally on all platforms. Windows accepts it, SDL accepts it. */
static void path_normalize(char *p_path)
{
  for(; *p_path; p_path++) {
    if(*p_path == '\\')
      *p_path = '/';
  }
}

static void path_terminate(char *p_path, size_t max)
{
  size_t len = strlen(p_path);
  if(len > 0 && p_path[len-1] != '/' && len+1 < max) {
    p_path[len] = '/';
    p_path[len+1] = '\0';
  }
}

/* A directory is a valid asset root when it contains the game data list */
static bool asset_root_valid(const char *p_root)
{
  char probe[PLATFORM_PATH_MAX];
  SDL_PathInfo info;

  snprintf(probe, sizeof(probe), "%sGameData/items.dat", p_root);
  return(SDL_GetPathInfo(probe, &info) && info.type == SDL_PATHTYPE_FILE);
}

static bool asset_root_try(const char *p_base, const char *p_relative)
{
  char tmp[PLATFORM_PATH_MAX];

  snprintf(tmp, sizeof(tmp), "%s%s", p_base ? p_base : "", p_relative);
  path_normalize(tmp);
  path_terminate(tmp, sizeof(tmp));

  if(asset_root_valid(tmp)) {
    snprintf(asset_root, sizeof(asset_root), "%s", tmp);
    return(true);
  }
  return(false);
}

static void asset_root_find(void)
{
#ifdef SDL_PLATFORM_ANDROID
  /* Packaged assets - SDL_IOFromFile() reads relative names from the APK */
  asset_root[0] = '\0';
  return;
#else
  const char *p_env = SDL_getenv("BERUSKY_DATA");
  if(p_env && p_env[0] && asset_root_try("", p_env))
    return;

  const char *p_base = SDL_GetBasePath();
  if(p_base) {
    static const char *relative[] = {
      "data",                 // <exe>/data          (portable / Windows layout)
      "../data",              // <build>/../data     (build dir inside the source tree)
      "../../data",
      "../share/berusky",     // <prefix>/bin -> <prefix>/share/berusky (installed)
      "../Resources",         // macOS bundle
      "."
    };
    for(size_t i = 0; i < sizeof(relative)/sizeof(relative[0]); i++) {
      if(asset_root_try(p_base, relative[i]))
        return;
    }
  }

#ifdef BERUSKY_DATA_DIR
  /* Compile-time location (source tree data dir or install location) */
  if(asset_root_try("", BERUSKY_DATA_DIR))
    return;
#endif

  /* Nothing found - leave it empty, loading will fail with a readable error */
  asset_root[0] = '\0';
#endif
}

bool platform_init(const char *argv0)
{
  asset_root_find();

  // BERUSKY_USER_DIR: portable / test mode - user data in the given directory
  const char *p_user_env = SDL_getenv("BERUSKY_USER_DIR");
  char *p_pref = NULL;
  if(p_user_env && p_user_env[0]) {
    SDL_CreateDirectory(p_user_env);
    p_pref = SDL_strdup(p_user_env);
  } else {
    p_pref = SDL_GetPrefPath(BERUSKY_ORGANIZATION, BERUSKY_APPLICATION);
  }
  if(p_pref) {
    snprintf(user_dir, sizeof(user_dir), "%s", p_pref);
    SDL_free(p_pref);
    path_normalize(user_dir);
    path_terminate(user_dir, sizeof(user_dir));
  } else {
    /* No writable location. Configuration falls back to the current dir. */
    snprintf(user_dir, sizeof(user_dir), "./");
  }

  /* Executable path */
  executable[0] = '\0';
#ifndef SDL_PLATFORM_ANDROID
  if(argv0 && argv0[0]) {
    snprintf(executable, sizeof(executable), "%s", argv0);
    path_normalize(executable);

    /* Bare command name (started through PATH) - look next to us */
    if(!strchr(executable, '/')) {
      const char *p_base = SDL_GetBasePath();
      if(p_base) {
        char tmp[PLATFORM_PATH_MAX];
        snprintf(tmp, sizeof(tmp), "%s%s", p_base, executable);
        path_normalize(tmp);
        SDL_PathInfo info;
        if(SDL_GetPathInfo(tmp, &info))
          snprintf(executable, sizeof(executable), "%s", tmp);
        else {
          snprintf(tmp, sizeof(tmp), "%s%s.exe", p_base, executable);
          path_normalize(tmp);
          if(SDL_GetPathInfo(tmp, &info))
            snprintf(executable, sizeof(executable), "%s", tmp);
        }
      }
    }
  }
#endif
  return(true);
}

void platform_shutdown(void)
{
}

const char * platform_asset_root(void)
{
  return(asset_root);
}

const char * platform_user_dir(void)
{
  return(user_dir);
}

const char * platform_executable(void)
{
  return(executable);
}

bool platform_home_dir(char *p_dir, size_t max)
{
  const char *p_home = SDL_GetUserFolder(SDL_FOLDER_HOME);
  if(!p_home) {
    if(max)
      p_dir[0] = '\0';
    return(false);
  }
  snprintf(p_dir, max, "%s", p_home);
  path_normalize(p_dir);

  /* SDL returns the folder with a trailing separator, we add ours in return_path() */
  size_t len = strlen(p_dir);
  if(len > 1 && p_dir[len-1] == '/')
    p_dir[len-1] = '\0';
  return(true);
}

bool platform_dir_create(const char *p_dir)
{
  return(SDL_CreateDirectory(p_dir));
}

bool platform_dir_exists(const char *p_dir)
{
  SDL_PathInfo info;
  return(SDL_GetPathInfo(p_dir, &info) && info.type == SDL_PATHTYPE_DIRECTORY);
}

void platform_message(bool error, const char *p_title, const char *p_text)
{
  fprintf(stderr, "%s: %s\n", p_title, p_text);

  if(!SDL_ShowSimpleMessageBox(error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION,
                               p_title, p_text, NULL)) {
    /* No dialog available (headless?) - stderr above has to be enough */
  }
}

bool platform_can_run_processes(void)
{
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
  return(false);
#else
  return(executable[0] != '\0');
#endif
}

bool platform_run_and_wait(const char * const *p_args, PLATFORM_IDLE idle)
{
  if(!platform_can_run_processes())
    return(false);

  // A regression test gives the started program its own script
  // (BERUSKY_TEST_CHILD_SCRIPT), not the one this process is replaying
  SDL_Environment *p_env = SDL_CreateEnvironment(true);
  const char *p_child_script = SDL_getenv("BERUSKY_TEST_CHILD_SCRIPT");
  if(p_env && SDL_getenv("BERUSKY_TEST_SCRIPT")) {
    if(p_child_script && p_child_script[0])
      SDL_SetEnvironmentVariable(p_env, "BERUSKY_TEST_SCRIPT", p_child_script, true);
    else
      SDL_UnsetEnvironmentVariable(p_env, "BERUSKY_TEST_SCRIPT");
    SDL_UnsetEnvironmentVariable(p_env, "BERUSKY_TEST_CHILD_SCRIPT");
  }

  SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)p_args);
  if(p_env)
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, p_env);
  SDL_Process *p_process = SDL_CreateProcessWithProperties(props);
  SDL_DestroyProperties(props);
  if(p_env)
    SDL_DestroyEnvironment(p_env);
  if(!p_process)
    return(false);

  int exit_code = 0;
  while(!SDL_WaitProcess(p_process, false, &exit_code)) {
    if(idle)
      idle();
    SDL_Delay(20);
  }
  SDL_DestroyProcess(p_process);
  return(true);
}
