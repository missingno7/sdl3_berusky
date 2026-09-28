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
 * On-screen touch controls.
 *
 * They live completely outside the game logic: the controls are drawn over
 * the presented picture (video.h overlay) in window pixels, and a finger on
 * them only produces the same neutral key presses (input::key_input()) as a
 * physical keyboard would. Fingers outside of the controls are turned into
 * the ordinary pointer events in logical game coordinates, so the existing
 * clickable menus work by touch.
 *
 *   [1][2][3][4][5]                              [RESET][MENU]
 *
 *
 *        [^]                                         [NEXT]
 *     [<]   [>]
 *        [v]
 *
 * The controls are shown while a level is played (the game key set is
 * active). They are enabled by default on Android / iOS, and can be switched
 * with "touch_controls = yes|no|auto" in the config file.
 */

#ifndef __TOUCH_CONTROLS_H__
#define __TOUCH_CONTROLS_H__

#include <SDL3/SDL.h>

class input;

#define TOUCH_BUTTONS  12

typedef class touch_controls {

  typedef struct {
    SDL_FRect     rect;        // window pixels
    int           key;         // neutral key (input.h)
    int           mods;
    const char   *label;
    SDL_FingerID  finger;      // finger holding it (0 = none)
    bool          pressed;
  } TOUCH_BUTTON;

  bool          enabled;
  bool          visible_now;               // shown (a level is played)
  int           out_width, out_height;     // layout is made for this window size
  TOUCH_BUTTON  button[TOUCH_BUTTONS];

private:

  void layout(int width, int height);
  TOUCH_BUTTON * button_at(float x, float y);

public:

  touch_controls(void);

  void init(bool enable);

  bool enabled_get(void)
  {
    return(enabled);
  }

  bool visible(void)
  {
    return(visible_now);
  }

  // Call once per game tick: controls are visible while a level is played
  // (the game key set is active)
  void update(class input *p_input);

  // A finger event (window pixels). Returns true when a control took it.
  bool finger_event(class input *p_input, const SDL_Event *p_event, float x, float y);

  // Release everything (a control was pressed while the game left the level)
  void release_all(class input *p_input);

  // Draw the controls (renderer without logical presentation)
  void draw(SDL_Renderer *p_renderer);

} TOUCH_CONTROLS;

#endif // __TOUCH_CONTROLS_H__
