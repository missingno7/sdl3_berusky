/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Render settings + layout - see render_layout.h */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "render_layout.h"
#include "utils.h"
#include "ini.h"

// -------------------------------------------------------
//   Names
// -------------------------------------------------------

static const char *filter_names[] = { "nearest", "linear", "pixelart" };

const char * image_filter_name(IMAGE_FILTER filter)
{
  return(filter >= 0 && filter <= FILTER_PIXELART ? filter_names[filter] : "?");
}

bool image_filter_parse(const char *p_name, IMAGE_FILTER *p_filter)
{
  for(int i = 0; i <= FILTER_PIXELART; i++) {
    if(!SDL_strcasecmp(p_name, filter_names[i])) {
      *p_filter = (IMAGE_FILTER)i;
      return(true);
    }
  }
  return(false);
}

SDL_ScaleMode image_filter_sdl(IMAGE_FILTER filter)
{
  switch(filter) {
    case FILTER_LINEAR:
      return(SDL_SCALEMODE_LINEAR);
    case FILTER_PIXELART:
      return(SDL_SCALEMODE_PIXELART);
    default:
      return(SDL_SCALEMODE_NEAREST);
  }
}

static const char *scaler_names[SCALER_NUM] = {
  "nearest", "linear", "pixelart", "scale2x", "legacy2x", "xbrz"
};

const char * asset_scaler_name(ASSET_SCALER scaler)
{
  return(scaler >= 0 && scaler < SCALER_NUM ? scaler_names[scaler] : "?");
}

bool asset_scaler_parse(const char *p_name, ASSET_SCALER *p_scaler)
{
  for(int i = 0; i < SCALER_NUM; i++) {
    if(!SDL_strcasecmp(p_name, scaler_names[i])) {
      *p_scaler = (ASSET_SCALER)i;
      return(true);
    }
  }
  return(false);
}

ASSET_SCALER asset_scaler_effective(ASSET_SCALER scaler)
{
  // xBRZ has its place in the CPU scaler table (image_asset.cpp) but the
  // implementation is not part of the game yet
  if(scaler == SCALER_XBRZ)
    return(SCALER_PIXELART);
  return(scaler);
}

bool asset_scaler_is_cpu(ASSET_SCALER scaler)
{
  scaler = asset_scaler_effective(scaler);
  return(scaler == SCALER_SCALE2X || scaler == SCALER_LEGACY2X || scaler == SCALER_XBRZ);
}

IMAGE_FILTER asset_scaler_filter(ASSET_SCALER scaler)
{
  switch(asset_scaler_effective(scaler)) {
    case SCALER_NEAREST:
      return(FILTER_NEAREST);
    case SCALER_LINEAR:
      return(FILTER_LINEAR);
    case SCALER_PIXELART:
      return(FILTER_PIXELART);
    default:
      // CPU scalers do the magnification, the rest (a fraction) is smoothed
      return(FILTER_LINEAR);
  }
}

// -------------------------------------------------------
//   Configuration
//
//   presentation        = fit        fit | integer
//   render_resolution   = native     native | integer | <scale>, e.g. 1 or 2.5
//   asset_scaler        = pixelart   nearest | linear | pixelart | scale2x | legacy2x | xbrz
//   presentation_filter = pixelart   nearest | linear | pixelart
//   window_scale        = auto       auto | N (initial window = composition * N)
//   vsync               = yes
//   debug_overlay       = no         (F12 toggles it)
//
//   The first SDL3 port had one key for both geometry and filtering:
//   scale_mode = integer | fit | smooth. It's still read when the new keys
//   are missing (integer -> presentation integer + nearest, fit -> nearest,
//   smooth -> linear).
// -------------------------------------------------------

#define INI_PRESENTATION         "presentation"
#define INI_RENDER_RESOLUTION    "render_resolution"
#define INI_ASSET_SCALER         "asset_scaler"
#define INI_PRESENTATION_FILTER  "presentation_filter"
#define INI_WINDOW_SCALE         "window_scale"

static bool token_bool(const char *p_value, bool *p_state)
{
  if(!SDL_strcasecmp(p_value, "yes") || !SDL_strcasecmp(p_value, "on") || !strcmp(p_value, "1")) {
    *p_state = true;
    return(true);
  }
  if(!SDL_strcasecmp(p_value, "no") || !SDL_strcasecmp(p_value, "off") || !strcmp(p_value, "0")) {
    *p_state = false;
    return(true);
  }
  return(false);
}

bool render_settings_set(RENDER_SETTINGS *p_settings, const char *p_key, const char *p_value)
{
  if(!SDL_strcasecmp(p_key, INI_PRESENTATION)) {
    if(!SDL_strcasecmp(p_value, "integer"))
      p_settings->presentation = PRESENT_INTEGER;
    else if(!SDL_strcasecmp(p_value, "fit"))
      p_settings->presentation = PRESENT_FIT;
    else
      return(false);
    return(true);
  }
  if(!SDL_strcasecmp(p_key, INI_RENDER_RESOLUTION)) {
    if(!SDL_strcasecmp(p_value, "native")) {
      p_settings->render_resolution = RENDER_RES_NATIVE;
    } else if(!SDL_strcasecmp(p_value, "integer")) {
      p_settings->render_resolution = RENDER_RES_INTEGER;
    } else {
      float scale = (float)atof(p_value);
      if(scale < 0.25f || scale > 16.0f)
        return(false);
      p_settings->render_resolution = RENDER_RES_FIXED;
      p_settings->render_scale = scale;
    }
    return(true);
  }
  if(!SDL_strcasecmp(p_key, INI_ASSET_SCALER))
    return(asset_scaler_parse(p_value, &p_settings->asset_scaler));
  if(!SDL_strcasecmp(p_key, INI_PRESENTATION_FILTER))
    return(image_filter_parse(p_value, &p_settings->presentation_filter));
  if(!SDL_strcasecmp(p_key, INI_WINDOW_SCALE)) {
    p_settings->window_scale = !SDL_strcasecmp(p_value, "auto") ? 0 : SDL_max(0, atoi(p_value));
    return(true);
  }
  if(!SDL_strcasecmp(p_key, "vsync"))
    return(token_bool(p_value, &p_settings->vsync));
  if(!SDL_strcasecmp(p_key, "debug_overlay"))
    return(token_bool(p_value, &p_settings->debug_overlay));
  return(false);
}

RENDER_SETTINGS render_settings_load(const char *p_ini_file)
{
  static const char *keys[] = { INI_PRESENTATION, INI_RENDER_RESOLUTION, INI_ASSET_SCALER,
                                INI_PRESENTATION_FILTER, INI_WINDOW_SCALE, "vsync", "debug_overlay" };
  RENDER_SETTINGS settings;
  char value[100];

  // Legacy key of the first SDL3 port
  char legacy[100];
  ini_read_string_file(p_ini_file, "scale_mode", legacy, sizeof(legacy), "");
  if(legacy[0]) {
    if(is_token(legacy, "integer")) {
      settings.presentation = PRESENT_INTEGER;
      settings.asset_scaler = SCALER_NEAREST;
    } else if(is_token(legacy, "smooth")) {
      settings.asset_scaler = SCALER_LINEAR;
      settings.presentation_filter = FILTER_LINEAR;
    } else {
      settings.asset_scaler = SCALER_NEAREST;
      settings.presentation_filter = FILTER_NEAREST;
    }
  }

  for(size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
    ini_read_string_file(p_ini_file, keys[i], value, sizeof(value), "");
    if(value[0] && !render_settings_set(&settings, keys[i], value))
      bprintf("Unknown %s = %s", keys[i], value);
  }

  if(settings.asset_scaler != asset_scaler_effective(settings.asset_scaler)) {
    bprintf("asset_scaler %s is not available, using %s",
            asset_scaler_name(settings.asset_scaler),
            asset_scaler_name(asset_scaler_effective(settings.asset_scaler)));
  }

  return(settings);
}

void render_settings_save(const char *p_ini_file, const RENDER_SETTINGS &settings)
{
  ini_write_string(p_ini_file, INI_PRESENTATION,
                   settings.presentation == PRESENT_INTEGER ? "integer" : "fit");
  ini_write_string(p_ini_file, INI_ASSET_SCALER, asset_scaler_name(settings.asset_scaler));
  ini_write_string(p_ini_file, INI_PRESENTATION_FILTER, image_filter_name(settings.presentation_filter));
}

// -------------------------------------------------------
//   Layout
// -------------------------------------------------------

render_layout::render_layout(void)
{
  memset(this, 0, sizeof(*this));
  pixel_density = 1.0f;
  view_scale = render_scale = 1.0f;
}

void render_layout::compute(int window_w_, int window_h_, int output_w_, int output_h_,
                            int logical_w_, int logical_h_, const RENDER_SETTINGS &settings,
                            int max_texture)
{
  window_w = window_w_ > 0 ? window_w_ : 1;
  window_h = window_h_ > 0 ? window_h_ : 1;
  output_w = output_w_ > 0 ? output_w_ : 1;
  output_h = output_h_ > 0 ? output_h_ : 1;
  logical_w = logical_w_ > 0 ? logical_w_ : 1;
  logical_h = logical_h_ > 0 ? logical_h_ : 1;

  pixel_density = (float)output_w / (float)window_w;
  output_aspect = (float)output_w / (float)output_h;
  logical_aspect = (float)logical_w / (float)logical_h;

  // The composition in the output: never distorted, centered
  float fit = SDL_min((float)output_w / logical_w, (float)output_h / logical_h);
  float scale = fit;
  if(settings.presentation == PRESENT_INTEGER && fit >= 1.0f)
    scale = floorf(fit + 1e-4f);

  int vw = (int)lroundf(logical_w * scale);
  int vh = (int)lroundf(logical_h * scale);
  if(vw < 1) vw = 1;
  if(vh < 1) vh = 1;
  if(vw > output_w) vw = output_w;
  if(vh > output_h) vh = output_h;

  viewport.x = (output_w - vw) / 2;
  viewport.y = (output_h - vh) / 2;
  viewport.w = vw;
  viewport.h = vh;
  view_scale = scale;

  // The scene's own resolution
  switch(settings.render_resolution) {
    case RENDER_RES_INTEGER:
      render_scale = scale >= 1.0f ? floorf(scale + 1e-4f) : scale;
      break;
    case RENDER_RES_FIXED:
      render_scale = settings.render_scale;
      break;
    case RENDER_RES_NATIVE:
    default:
      render_scale = scale;
      break;
  }

  if(max_texture > 0) {
    float max_scale = SDL_min((float)max_texture / logical_w, (float)max_texture / logical_h);
    if(render_scale > max_scale)
      render_scale = max_scale;
  }

  if(settings.render_resolution == RENDER_RES_NATIVE && render_scale == scale) {
    // Exactly the viewport: presented 1:1
    target_w = vw;
    target_h = vh;
  } else {
    target_w = (int)lroundf(logical_w * render_scale);
    target_h = (int)lroundf(logical_h * render_scale);
    if(target_w < 1) target_w = 1;
    if(target_h < 1) target_h = 1;
  }
}

void render_layout::output_to_logical(float ox, float oy, float *p_lx, float *p_ly) const
{
  *p_lx = (ox - viewport.x) * logical_w / (float)viewport.w;
  *p_ly = (oy - viewport.y) * logical_h / (float)viewport.h;
}

void render_layout::logical_to_output(float lx, float ly, float *p_ox, float *p_oy) const
{
  *p_ox = viewport.x + lx * viewport.w / (float)logical_w;
  *p_oy = viewport.y + ly * viewport.h / (float)logical_h;
}

void render_layout::window_to_logical(float wx, float wy, float *p_lx, float *p_ly) const
{
  output_to_logical(wx * output_w / (float)window_w, wy * output_h / (float)window_h, p_lx, p_ly);
}

void render_layout::logical_to_window(float lx, float ly, float *p_wx, float *p_wy) const
{
  float ox, oy;
  logical_to_output(lx, ly, &ox, &oy);
  *p_wx = ox * window_w / (float)output_w;
  *p_wy = oy * window_h / (float)output_h;
}
