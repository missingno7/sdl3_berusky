/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Video presentation backend (SDL3) - see video.h */

#include <stdio.h>
#include <string.h>

#include "video.h"
#include "utils.h"
#include "ini.h"

#define FRAMEBUFFER_FORMAT  SDL_PIXELFORMAT_XRGB8888

video_backend::video_backend(void)
: p_window(NULL), p_renderer(NULL), p_texture(NULL),
  logical_width(0), logical_height(0),
  fullscreen(false), texture_lost(false), repaint(false)
{
}

video_backend::~video_backend(void)
{
  destroy();
}

bool video_backend::texture_create(void)
{
  if(p_texture) {
    SDL_DestroyTexture(p_texture);
    p_texture = NULL;
  }

  p_texture = SDL_CreateTexture(p_renderer, FRAMEBUFFER_FORMAT, SDL_TEXTUREACCESS_STREAMING,
                                logical_width, logical_height);
  if(!p_texture) {
    bprintf("SDL_CreateTexture() failed: %s", SDL_GetError());
    return(false);
  }

  // Pixel-perfect by default. The smooth mode is optional.
  SDL_SetTextureScaleMode(p_texture, settings.scale_mode == SCALE_FIT_LINEAR ?
                                     SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);

  // Nothing is uploaded yet
  texture_lost = true;
  return(true);
}

// Fixed logical resolution -> any window. Aspect ratio is always preserved.
void video_backend::presentation_set(void)
{
  SDL_RendererLogicalPresentation mode = (settings.scale_mode == SCALE_INTEGER) ?
                                         SDL_LOGICAL_PRESENTATION_INTEGER_SCALE :
                                         SDL_LOGICAL_PRESENTATION_LETTERBOX;

  if(!SDL_SetRenderLogicalPresentation(p_renderer, logical_width, logical_height, mode)) {
    bprintf("SDL_SetRenderLogicalPresentation() failed: %s", SDL_GetError());
  }
}

bool video_backend::create(int width, int height, bool fullscreen_, const VIDEO_SETTINGS &settings_)
{
  settings = settings_;
  if(settings.window_scale < 1)
    settings.window_scale = 1;

  logical_width = width;
  logical_height = height;

  if(!p_window) {
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if(fullscreen_)
      flags |= SDL_WINDOW_FULLSCREEN;

    p_window = SDL_CreateWindow("Berusky", width*settings.window_scale, height*settings.window_scale, flags);
    if(!p_window) {
      berror("Unable to create the window: %s", SDL_GetError());
    }

    p_renderer = SDL_CreateRenderer(p_window, NULL);
    if(!p_renderer) {
      berror("Unable to create the renderer: %s", SDL_GetError());
    }
    bprintf("Renderer: %s", SDL_GetRendererName(p_renderer));

    fullscreen = fullscreen_;
  } else {
    // Just a different logical resolution (double-size <-> original)
    if(!fullscreen) {
      SDL_SetWindowSize(p_window, width*settings.window_scale, height*settings.window_scale);
      SDL_SetWindowPosition(p_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
  }

  SDL_SetRenderVSync(p_renderer, settings.vsync ? 1 : 0);

  presentation_set();

  if(!texture_create()) {
    berror("Unable to create the framebuffer texture: %s", SDL_GetError());
  }

  repaint = true;
  return(true);
}

void video_backend::destroy(void)
{
  if(p_texture) {
    SDL_DestroyTexture(p_texture);
    p_texture = NULL;
  }
  if(p_renderer) {
    SDL_DestroyRenderer(p_renderer);
    p_renderer = NULL;
  }
  if(p_window) {
    SDL_DestroyWindow(p_window);
    p_window = NULL;
  }
}

void video_backend::title_set(const char *p_title)
{
  if(p_window)
    SDL_SetWindowTitle(p_window, p_title);
}

bool video_backend::fullscreen_set(bool state)
{
  if(!p_window)
    return(false);

  if(!SDL_SetWindowFullscreen(p_window, state)) {
    bprintf("SDL_SetWindowFullscreen() failed: %s", SDL_GetError());
    return(false);
  }

  fullscreen = state;
  repaint = true;
  return(true);
}

void video_backend::upload(SDL_Surface *p_framebuffer, const SDL_Rect *p_rects, int num)
{
  if(!p_texture || !p_framebuffer)
    return;

  if(!p_rects || num <= 0) {
    SDL_UpdateTexture(p_texture, NULL, p_framebuffer->pixels, p_framebuffer->pitch);
  } else {
    const SDL_Rect whole = {0, 0, p_framebuffer->w, p_framebuffer->h};
    const int      bpp = SDL_BYTESPERPIXEL(p_framebuffer->format);

    for(int i = 0; i < num; i++) {
      SDL_Rect r;
      if(!SDL_GetRectIntersection(p_rects+i, &whole, &r))
        continue;

      const Uint8 *p_pixels = (const Uint8 *)p_framebuffer->pixels + r.y*p_framebuffer->pitch + r.x*bpp;
      SDL_UpdateTexture(p_texture, &r, p_pixels, p_framebuffer->pitch);
    }
  }
  repaint = true;
}

void video_backend::present(void)
{
  if(!p_renderer || !p_texture)
    return;

  // The letterbox bars must be cleared, too - the back buffer is undefined
  SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 255);
  SDL_RenderClear(p_renderer);
  SDL_RenderTexture(p_renderer, p_texture, NULL, NULL);
  SDL_RenderPresent(p_renderer);
  repaint = false;
}

SDL_Surface * video_backend::capture(void)
{
  if(!p_renderer || !p_texture)
    return(NULL);

  SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 255);
  SDL_RenderClear(p_renderer);
  SDL_RenderTexture(p_renderer, p_texture, NULL, NULL);
  SDL_Surface *p_surface = SDL_RenderReadPixels(p_renderer, NULL);
  return(p_surface);
}

void video_backend::size_set(int width, int height)
{
  if(p_window && !fullscreen) {
    SDL_SetWindowSize(p_window, width, height);
    repaint = true;
  }
}

bool video_backend::present_if_needed(SDL_Surface *p_framebuffer)
{
  if(texture_lost) {
    upload(p_framebuffer);
    texture_lost = false;
  }

  if(repaint) {
    present();
    return(true);
  }
  return(false);
}

void video_backend::event_to_logical(SDL_Event *p_event)
{
  if(p_renderer)
    SDL_ConvertEventToRenderCoordinates(p_renderer, p_event);
}

bool video_backend::window_to_logical(float window_x, float window_y, float *p_logical_x, float *p_logical_y)
{
  if(!p_renderer)
    return(false);
  return(SDL_RenderCoordinatesFromWindow(p_renderer, window_x, window_y, p_logical_x, p_logical_y));
}

// -------------------------------------------------------
//   Configuration
//
//   window_scale = 1        initial window size, multiple of the game resolution
//   scale_mode   = integer  integer | fit | smooth
//   vsync        = yes
// -------------------------------------------------------
VIDEO_SETTINGS video_settings_load(const char *p_ini_file)
{
  VIDEO_SETTINGS settings;

  settings.window_scale = ini_read_int_file(p_ini_file, "window_scale", 1);
  settings.vsync = ini_read_bool_file(p_ini_file, "vsync", TRUE) != 0;

  char mode[100];
  ini_read_string_file(p_ini_file, "scale_mode", mode, sizeof(mode), "integer");
  if(is_token(mode, "fit"))
    settings.scale_mode = SCALE_FIT_NEAREST;
  else if(is_token(mode, "smooth"))
    settings.scale_mode = SCALE_FIT_LINEAR;
  else
    settings.scale_mode = SCALE_INTEGER;

  return(settings);
}
