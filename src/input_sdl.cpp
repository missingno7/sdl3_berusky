/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Platform input backend (SDL3) - see input_backend.h */

#include "berusky.h"
#include "input_backend.h"
#include "test_script.h"
#include "touch_controls.h"

// -------------------------------------------------------
//   Keyboard: SDL keycode -> neutral key
// -------------------------------------------------------

static KEYTYPE key_translate(SDL_Keycode key, SDL_Scancode scancode)
{
  // Letters and digits (SDL3 uses lower-case letters, digits as ASCII)
  if(key >= SDLK_A && key <= SDLK_Z)
    return((KEYTYPE)key);
  if(key >= SDLK_0 && key <= SDLK_9)
    return((KEYTYPE)key);

  switch(key) {
    case SDLK_ESCAPE:       return(K_ESC);
    case SDLK_RETURN:
    case SDLK_KP_ENTER:     return(K_ENTER);
    case SDLK_TAB:          return(K_TAB);
    case SDLK_BACKSPACE:    return(K_BKSP);
    case SDLK_DELETE:       return(K_DEL);
    case SDLK_SPACE:        return(K_SPACE);

    case SDLK_F1:           return(K_F1);
    case SDLK_F2:           return(K_F2);
    case SDLK_F3:           return(K_F3);
    case SDLK_F4:           return(K_F4);
    case SDLK_F5:           return(K_F5);
    case SDLK_F6:           return(K_F6);
    case SDLK_F7:           return(K_F7);
    case SDLK_F8:           return(K_F8);
    case SDLK_F9:           return(K_F9);
    case SDLK_F10:          return(K_F10);
    case SDLK_F11:          return(K_F11);
    case SDLK_F12:          return(K_F12);

    case SDLK_UP:           return(K_UP);
    case SDLK_DOWN:         return(K_DOWN);
    case SDLK_LEFT:         return(K_LEFT);
    case SDLK_RIGHT:        return(K_RIGHT);
    case SDLK_HOME:         return(K_HOME);
    case SDLK_END:          return(K_END);
    case SDLK_PAGEUP:       return(K_PGUP);
    case SDLK_PAGEDOWN:     return(K_PGDN);
    case SDLK_INSERT:       return(K_INSERT);

    case SDLK_MINUS:        return(K_MINUS);
    case SDLK_PLUS:         return(K_PLUS);
    case SDLK_LEFTBRACKET:  return(K_BRACKET_L);
    case SDLK_RIGHTBRACKET: return(K_BRACKET_R);
    case SDLK_SEMICOLON:    return(K_SEMICOL);
    case SDLK_APOSTROPHE:   return(K_QUOTE);
    case SDLK_BACKSLASH:    return(K_BACKSLASH);
    case SDLK_COMMA:        return(K_COMMA);
    case SDLK_PERIOD:       return(K_PERIOD);
    case SDLK_SLASH:        return(K_SLASH);

    case SDLK_AC_BACK:      return(K_ESC);      // Android back button

    case SDLK_KP_0:         return(KP_0);
    case SDLK_KP_1:         return(KP_1);
    case SDLK_KP_2:         return(KP_2);
    case SDLK_KP_3:         return(KP_3);
    case SDLK_KP_4:         return(KP_4);
    case SDLK_KP_5:         return(KP_5);
    case SDLK_KP_6:         return(KP_6);
    case SDLK_KP_7:         return(KP_7);
    case SDLK_KP_8:         return(KP_8);
    case SDLK_KP_9:         return(KP_9);

    default:
      break;
  }

  // Layouts where the key doesn't produce a Latin letter or a digit without
  // shift (AZERTY number row, non-Latin layouts): use the physical key.
  if(scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
    return((KEYTYPE)('a' + (scancode - SDL_SCANCODE_A)));
  if(scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
    return((KEYTYPE)('1' + (scancode - SDL_SCANCODE_1)));
  if(scancode == SDL_SCANCODE_0)
    return(K_0);

  return(K_NONE);
}

static KEYMOD mods_translate(SDL_Keymod mod)
{
  KEYMOD ret = 0;
  if(mod & SDL_KMOD_SHIFT)
    ret |= K_SHIFT_MASK;
  if(mod & SDL_KMOD_CTRL)
    ret |= K_CTRL_MASK;
  if(mod & SDL_KMOD_ALT)
    ret |= K_ALT_MASK;
  return(ret);
}

// -------------------------------------------------------
//   Gamepad: buttons -> neutral keys
//
//   Note: written against the SDL3 API but not tested with real hardware yet.
// -------------------------------------------------------

static KEYTYPE gamepad_button_translate(Uint8 button)
{
  switch(button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:        return(K_UP);
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return(K_DOWN);
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return(K_LEFT);
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return(K_RIGHT);
    case SDL_GAMEPAD_BUTTON_SOUTH:          return(K_ENTER);   // confirm
    case SDL_GAMEPAD_BUTTON_EAST:           return(K_ESC);     // back / menu
    case SDL_GAMEPAD_BUTTON_START:          return(K_ESC);
    case SDL_GAMEPAD_BUTTON_WEST:           return(K_TAB);     // next player
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return(K_TAB);
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return(K_TAB);
    case SDL_GAMEPAD_BUTTON_NORTH:          return(K_F1);      // help
    default:                                return(K_NONE);
  }
}

// -------------------------------------------------------
//   Touch: fingers -> touch controls (keys) or pointer (logical coordinates)
// -------------------------------------------------------

static TOUCH_CONTROLS touch;

static inline tpos pointer_coord(float value);

static void touch_overlay(SDL_Renderer *p_renderer, void *p_data)
{
  ((TOUCH_CONTROLS *)p_data)->draw(p_renderer);
}

// touch_controls = yes | no | auto (auto: on for phones and tablets)
static bool touch_controls_wanted(void)
{
  const char *p_env = SDL_getenv("BERUSKY_TOUCH_CONTROLS");
  if(p_env && p_env[0])
    return(p_env[0] != '0');

  char value[100];
  ini_read_string_file(INI_FILE, "touch_controls", value, sizeof(value), "auto");
  if(is_token(value, "yes") || is_token(value, "on") || is_token(value, "1"))
    return(true);
  if(is_token(value, "no") || is_token(value, "off") || is_token(value, "0"))
    return(false);

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
  return(true);
#else
  return(false);
#endif
}

// The finger that is used as the mouse pointer (menus)
static bool          pointer_finger_active = false;
static SDL_FingerID  pointer_finger = 0;

static void finger_event(INPUT *p_input, SDL_Event *p_event)
{
  if(!p_grf)
    return;

  VIDEO_BACKEND *p_video = p_grf->video_get();

  // Finger coordinates are 0..1 of the window
  int ww, wh;
  p_video->window_size(&ww, &wh);
  const float wx = p_event->tfinger.x * ww;
  const float wy = p_event->tfinger.y * wh;
  const float density = p_video->pixel_density();
  const SDL_FingerID id = p_event->tfinger.fingerID;

  // Controls and swipes in a level (window pixels). The pointer finger
  // stays the pointer (it went down in a menu that started the level).
  if(!(pointer_finger_active && pointer_finger == id)) {
    float tap_x, tap_y;
    switch(touch.finger_event(p_input, p_event, wx * density, wy * density, &tap_x, &tap_y)) {
      case TOUCH_USED:
        p_video->repaint_request();
        return;
      case TOUCH_TAP:
        {
          // A click there (a bug in the top panel selects it)
          float lx, ly;
          if(p_video->window_to_logical(tap_x / density, tap_y / density, &lx, &ly)) {
            const tpos x = pointer_coord(lx), y = pointer_coord(ly);
            p_input->mouse_input(x, y, BUTTON_NONE, 0);
            p_input->mouse_input(x, y, BUTTON_DOWN, BUTTON_LEFT);
            p_input->mouse_input(x, y, BUTTON_UP, BUTTON_LEFT);
          }
          p_video->repaint_request();
        }
        return;
      default:
        break;
    }
  }

  // Any other finger works as the mouse in logical game coordinates
  float lx, ly;
  if(!p_video->window_to_logical(wx, wy, &lx, &ly))
    return;
  const tpos x = pointer_coord(lx), y = pointer_coord(ly);

  switch(p_event->type) {
    case SDL_EVENT_FINGER_DOWN:
      if(!pointer_finger_active) {
        pointer_finger_active = true;
        pointer_finger = id;
        p_input->mouse_input(x, y, BUTTON_NONE, 0);
        p_input->mouse_input(x, y, BUTTON_DOWN, BUTTON_LEFT);
      }
      break;
    case SDL_EVENT_FINGER_MOTION:
      if(pointer_finger_active && pointer_finger == id)
        p_input->mouse_input(x, y, BUTTON_DOWN, BUTTON_LEFT);
      break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
      if(pointer_finger_active && pointer_finger == id) {
        pointer_finger_active = false;
        p_input->mouse_input(x, y, BUTTON_UP, BUTTON_LEFT);
      }
      break;
    default:
      break;
  }
}

static void backend_init(void)
{
  static bool initialized = false;
  if(initialized)
    return;
  initialized = true;

  // Fingers are handled here (touch controls + logical coordinates), not
  // turned into mouse events by SDL
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");

  touch.init(touch_controls_wanted());
  if(touch.enabled_get() && p_grf)
    p_grf->video_get()->overlay_set(touch_overlay, &touch);

  if(!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
    bprintf("Gamepads are not available: %s", SDL_GetError());
    return;
  }

  int count = 0;
  SDL_JoystickID *p_ids = SDL_GetGamepads(&count);
  for(int i = 0; i < count; i++)
    SDL_OpenGamepad(p_ids[i]);
  SDL_free(p_ids);
}

// -------------------------------------------------------
//   Pointer: window coordinates -> logical game coordinates
// -------------------------------------------------------

static inline tpos pointer_coord(float value)
{
  return((tpos)SDL_floorf(value));
}

static void window_event(SDL_Event *p_event)
{
  if(!p_grf)
    return;

  VIDEO_BACKEND *p_video = p_grf->video_get();

  switch(p_event->type) {
    case SDL_EVENT_RENDER_DEVICE_RESET:
      bprintf("Render device reset");
      p_video->device_reset();
      break;
    case SDL_EVENT_RENDER_TARGETS_RESET:
      bprintf("Render targets reset");
      p_video->targets_reset();
      break;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
    case SDL_EVENT_WINDOW_SHOWN:
      // Mobile: the window's surface is new and ready some time later
      p_video->repaint_burst();
      break;
    case SDL_EVENT_WINDOW_MINIMIZED:
      bprintf("App iconified\n");
      break;
    case SDL_EVENT_WINDOW_RESTORED:
      bprintf("App activated\n");
      p_video->repaint_burst();
      break;
    default:
      // exposed, resized, pixel density / fullscreen changed...
      p_video->repaint_request();
      break;
  }
}

// -------------------------------------------------------
//   Event loop
// -------------------------------------------------------

// The input the last poll worked for (releases during input_backend_idle())
static INPUT *p_last_input = NULL;

void input_backend_idle(void)
{
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_EVENT_KEY_UP:
        if(p_last_input) {
          KEYTYPE key = key_translate(event.key.key, event.key.scancode);
          if(key != K_NONE)
            p_last_input->key_input(key, mods_translate(event.key.mod), false);
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if(p_last_input && p_grf && event.button.button >= BUTTON_LEFT && event.button.button <= BUTTON_RIGHT) {
          p_grf->video_get()->event_to_logical(&event);
          p_last_input->mouse_input(pointer_coord(event.button.x), pointer_coord(event.button.y),
                                    BUTTON_UP, event.button.button);
        }
        break;
      default:
        if((event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) ||
           event.type == SDL_EVENT_RENDER_TARGETS_RESET ||
           event.type == SDL_EVENT_RENDER_DEVICE_RESET) {
          window_event(&event);
        }
        break;
    }
  }

  if(p_grf)
    p_grf->present_if_needed();
}

bool input_backend_poll(class input *p_input_, bool wait)
{
  INPUT *p_input = (INPUT *)p_input_;
  p_last_input = p_input;
  SDL_Event event;
  bool ret;

  backend_init();

  // Touch controls are shown while a level is played
  {
    if(touch.update(p_input) && p_grf)
      p_grf->video_get()->repaint_request();
  }

  // Scripted input (regression tests), does nothing normally
  if(test_script_poll(p_input_))
    return(true);

  // Loop until there are no SDL events left on the queue
  if(wait) {
    // A running test script must be polled even when there are no events
    ret = test_script_active() ? SDL_WaitEventTimeout(&event, 20) : SDL_WaitEvent(&event);
  }
  else {
    ret = SDL_PollEvent(&event);
  }

  // A test script is the only source of input - real devices are ignored,
  // so the result doesn't depend on what the user does with the machine
  const bool scripted = test_script_active();

  while(ret) {
    if(scripted && ((event.type >= SDL_EVENT_KEY_DOWN && event.type <= SDL_EVENT_KEY_UP) ||
                    (event.type >= SDL_EVENT_MOUSE_MOTION && event.type <= SDL_EVENT_MOUSE_WHEEL) ||
                    (event.type >= SDL_EVENT_GAMEPAD_AXIS_MOTION && event.type <= SDL_EVENT_GAMEPAD_TOUCHPAD_UP) ||
                    ((event.type >= SDL_EVENT_FINGER_DOWN && event.type <= SDL_EVENT_FINGER_CANCELED) &&
                     event.tfinger.touchID != TEST_TOUCH_ID))) {
      ret = SDL_PollEvent(&event);
      continue;
    }

    switch (event.type) {
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        {
          bool pressed = (event.type == SDL_EVENT_KEY_DOWN);

          // F12: the renderer's diagnostics overlay (not a game key)
          if(event.key.key == SDLK_F12) {
            if(pressed && !event.key.repeat && p_grf)
              p_grf->video_get()->debug_overlay_toggle();
            break;
          }

          // Key repeat is off unless the input asks for it
          if(pressed && event.key.repeat && !p_input->key_repeat_get())
            break;
          KEYTYPE key = key_translate(event.key.key, event.key.scancode);
          if(key != K_NONE)
            p_input->key_input(key, mods_translate(event.key.mod), pressed);
        }
        break;

      case SDL_EVENT_MOUSE_MOTION:
        {
          if(p_grf)
            p_grf->video_get()->event_to_logical(&event);

          tpos x = pointer_coord(event.motion.x),
               y = pointer_coord(event.motion.y);
          bool pressed = FALSE;

          // left, middle, right button
          for(int i = BUTTON_LEFT; i <= BUTTON_RIGHT; i++) {
            if(event.motion.state & SDL_BUTTON_MASK(i)) {
              p_input->mouse_input(x, y, BUTTON_DOWN, i);
              pressed = TRUE;
            }
          }
          if(!pressed) {
            p_input->mouse_input(x, y, BUTTON_NONE, 0);
          }
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
        {
          if(p_grf)
            p_grf->video_get()->event_to_logical(&event);

          // SDL3 button numbers are the same as BUTTON_xxx for left, middle, right.
          // Other buttons (X1, X2) aren't used by the game. The wheel is a separate event.
          if(event.button.button >= BUTTON_LEFT && event.button.button <= BUTTON_RIGHT) {
            p_input->mouse_input(pointer_coord(event.button.x), pointer_coord(event.button.y),
                                 event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? BUTTON_DOWN : BUTTON_UP,
                                 event.button.button);
          }
        }
        break;

      case SDL_EVENT_MOUSE_WHEEL:
        {
          // The wheel was a pair of button 4/5 events in SDL 1.2
          int button = (event.wheel.y > 0) ? WHEEL_UP : (event.wheel.y < 0 ? WHEEL_DOWN : NO_BUTTON);
          if(button != NO_BUTTON) {
            MOUSE_STATE *p_state = p_input->mouse_state_get();
            p_input->mouse_input(p_state->rect.x, p_state->rect.y, BUTTON_DOWN, button);
            p_input->mouse_input(p_state->rect.x, p_state->rect.y, BUTTON_UP, button);
          }
        }
        break;

      case SDL_EVENT_FINGER_DOWN:
      case SDL_EVENT_FINGER_UP:
      case SDL_EVENT_FINGER_MOTION:
      case SDL_EVENT_FINGER_CANCELED:
        finger_event(p_input, &event);
        break;

      case SDL_EVENT_GAMEPAD_ADDED:
        SDL_OpenGamepad(event.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        {
          SDL_Gamepad *p_pad = SDL_GetGamepadFromID(event.gdevice.which);
          if(p_pad)
            SDL_CloseGamepad(p_pad);
        }
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
          KEYTYPE key = gamepad_button_translate(event.gbutton.button);
          if(key != K_NONE)
            p_input->key_input(key, 0, event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        }
        break;

      case SDL_EVENT_QUIT:
        return(true);

      default:
        if((event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) ||
           event.type == SDL_EVENT_RENDER_TARGETS_RESET ||
           event.type == SDL_EVENT_RENDER_DEVICE_RESET ||
           event.type == SDL_EVENT_DID_ENTER_FOREGROUND) {
          window_event(&event);
        }
        break;
    }

    ret = SDL_PollEvent(&event);
  }

  // The window was damaged (expose, resize, ...)
  if(p_grf)
    p_grf->present_if_needed();

  return(false);
}
