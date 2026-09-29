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
 * The audio layer - see audio.h and docs/AUDIO.md
 */
#include <stdarg.h>
#include <time.h>

#include "berusky.h"
#include "audio.h"
#include "audio_backend.h"
#include "test_script.h"

GAME_AUDIO audio;

/*
 * Track lists of SOUND.C - the same numbers are in the data section of the
 * shipped BERUSKY.EXE (ok_hudby, hudby, ...)
 */
static const int tracks_menu[]    = { 4, 5, 6, 7, 8 };
static const int tracks_level[]   = { 0, 1, 2, 4, 3, 5, 6, 7, 8, 9, 10, 11, 12,
                                      13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                      23, 24, 25 };
static const int tracks_credits[] = { 0, 1, 2, 3 };
static const int tracks_outro[]   = { 27, 28, 27, 28, 29 };   // HUDBA_OUTRO_1..5
#define TRACK_INTRO 26

#define TRACKS(list) (list), (int)(sizeof(list)/sizeof((list)[0]))

static const char *sound_names[SOUND_NUM] = {
  "blue_flash", "green_flash", "blue_stone", "iron_stone", "steps_mud",
  "steps_marble", "steps_background", "menu_move", "menu_click", "bump",
  "unlock", "exit_open", "door_open", "push", "pickup", "explosion",
  "door_close", "level_done", "switch", "bar"
};

static const char *context_names[MUSIC_CONTEXT_NUM] = {
  "none", "menu", "level", "credits", "intro", "outro"
};

const char * sound_name(SOUND_ID id)
{
  return(id >= 0 && id < SOUND_NUM ? sound_names[id] : "?");
}

const char * music_context_name(MUSIC_CONTEXT ctx)
{
  return(ctx >= 0 && ctx < MUSIC_CONTEXT_NUM ? context_names[ctx] : "?");
}

// -------------------------------------------------------
// Shuffle bag of SOUND.C (hraj_hudbu_levelu() and the others)
// -------------------------------------------------------

music_bag::music_bag(const int *p_tracks_, int num_)
: p_tracks(p_tracks_), num(num_)
{
  assert(num > 0 && num <= MUSIC_TRACKS);
  reset();
}

void music_bag::reset(void)
{
  for(int i = 0; i < MUSIC_TRACKS; i++)
    played[i] = FALSE;
}

int music_bag::next(unsigned random_number)
{
  int i = (int)(random_number % (unsigned)num);
  int j = 0;

  while(played[i]) {
    if(++i >= num)
      i = 0;
    j++;
    if(j >= num) {
      // all of them were played - start again (at the first one)
      i = 0;
      for(j = 0; j < num; j++)
        played[j] = FALSE;
    }
  }

  played[i] = TRUE;
  return(p_tracks[i]);
}

// -------------------------------------------------------
// The audio layer
// -------------------------------------------------------

game_audio::game_audio(void)
: p_backend(NULL),
  sound_on(TRUE), music_on(TRUE), sound_vol(86), music_vol(23),
  voice_next(0),
  context(MUSIC_NONE), track(MUSIC_NO_TRACK),
  pending_context(MUSIC_NONE), pending_param(0), pending_tick(0),
  bag_menu(TRACKS(tracks_menu)),
  bag_level(TRACKS(tracks_level)),
  bag_credits(TRACKS(tracks_credits)),
  random_state(1), ticks(0), logging(FALSE)
{
  sound_dir[0] = music_dir[0] = '\0';
  for(int i = 0; i < SOUND_NUM; i++) {
    p_samples[i] = NULL;
    sample_length[i] = 0;
    sample_missing_logged[i] = FALSE;
  }
  for(int i = 0; i < MUSIC_TRACKS; i++)
    track_missing_logged[i] = FALSE;
  for(int i = 0; i < AUDIO_VOICES; i++) {
    voice_sound[i] = -1;
    voice_stop[i] = 0;
  }
}

game_audio::~game_audio(void)
{
  shutdown();
}

void game_audio::log(const char *p_text, ...)
{
  if(!logging)
    return;

  char    line[300];
  va_list arguments;

  int len = snprintf(line, sizeof(line), "%6ld ", ticks);
  va_start(arguments, p_text);
  vsnprintf(line+len, sizeof(line)-len, p_text, arguments);
  va_end(arguments);

  event_log += line;
  event_log += '\n';
}

// random() of the DOS game is replaced by a small generator of our own:
// the same sequence on every platform, fixed seed in the tests
unsigned game_audio::rand_next(void)
{
  // xorshift32
  unsigned x = random_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  random_state = x;
  return(x >> 8);
}

/*
 * Volumes are percent of full volume. The DOS game used the MIDAS scale
 * 0-64 with the defaults 55 (sound) and 15 (music): 86 % and 23 %.
 */
#define INI_SOUND           "sound"
#define INI_MUSIC           "music"
#define INI_SOUND_VOLUME    "sound_volume"
#define INI_MUSIC_VOLUME    "music_volume"
#define DEFAULT_SOUND_VOLUME 86
#define DEFAULT_MUSIC_VOLUME 23

static int volume_clamp(int volume)
{
  return(volume < 0 ? 0 : (volume > 100 ? 100 : volume));
}

void game_audio::settings_load(const char *p_ini_file)
{
  sound_on = ini_read_bool_file(p_ini_file, INI_SOUND, TRUE);
  music_on = ini_read_bool_file(p_ini_file, INI_MUSIC, TRUE);
  sound_vol = volume_clamp(ini_read_int_file(p_ini_file, INI_SOUND_VOLUME, DEFAULT_SOUND_VOLUME));
  music_vol = volume_clamp(ini_read_int_file(p_ini_file, INI_MUSIC_VOLUME, DEFAULT_MUSIC_VOLUME));
}

void game_audio::settings_save(const char *p_ini_file)
{
  char tmp[20];
  ini_write_string(p_ini_file, INI_SOUND, sound_on ? "yes" : "no");
  ini_write_string(p_ini_file, INI_MUSIC, music_on ? "yes" : "no");
  ini_write_string(p_ini_file, INI_SOUND_VOLUME, my_itoa(10, tmp, sound_vol));
  ini_write_string(p_ini_file, INI_MUSIC_VOLUME, my_itoa(10, tmp, music_vol));
}

bool game_audio::sample_load(int id)
{
  char   file[MAX_FILENAME];
  t_off  bytes = 0;

  snprintf(file, sizeof(file), "smp_%03d.raw", id);
  char *p_data = (char *)file_load(sound_dir, file, &bytes, 0, FALSE);
  if(!p_data || bytes < 2) {
    if(p_data)
      ffree(p_data);
    if(!sample_missing_logged[id]) {
      bprintf("Audio: sound %s (%s/%s) is missing, it'll be silent", sound_name((SOUND_ID)id), sound_dir, file);
      sample_missing_logged[id] = TRUE;
    }
    return(FALSE);
  }

  // 16-bit signed little endian
  int    samples = (int)(bytes / 2);
  short *p_pcm = (short *)p_data;
  for(int i = 0; i < samples; i++) {
    Uint16 v;
    memcpy(&v, p_data + i*2, 2);
    p_pcm[i] = (short)SDL_Swap16LE(v);
  }

  p_samples[id] = p_pcm;
  sample_length[id] = samples;
  return(TRUE);
}

void game_audio::init(const char *p_ini_file, const char *p_sound_dir, const char *p_music_dir,
                      audio_backend *p_backend_)
{
  shutdown();

  settings_load(p_ini_file);

  snprintf(sound_dir, sizeof(sound_dir), "%s", p_sound_dir);
  snprintf(music_dir, sizeof(music_dir), "%s", p_music_dir);

  // The tests need the same track choice and a log of what was played
  bool test = test_script_active();
  random_seed(test ? 1 : (unsigned)(SDL_GetTicks() ^ (Uint64)time(NULL)));
  logging = test;

  for(int i = 0; i < SOUND_NUM; i++)
    sample_load(i);

  if(p_backend_) {
    p_backend = p_backend_;
  } else if((test && !SDL_getenv("BERUSKY_TEST_AUDIO")) || SDL_getenv("BERUSKY_NO_AUDIO")) {
    // Test scripts run without a sound device - nothing depends on its
    // timing (BERUSKY_TEST_AUDIO=1 plays it anyway)
    p_backend = audio_backend_null_create();
  } else {
    p_backend = audio_backend_sdl_create();
  }

  if(!p_backend->open()) {
    bprintf("Audio: no audio output, the game is silent");
    delete p_backend;
    p_backend = audio_backend_null_create();
    p_backend->open();
  }

  gains_update();

  bprintf("Audio: sound %s (%d %%), music %s (%d %%)",
          sound_on ? "on" : "off", sound_vol, music_on ? "on" : "off", music_vol);
}

void game_audio::shutdown(void)
{
  if(p_backend) {
    p_backend->music_stop();
    for(int i = 0; i < AUDIO_VOICES; i++)
      p_backend->voice_stop(i);
    p_backend->close();
    delete p_backend;
    p_backend = NULL;
  }

  for(int i = 0; i < SOUND_NUM; i++) {
    if(p_samples[i]) {
      ffree(p_samples[i]);
      p_samples[i] = NULL;
    }
    sample_length[i] = 0;
  }

  context = MUSIC_NONE;
  track = MUSIC_NO_TRACK;
  pending_context = MUSIC_NONE;
}

void game_audio::gains_update(void)
{
  if(p_backend) {
    p_backend->voice_gain_set(sound_vol / 100.0f);
    p_backend->music_volume_set(music_vol);
  }
}

void game_audio::sound_enable(bool state)
{
  sound_on = state;
  if(!sound_on)
    sounds_stop();
  log("sound %s", state ? "on" : "off");
}

void game_audio::music_enable(bool state)
{
  if(music_on == state)
    return;

  music_on = state;
  log("music %s", state ? "on" : "off");

  if(!music_on) {
    if(p_backend)
      p_backend->music_stop();
  } else if(context != MUSIC_NONE && track != MUSIC_NO_TRACK) {
    // the music of the situation we're in
    track_play(context, track);
  }
}

void game_audio::sound_volume_set(int volume)
{
  sound_vol = volume_clamp(volume);
  gains_update();
}

void game_audio::music_volume_set(int volume)
{
  music_vol = volume_clamp(volume);
  gains_update();
}

void game_audio::tick(void)
{
  ticks++;

  for(int i = 0; i < AUDIO_VOICES; i++) {
    if(voice_stop[i] && ticks >= voice_stop[i]) {
      voice_stop[i] = 0;
      log("cut voice %d", i);
      if(p_backend)
        p_backend->voice_stop(i);
    }
  }

  if(pending_context != MUSIC_NONE && ticks >= pending_tick) {
    MUSIC_CONTEXT ctx = pending_context;
    pending_context = MUSIC_NONE;
    music_start(ctx, pending_param);
  }
}

/*
 * hraj_sampl() + updatuj_samply() of SOUND.C: the next of the 8 effect
 * channels (MIDAS assigns them round robin, the priority is only stored)
 * gets the sound, it's cut after the given length.
 */
void game_audio::sound(SOUND_ID id, int length, int priority)
{
  if(id < 0 || id >= SOUND_NUM)
    return;

  if(!sound_on) {
    log("sound %s off", sound_name(id));
    return;
  }

  if(!p_samples[id]) {
    log("sound %s missing", sound_name(id));
    return;
  }

  int voice = voice_next;
  voice_next = (voice_next + 1) % AUDIO_VOICES;

  voice_sound[voice] = id;
  voice_stop[voice] = length > 0 ? ticks + length : 0;

  log("sound %s voice %d length %d priority %d", sound_name(id), voice, length, priority);

  if(p_backend)
    p_backend->voice_play(voice, p_samples[id], sample_length[id], SOUND_RATE);
}

void game_audio::sounds_stop(void)
{
  for(int i = 0; i < AUDIO_VOICES; i++) {
    voice_stop[i] = 0;
    if(p_backend)
      p_backend->voice_stop(i);
  }
}

music_bag * game_audio::music_bag_get(MUSIC_CONTEXT ctx)
{
  switch(ctx) {
    case MUSIC_MENU:
      return(&bag_menu);
    case MUSIC_LEVEL:
      return(&bag_level);
    case MUSIC_CREDITS:
      return(&bag_credits);
    default:
      return(NULL);
  }
}

int game_audio::music_outro_track(int level_set)
{
  int num = (int)(sizeof(tracks_outro)/sizeof(tracks_outro[0]));
  return(level_set >= 0 && level_set < num ? tracks_outro[level_set] : MUSIC_NO_TRACK);
}

/*
 * hraj_modul() of SOUND.C: loads x_NNN.xm and plays it in a loop. Starting
 * a module reallocated the MIDAS channels, which silenced all sound effects
 * - kept, the level-done sound ends that way when the menu music starts.
 * Nothing happens with the music switched off (the track is still chosen).
 */
void game_audio::track_play(MUSIC_CONTEXT ctx, int new_track)
{
  context = ctx;
  track = new_track;

  log("music %s track %d%s", music_context_name(ctx), new_track, music_on ? "" : " (music off)");

  if(!music_on || !p_backend || new_track < 0 || new_track >= MUSIC_TRACKS)
    return;

  p_backend->music_stop();

  char  file[MAX_FILENAME];
  t_off size = 0;
  snprintf(file, sizeof(file), "x_%03d.xm", new_track);
  void *p_data = file_load(music_dir, file, &size, 0, FALSE);

  if(!p_data) {
    if(!track_missing_logged[new_track]) {
      bprintf("Audio: music %s/%s is missing", music_dir, file);
      track_missing_logged[new_track] = TRUE;
    }
    return;
  }

  sounds_stop();

  if(!p_backend->music_play(p_data, size))
    bprintf("Audio: can't play %s/%s", music_dir, file);

  ffree(p_data);
}

void game_audio::music_start(MUSIC_CONTEXT ctx, int param)
{
  int new_track = MUSIC_NO_TRACK;

  switch(ctx) {
    case MUSIC_MENU:
    case MUSIC_LEVEL:
    case MUSIC_CREDITS:
      new_track = music_bag_get(ctx)->next(rand_next());
      break;
    case MUSIC_INTRO:
      new_track = TRACK_INTRO;
      break;
    case MUSIC_OUTRO:
      new_track = music_outro_track(param);
      break;
    default:
      music_stop();
      return;
  }

  track_play(ctx, new_track);
}

void game_audio::music_menu(void)
{
  // a menu opened before a delayed start came - the menu music now
  pending_context = MUSIC_NONE;

  if(context != MUSIC_MENU)
    music_start(MUSIC_MENU, 0);
}

void game_audio::music_menu_after_level(void)
{
  pending_context = MUSIC_MENU;
  pending_param = 0;
  pending_tick = ticks + MUSIC_TICKS_AFTER_LEVEL;
}

void game_audio::music_level(void)
{
  pending_context = MUSIC_NONE;
  music_start(MUSIC_LEVEL, 0);
}

void game_audio::music_credits(void)
{
  pending_context = MUSIC_NONE;
  music_start(MUSIC_CREDITS, 0);
}

void game_audio::music_outro(int level_set)
{
  if(music_outro_track(level_set) == MUSIC_NO_TRACK)
    return;

  pending_context = MUSIC_OUTRO;
  pending_param = level_set;
  pending_tick = ticks + MUSIC_TICKS_AFTER_LEVEL;
}

void game_audio::music_stop(void)
{
  pending_context = MUSIC_NONE;

  if(context != MUSIC_NONE)
    log("music stop");

  context = MUSIC_NONE;
  track = MUSIC_NO_TRACK;

  if(p_backend)
    p_backend->music_stop();
}

bool game_audio::event_process(level_event *p_event)
{
  switch(p_event->action_get()) {
    case SN_PLAY_SAMPLE:
      sound((SOUND_ID)p_event->param_int_get(PARAM_0),
            p_event->param_int_get(PARAM_1),
            p_event->param_int_get(PARAM_2));
      return(TRUE);

    case SN_PLAY_MUSIC:
      switch((MUSIC_CONTEXT)p_event->param_int_get(PARAM_0)) {
        case MUSIC_MENU:
          music_menu();
          break;
        case MUSIC_LEVEL:
          music_level();
          break;
        case MUSIC_CREDITS:
          music_credits();
          break;
        case MUSIC_OUTRO:
          music_outro(p_event->param_int_get(PARAM_1));
          break;
        default:
          music_stop();
          break;
      }
      return(TRUE);

    case SN_STOP_MUSIC:
      music_stop();
      return(TRUE);

    default:
      return(FALSE);
  }
}

// -------------------------------------------------------
// No output
// -------------------------------------------------------

class audio_backend_null : public audio_backend {

public:

  bool open(void) { return(TRUE); }
  void close(void) {}
  void voice_play(int voice, const short *p_pcm, int samples, int rate) {}
  void voice_stop(int voice) {}
  void voice_gain_set(float gain) {}
  bool music_play(const void *p_data, size_t size) { return(TRUE); }
  void music_stop(void) {}
  void music_volume_set(int volume) {}
};

AUDIO_BACKEND * audio_backend_null_create(void)
{
  return(new audio_backend_null);
}
