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
 * Sound effects and music - the behavior of the DOS original (SOUND.C,
 * see docs/AUDIO.md and docs/DOS_ORIGINAL.md) without its MIDAS backend.
 *
 *   game logic / menus
 *        |  SN_ events (events.h) or the calls below
 *        v
 *   GAME_AUDIO   - sound ids, music categories + track selection,
 *        |         8 effect voices (round robin, cut after a length),
 *        |         on/off + volumes, the game tick clock, event log
 *        v
 *   AUDIO_BACKEND (audio_backend.h) - SDL3 audio streams + libxmp,
 *                  or nothing (tests, no device)
 *
 * Everything here is driven by the 30 Hz game tick (tick()), never by the
 * wall clock, so it doesn't change the game's timing and the event log is
 * the same in every run of a test script.
 */
#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <string>

class audio_backend;
class level_event;

// The DOS sample numbers (SOUND.H) = data/Sound/smp_NNN.raw
typedef enum {
  SOUND_BLUE_FLASH = 0,     // MODRY_BLESK    - cyber door closes behind a bug
  SOUND_GREEN_FLASH,        // ZELENY_BLESK   - not used by the DOS game
  SOUND_BLUE_STONE,         // MODRY_KAMEN    - stone (variation 1) broken
  SOUND_IRON_STONE,         // ZELEZNY_KAMEN  - stone (variation 0) broken
  SOUND_STEPS_MUD,          // KROKY_BLATO    - step onto floor variation < 5
  SOUND_STEPS_MARBLE,       // KROKY_MRAMOR   - step onto floor variation >= 5
  SOUND_STEPS_BACKGROUND,   // KROKY_POZADI   - step onto a cell without floor
  SOUND_MENU_MOVE,          // MENU_SKOK      - another menu item highlighted
  SOUND_MENU_CLICK,         // MENU_KLIK      - menu item chosen
  SOUND_BUMP,               // NARAZ          - not used by the DOS game
  SOUND_UNLOCK,             // ODEMYK         - color door unlocked
  SOUND_EXIT_OPEN,          // OTEVRENI_EXITU - the fifth key taken
  SOUND_DOOR_OPEN,          // OTEVRENI_DVERI - classic color door opens
  SOUND_PUSH,               // POSUN          - box pushed
  SOUND_PICKUP,             // SEBRANI        - key / pickax / color key taken
  SOUND_EXPLOSION,          // VYBUCH         - explosive + box
  SOUND_DOOR_CLOSE,         // ZAVRENI        - classic door closes
  SOUND_LEVEL_DONE,         // SAMPL_OK       - bug in the open exit
  SOUND_SWITCH,             // PREPNI         - another bug selected
  SOUND_BAR,                // LISTA          - not used by the DOS game
  SOUND_NUM
} SOUND_ID;

/*
 * How long a sound may play (it's cut then), in game ticks (30 Hz). The DOS
 * game counted 60 Hz display ticks; FPS there was one second.
 */
#define SOUND_TICKS_SECOND      30   // DELKA_* = FPS
#define SOUND_TICKS_MENU         9   // TPS = 18 ticks - the menus of MENU.C
#define SOUND_TICKS_STONE       18   // DELKA_*_KAMEN = 9*4
#define SOUND_TICKS_EXPLOSION   24   // DELKA_VYBUCH = 48
#define SOUND_TICKS_LEVEL_DONE  60   // DELKA_SAMPL_OK = 2*FPS
// Steps and pushing last one step: DELKA_KROKY_2 / _1 = 20 / 10 DOS ticks,
// the same as the move animation of the port
#define SOUND_TICKS_STEP        (ANIM_MOVE_FRAMES+1)
#define SOUND_TICKS_STEP_FAST   (ANIM_MOVE_FRAMES_FAST+1)

// Priorities of SOUND.H. MIDAS stored them but its auto channels are
// assigned round robin (see docs), so they don't change anything; they're
// kept for the log and the documentation.
#define SOUND_PRIORITY_STEPS     0
#define SOUND_PRIORITY_MENU      1
#define SOUND_PRIORITY_SWITCH    1
#define SOUND_PRIORITY_PUSH      1
#define SOUND_PRIORITY_UNLOCK    1
#define SOUND_PRIORITY_DOOR      2   // PRIORITA_ZAVRENI, OTEVRENI_*, MODRY_KAMEN
#define SOUND_PRIORITY_PICKUP    3   // SEBRANI, VYBUCH, SAMPL_OK
#define SOUND_PRIORITY_IRON_STONE 36 // the DOS call passes the length as priority

// Effect voices: the 8 MIDAS auto effect channels (SAMPL_KANALU)
#define AUDIO_VOICES             8
// Sample rate of the effects (MIDASplaySample RATE)
#define SOUND_RATE           20050

typedef enum {
  MUSIC_NONE = 0,
  MUSIC_MENU,           // hraj_hudbu_menu()    - tracks 4-8
  MUSIC_LEVEL,          // hraj_hudbu_levelu()  - tracks 0-25
  MUSIC_CREDITS,        // hraj_hudbu_credits() - tracks 0-3
  MUSIC_INTRO,          // hraj_hudbu_intro()   - track 26
  MUSIC_OUTRO,          // hraj_hudbu_outro()   - 27, 28, 27, 28, 29
  MUSIC_CONTEXT_NUM
} MUSIC_CONTEXT;

#define MUSIC_TRACKS            30
#define MUSIC_NO_TRACK          (-1)

// The DOS game waited 30 ticks of its 18.2 Hz clock (cekej(30)) after a
// solved level before the menu music started - SAMPL_OK plays meanwhile
#define MUSIC_TICKS_AFTER_LEVEL  50

/*
 * Track choice of SOUND.C: a random track of the list that wasn't played
 * yet; when all were played, the bag is emptied and - a quirk of the DOS
 * loop - the first track of the list comes next.
 */
class music_bag {

  const int *p_tracks;
  int        num;
  bool       played[MUSIC_TRACKS];

public:

  music_bag(const int *p_tracks_, int num_);

  // random_number = a random number (the DOS random())
  int  next(unsigned random_number);
  void reset(void);
  int  size(void) { return(num); }
  int  track(int i) { return(p_tracks[i]); }
};

typedef class game_audio {

  audio_backend *p_backend;

  // settings
  bool      sound_on;
  bool      music_on;
  int       sound_vol;         // 0 - 100 %
  int       music_vol;

  // data
  char      sound_dir[1000];
  char      music_dir[1000];
  short    *p_samples[SOUND_NUM];
  int       sample_length[SOUND_NUM];     // in samples
  bool      sample_missing_logged[SOUND_NUM];
  bool      track_missing_logged[MUSIC_TRACKS];

  // effect voices
  int       voice_next;
  int       voice_sound[AUDIO_VOICES];
  long      voice_stop[AUDIO_VOICES];     // tick to cut the sound at, 0 = none

  // music
  MUSIC_CONTEXT context;
  int       track;                        // playing (or selected) track
  MUSIC_CONTEXT pending_context;          // delayed start
  int       pending_param;
  long      pending_tick;

  music_bag bag_menu;
  music_bag bag_level;
  music_bag bag_credits;

  unsigned  random_state;
  long      ticks;

  bool      logging;
  std::string event_log;

private:

  void      log(const char *p_text, ...);
  unsigned  rand_next(void);
  void      track_play(MUSIC_CONTEXT ctx, int track);
  void      music_start(MUSIC_CONTEXT ctx, int param);
  bool      sample_load(int id);
  void      gains_update(void);

public:

  game_audio(void);
  ~game_audio(void);

  // Reads the settings (sound, music, sound_volume, music_volume) and the
  // samples, opens the backend (NULL = the SDL backend; tests pass their own)
  void      init(const char *p_ini_file, const char *p_sound_dir, const char *p_music_dir,
                 audio_backend *p_backend = NULL);
  void      shutdown(void);

  // Settings (saved to the config file by settings_save())
  bool      sound_enabled(void) { return(sound_on); }
  bool      music_enabled(void) { return(music_on); }
  int       sound_volume(void)  { return(sound_vol); }
  int       music_volume(void)  { return(music_vol); }
  void      sound_enable(bool state);
  void      music_enable(bool state);
  void      sound_volume_set(int volume);
  void      music_volume_set(int volume);
  void      settings_load(const char *p_ini_file);
  void      settings_save(const char *p_ini_file);

  // One game tick (30 Hz) - cuts sounds, starts delayed music
  void      tick(void);
  long      ticks_get(void) { return(ticks); }

  // Sound effect; length = ticks until it's cut (0 = the whole sample)
  void      sound(SOUND_ID id, int length = SOUND_TICKS_SECOND, int priority = SOUND_PRIORITY_MENU);
  void      sounds_stop(void);

  // Music of a situation of the game (see docs/AUDIO.md)
  void      music_menu(void);             // menu music unless it's playing
  void      music_menu_after_level(void); // menu music MUSIC_TICKS_AFTER_LEVEL later
  void      music_level(void);            // a new level track (level start, N)
  void      music_credits(void);
  void      music_outro(int level_set);   // episode end, after MUSIC_TICKS_AFTER_LEVEL
  void      music_stop(void);

  MUSIC_CONTEXT music_context(void) { return(pending_context != MUSIC_NONE ? pending_context : context); }
  int       music_track(void) { return(track); }

  // Track lists of SOUND.C
  static int music_outro_track(int level_set);
  music_bag *music_bag_get(MUSIC_CONTEXT ctx);

  // SN_ events (events.h). Returns true when the event was an audio one.
  bool      event_process(level_event *p_event);

  // Log of all requests - for the regression tests
  void      log_enable(bool state) { logging = state; }
  const std::string & log_get(void) { return(event_log); }
  void      log_clear(void) { event_log.clear(); }

  // Deterministic track choice (tests)
  void      random_seed(unsigned seed) { random_state = seed ? seed : 1; }

} GAME_AUDIO;

extern GAME_AUDIO audio;

const char * sound_name(SOUND_ID id);
const char * music_context_name(MUSIC_CONTEXT ctx);

#endif // __AUDIO_H__
