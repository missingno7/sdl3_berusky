/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* On-screen touch controls - see touch_controls.h */

#include "berusky.h"
#include "touch_controls.h"

// A tap holds its key at least this many game ticks
#define TOUCH_MIN_TICKS      2

// A finger kept down this long after a swipe walks on (the game runs at 30
// ticks per second, a step takes 9 ticks)
#define SWIPE_HOLD_TICKS     8

// A step key the game doesn't take in this time is let go (the level ended...)
#define SWIPE_STEP_TIMEOUT   45

// The indicator fades out in this many ticks after the finger was lifted
#define SWIPE_FADE_TICKS     8

// indexes into button[]
enum {
  TB_MENU, TB_RESTART
};

// swipe directions
enum {
  SWIPE_UP, SWIPE_DOWN, SWIPE_LEFT, SWIPE_RIGHT
};

static const int   swipe_key[4] = { K_UP, K_DOWN, K_LEFT, K_RIGHT };
static const float swipe_dx[4]  = { 0, 0, -1, 1 };
static const float swipe_dy[4]  = { -1, 1, 0, 0 };

touch_controls::touch_controls(void)
: enabled(false), visible_now(false), out_width(0), out_height(0), step_queue_len(0)
{
  memset(button, 0, sizeof(button));
  memset(&swipe, 0, sizeof(swipe));
  memset(&step_key, 0, sizeof(step_key));
  memset(&indicator, 0, sizeof(indicator));
  step_key.dir = -1;

  static const struct { int key; int mods; const char *label; } defs[TOUCH_BUTTONS] = {
    { K_ESC,   0,            "MENU"  },
    { K_R,     K_CTRL_MASK,  "RESET" },
  };

  for(int i = 0; i < TOUCH_BUTTONS; i++) {
    button[i].key = defs[i].key;
    button[i].mods = defs[i].mods;
    button[i].label = defs[i].label;
  }
}

void touch_controls::init(bool enable)
{
  enabled = enable;
  visible_now = false;
}

// -------------------------------------------------------
//   The direction key of the swipes
// -------------------------------------------------------

void touch_controls::step_key_press(class input *p_input, int dir, bool hold)
{
  INPUT *p_in = (INPUT *)p_input;

  step_key_release(p_input);
  step_key.dir = dir;
  step_key.hold = hold;
  step_key.sent = p_in->group_events_sent_get();
  step_key.ticks = 0;
  p_in->key_input(swipe_key[dir], 0, true);
}

void touch_controls::step_key_release(class input *p_input)
{
  if(step_key.dir >= 0) {
    ((INPUT *)p_input)->key_input(swipe_key[step_key.dir], 0, false);
    step_key.dir = -1;
  }
}

// A step key is held until the game took the move (the bug may still walk
// when it's swiped), then the next queued step goes
void touch_controls::steps_advance(class input *p_input)
{
  INPUT *p_in = (INPUT *)p_input;

  if(step_key.dir >= 0 && !step_key.hold) {
    if(p_in->group_events_sent_get() == step_key.sent && step_key.ticks < SWIPE_STEP_TIMEOUT)
      return;
    step_key_release(p_input);
  }

  if(step_key.dir < 0 && step_queue_len > 0) {
    int dir = step_queue[0];
    step_queue_len--;
    memmove(step_queue, step_queue + 1, step_queue_len * sizeof(step_queue[0]));
    step_key_press(p_input, dir, false);
  }
}

// -------------------------------------------------------
//   Swipes
// -------------------------------------------------------

// How far a finger has to move to make a swipe
float touch_controls::swipe_distance(void)
{
  if(!out_width || !out_height)
    return(40.0f);
  const float shorter = (float)(out_width < out_height ? out_width : out_height);
  return(SDL_max(shorter * 0.06f, 16.0f));
}

// The finger moved far enough in a new direction
void touch_controls::swipe_direction_start(class input *p_input, int dir, float x, float y)
{
  // the indicator is where this direction started
  indicator.shown = true;
  indicator.x = swipe.anchor_x;
  indicator.y = swipe.anchor_y;
  indicator.dir = dir;
  indicator.fade = 0;

  swipe.dir = dir;
  swipe.ticks = 0;
  swipe.anchor_x = x;
  swipe.anchor_y = y;

  if(swipe.state == SWIPE_HOLD) {
    // walks on, the other way
    step_key_press(p_input, dir, true);
    indicator.walk = true;
  } else {
    swipe.state = SWIPE_STEP;
    indicator.walk = false;
    if(step_queue_len < TOUCH_STEP_QUEUE)
      step_queue[step_queue_len++] = dir;
    steps_advance(p_input);
  }
}

void touch_controls::swipe_end(class input *p_input, bool lifted)
{
  if(swipe.state == SWIPE_HOLD && step_key.hold) {
    // Not a single step in the new direction yet - make it one
    if(((INPUT *)p_input)->group_events_sent_get() == step_key.sent)
      step_key.hold = false;
    else
      step_key_release(p_input);
  }

  if(indicator.shown && swipe.state != SWIPE_WAIT)
    indicator.fade = lifted ? SWIPE_FADE_TICKS : 0;
  if(!indicator.fade)
    indicator.shown = false;

  swipe.state = SWIPE_NONE;
  swipe.finger = 0;
}

// -------------------------------------------------------
//   Game tick
// -------------------------------------------------------

bool touch_controls::update(class input *p_input)
{
  bool now = enabled && ((INPUT *)p_input)->keyset_get() == &game_keys;
  bool changed = (now != visible_now);

  // The game left the level while a finger was on a control
  if(visible_now && !now)
    release_all(p_input);

  // A quick tap still holds the key for a game tick (movement keys are
  // read as held, a press + release between two ticks would be lost)
  for(int i = 0; i < TOUCH_BUTTONS; i++) {
    TOUCH_BUTTON *p_b = button + i;
    if(!p_b->pressed)
      continue;
    p_b->held_ticks++;
    if(p_b->release_pending && p_b->held_ticks >= TOUCH_MIN_TICKS) {
      ((INPUT *)p_input)->key_input(p_b->key, 0, false);
      p_b->pressed = false;
      p_b->release_pending = false;
      changed = true;
    }
  }

  if(now) {
    if(step_key.dir >= 0)
      step_key.ticks++;
    steps_advance(p_input);

    if(swipe.state == SWIPE_STEP || swipe.state == SWIPE_HOLD)
      swipe.ticks++;

    // The finger stays down after the swipe: walk on, once the swiped
    // steps were taken
    if(swipe.state == SWIPE_STEP && swipe.ticks >= SWIPE_HOLD_TICKS &&
       step_key.dir < 0 && step_queue_len == 0) {
      swipe.state = SWIPE_HOLD;
      step_key_press(p_input, swipe.dir, true);
      indicator.walk = true;
      changed = true;
    }

    if(indicator.fade > 0) {
      if(--indicator.fade == 0)
        indicator.shown = false;
      changed = true;
    }
  }

  visible_now = now;
  return(changed);
}

void touch_controls::release_all(class input *p_input)
{
  for(int i = 0; i < TOUCH_BUTTONS; i++) {
    if(button[i].pressed) {
      ((INPUT *)p_input)->key_input(button[i].key, 0, false);
      button[i].pressed = false;
      button[i].finger = 0;
      button[i].release_pending = false;
    }
  }

  step_key_release(p_input);
  step_queue_len = 0;
  swipe.state = SWIPE_NONE;
  swipe.finger = 0;
  indicator.shown = false;
  indicator.fade = 0;
}

// -------------------------------------------------------
//   Layout
// -------------------------------------------------------

// The buttons are in the top-right corner of the window (not in the game
// picture) - on a wide phone screen in the free space next to the game.
// All the rest of the screen is for swipes.
void touch_controls::layout(int width, int height)
{
  out_width = width;
  out_height = height;

  const float shorter = (float)(width < height ? width : height);
  const float u = shorter * 0.16f;          // button size
  const float m = shorter * 0.03f;          // margin

  button[TB_MENU].rect    = { width - m - u, m, u, u };
  button[TB_RESTART].rect = { width - m - 2.2f*u, m, 1.1f*u, u };
}

touch_controls::TOUCH_BUTTON * touch_controls::button_at(float x, float y)
{
  for(int i = 0; i < TOUCH_BUTTONS; i++) {
    SDL_FPoint pt = { x, y };
    if(SDL_PointInRectFloat(&pt, &button[i].rect))
      return(button + i);
  }
  return(NULL);
}

// -------------------------------------------------------
//   Fingers
// -------------------------------------------------------

TOUCH_RESULT touch_controls::finger_event(class input *p_input, const SDL_Event *p_event,
                                          float x, float y, float *p_tap_x, float *p_tap_y)
{
  if(!visible_now)
    return(TOUCH_NOT_USED);

  INPUT *p_in = (INPUT *)p_input;
  const SDL_FingerID id = p_event->tfinger.fingerID;

  switch(p_event->type) {
    case SDL_EVENT_FINGER_DOWN:
      {
        TOUCH_BUTTON *p_button = button_at(x, y);
        if(p_button) {
          if(!p_button->pressed) {
            p_button->pressed = true;
            p_button->finger = id;
            p_button->held_ticks = 0;
            p_button->release_pending = false;
            p_in->key_input(p_button->key, p_button->mods, true);
          }
          return(TOUCH_USED);
        }

        // One finger walks, the others are ignored
        if(swipe.state == SWIPE_NONE) {
          swipe.state = SWIPE_WAIT;
          swipe.finger = id;
          swipe.start_x = swipe.anchor_x = x;
          swipe.start_y = swipe.anchor_y = y;
          swipe.ticks = 0;
        }
        return(TOUCH_USED);
      }

    case SDL_EVENT_FINGER_MOTION:
      if(swipe.state != SWIPE_NONE && swipe.finger == id) {
        const float dx = x - swipe.anchor_x;
        const float dy = y - swipe.anchor_y;

        // Further in the current direction - it's still the same swipe
        if(swipe.state != SWIPE_WAIT &&
           dx*swipe_dx[swipe.dir] + dy*swipe_dy[swipe.dir] > 0) {
          swipe.anchor_x = x;
          swipe.anchor_y = y;
          return(TOUCH_USED);
        }

        const float dist = swipe_distance();
        if(dx*dx + dy*dy >= dist*dist) {
          int dir;
          if(SDL_fabsf(dx) >= SDL_fabsf(dy))
            dir = dx < 0 ? SWIPE_LEFT : SWIPE_RIGHT;
          else
            dir = dy < 0 ? SWIPE_UP : SWIPE_DOWN;
          if(swipe.state == SWIPE_WAIT || dir != swipe.dir)
            swipe_direction_start(p_input, dir, x, y);
        }
      }
      return(TOUCH_USED);

    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
      for(int i = 0; i < TOUCH_BUTTONS; i++) {
        if(button[i].pressed && button[i].finger == id && !button[i].release_pending) {
          button[i].finger = 0;
          if(button[i].held_ticks < TOUCH_MIN_TICKS) {
            button[i].release_pending = true;     // update() releases it
          } else {
            button[i].pressed = false;
            p_in->key_input(button[i].key, 0, false);
          }
          return(TOUCH_USED);
        }
      }

      if(swipe.state != SWIPE_NONE && swipe.finger == id) {
        const bool tap = (swipe.state == SWIPE_WAIT && p_event->type == SDL_EVENT_FINGER_UP);
        const float tx = swipe.start_x, ty = swipe.start_y;
        swipe_end(p_input, p_event->type == SDL_EVENT_FINGER_UP);
        if(tap) {
          *p_tap_x = tx;
          *p_tap_y = ty;
          return(TOUCH_TAP);
        }
      }
      return(TOUCH_USED);

    default:
      return(TOUCH_NOT_USED);
  }
}

// -------------------------------------------------------
//   Drawing
// -------------------------------------------------------

static void geometry_color(SDL_Vertex *p_v, int num, Uint8 r, Uint8 g, Uint8 b, float alpha)
{
  for(int i = 0; i < num; i++)
    p_v[i].color = { r / 255.0f, g / 255.0f, b / 255.0f, alpha };
}

// A filled disc
static void disc_draw(SDL_Renderer *p_renderer, float cx, float cy, float radius,
                      Uint8 r, Uint8 g, Uint8 b, float alpha)
{
  #define DISC_SEGMENTS 40
  SDL_Vertex v[DISC_SEGMENTS + 1];
  int        index[DISC_SEGMENTS * 3];

  memset(v, 0, sizeof(v));
  v[0].position = { cx, cy };
  for(int i = 0; i < DISC_SEGMENTS; i++) {
    const float a = i * 2.0f * SDL_PI_F / DISC_SEGMENTS;
    v[i + 1].position = { cx + SDL_cosf(a) * radius, cy + SDL_sinf(a) * radius };
    index[i*3 + 0] = 0;
    index[i*3 + 1] = i + 1;
    index[i*3 + 2] = (i + 1) % DISC_SEGMENTS + 1;
  }
  geometry_color(v, DISC_SEGMENTS + 1, r, g, b, alpha);
  SDL_RenderGeometry(p_renderer, NULL, v, DISC_SEGMENTS + 1, index, DISC_SEGMENTS * 3);
}

// A ring (outline of a disc)
static void ring_draw(SDL_Renderer *p_renderer, float cx, float cy, float radius, float width,
                      Uint8 r, Uint8 g, Uint8 b, float alpha)
{
  #define RING_SEGMENTS 40
  SDL_Vertex v[RING_SEGMENTS * 2];
  int        index[RING_SEGMENTS * 6];

  memset(v, 0, sizeof(v));
  for(int i = 0; i < RING_SEGMENTS; i++) {
    const float a = i * 2.0f * SDL_PI_F / RING_SEGMENTS;
    const float c = SDL_cosf(a), s = SDL_sinf(a);
    v[i*2 + 0].position = { cx + c * radius, cy + s * radius };
    v[i*2 + 1].position = { cx + c * (radius - width), cy + s * (radius - width) };

    const int n = (i + 1) % RING_SEGMENTS;
    index[i*6 + 0] = i*2;
    index[i*6 + 1] = i*2 + 1;
    index[i*6 + 2] = n*2;
    index[i*6 + 3] = n*2;
    index[i*6 + 4] = i*2 + 1;
    index[i*6 + 5] = n*2 + 1;
  }
  geometry_color(v, RING_SEGMENTS * 2, r, g, b, alpha);
  SDL_RenderGeometry(p_renderer, NULL, v, RING_SEGMENTS * 2, index, RING_SEGMENTS * 6);
}

// A chevron ">" pointing to (ux, uy), centred at (cx, cy)
static void chevron_draw(SDL_Renderer *p_renderer, float cx, float cy, float ux, float uy,
                         float size, float alpha)
{
  const float px = -uy, py = ux;            // across the direction
  const float along = 0.30f * size;         // tip in front of the centre
  const float wing = 0.50f * size;          // half of the height
  const float thick = 0.30f * size;         // along the direction

  // the middle of the shape at the centre
  cx += ux * thick / 2;
  cy += uy * thick / 2;

  // outer "V" and the same one moved back by the thickness
  const SDL_FPoint tip   = { cx + ux*along,              cy + uy*along };
  const SDL_FPoint wing1 = { cx - ux*along + px*wing,    cy - uy*along + py*wing };
  const SDL_FPoint wing2 = { cx - ux*along - px*wing,    cy - uy*along - py*wing };

  SDL_Vertex v[6];
  memset(v, 0, sizeof(v));
  v[0].position = wing1;
  v[1].position = tip;
  v[2].position = wing2;
  v[3].position = { wing2.x - ux*thick, wing2.y - uy*thick };
  v[4].position = { tip.x - ux*thick,   tip.y - uy*thick };
  v[5].position = { wing1.x - ux*thick, wing1.y - uy*thick };
  geometry_color(v, 6, 240, 190, 40, alpha);

  static const int index[12] = { 0, 1, 4,  0, 4, 5,  1, 2, 3,  1, 3, 4 };
  SDL_RenderGeometry(p_renderer, NULL, v, 6, index, 12);
}

// ">" - one step, ">>" - walks on
void touch_controls::indicator_draw(SDL_Renderer *p_renderer)
{
  if(!indicator.shown)
    return;

  const float alpha = indicator.fade > 0 ? (float)indicator.fade / (SWIPE_FADE_TICKS + 1) : 1.0f;
  const float shorter = (float)(out_width < out_height ? out_width : out_height);
  const float radius = shorter * 0.075f;

  // whole on the screen
  const float cx = SDL_clamp(indicator.x, radius, out_width - radius);
  const float cy = SDL_clamp(indicator.y, radius, out_height - radius);

  const float ux = swipe_dx[indicator.dir], uy = swipe_dy[indicator.dir];

  disc_draw(p_renderer, cx, cy, radius, 20, 20, 20, 0.6f * alpha);
  ring_draw(p_renderer, cx, cy, radius, SDL_max(radius * 0.06f, 1.5f), 240, 190, 40, 0.65f * alpha);
  if(indicator.walk) {
    const float shift = 0.26f * radius;
    chevron_draw(p_renderer, cx - ux*shift, cy - uy*shift, ux, uy, radius, alpha);
    chevron_draw(p_renderer, cx + ux*shift, cy + uy*shift, ux, uy, radius, alpha);
  } else {
    chevron_draw(p_renderer, cx, cy, ux, uy, radius, alpha);
  }
}

void touch_controls::draw(SDL_Renderer *p_renderer)
{
  if(!visible_now)
    return;

  int w = 0, h = 0;
  SDL_GetRenderOutputSize(p_renderer, &w, &h);
  if(w != out_width || h != out_height)
    layout(w, h);

  SDL_SetRenderDrawBlendMode(p_renderer, SDL_BLENDMODE_BLEND);

  // text: whole multiples of the 8x8 debug font, about a fifth of the button height
  const float button_size = (float)(out_width < out_height ? out_width : out_height) * 0.16f;

  for(int i = 0; i < TOUCH_BUTTONS; i++) {
    const TOUCH_BUTTON *p_b = button + i;

    SDL_SetRenderDrawColor(p_renderer, 20, 20, 20, p_b->pressed ? 200 : 110);
    SDL_RenderFillRect(p_renderer, &p_b->rect);
    SDL_SetRenderDrawColor(p_renderer, 240, 190, 40, p_b->pressed ? 255 : 170);
    SDL_RenderRect(p_renderer, &p_b->rect);

    // 8x8 debug font, scaled up
    const float text_scale = button_size < 48.0f ? 1.0f : (float)(int)(button_size / 48.0f);
    const float text_w = SDL_strlen(p_b->label) * 8.0f * text_scale;
    SDL_SetRenderScale(p_renderer, text_scale, text_scale);
    SDL_RenderDebugText(p_renderer,
                        (p_b->rect.x + (p_b->rect.w - text_w) / 2) / text_scale,
                        (p_b->rect.y + (p_b->rect.h - 8.0f*text_scale) / 2) / text_scale,
                        p_b->label);
    SDL_SetRenderScale(p_renderer, 1.0f, 1.0f);
  }

  indicator_draw(p_renderer);
}
