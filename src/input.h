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

/* Interface for keyboard/mouse/everything
*/

#ifndef __INPUT_H__
#define __INPUT_H__

/*
 * Input architecture
 *
 *   physical devices           neutral keys           game events
 *   (keyboard, gamepad,   ->   (K_xxx below,     ->   (LEVEL_EVENT, see the
 *    touch buttons, ...)        input::key_input)      key sets in input.cpp)
 *
 * The game never sees SDL key codes. The SDL3 input backend (input_sdl.cpp)
 * translates whatever the device produces to the neutral key codes, and the
 * game reacts only to the "key sets" (EVENT_KEY_SET) that map those keys to
 * game events (move, switch player, pause, ...). A touch screen control or a
 * gamepad only has to call input::key_input() with the same neutral key
 * codes to drive the game - nothing in the game assumes a keyboard.
 *
 * Mouse and touch pointers are reported in logical game coordinates
 * (see video.h), never in window pixels.
 */

/* Neutral key codes.
   Printable keys use their ASCII value, so the keys typed into the
   profile-name / level-name inputs are directly characters. */

#define  KEYNUM    512

#define  K_NONE    0

#define  K_BKSP    8
#define  K_TAB     9
#define  K_ENTER   13
#define  K_ESC     27
#define  K_SPACE   ' '
#define  K_DEL     127

#define  K_1       '1'
#define  K_2       '2'
#define  K_3       '3'
#define  K_4       '4'
#define  K_5       '5'
#define  K_6       '6'
#define  K_7       '7'
#define  K_8       '8'
#define  K_9       '9'
#define  K_0       '0'

#define  K_A       'a'
#define  K_B       'b'
#define  K_C       'c'
#define  K_D       'd'
#define  K_E       'e'
#define  K_F       'f'
#define  K_G       'g'
#define  K_H       'h'
#define  K_I       'i'
#define  K_J       'j'
#define  K_K       'k'
#define  K_L       'l'
#define  K_M       'm'
#define  K_N       'n'
#define  K_O       'o'
#define  K_P       'p'
#define  K_Q       'q'
#define  K_R       'r'
#define  K_S       's'
#define  K_T       't'
#define  K_U       'u'
#define  K_V       'v'
#define  K_W       'w'
#define  K_X       'x'
#define  K_Y       'y'
#define  K_Z       'z'

#define  K_MINUS      '-'
#define  K_PLUS       '+'
#define  K_BRACKET_L  '['
#define  K_BRACKET_R  ']'
#define  K_SEMICOL    ';'
#define  K_QUOTE      '\''
#define  K_BACKSLASH  '\\'
#define  K_COMMA      ','
#define  K_PERIOD     '.'
#define  K_SLASH      '/'

// Non-printable keys
#define  K_F1      256
#define  K_F2      257
#define  K_F3      258
#define  K_F4      259
#define  K_F5      260
#define  K_F6      261
#define  K_F7      262
#define  K_F8      263
#define  K_F9      264
#define  K_F10     265
#define  K_F11     266
#define  K_F12     267

#define  K_UP      270
#define  K_DOWN    271
#define  K_LEFT    272
#define  K_RIGHT   273

#define  K_HOME    274
#define  K_END     275
#define  K_PGUP    276
#define  K_PGDN    277
#define  K_INSERT  278

#define  KP_0      280
#define  KP_1      281
#define  KP_2      282
#define  KP_3      283
#define  KP_4      284
#define  KP_5      285
#define  KP_6      286
#define  KP_7      287
#define  KP_8      288
#define  KP_9      289

// Modifiers (bit mask)
#define  K_SHIFT_MASK 0x1
#define  K_CTRL_MASK  0x2
#define  K_ALT_MASK   0x4

typedef  int          KEYTYPE;
typedef  int          KEYMOD;

#define  KEY_PRESSED                  0x1     // key is pressed
#define  KEY_CLEAR_AFTER_PRESS        0x2     // clear key after press
#define  KEY_GROUP_BLOCK              0x8     // Key is member of group

#define  KEY_GROUP_BLOCK_MOVE         0x100

inline bool in_rect(RECT &dst, tpos x, tpos y)
{
  return(x >= dst.x && y >= dst.y && x < dst.x+dst.w && y < dst.y+dst.h);
}

inline bool in_rect(RECT &dst, RECT &src)
{
  return(src.x >= dst.x && src.y >= dst.y && 
         src.x < dst.x+dst.w && src.y < dst.y+dst.h);
}

// ------------------------------------------------------------
// Game input info
// ------------------------------------------------------------

typedef struct event_key {

  LEVEL_EVENT e1;
  LEVEL_EVENT e2;

  KEYTYPE     key;      // active key

  bool        alt;
  bool        ctrl;
  bool        shift;

  int         flag;
  int         group;    // group flag

} EVENT_KEY;

typedef struct event_key_set {

  EVENT_KEY *p_keys;
  int        keynum;

} EVENT_KEY_SET;

// -------------------------------------------------------
// Mouse UI
// -------------------------------------------------------

#define NO_BUTTON             0
#define MOUSE_BUTTONS         6

#define BUTTON_LEFT           1
#define BUTTON_MIDDLE         2
#define BUTTON_RIGHT          3
#define WHEEL_UP              4
#define WHEEL_DOWN            5

#define MASK_BUTTON_LEFT      0x02
#define MASK_BUTTON_MIDDLE    0x04
#define MASK_BUTTON_RIGHT     0x08
#define MASK_WHEEL_UP         0x10
#define MASK_WHEEL_DOWN       0x20

typedef enum { 

  BUTTON_NONE = 0,
  BUTTON_DOWN,
  BUTTON_UP,

} MOUSE_BUTTON_STATE;


typedef class mouse_state {

public:

  RECT                rect;
  MOUSE_BUTTON_STATE  button[MOUSE_BUTTONS];
  int                 key;

public:
  
  bool in_rect(RECT dst)
  {
    return(::in_rect(dst, rect.x, rect.y));
  }

public:

  mouse_state(void)
  {
    memset(this,0,sizeof(*this));
  }

  mouse_state(RECT r)
  {
    rect = r;
    memset(button,0,sizeof(button[0])*MOUSE_BUTTONS);
  }

  mouse_state(RECT r, int buttons, int key_ = 0)
  {
    rect = r;
  
    int i;
    for(i = 0; i < MOUSE_BUTTONS; i++)
      button[i] = (0x1&(buttons >> i)) ? BUTTON_DOWN : BUTTON_NONE;
    
    key = key_;
  }

  mouse_state(RECT r, int buttons, MOUSE_BUTTON_STATE state)
  {
    rect = r;
    int i;
    for(i = 0; i < MOUSE_BUTTONS; i++)
      button[i] = (0x1&(buttons >> i)) ? state : BUTTON_NONE;
  }

} MOUSE_STATE;

// Activators of events
#define MEVENT_MOUSE_BUTTONS          0x01
#define MEVENT_MOUSE_IN               0x02
#define MEVENT_MOUSE_OUT              0x04
#define MEVENT_KEY                    0x08

// Modificators of events
#define MEVENT_ACTIVATE_ONCE          0x10

// Run external evet instead of the given one(s)
#define MEVENT_MOUSE_EXTERNAL         0x20

#define MEVENTS                       3

typedef struct mouse_event : public llist_item 
{
  
  MOUSE_STATE        mstate;
  int                flag;
  int                event_num;
  LEVEL_EVENT        event[MEVENTS];
  bool               last_state;

  mouse_event(void)
  {
  }

  mouse_event(MOUSE_STATE state, int flg, int handle)
  {
    mstate = state;
    flag = flg;    
    last_state = FALSE;
    event_num = 0;
    event[0].params_set(ET(INT_TO_POINTER(handle)));
  }

  mouse_event(MOUSE_STATE state, int flg, LEVEL_EVENT ev)
  {
    mstate = state;
    flag = flg;
    event[0] = ev;
    event_num = 1;
    last_state = FALSE;
  }

  mouse_event(MOUSE_STATE state, int flg, LEVEL_EVENT ev, LEVEL_EVENT ev1)
  {
    mstate = state;
    flag = flg;
    event[0] = ev;
    event[1] = ev1;
    event_num = 2;
    last_state = FALSE;
  }

  mouse_event(MOUSE_STATE state, int flg, LEVEL_EVENT ev, LEVEL_EVENT ev1, LEVEL_EVENT ev2)
  {    
    mstate = state;
    flag = flg;
    event[0] = ev;
    event[1] = ev1;
    event[2] = ev2;
    event_num = 3;
    last_state = FALSE;
  }

  void state_clear(void)
  {
    last_state = FALSE;
  }

} MOUSE_EVENT;

// ------------------------------------------------------------
// Basic game input class
// ------------------------------------------------------------
#define INPUT_BLOCK_SETS       0x1   // Block all key-sets (is captured by console)                                       
#define INPUT_EVENT_LOOP_WAIT  0x2   // wait for events

typedef class input {  

  LEVEL_EVENT_QUEUE input_queue;     // All input events come there

  EVENT_KEY_SET    *p_set;           // Active event-set for keys
  int               group;           // global group flag

  MOUSE_STATE       mstate;          // Last mouse state

  LLIST_HEAD        mevents;

  int               flag;            // current input-interface flags

  bool              key_state[KEYNUM]; // neutral keys held right now
  bool              key_repeat_enabled;

  unsigned int      group_events_sent; // events sent by keys of a group (moves)

private:

  void key_block(int group_mask, bool block);

  void events_game(LEVEL_EVENT_QUEUE *p_queue);
  
public:
  
  input(void) : p_set(NULL), group(0), flag(0), key_repeat_enabled(false),
                group_events_sent(0)
  {
    memset(key_state, 0, sizeof(key_state));
  };

  bool key_status(KEYTYPE key);

  // Key repeat (the editor uses it). Repeated key-down events are ignored
  // by the input backend when it's off.
  void key_repeat(bool state);
  bool key_repeat_get(void)
  {
    return(key_repeat_enabled);
  }

  // Block/Unblock all input
  void block(bool state);
  
  // General interface for all events  
  void events_loop(LEVEL_EVENT_QUEUE *p_queue);
  void events_wait(bool state);
  
  // Keyboard interface
  void keyset_set(EVENT_KEY_SET *p_keyset);
  EVENT_KEY_SET * keyset_get(void)
  {
    return(p_set);
  }
  // A neutral key (K_xxx) was pressed / released by any device.
  // modification is a bit mask of K_xxx_MASK modifiers that are held.
  void key_input(KEYTYPE key, KEYMOD modification, bool pressed);
  void key_add(LEVEL_EVENT_QUEUE *p_queue);

  // Grows whenever a held key of a group (a move) sent its event - a key
  // can be held just until the game took it (touch swipes)
  unsigned int group_events_sent_get(void)
  {
    return(group_events_sent);
  }

  // Mouse interface
  void mouse_input(tpos mx, tpos my, MOUSE_BUTTON_STATE state, int button);

  void mevent_state_clear(MOUSE_EVENT *p_first = NULL);
  void mevent_clear(void);
  
  MOUSE_EVENT * mevent_add(MOUSE_EVENT event);
  MOUSE_EVENT * mevent_add(MOUSE_EVENT *p_event, int num);
  void          mevent_remove(MOUSE_EVENT *p_first, int num);
  
  LLIST_HEAD  * mevents_get(void)
  {
    return(&mevents);
  }
  
  MOUSE_STATE * mouse_state_get(void)
  {
    return(&mstate);
  }

} INPUT;

extern EVENT_KEY_SET game_keys;
extern EVENT_KEY_SET suspend_keys;
extern EVENT_KEY_SET menu_keys;
extern EVENT_KEY_SET editor_keys;

bool key_to_ascii(int key, char *p_char);

#endif // __INPUT_H__
