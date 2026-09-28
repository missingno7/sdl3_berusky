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

// indexes into button[]
enum {
  TB_UP, TB_DOWN, TB_LEFT, TB_RIGHT,
  TB_NEXT, TB_MENU, TB_RESTART,
  TB_PLAYER_1, TB_PLAYER_2, TB_PLAYER_3, TB_PLAYER_4, TB_PLAYER_5
};

touch_controls::touch_controls(void)
: enabled(false), visible_now(false), out_width(0), out_height(0)
{
  memset(button, 0, sizeof(button));

  static const struct { int key; int mods; const char *label; } defs[TOUCH_BUTTONS] = {
    { K_UP,    0,            "UP"    },
    { K_DOWN,  0,            "DOWN"  },
    { K_LEFT,  0,            "LEFT"  },
    { K_RIGHT, 0,            "RIGHT" },
    { K_TAB,   0,            "NEXT"  },
    { K_ESC,   0,            "MENU"  },
    { K_R,     K_CTRL_MASK,  "RESET" },
    { K_1,     0,            "1"     },
    { K_2,     0,            "2"     },
    { K_3,     0,            "3"     },
    { K_4,     0,            "4"     },
    { K_5,     0,            "5"     }
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

void touch_controls::update(class input *p_input)
{
  bool now = enabled && ((INPUT *)p_input)->keyset_get() == &game_keys;

  // The game left the level while a finger was on a control
  if(visible_now && !now)
    release_all(p_input);

  visible_now = now;
}

void touch_controls::release_all(class input *p_input)
{
  for(int i = 0; i < TOUCH_BUTTONS; i++) {
    if(button[i].pressed) {
      ((INPUT *)p_input)->key_input(button[i].key, 0, false);
      button[i].pressed = false;
      button[i].finger = 0;
    }
  }
}

// Controls are placed in the window (not in the game picture): the D-pad in the
// bottom-left corner, the actions in the bottom-right and top-right. On a wide
// phone screen they end up in the free space next to the letterboxed game.
void touch_controls::layout(int width, int height)
{
  out_width = width;
  out_height = height;

  const float shorter = (float)(width < height ? width : height);
  const float u = shorter * 0.16f;          // button size
  const float m = shorter * 0.03f;          // margin

  // D-pad
  const float cx = m + 1.5f*u;
  const float cy = height - m - 1.5f*u;
  button[TB_UP].rect    = { cx - u/2, cy - 1.5f*u, u, u };
  button[TB_DOWN].rect  = { cx - u/2, cy + 0.5f*u, u, u };
  button[TB_LEFT].rect  = { cx - 1.5f*u, cy - u/2, u, u };
  button[TB_RIGHT].rect = { cx + 0.5f*u, cy - u/2, u, u };

  // Actions
  button[TB_NEXT].rect    = { width - m - 1.6f*u, height - m - 1.6f*u, 1.6f*u, 1.6f*u };
  button[TB_MENU].rect    = { width - m - u, m, u, u };
  button[TB_RESTART].rect = { width - m - 2.2f*u, m, 1.1f*u, u };

  // Player selection
  const float p = 0.75f * u;
  for(int i = 0; i < 5; i++)
    button[TB_PLAYER_1 + i].rect = { m + i*(p + m), m, p, p };
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

bool touch_controls::finger_event(class input *p_input, const SDL_Event *p_event, float x, float y)
{
  if(!visible_now)
    return(false);

  INPUT *p_in = (INPUT *)p_input;
  const SDL_FingerID id = p_event->tfinger.fingerID;

  switch(p_event->type) {
    case SDL_EVENT_FINGER_DOWN:
      {
        TOUCH_BUTTON *p_button = button_at(x, y);
        if(!p_button || p_button->pressed)
          return(false);
        p_button->pressed = true;
        p_button->finger = id;
        p_in->key_input(p_button->key, p_button->mods, true);
        return(true);
      }
    case SDL_EVENT_FINGER_MOTION:
      // A finger that started on a control belongs to it, whatever it does later
      for(int i = 0; i < TOUCH_BUTTONS; i++) {
        if(button[i].pressed && button[i].finger == id)
          return(true);
      }
      return(false);
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
      for(int i = 0; i < TOUCH_BUTTONS; i++) {
        if(button[i].pressed && button[i].finger == id) {
          button[i].pressed = false;
          button[i].finger = 0;
          p_in->key_input(button[i].key, 0, false);
          return(true);
        }
      }
      return(false);
    default:
      return(false);
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
}
