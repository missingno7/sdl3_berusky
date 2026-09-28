/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Video backend (SDL3) - see video.h */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "video.h"
#include "utils.h"
#include "test_script.h"

// Texture caches must forget regions of images that are released
static scene_renderer *p_active_scene = NULL;

static void region_release(IMAGE_REGION *p_region)
{
  if(p_active_scene)
    p_active_scene->region_forget(p_region);
}

video_backend::video_backend(void)
: p_window(NULL), p_renderer(NULL), p_scene(NULL),
  logical_width(0), logical_height(0), max_texture(0),
  overlay(NULL), overlay_data(NULL),
  fullscreen(false), repaint(false), scene_changed(false), replay_pending(false), textures_lost(false),
  rate_start(0), rate_frames(0), rate(0)
{
}

video_backend::~video_backend(void)
{
  destroy();
}

// Initial window size: the composition times window_scale, "auto" = the
// biggest whole multiple that fits the desktop comfortably
static int window_scale_auto(int width, int height)
{
  SDL_Rect bounds;
  SDL_DisplayID display = SDL_GetPrimaryDisplay();
  if(!display || !SDL_GetDisplayUsableBounds(display, &bounds))
    return(1);

  int scale = (int)SDL_min(bounds.w * 0.9f / width, bounds.h * 0.9f / height);
  return(scale < 1 ? 1 : scale);
}

bool video_backend::create(int width, int height, bool fullscreen_, const RENDER_SETTINGS &settings_)
{
  settings = settings_;
  logical_width = width;
  logical_height = height;

  int window_scale = settings.window_scale;
  if(window_scale <= 0)
    window_scale = test_script_active() ? 1 : window_scale_auto(width, height);

  if(!p_window) {
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if(fullscreen_)
      flags |= SDL_WINDOW_FULLSCREEN;

    p_window = SDL_CreateWindow("Berusky", width*window_scale, height*window_scale, flags);
    if(!p_window) {
      berror("Unable to create the window: %s", SDL_GetError());
    }

    p_renderer = SDL_CreateRenderer(p_window, NULL);
    if(!p_renderer) {
      berror("Unable to create the renderer: %s", SDL_GetError());
    }
    max_texture = (int)SDL_GetNumberProperty(SDL_GetRendererProperties(p_renderer),
                                             SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
    bprintf("Renderer: %s, max texture %d", SDL_GetRendererName(p_renderer), max_texture);

    scene.init(p_renderer, settings.asset_scaler);
    p_active_scene = &scene;
    image_asset::region_release_set(region_release);

    fullscreen = fullscreen_;
    rate_start = SDL_GetTicks();
  } else if(!fullscreen) {
    // A different composition (another layout profile)
    SDL_SetWindowSize(p_window, width*window_scale, height*window_scale);
    SDL_SetWindowPosition(p_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  }

  SDL_SetRenderVSync(p_renderer, settings.vsync ? 1 : 0);

  replay_pending = true;
  layout_update();
  repaint = true;
  return(true);
}

void video_backend::destroy(void)
{
  if(p_active_scene == &scene) {
    p_active_scene = NULL;
    image_asset::region_release_set(NULL);
  }
  scene.shutdown();
  if(p_renderer) {
    SDL_DestroyRenderer(p_renderer);
    p_renderer = NULL;
  }
  if(p_window) {
    SDL_DestroyWindow(p_window);
    p_window = NULL;
  }
}

void video_backend::scene_set(canvas *p_canvas)
{
  if(p_scene)
    p_scene->listener_set(NULL);
  p_scene = p_canvas;
  if(p_scene)
    p_scene->listener_set(this);
  replay_pending = true;
  repaint = true;
}

// Layout from the current output size. The scene is rendered again when the
// render target changes (window resized, moved to a HiDPI display, settings).
bool video_backend::layout_update(void)
{
  if(!p_renderer)
    return(false);

  int ww = 0, wh = 0, ow = 0, oh = 0;
  SDL_GetWindowSize(p_window, &ww, &wh);
  SDL_GetRenderOutputSize(p_renderer, &ow, &oh);
  if(ow <= 0 || oh <= 0 || ww <= 0 || wh <= 0)
    return(false);               // minimized

  layout.compute(ww, wh, ow, oh, logical_width, logical_height, settings, max_texture);

  if(textures_lost) {
    scene.textures_drop();
    scene.target_drop();         // the target itself is gone too
    textures_lost = false;
    replay_pending = true;
  }

  if(scene.target_set(layout.target_w, layout.target_h, layout.render_scale))
    replay_pending = true;

  if(replay_pending) {
    scene.replay(p_scene);
    replay_pending = false;
    return(true);
  }
  return(false);
}

void video_backend::canvas_op(const DRAW_OP &op)
{
  if(!scene.target_get())
    layout_update();
  scene.draw(op);
  scene_changed = true;
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
  SDL_SyncWindow(p_window);

  fullscreen = state;
  repaint = true;
  return(true);
}

void video_backend::size_set(int width, int height)
{
  if(p_window && !fullscreen) {
    SDL_SetWindowSize(p_window, width, height);
    SDL_SyncWindow(p_window);
    repaint = true;
  }
}

void video_backend::settings_set(const RENDER_SETTINGS &settings_)
{
  const bool scaler_changed = settings.asset_scaler != settings_.asset_scaler;
  settings = settings_;

  if(p_renderer)
    SDL_SetRenderVSync(p_renderer, settings.vsync ? 1 : 0);
  if(scaler_changed)
    scene.scaler_set(settings.asset_scaler);

  replay_pending = true;
  repaint = true;
}

void video_backend::debug_overlay_toggle(void)
{
  settings.debug_overlay = !settings.debug_overlay;
  repaint = true;
}

// Everything into the back buffer
void video_backend::render(void)
{
  SDL_SetRenderTarget(p_renderer, NULL);

  // The bars around the viewport must be cleared, the back buffer is undefined
  SDL_SetRenderDrawBlendMode(p_renderer, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 255);
  SDL_RenderClear(p_renderer);

  SDL_Texture *p_target = scene.target_get();
  if(p_target) {
    const SDL_FRect viewport = { (float)layout.viewport.x, (float)layout.viewport.y,
                                 (float)layout.viewport.w, (float)layout.viewport.h };
    // Only used when the render target isn't the viewport's size
    SDL_SetTextureScaleMode(p_target, image_filter_sdl(settings.presentation_filter));
    SDL_RenderTexture(p_renderer, p_target, NULL, &viewport);
  }

  // Overlays are laid out in output pixels
  if(overlay)
    overlay(p_renderer, overlay_data);

  if(settings.debug_overlay)
    debug_draw();
}

void video_backend::present(void)
{
  if(!p_renderer)
    return;

  layout_update();
  render();
  SDL_RenderPresent(p_renderer);
  repaint = false;
  scene_changed = false;

  rate_frames++;
  Uint64 now = SDL_GetTicks();
  if(now - rate_start >= 1000) {
    rate = rate_frames * 1000.0f / (float)(now - rate_start);
    rate_frames = 0;
    rate_start = now;
    if(settings.debug_overlay)
      repaint = true;
  }
}

bool video_backend::present_if_needed(void)
{
  if(!p_renderer)
    return(false);

  // Resized / moved to another display without an event we saw
  int ow = 0, oh = 0;
  SDL_GetRenderOutputSize(p_renderer, &ow, &oh);
  if(ow != layout.output_w || oh != layout.output_h || textures_lost || replay_pending)
    repaint = true;

  if(repaint) {
    present();
    return(true);
  }
  return(false);
}

SDL_Surface * video_backend::capture(void)
{
  if(!p_renderer)
    return(NULL);

  layout_update();
  render();
  return(SDL_RenderReadPixels(p_renderer, NULL));
}

SDL_Surface * video_backend::capture_scene(void)
{
  if(!p_renderer)
    return(NULL);

  layout_update();
  return(scene.target_read());
}

bool video_backend::scene_dump(const char *p_file)
{
  if(!p_scene)
    return(false);

  FILE *f = fopen(p_file, "w");
  if(!f)
    return(false);
  p_scene->dump(f);
  fclose(f);
  return(true);
}

// -------------------------------------------------------
//   Diagnostics overlay (debug_overlay = yes, F12)
// -------------------------------------------------------

void video_backend::debug_draw(void)
{
  // Which native density is drawn for the visible scene
  int by_density[5] = { 0, 0, 0, 0, 0 };
  int images = 0, fills = 0;
  if(p_scene) {
    for(size_t i = 0; i < p_scene->op_count(); i++) {
      const DRAW_OP &op = p_scene->op_get(i);
      if(op.type != OP_IMAGE) {
        fills++;
        continue;
      }
      image_asset *p_asset = op.p_region->p_asset;
      int d = (int)lroundf(p_asset->variant_get(p_asset->variant_select(layout.render_scale))->density);
      by_density[SDL_clamp(d, 1, 5) - 1]++;
      images++;
    }
  }

  static const char *res_names[] = { "native", "integer", "fixed" };
  const SCENE_STATS &stats = scene.stats_get();
  char lines[8][160];
  int  n = 0;

  snprintf(lines[n++], 160, "window %dx%d  output %dx%d  density %.2f",
           layout.window_w, layout.window_h, layout.output_w, layout.output_h, layout.pixel_density);
  snprintf(lines[n++], 160, "composition %dx%d (%.3f)  output aspect %.3f",
           layout.logical_w, layout.logical_h, layout.logical_aspect, layout.output_aspect);
  snprintf(lines[n++], 160, "viewport %d,%d %dx%d  view scale %.3f  %s",
           layout.viewport.x, layout.viewport.y, layout.viewport.w, layout.viewport.h, layout.view_scale,
           settings.presentation == PRESENT_INTEGER ? "integer" : "fit");
  snprintf(lines[n++], 160, "render target %dx%d  render scale %.3f (%s)",
           layout.target_w, layout.target_h, layout.render_scale, res_names[settings.render_resolution]);
  snprintf(lines[n++], 160, "asset scaler %s  presentation filter %s",
           asset_scaler_name(asset_scaler_effective(settings.asset_scaler)),
           image_filter_name(settings.presentation_filter));
  snprintf(lines[n++], 160, "scene %d images (1x %d, 2x %d, 3x %d, 4x %d, >4x %d), %d fills",
           images, by_density[0], by_density[1], by_density[2], by_density[3], by_density[4], fills);
  snprintf(lines[n++], 160, "textures %d  draws %d  cpu scaled %d  replays %d",
           stats.textures, stats.draws, stats.cpu_scaled, stats.replays);
  snprintf(lines[n++], 160, "present %.1f/s  renderer %s", rate, SDL_GetRendererName(p_renderer));

  const float text_scale = layout.pixel_density >= 1.5f ? floorf(layout.pixel_density) : 1.0f;
  float width = 0;
  for(int i = 0; i < n; i++)
    width = SDL_max(width, (float)strlen(lines[i]) * 8);

  SDL_SetRenderScale(p_renderer, text_scale, text_scale);
  SDL_SetRenderDrawBlendMode(p_renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 180);
  const SDL_FRect back = { 0, 0, width + 8, n * 10.0f + 6 };
  SDL_RenderFillRect(p_renderer, &back);
  SDL_SetRenderDrawColor(p_renderer, 120, 255, 120, 255);
  for(int i = 0; i < n; i++)
    SDL_RenderDebugText(p_renderer, 4, 4 + i * 10.0f, lines[i]);
  SDL_SetRenderScale(p_renderer, 1.0f, 1.0f);

  // The composition's outline
  SDL_SetRenderDrawColor(p_renderer, 255, 60, 60, 255);
  const SDL_FRect vp = { (float)layout.viewport.x, (float)layout.viewport.y,
                         (float)layout.viewport.w, (float)layout.viewport.h };
  SDL_RenderRect(p_renderer, &vp);
}

// -------------------------------------------------------
//   Coordinates
// -------------------------------------------------------

void video_backend::event_to_logical(SDL_Event *p_event)
{
  if(!layout.valid())
    return;

  float x, y;
  switch(p_event->type) {
    case SDL_EVENT_MOUSE_MOTION:
      {
        layout.window_to_logical(p_event->motion.x, p_event->motion.y, &x, &y);
        const float units = layout.pixel_density / layout.view_scale;
        p_event->motion.x = x;
        p_event->motion.y = y;
        p_event->motion.xrel *= units;
        p_event->motion.yrel *= units;
      }
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      layout.window_to_logical(p_event->button.x, p_event->button.y, &x, &y);
      p_event->button.x = x;
      p_event->button.y = y;
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      layout.window_to_logical(p_event->wheel.mouse_x, p_event->wheel.mouse_y, &x, &y);
      p_event->wheel.mouse_x = x;
      p_event->wheel.mouse_y = y;
      break;
    default:
      break;
  }
}

bool video_backend::window_to_logical(float window_x, float window_y, float *p_logical_x, float *p_logical_y)
{
  if(!layout.valid())
    return(false);
  layout.window_to_logical(window_x, window_y, p_logical_x, p_logical_y);
  return(true);
}

bool video_backend::logical_to_window(float logical_x, float logical_y, float *p_window_x, float *p_window_y)
{
  if(!layout.valid())
    return(false);
  layout.logical_to_window(logical_x, logical_y, p_window_x, p_window_y);
  return(true);
}

void video_backend::window_size(int *p_width, int *p_height)
{
  *p_width = *p_height = 0;
  if(p_window)
    SDL_GetWindowSize(p_window, p_width, p_height);
}

float video_backend::pixel_density(void)
{
  int ww = 0, wh = 0, ow = 0, oh = 0;
  if(!p_window || !p_renderer)
    return(1.0f);
  SDL_GetWindowSize(p_window, &ww, &wh);
  SDL_GetRenderOutputSize(p_renderer, &ow, &oh);
  return(ww > 0 ? (float)ow / (float)ww : 1.0f);
}
