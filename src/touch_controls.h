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
 * the presented picture (video.h overlay) in window pixels, and a finger
 * only produces the same neutral key presses (input::key_input()) as a
 * physical keyboard would, or a click of the pointer.
 *
 * While a level is played:
 *
 *    [bug][bug][bug][bug][bug]                    [RESET][MENU]
 *
 *                  swipe anywhere to walk
 *
 * - A swipe is one step in its direction. An indicator at the place where
 *   the swipe started shows ">" (turned to the direction).
 * - A swipe with the finger kept down walks on (">>") until the finger is
 *   lifted. Moving the held finger to another direction turns the bug.
 * - Swipes made while the bug still walks are queued (a few of them), so
 *   quick swipes are not lost: a step key is held until the game took it.
 * - A tap is a click at its place - a tap on a bug in the top panel selects
 *   it (input::mouse_input()).
 *
 * Fingers are the mouse pointer outside of a level (menus). The controls
 * are shown while a level is played (the game key set is active). They are
 * enabled by default on Android / iOS, and can be switched with
 * "touch_controls = yes|no|auto" in the config file.
 */

#ifndef __TOUCH_CONTROLS_H__
#define __TOUCH_CONTROLS_H__

#include <SDL3/SDL.h>

class input;

#define TOUCH_BUTTONS     2
#define TOUCH_STEP_QUEUE  4

// What a finger event was
typedef enum {
  TOUCH_NOT_USED,   // not a level - the finger is the pointer
  TOUCH_USED,       // a control or a swipe took it
  TOUCH_TAP         // a tap at (tap_x, tap_y): click there
} TOUCH_RESULT;

typedef class touch_controls {

  typedef struct {
    SDL_FRect     rect;        // window pixels
    int           key;         // neutral key (input.h)
    int           mods;
    const char   *label;
    SDL_FingerID  finger;      // finger holding it (0 = none)
    bool          pressed;
    int           held_ticks;  // game ticks since it was pressed
    bool          release_pending; // released before the game saw it held
  } TOUCH_BUTTON;

  typedef enum {
    SWIPE_NONE,     // no finger
    SWIPE_WAIT,     // finger down, not moved far enough yet (a tap?)
    SWIPE_STEP,     // swiped - one step
    SWIPE_HOLD      // swiped and held - walks on
  } SWIPE_STATE;

  // The finger that walks with the bug
  typedef struct {
    SWIPE_STATE   state;
    SDL_FingerID  finger;
    float         start_x, start_y;   // where it went down
    float         anchor_x, anchor_y; // furthest point in the current direction
    int           dir;                // current direction (SWIPE_xxx in .cpp)
    int           ticks;              // game ticks since that direction was swiped
  } SWIPE;

  // The direction key the swipes press
  typedef struct {
    int           dir;         // -1 = none
    bool          hold;        // until the finger is lifted / until the game took the step
    unsigned int  sent;        // input::group_events_sent_get() when pressed
    int           ticks;       // game ticks since pressed
  } SWIPE_KEY;

  // What the indicator shows
  typedef struct {
    bool          shown;
    float         x, y;        // centre (window pixels)
    int           dir;
    bool          walk;        // ">>" - walks on
    int           fade;        // ticks left after the finger was lifted
  } SWIPE_INDICATOR;

  bool             enabled;
  bool             visible_now;               // shown (a level is played)
  int              out_width, out_height;     // layout is made for this window size
  TOUCH_BUTTON     button[TOUCH_BUTTONS];

  SWIPE            swipe;
  SWIPE_KEY        step_key;
  int              step_queue[TOUCH_STEP_QUEUE];
  int              step_queue_len;
  SWIPE_INDICATOR  indicator;

private:

  void layout(int width, int height);
  TOUCH_BUTTON * button_at(float x, float y);

  float swipe_distance(void);
  void  swipe_direction_start(class input *p_input, int dir, float x, float y);
  void  swipe_end(class input *p_input, bool lifted);

  void  step_key_press(class input *p_input, int dir, bool hold);
  void  step_key_release(class input *p_input);
  void  steps_advance(class input *p_input);

  void  indicator_draw(SDL_Renderer *p_renderer);

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
  // (the game key set is active). Returns true when the picture of the
  // controls changed.
  bool update(class input *p_input);

  // A finger event (window pixels). A tap gives its place in p_tap_x/y.
  TOUCH_RESULT finger_event(class input *p_input, const SDL_Event *p_event, float x, float y,
                            float *p_tap_x, float *p_tap_y);

  // Release everything (a control was pressed while the game left the level)
  void release_all(class input *p_input);

  // Draw the controls (renderer without logical presentation)
  void draw(SDL_Renderer *p_renderer);

} TOUCH_CONTROLS;

#endif // __TOUCH_CONTROLS_H__
