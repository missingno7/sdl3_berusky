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
 * Video backend (SDL3): window, renderer, presentation.
 *
 *   game (logical units) -> screen canvas (scene.h, retained display list)
 *        -> scene_renderer: GPU compositing into a render target at the
 *           render scale derived from the output (render_layout.h)
 *        -> video_backend: the render target is placed into the window's
 *           content viewport (aspect ratio kept, black bars elsewhere),
 *           touch controls and the diagnostics overlay are drawn over it in
 *           output pixels, SDL_RenderPresent
 *
 * This class owns SDL_Window / SDL_Renderer, fullscreen, HiDPI, the output
 * size and the window <-> logical coordinate conversion. It knows nothing
 * about the game's layout; the game only tells it the composition size.
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <SDL3/SDL.h>

#include "render_layout.h"
#include "scene.h"

// Something drawn over the game picture in output pixels (touch controls).
typedef void (*VIDEO_OVERLAY)(SDL_Renderer *p_renderer, void *p_data);

typedef class video_backend : public canvas_listener {

  SDL_Window     *p_window;
  SDL_Renderer   *p_renderer;

  RENDER_SETTINGS settings;
  RENDER_LAYOUT   layout;
  scene_renderer  scene;
  canvas         *p_scene;            // the screen canvas

  int             logical_width;      // the composition
  int             logical_height;
  int             max_texture;

  VIDEO_OVERLAY   overlay;
  void           *overlay_data;

  bool            fullscreen;
  bool            repaint;            // the window has to be presented again
  bool            scene_changed;      // something was drawn since the last present
  bool            replay_pending;     // the render target must be drawn again
  bool            textures_lost;      // the render device was reset

  // present rate for the diagnostics overlay
  Uint64          rate_start;
  int             rate_frames;
  float           rate;

private:

  bool layout_update(void);
  void render(void);
  void debug_draw(void);

public:

  video_backend(void);
  ~video_backend(void);

  // Creates window + renderer for a composition of the given logical size.
  // When the window exists only the composition changes.
  bool create(int width, int height, bool fullscreen_, const RENDER_SETTINGS &settings_);
  void destroy(void);

  bool is_created(void)
  {
    return(p_window != NULL);
  }

  // The canvas that is shown in the window (the game's screen)
  void scene_set(canvas *p_canvas);

  // canvas_listener: a new operation of the screen canvas
  virtual void canvas_op(const DRAW_OP &op);

  // Window
  void title_set(const char *p_title);
  bool fullscreen_set(bool state);
  void size_set(int width, int height);
  bool fullscreen_get(void)
  {
    return(fullscreen);
  }

  // Render settings can change at run time (settings menu, tests)
  const RENDER_SETTINGS & settings_get(void)
  {
    return(settings);
  }
  void settings_set(const RENDER_SETTINGS &settings_);
  void debug_overlay_toggle(void);

  const RENDER_LAYOUT & layout_get(void)
  {
    return(layout);
  }

  // Shows the scene in the window
  void present(void);
  // Presents when something was drawn, the window was damaged or resized
  bool present_if_needed(void);
  bool scene_changed_get(void)
  {
    return(scene_changed);
  }

  void overlay_set(VIDEO_OVERLAY overlay_, void *p_data)
  {
    overlay = overlay_;
    overlay_data = p_data;
  }

  // What is in the window right now (output pixels, bars and overlays
  // included) and the scene at the render resolution. Used by tests; the
  // caller destroys the surface.
  SDL_Surface * capture(void);
  SDL_Surface * capture_scene(void);
  // The screen canvas as text (logical units)
  bool scene_dump(const char *p_file);

  // Call when the window content may be damaged (exposed, resized, ...)
  void repaint_request(void)
  {
    repaint = true;
  }

  // SDL_EVENT_RENDER_DEVICE_RESET: every texture is gone
  void device_reset(void)
  {
    textures_lost = true;
    repaint = true;
  }
  // SDL_EVENT_RENDER_TARGETS_RESET: the render target lost its content
  void targets_reset(void)
  {
    replay_pending = true;
    repaint = true;
  }

  // Coordinates: window (events) -> logical (the composition).
  // Works for mouse and touch, HiDPI and letterboxing included.
  void event_to_logical(SDL_Event *p_event);
  bool window_to_logical(float window_x, float window_y, float *p_logical_x, float *p_logical_y);
  bool logical_to_window(float logical_x, float logical_y, float *p_window_x, float *p_window_y);

  // Output pixels per one window coordinate (HiDPI)
  float pixel_density(void);
  void  window_size(int *p_width, int *p_height);

  SDL_Window * window_get(void)
  {
    return(p_window);
  }

  SDL_Renderer * renderer_get(void)
  {
    return(p_renderer);
  }

} VIDEO_BACKEND;

#endif // __VIDEO_H__
