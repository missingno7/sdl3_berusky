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
 * Render settings and the render layout.
 *
 * The game draws in LOGICAL units: a composition (640x480 units for the game,
 * 1280x900 units for the editor) in which one level cell is 20 units (40 in
 * the editor). Nothing in the game knows how big the window is.
 *
 * RENDER_LAYOUT is the one place that decides how that composition maps to the
 * output (the window's pixels):
 *
 *   window coordinates  (events; SDL window size)
 *        x pixel_density
 *   output pixels       (SDL_GetRenderOutputSize; HiDPI aware)
 *        viewport = the composition fitted into the output, aspect ratio kept,
 *                   centered (black bars on 16:9, ultrawide, portrait...)
 *   view_scale          output pixels per logical unit
 *   render_scale        render target pixels per logical unit - the scene is
 *                       RENDERED at this resolution (not upscaled from 640x480)
 *
 * With the default render_resolution = native the render target is exactly
 * the viewport: the scene is drawn 1:1 at the output resolution and every
 * asset is scaled once, from its own native resolution.
 */

#ifndef __RENDER_LAYOUT_H__
#define __RENDER_LAYOUT_H__

#include <SDL3/SDL.h>

// How the composition is fitted into the window
typedef enum {

  PRESENT_FIT = 0,          // largest size keeping the aspect ratio
  PRESENT_INTEGER           // largest whole multiple (pixel-art purists)

} PRESENTATION_MODE;

// Resolution the scene is rendered at
typedef enum {

  RENDER_RES_NATIVE = 0,    // the viewport's own resolution (default)
  RENDER_RES_INTEGER,       // whole multiple of the composition, then filtered to the viewport
  RENDER_RES_FIXED          // render_scale units -> pixels (1 = classic 640x480)

} RENDER_RESOLUTION;

// GPU sampling filter (SDL_ScaleMode)
typedef enum {

  FILTER_NEAREST = 0,
  FILTER_LINEAR,
  FILTER_PIXELART

} IMAGE_FILTER;

// How a (low resolution) asset is scaled to the render resolution.
// The first three are sampling filters done by the GPU while compositing,
// the others are CPU algorithms that make a cached, pre-scaled copy of the
// asset (see image_asset.h) which is then drawn with linear filtering.
typedef enum {

  SCALER_NEAREST = 0,
  SCALER_LINEAR,
  SCALER_PIXELART,
  SCALER_SCALE2X,           // EPX / AdvMAME2x / 3x (CPU)
  SCALER_LEGACY2X,          // the interpolating 2x enlarger of Berusky 1.7 double size (CPU)
  SCALER_XBRZ,              // reserved - not built in yet, falls back to pixelart

  SCALER_NUM

} ASSET_SCALER;

const char *   image_filter_name(IMAGE_FILTER filter);
bool           image_filter_parse(const char *p_name, IMAGE_FILTER *p_filter);
SDL_ScaleMode  image_filter_sdl(IMAGE_FILTER filter);

const char *   asset_scaler_name(ASSET_SCALER scaler);
bool           asset_scaler_parse(const char *p_name, ASSET_SCALER *p_scaler);
// The scaler that is really used (unavailable ones fall back)
ASSET_SCALER   asset_scaler_effective(ASSET_SCALER scaler);
bool           asset_scaler_is_cpu(ASSET_SCALER scaler);
// Filter used when an asset is drawn with the given scaler
IMAGE_FILTER   asset_scaler_filter(ASSET_SCALER scaler);

typedef struct render_settings {

  int                window_scale;         // initial window = composition * window_scale, 0 = auto
  PRESENTATION_MODE  presentation;
  RENDER_RESOLUTION  render_resolution;
  float              render_scale;         // for RENDER_RES_FIXED
  ASSET_SCALER       asset_scaler;
  IMAGE_FILTER       presentation_filter;  // render target -> viewport (only matters when they differ)
  bool               vsync;
  bool               debug_overlay;

  render_settings(void)
  : window_scale(0), presentation(PRESENT_FIT), render_resolution(RENDER_RES_NATIVE),
    render_scale(1.0f), asset_scaler(SCALER_PIXELART), presentation_filter(FILTER_PIXELART),
    vsync(true), debug_overlay(false)
  {
  }

} RENDER_SETTINGS;

// Reads the settings from the config file (see render_layout.cpp for keys and
// the migration of the older scale_mode key).
RENDER_SETTINGS render_settings_load(const char *p_ini_file);
// Writes the settings the user can change in the game menu
void            render_settings_save(const char *p_ini_file, const RENDER_SETTINGS &settings);

typedef struct render_layout {

  // Inputs
  int       window_w, window_h;       // window coordinates (what events use)
  int       output_w, output_h;       // renderer output in pixels
  int       logical_w, logical_h;     // the composition, logical units

  // Results
  float     pixel_density;            // output pixels per window coordinate
  float     output_aspect;
  float     logical_aspect;
  SDL_Rect  viewport;                 // the composition in output pixels
  float     view_scale;               // output pixels per logical unit
  float     render_scale;             // render target pixels per logical unit
  int       target_w, target_h;       // render target size

  render_layout(void);

  // max_texture: biggest texture the renderer can make (0 = unknown)
  void compute(int window_w_, int window_h_, int output_w_, int output_h_,
               int logical_w_, int logical_h_, const RENDER_SETTINGS &settings,
               int max_texture = 0);

  bool valid(void) const
  {
    return(target_w > 0 && target_h > 0);
  }

  // Window coordinates <-> logical units (mouse, touch). Points outside the
  // viewport give coordinates outside the composition.
  void window_to_logical(float wx, float wy, float *p_lx, float *p_ly) const;
  void logical_to_window(float lx, float ly, float *p_wx, float *p_wy) const;

  // Output pixels <-> logical units
  void output_to_logical(float ox, float oy, float *p_lx, float *p_ly) const;
  void logical_to_output(float lx, float ly, float *p_ox, float *p_oy) const;

} RENDER_LAYOUT;

#endif // __RENDER_LAYOUT_H__
