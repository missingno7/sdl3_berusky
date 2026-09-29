/*
 *        .þÛÛþ þ    þ þÛÛþ.     þ    þ þÛÛÛþ.  þÛÛÛþ .þÛÛþ. þ    þ
 *       .þ   Û Ûþ.  Û Û   þ.    Û    Û Û    þ  Û.    Û.   Û Ûþ.  Û
 *       Û    Û Û Û  Û Û    Û    Û   þ. Û.   Û  Û     Û    Û Û Û  Û
 *     .þþÛÛÛÛþ Û  Û Û þÛÛÛÛþþ.  þþÛÛ.  þþÛÛþ.  þÛ    Û    Û Û  Û Û
 *    .Û      Û Û  .þÛ Û      Û. Û   Û  Û    Û  Û.    þ.   Û Û  .þÛ
 *    þ.      þ þ    þ þ      .þ þ   .þ þ    .þ þÛÛÛþ .þÛÛþ. þ    þ
 *
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Global input interface
*/
#include "berusky.h" 
#include "berusky_gui.h"
#include "editor.h"
#include "input_backend.h"

/* KB definitions
*/
EVENT_KEY game_key_array[] = 
{
  {LEVEL_EVENT(GL_PLAYER_MOVE, 0,-1),LEVEL_EVENT(EV_NONE), K_UP,   0,0,0,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},
  {LEVEL_EVENT(GL_PLAYER_MOVE, 0, 1),LEVEL_EVENT(EV_NONE), K_DOWN, 0,0,0,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},
  {LEVEL_EVENT(GL_PLAYER_MOVE,-1, 0),LEVEL_EVENT(EV_NONE), K_LEFT, 0,0,0,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},
  {LEVEL_EVENT(GL_PLAYER_MOVE, 1, 0),LEVEL_EVENT(EV_NONE), K_RIGHT,0,0,0,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},

  {LEVEL_EVENT(GL_PLAYER_MOVE_FAST, 0,-1),LEVEL_EVENT(EV_NONE), K_UP,   0,0,1,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},
  {LEVEL_EVENT(GL_PLAYER_MOVE_FAST, 0, 1),LEVEL_EVENT(EV_NONE), K_DOWN, 0,0,1,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},
  {LEVEL_EVENT(GL_PLAYER_MOVE_FAST,-1, 0),LEVEL_EVENT(EV_NONE), K_LEFT, 0,0,1,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},
  {LEVEL_EVENT(GL_PLAYER_MOVE_FAST, 1, 0),LEVEL_EVENT(EV_NONE), K_RIGHT,0,0,1,KEY_GROUP_BLOCK, KEY_GROUP_BLOCK_MOVE},

  {LEVEL_EVENT(GL_PLAYER_SWITCH, SELECT_PLAYER_NEXT, 0),LEVEL_EVENT(EV_NONE), K_TAB, 0,0,0,KEY_CLEAR_AFTER_PRESS},

  {LEVEL_EVENT(GL_PLAYER_SWITCH,  0, 0),LEVEL_EVENT(EV_NONE), K_1, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GL_PLAYER_SWITCH,  1, 0),LEVEL_EVENT(EV_NONE), K_2, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GL_PLAYER_SWITCH,  2, 0),LEVEL_EVENT(EV_NONE), K_3, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GL_PLAYER_SWITCH,  3, 0),LEVEL_EVENT(EV_NONE), K_4, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GL_PLAYER_SWITCH,  4, 0),LEVEL_EVENT(EV_NONE), K_5, 0,0,0,KEY_CLEAR_AFTER_PRESS},

  // N - the next music track (the DOS game had it too)
  {LEVEL_EVENT(SN_PLAY_MUSIC, (int)MUSIC_LEVEL),LEVEL_EVENT(EV_NONE), K_N, 0,0,0,KEY_CLEAR_AFTER_PRESS},

  {LEVEL_EVENT(GC_SUSPEND_LEVEL),LEVEL_EVENT(GC_MENU_IN_GAME),   K_ESC, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_SUSPEND_LEVEL),LEVEL_EVENT(GC_MENU_HELP,TRUE), K_F1, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_SUSPEND_LEVEL),LEVEL_EVENT(GC_MENU_LEVEL_HINT,TRUE), K_F1, 0,1,0,KEY_CLEAR_AFTER_PRESS},

  {LEVEL_EVENT(GC_SUSPEND_LEVEL),LEVEL_EVENT(EV_NONE),K_S, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_RESTART_LEVEL),LEVEL_EVENT(EV_NONE),K_R, 0,1,0,KEY_CLEAR_AFTER_PRESS},

  {LEVEL_EVENT(GC_SAVE_LEVEL),LEVEL_EVENT(GC_RESTORE_LEVEL),K_F2, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_LOAD_LEVEL),LEVEL_EVENT(GC_RESTORE_LEVEL),K_F3, 0,0,0,KEY_CLEAR_AFTER_PRESS},

  {LEVEL_EVENT(GC_MENU_QUIT),LEVEL_EVENT(EV_NONE), K_X, 1,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_QUIT),LEVEL_EVENT(EV_NONE), K_X, 0,1,0,KEY_CLEAR_AFTER_PRESS},

  // A key for cheating - E+SHIFT
  {LEVEL_EVENT(GC_STOP_LEVEL, TRUE, TRUE),LEVEL_EVENT(EV_NONE), K_E, 0,0,1, KEY_CLEAR_AFTER_PRESS}
};

EVENT_KEY_SET game_keys = { game_key_array , sizeof(game_key_array) / sizeof(game_key_array[0]) };

/* KB definitions
*/
EVENT_KEY suspend_key_array[] = 
{
  {LEVEL_EVENT(GC_RESTORE_LEVEL),LEVEL_EVENT(SN_PLAY_SAMPLE, (int)SOUND_MENU_CLICK, SOUND_TICKS_MENU, SOUND_PRIORITY_MENU), K_ESC, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_RESTORE_LEVEL),LEVEL_EVENT(EV_NONE), K_R, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_RESTART_LEVEL),LEVEL_EVENT(EV_NONE), K_R, 0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_QUIT),    LEVEL_EVENT(EV_NONE), K_X, 1,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_QUIT),    LEVEL_EVENT(EV_NONE), K_X, 0,1,0,KEY_CLEAR_AFTER_PRESS}
};

EVENT_KEY_SET suspend_keys = { suspend_key_array, sizeof(suspend_key_array) / sizeof(suspend_key_array[0]) };

/* KB definitions
*/
EVENT_KEY menu_key_array[] = 
{
  {LEVEL_EVENT(GC_MENU_START), LEVEL_EVENT(EV_NONE),K_ESC,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_START), LEVEL_EVENT(EV_NONE),K_ENTER, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_START), LEVEL_EVENT(EV_NONE),K_SPACE, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_QUIT),  LEVEL_EVENT(EV_NONE),K_X,     1,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(GC_MENU_QUIT),  LEVEL_EVENT(EV_NONE),K_X,     0,1,0,KEY_CLEAR_AFTER_PRESS}
};

EVENT_KEY_SET menu_keys = { menu_key_array, sizeof(menu_key_array) / sizeof(menu_key_array[0]) };

/* KB definitions
*/
EVENT_KEY editor_key_array[] = 
{
  {LEVEL_EVENT(GC_MENU_QUIT),                               LEVEL_EVENT(EV_NONE),K_ESC, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_LAYER,LAYER_GRID,LAYER_CHANGE),     LEVEL_EVENT(EV_NONE),K_1,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_LAYER,LAYER_FLOOR,LAYER_CHANGE),    LEVEL_EVENT(EV_NONE),K_2,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_LAYER,LAYER_ITEMS,LAYER_CHANGE),    LEVEL_EVENT(EV_NONE),K_3,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_LAYER,LAYER_PLAYER,LAYER_CHANGE),   LEVEL_EVENT(EV_NONE),K_4,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_HELP),                                    LEVEL_EVENT(EV_NONE),K_F1,  0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_NEW),                               LEVEL_EVENT(EV_NONE),K_N,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SAVE),                              LEVEL_EVENT(EV_NONE),K_F2,  0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SAVE_AS),                           LEVEL_EVENT(EV_NONE),K_F2,  0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_LOAD),                              LEVEL_EVENT(EV_NONE),K_F3,  0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_UNDO),                                    LEVEL_EVENT(EV_NONE),K_U,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_REDO),                                    LEVEL_EVENT(EV_NONE),K_R,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_RUN),                               LEVEL_EVENT(EV_NONE),K_F9,  0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_FULLSCREEN),                        LEVEL_EVENT(EV_NONE),K_F10, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SHADER),                            LEVEL_EVENT(EV_NONE),K_S,   0,1,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_ROTATE_SELECTION),                        LEVEL_EVENT(EV_NONE),K_R,   0,0,1,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_RECTANGLE_SELECTION,FALSE),         LEVEL_EVENT(EV_NONE),K_D,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_RECTANGLE_SELECTION,TRUE),          LEVEL_EVENT(EV_NONE),K_F,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SELECT_LAYER,LAYER_FLOOR),          LEVEL_EVENT(EV_NONE),K_1,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SELECT_LAYER,LAYER_ITEMS),          LEVEL_EVENT(EV_NONE),K_2,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SELECT_LAYER,LAYER_PLAYER),         LEVEL_EVENT(EV_NONE),K_3,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_SELECT_LAYER,ALL_LEVEL_LAYERS),     LEVEL_EVENT(EV_NONE),K_4,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_CHANGE_BACKGROUND),                 LEVEL_EVENT(EV_NONE),K_B,   0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOUSE_PANEL_SCROLL,-ITEMS_IN_PANEL),LEVEL_EVENT(EV_NONE),K_PGUP,0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOUSE_PANEL_SCROLL,ITEMS_IN_PANEL), LEVEL_EVENT(EV_NONE),K_PGDN,0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOUSE_PANEL_SCROLL,ITEMS_START),    LEVEL_EVENT(EV_NONE),K_HOME,0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOUSE_PANEL_SCROLL,ITEMS_END),      LEVEL_EVENT(EV_NONE),K_END, 0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOVE,0, 1),                         LEVEL_EVENT(EV_NONE),K_UP,  0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOVE,0,-1),                         LEVEL_EVENT(EV_NONE),K_DOWN,0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOVE,1,0),                          LEVEL_EVENT(EV_NONE),K_LEFT,0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_LEVEL_MOVE,-1, 0),                        LEVEL_EVENT(EV_NONE),K_RIGHT,0,0,0,KEY_CLEAR_AFTER_PRESS},
  {LEVEL_EVENT(ED_CURSOR_PICKUP),                           LEVEL_EVENT(EV_NONE),K_P,    0,0,0,KEY_CLEAR_AFTER_PRESS},

  //{LEVEL_EVENT(EV_TEST),                                  LEVEL_EVENT(EV_NONE),K_A, 0,0,0,KEY_CLEAR_AFTER_PRESS},
};

EVENT_KEY_SET editor_keys = { editor_key_array, sizeof(editor_key_array) / sizeof(editor_key_array[0]) };

bool key_to_ascii(int key, char *p_char)
{
  if(key < 127) {
    *p_char = key;
    return(TRUE);
  }
  else {
    return(FALSE);
  }
}

// -------------------------------------------------------------------------
// Keyboard interface
// -------------------------------------------------------------------------
bool input::key_status(KEYTYPE key)
{
  return(key > 0 && key < KEYNUM && key_state[key]);
}

void input::keyset_set(EVENT_KEY_SET *p_keyset)
{
  p_set = p_keyset;

  if(p_set) {
    EVENT_KEY *p_key = p_set->p_keys;
    int        keynum = p_set->keynum;
    int        i;
  
    for(i = 0; i < keynum; i++, p_key++) {
      p_key->flag = p_key->flag&~KEY_PRESSED;      
    }
  }
}

// group_mask == 0 -> mask all groups
void input::key_block(int group_mask, bool block)
{ 
  if(!group_mask)
    group_mask = ~group_mask;
  group = block ? group | group_mask : group & (~group_mask);
}

void input::key_add(LEVEL_EVENT_QUEUE *p_queue)
{
  // check evets-list
  if(!p_set)
    return;
 
  EVENT_KEY *p_key = p_set->p_keys;
  int        keynum = p_set->keynum;
  int        i;
  
  for(i = 0; i < keynum; i++, p_key++) {  
    // check group - is already present?
    if(p_key->flag&KEY_PRESSED && !(p_key->flag&KEY_GROUP_BLOCK && p_key->group&group)) {
      // okay, add an event      
      p_queue->add(LEVEL_EVENT(p_key->e1));
      if(p_key->e2.valid())
        p_queue->add(LEVEL_EVENT(p_key->e2));
      
      if(p_key->flag&KEY_GROUP_BLOCK) {
        group |= p_key->group;
        group_events_sent++;
      }

      if(p_key->flag&KEY_CLEAR_AFTER_PRESS)
        p_key->flag &= ~KEY_PRESSED;
    }
  }
  p_queue->commit();
}


void input::events_game(LEVEL_EVENT_QUEUE *p_queue)
{
  while(!p_queue->empty()) {
    
    LEVEL_EVENT ev = p_queue->get();
  
    if(ev.action_get() == GI_BLOCK_KEYS) {
      // format: [UI_BLOCK_KEYS, key_group, blocked]
      key_block(ev.param_int_get(PARAM_GROUP), ev.param_int_get(PARAM_BLOCK));
    } else if(ev.action_get() == GI_UNBLOCK_ALL) {
      key_block(0, FALSE);
    } else {
      p_queue->add(ev);
    }
  }

  p_queue->commit();
}

void input::key_input(KEYTYPE key, KEYMOD modification, bool pressed)
{
  // Remember the state of all keys - this replaces polling of the SDL 1.2
  // keyboard state, and it doesn't depend on any physical device
  if(key > 0 && key < KEYNUM)
    key_state[key] = pressed;

  // check input queue
  if(pressed) {
    input_queue.add(LEVEL_EVENT(GI_KEY_DOWN, (int)key));
    input_queue.commit();
  }  

  // check evets-list
  if(!p_set || flag&INPUT_BLOCK_SETS)
    return;
 
  EVENT_KEY *p_key = p_set->p_keys;
  int        keynum = p_set->keynum;
  int        i;

  bool shift = (modification&K_SHIFT_MASK) != 0;
  bool ctrl = (modification&K_CTRL_MASK) != 0;
  
  // Update all key events regard to current keyboard state
  for(i = 0; i < keynum; i++, p_key++) {
    assert(p_key->key >= 0 && p_key->key < KEYNUM);
    if(key_state[p_key->key] && ctrl == p_key->ctrl && shift == p_key->shift) {
      p_key->flag = p_key->flag|KEY_PRESSED;
    }
    else if(!(p_key->flag&KEY_CLEAR_AFTER_PRESS)) {
      p_key->flag = p_key->flag&~KEY_PRESSED;
    }
    // One-shot keys (KEY_CLEAR_AFTER_PRESS) stay pressed until key_add()
    // sends their event. This way a very short tap (down + up between two
    // game ticks - typical for a touch screen) isn't lost.
  }  
}

void input::events_loop(LEVEL_EVENT_QUEUE *p_queue)
{
  // Clear input queue
  input_queue.clear();

  // Read all events from the platform: keyboard, mouse, touch, gamepad...
  // They call key_input() / mouse_input() of this class.
  bool quit = input_backend_poll(this, (flag&INPUT_EVENT_LOOP_WAIT) != 0);
  if(quit) {
    p_queue->add(LEVEL_EVENT(GC_MENU_QUIT));
    p_queue->commit();
    return;
  }

  // Add all input events
  p_queue->add(&input_queue);
  p_queue->commit();

  // Add events from key-sets
  events_game(p_queue);
  key_add(p_queue);
}

// -------------------------------------------------------------------------
// Mouse interface
// -------------------------------------------------------------------------

void input::mouse_input(tpos mx, tpos my, MOUSE_BUTTON_STATE state, int button)
{
  // bprintf("Mouse state [%d,%d], state %d, button %d", mx, my, state, button);

  /* Save recent mouse state */
  mstate.rect.x = mx;
  mstate.rect.y = my;

  if(flag&INPUT_BLOCK_SETS)
    return;

  if(state != NO_BUTTON && button < MOUSE_BUTTONS) {
    mstate.button[button] = state;
  }

  // In a level a click on a bug in the top panel selects it (as keys 1-5)
  if(p_set == &game_keys && state == BUTTON_DOWN && button == BUTTON_LEFT) {
    for(int i = 0; i < MAX_PLAYERS; i++) {
      RECT r = top_panel_player_rect(i);
      if(in_rect(r, mx, my)) {
        input_queue.add(LEVEL_EVENT(GL_PLAYER_SWITCH, i, 0));
        input_queue.commit();
        break;
      }
    }
  }

  /* Process all events */
  if(!mevents.is_empty()) {  
    MOUSE_EVENT *p_ev = reinterpret_cast<MOUSE_EVENT *>(mevents.list_get_first());
    while(p_ev) {
      bool cond_button = TRUE;
      bool cond_key = TRUE;
      bool cond_area = FALSE;
      bool active;
    
      if(p_ev->flag&MEVENT_MOUSE_BUTTONS) {
        int j;
        for(j = 0; j < MOUSE_BUTTONS; j++) {
          /* button is required and pressed - process it */
          if(mstate.button[j] && mstate.button[j] == p_ev->mstate.button[j]) {
            break;
          }          
        }
        cond_button = (j != MOUSE_BUTTONS);
      }
    
      if(p_ev->flag&MEVENT_MOUSE_IN) {
        cond_area = in_rect(p_ev->mstate.rect,mx,my);
      } else if(p_ev->flag&MEVENT_MOUSE_OUT) {
        cond_area = !in_rect(p_ev->mstate.rect,mx,my);
      }
    
      if(p_ev->flag&MEVENT_KEY) {
        cond_key = p_ev->mstate.key && key_status(p_ev->mstate.key);
      }
          
      if(cond_button && cond_key && cond_area) {
        if(p_ev->flag&MEVENT_ACTIVATE_ONCE) {
          if(p_ev->last_state) {
            active = FALSE;
          } else {
            active = p_ev->last_state = TRUE;
          }           
        }
        else
          active = TRUE;
      } else {
        active = p_ev->last_state = FALSE;
      }
      
      if(active) {
        if(p_ev->flag&MEVENT_MOUSE_EXTERNAL) {          
          if(p_ev->flag&MEVENT_MOUSE_BUTTONS)
            input_queue.add(LEVEL_EVENT(GI_MOUSE_EVENT, p_ev->event[0].param_int_get(PARAM_0), mx, my, state, button));
          else
            input_queue.add(LEVEL_EVENT(GI_MOUSE_EVENT, p_ev->event[0].param_int_get(PARAM_0), mx, my, 0, 0));
        }
        else {
          input_queue.add(p_ev->event,p_ev->event_num);
        }
      }
      p_ev = reinterpret_cast<MOUSE_EVENT *>(p_ev->list_next());
    }
    
    input_queue.commit();
  }

  mstate.button[button] = BUTTON_NONE;
}

void input::mevent_state_clear(MOUSE_EVENT *p_first)
{
  MOUSE_EVENT *p_event = p_first ? p_first : 
                         reinterpret_cast<MOUSE_EVENT *>(mevents.list_get_first());
  while(p_event) {
    p_event->state_clear();
    p_event = reinterpret_cast<MOUSE_EVENT *>(p_event->list_next());
  }
}

void input::mevent_clear(void)
{  
  mevents.list_clear();
}

MOUSE_EVENT * input::mevent_add(MOUSE_EVENT event)
{  
  return(mevent_add(&event, 1));
}

MOUSE_EVENT * input::mevent_add(MOUSE_EVENT *p_event, int num)
{
  MOUSE_EVENT *p_first = reinterpret_cast<MOUSE_EVENT *>(mmemcpy(p_event, sizeof(MOUSE_EVENT)));
  mevents.list_insert_last(p_first);

  for(int i = 1; i < num; i++) {
    mevents.list_insert_last(reinterpret_cast<MOUSE_EVENT *>(mmemcpy(p_event+i, sizeof(MOUSE_EVENT))));
  }

  // Initialize all
  mevent_state_clear(p_first);

  return(p_first);
}

void input::mevent_remove(MOUSE_EVENT *p_first, int num)
{
  for(int i = 0; i < num; i++) {
    assert(p_first);
    MOUSE_EVENT *p_next = reinterpret_cast<MOUSE_EVENT *>(p_first->list_next());
    mevents.list_remove(p_first);
    ffree(p_first);
    p_first = p_next;
  }  
}

void input::block(bool state)
{
  flag = state ? flag|INPUT_BLOCK_SETS : flag&~INPUT_BLOCK_SETS;
}

void input::key_repeat(bool state)
{
  key_repeat_enabled = state;
}

void input::events_wait(bool state)
{
  flag = state ? flag|INPUT_EVENT_LOOP_WAIT : flag&~INPUT_EVENT_LOOP_WAIT;
}
