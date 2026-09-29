/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Scripted input + screenshots for regression tests - see test_script.h */

#include "berusky.h"
#include "test_script.h"

#define SCRIPT_MAX_LEN  100000

static char   *p_script = NULL;      // whole script
static size_t  script_pos = 0;
static bool    script_loaded = false;
static bool    script_enabled = false;

static int     wait_ticks = 0;
static long    tick_count = 0;

// Delayed release of pressed keys / mouse button
static KEYTYPE release_key = K_NONE;
static int     release_key_mods = 0;
static int     release_key_ticks = 0;
static bool    release_button = false;
static int     release_button_ticks = 0;
static tpos    release_x = 0, release_y = 0;

static void script_load(void)
{
  script_loaded = true;

  const char *p_file = SDL_getenv("BERUSKY_TEST_SCRIPT");
  if(!p_file || !p_file[0])
    return;

  size_t size = 0;
  p_script = (char *)SDL_LoadFile(p_file, &size);
  if(!p_script) {
    bprintf("Test script %s: %s", p_file, SDL_GetError());
    return;
  }

  script_enabled = true;
  bprintf("Test script %s (%d bytes)", p_file, (int)size);
}

bool test_script_active(void)
{
  if(!script_loaded)
    script_load();
  return(script_enabled);
}

static KEYTYPE key_by_name(const char *p_name)
{
  static const struct { const char *name; KEYTYPE key; } names[] = {
    {"up", K_UP}, {"down", K_DOWN}, {"left", K_LEFT}, {"right", K_RIGHT},
    {"tab", K_TAB}, {"enter", K_ENTER}, {"esc", K_ESC}, {"space", K_SPACE},
    {"bksp", K_BKSP}, {"del", K_DEL},
    {"f1", K_F1}, {"f2", K_F2}, {"f3", K_F3}, {"f4", K_F4}, {"f5", K_F5}, {"f6", K_F6},
    {"f7", K_F7}, {"f8", K_F8}, {"f9", K_F9}, {"f10", K_F10}, {"f11", K_F11}, {"f12", K_F12}
  };

  for(size_t i = 0; i < sizeof(names)/sizeof(names[0]); i++) {
    if(!SDL_strcasecmp(p_name, names[i].name))
      return(names[i].key);
  }

  // single letter or digit
  if(p_name[0] && !p_name[1] && SDL_isalnum(p_name[0]))
    return((KEYTYPE)SDL_tolower(p_name[0]));

  bprintf("Test script: unknown key '%s'", p_name);
  return(K_NONE);
}

static void out_path(const char *p_file, char *p_path, size_t max)
{
  const char *p_out = SDL_getenv("BERUSKY_TEST_OUT");
  if(p_out && p_out[0])
    snprintf(p_path, max, "%s/%s", p_out, p_file);
  else
    snprintf(p_path, max, "%s", p_file);
}

// The screen's display list: layout regression independent of the resolution
static void layout_screenshot(const char *p_file)
{
  if(!p_grf)
    return;

  char path[MAX_FILENAME];
  out_path(p_file, path, sizeof(path));
  if(!p_grf->video_get()->scene_dump(path))
    bprintf("Test script: unable to save %s", path);
  else
    bprintf("Test script: saved %s", path);
}

// What the audio layer was asked to play (sound ids, music tracks, game
// ticks) - the audio regression test without a sound device
static void audio_log_save(const char *p_file)
{
  char path[MAX_FILENAME];
  out_path(p_file, path, sizeof(path));

  const std::string &log = audio.log_get();
  SDL_IOStream *p_io = SDL_IOFromFile(path, "wb");
  if(!p_io || SDL_WriteIO(p_io, log.data(), log.size()) != log.size())
    bprintf("Test script: unable to save %s", path);
  else
    bprintf("Test script: saved %s", path);
  if(p_io)
    SDL_CloseIO(p_io);
}

static void settings_set(const char *p_key, const char *p_value)
{
  if(!p_grf)
    return;

  RENDER_SETTINGS settings = p_grf->video_get()->settings_get();
  if(render_settings_set(&settings, p_key, p_value))
    p_grf->video_get()->settings_set(settings);
  else
    bprintf("Test script: unknown setting %s = %s", p_key, p_value);
}

static void screenshot(const char *p_file)
{
  if(!p_grf)
    return;

  char path[MAX_FILENAME];
  const char *p_out = SDL_getenv("BERUSKY_TEST_OUT");
  if(p_out && p_out[0])
    snprintf(path, sizeof(path), "%s/%s", p_out, p_file);
  else
    snprintf(path, sizeof(path), "%s", p_file);

  // The scene at its render resolution
  SDL_Surface *p_surface = p_grf->video_get()->capture_scene();
  if(!p_surface || !SDL_SaveBMP(p_surface, path))
    bprintf("Test script: unable to save %s: %s", path, SDL_GetError());
  else
    bprintf("Test script: saved %s (%dx%d)", path, p_surface->w, p_surface->h);
  SDL_DestroySurface(p_surface);
}

// A finger event as a touch screen would send it. Position is logical game
// coordinates, or 0..1000 of the window when window_units is set.
static void finger_push(bool window_units, const char *p_action, int id, float x, float y)
{
  if(!p_grf)
    return;

  VIDEO_BACKEND *p_video = p_grf->video_get();
  int   ww, wh;
  float wx, wy;

  p_video->window_size(&ww, &wh);
  if(window_units) {
    wx = x / 1000.0f * ww;
    wy = y / 1000.0f * wh;
  } else if(!p_video->logical_to_window(x, y, &wx, &wy)) {
    return;
  }

  SDL_Event event;
  SDL_zero(event);
  if(!SDL_strcasecmp(p_action, "down"))
    event.type = SDL_EVENT_FINGER_DOWN;
  else if(!SDL_strcasecmp(p_action, "up"))
    event.type = SDL_EVENT_FINGER_UP;
  else
    event.type = SDL_EVENT_FINGER_MOTION;

  event.tfinger.touchID = TEST_TOUCH_ID;
  event.tfinger.fingerID = id;
  event.tfinger.x = wx / ww;
  event.tfinger.y = wy / wh;
  event.tfinger.pressure = 1.0f;
  event.tfinger.timestamp = SDL_GetTicksNS();
  event.tfinger.windowID = SDL_GetWindowID(p_video->window_get());
  SDL_PushEvent(&event);
}

// What is really in the window: the presentation (scaling, letterbox)
static void window_screenshot(const char *p_file)
{
  if(!p_grf)
    return;

  char path[MAX_FILENAME];
  const char *p_out = SDL_getenv("BERUSKY_TEST_OUT");
  if(p_out && p_out[0])
    snprintf(path, sizeof(path), "%s/%s", p_out, p_file);
  else
    snprintf(path, sizeof(path), "%s", p_file);

  SDL_Surface *p_surface = p_grf->video_get()->capture();
  if(!p_surface) {
    bprintf("Test script: window capture failed: %s", SDL_GetError());
    return;
  }
  SDL_SaveBMP(p_surface, path);
  bprintf("Test script: saved %s (%dx%d)", path, p_surface->w, p_surface->h);
  SDL_DestroySurface(p_surface);
}

// Read the next line of the script. Returns false at the end.
static bool line_next(char *p_line, size_t max)
{
  while(p_script[script_pos]) {
    size_t i = 0;
    while(p_script[script_pos] && p_script[script_pos] != '\n') {
      if(i+1 < max && p_script[script_pos] != '\r')
        p_line[i++] = p_script[script_pos];
      script_pos++;
    }
    if(p_script[script_pos] == '\n')
      script_pos++;
    p_line[i] = '\0';

    // skip empty lines and comments
    char *p_start = p_line;
    while(*p_start == ' ' || *p_start == '\t')
      p_start++;
    if(*p_start && *p_start != '#') {
      if(p_start != p_line)
        memmove(p_line, p_start, strlen(p_start)+1);
      return(true);
    }
  }
  return(false);
}

long test_script_ticks(void)
{
  return(tick_count);
}

bool test_script_poll(class input *p_input_)
{
  if(!test_script_active())
    return(false);

  tick_count++;

  INPUT *p_input = (INPUT *)p_input_;

  // Pending releases
  if(release_key != K_NONE && --release_key_ticks <= 0) {
    p_input->key_input(release_key, 0, false);
    release_key = K_NONE;
  }
  if(release_button && --release_button_ticks <= 0) {
    p_input->mouse_input(release_x, release_y, BUTTON_UP, BUTTON_LEFT);
    release_button = false;
  }

  if(wait_ticks > 0) {
    wait_ticks--;
    return(false);
  }

  char line[500];
  while(line_next(line, sizeof(line))) {
    char cmd[100] = "", arg1[200] = "", arg2[100] = "", arg3[100] = "";
    int n = sscanf(line, "%99s %199s %99s %99s", cmd, arg1, arg2, arg3);
    if(n < 1)
      continue;

    if(!SDL_strcasecmp(cmd, "wait")) {
      wait_ticks = atoi(arg1);
      if(wait_ticks > 0) {
        wait_ticks--;
        return(false);
      }
    }
    else if(!SDL_strcasecmp(cmd, "key") || !SDL_strcasecmp(cmd, "keydown") || !SDL_strcasecmp(cmd, "keyup")) {
      KEYTYPE key = key_by_name(arg1);
      int mods = 0;
      for(int i = 2; i <= n; i++) {
        const char *p_mod = (i == 2) ? arg2 : arg3;
        if(!SDL_strcasecmp(p_mod, "shift"))
          mods |= K_SHIFT_MASK;
        else if(!SDL_strcasecmp(p_mod, "ctrl"))
          mods |= K_CTRL_MASK;
      }
      if(key != K_NONE) {
        if(!SDL_strcasecmp(cmd, "keyup")) {
          p_input->key_input(key, mods, false);
        } else {
          p_input->key_input(key, mods, true);
          if(!SDL_strcasecmp(cmd, "key")) {
            release_key = key;
            release_key_mods = mods;
            release_key_ticks = 2;
            wait_ticks = 2;
            return(false);
          }
        }
      }
    }
    else if(!SDL_strcasecmp(cmd, "move")) {
      p_input->mouse_input(atoi(arg1), atoi(arg2), BUTTON_NONE, 0);
    }
    else if(!SDL_strcasecmp(cmd, "click")) {
      tpos x = atoi(arg1), y = atoi(arg2);
      p_input->mouse_input(x, y, BUTTON_NONE, 0);
      p_input->mouse_input(x, y, BUTTON_DOWN, BUTTON_LEFT);
      release_button = true;
      release_button_ticks = 2;
      release_x = x;
      release_y = y;
      wait_ticks = 2;
      return(false);
    }
    else if(!SDL_strcasecmp(cmd, "shot")) {
      screenshot(arg1);
    }
    else if(!SDL_strcasecmp(cmd, "touch") || !SDL_strcasecmp(cmd, "touchw")) {
      char  action[32] = "";
      int   id = 0;
      float x = 0, y = 0;
      sscanf(line, "%*s %31s %d %f %f", action, &id, &x, &y);
      finger_push(!SDL_strcasecmp(cmd, "touchw"), action, id, x, y);
    }
    else if(!SDL_strcasecmp(cmd, "windowshot")) {
      window_screenshot(arg1);
    }
    else if(!SDL_strcasecmp(cmd, "layoutshot")) {
      layout_screenshot(arg1);
    }
    else if(!SDL_strcasecmp(cmd, "audiolog")) {
      audio_log_save(arg1);
    }
    else if(!SDL_strcasecmp(cmd, "bench")) {
      if(p_grf) {
        const RENDER_LAYOUT &layout = p_grf->video_get()->layout_get();
        float ms = p_grf->video_get()->benchmark(atoi(arg1));
        bprintf("Test script: bench %s: %dx%d scene, %d operations: %.2f ms per full render + present",
                p_grf->video_get()->renderer_name(), layout.target_w, layout.target_h,
                (int)p_grf->screen_surface_get()->canvas_peek()->op_count(), ms);
      }
    }
    else if(!SDL_strcasecmp(cmd, "set")) {
      settings_set(arg1, arg2);
      // replayed and presented on the next poll
    }
    else if(!SDL_strcasecmp(cmd, "window")) {
      if(p_grf)
        p_grf->video_get()->size_set(atoi(arg1), atoi(arg2));
      wait_ticks = 5;
      return(false);
    }
    else if(!SDL_strcasecmp(cmd, "fullscreen")) {
      if(p_grf && (p_grf->fullscreen_get() != (atoi(arg1) != 0)))
        p_grf->fullscreen_toggle();
      wait_ticks = 15;
      return(false);
    }
    else if(!SDL_strcasecmp(cmd, "quit")) {
      return(true);
    }
    else {
      bprintf("Test script: unknown command '%s'", cmd);
    }
  }

  // end of the script
  return(true);
}
