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
 * Video presentation backend (SDL3).
 *
 * The game renders into a CPU framebuffer (an SDL_Surface owned by graph_2d)
 * with the original software blitter. This class is the ONLY place that knows
 * how that framebuffer gets to the screen:
 *
 *   software framebuffer -> SDL_UpdateTexture -> SDL_RenderTexture
 *                        -> SDL_RenderPresent -> SDL_Window
 *
 * The framebuffer has a fixed logical resolution (640x480, or 1280x900 in
 * double-size mode). The window can have any size / DPI / aspect ratio: SDL's
 * logical presentation scales and letterboxes the picture, and converts
 * mouse / touch coordinates back to the logical (framebuffer) coordinates.
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <SDL3/SDL.h>

typedef enum {

  SCALE_INTEGER = 0,        // integer multiples only, crisp pixels (default)
  SCALE_FIT_NEAREST,        // fit the window keeping aspect, nearest filter
  SCALE_FIT_LINEAR          // fit the window keeping aspect, smooth filter

} VIDEO_SCALE_MODE;

typedef struct video_settings {

  int              window_scale;     // initial window size = logical size * window_scale
  VIDEO_SCALE_MODE scale_mode;
  bool             vsync;

  video_settings(void) : window_scale(1), scale_mode(SCALE_INTEGER), vsync(true) {}

} VIDEO_SETTINGS;

typedef class video_backend {

  SDL_Window   *p_window;
  SDL_Renderer *p_renderer;
  SDL_Texture  *p_texture;

  int           logical_width;
  int           logical_height;

  VIDEO_SETTINGS settings;

  bool          fullscreen;
  bool          texture_lost;       // texture content must be uploaded again
  bool          repaint;            // window needs to be presented again

private:

  bool texture_create(void);
  void presentation_set(void);

public:

  video_backend(void);
  ~video_backend(void);

  // Creates window + renderer + texture for the given logical resolution.
  // When the window exists it's just resized.
  bool create(int width, int height, bool fullscreen_, const VIDEO_SETTINGS &settings_);
  void destroy(void);

  bool is_created(void)
  {
    return(p_window != NULL);
  }

  // Window
  void title_set(const char *p_title);
  bool fullscreen_set(bool state);
  bool fullscreen_get(void)
  {
    return(fullscreen);
  }

  // Upload (parts of) the framebuffer to the texture. No rectangles = whole surface.
  void upload(SDL_Surface *p_framebuffer, const SDL_Rect *p_rects = NULL, int num = 0);

  // Render the texture to the window
  void present(void);

  // Call when the window content may be damaged (exposed, resized, ...).
  // The window is presented again in present_if_needed().
  void repaint_request(void)
  {
    repaint = true;
  }
  bool present_if_needed(SDL_Surface *p_framebuffer);

  // Render device was reset - the texture must be filled again
  void device_reset(void)
  {
    texture_lost = true;
    repaint = true;
  }

  // Coordinates: window (physical, event) -> logical framebuffer.
  // Works for mouse and touch events, HiDPI and letterboxing included.
  void event_to_logical(SDL_Event *p_event);
  bool window_to_logical(float window_x, float window_y, float *p_logical_x, float *p_logical_y);

  SDL_Window   * window_get(void)
  {
    return(p_window);
  }

  SDL_Renderer * renderer_get(void)
  {
    return(p_renderer);
  }

} VIDEO_BACKEND;

// Reads the optional video settings from the config file
VIDEO_SETTINGS video_settings_load(const char *p_ini_file);

#endif // __VIDEO_H__
