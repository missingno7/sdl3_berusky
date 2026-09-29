/*
 * Berusky (C) AnakreoN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Unit test of the languages (src/lang.h), the Czech texts and data files,
 * the audio settings and the audio layer (src/audio.h) - the latter with a
 * backend that only records what it was asked to do, so the test needs no
 * sound device and doesn't depend on timing.
 *
 *   ctest --test-dir build   (or run berusky_audio_lang_test directly)
 */

#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>

#include "berusky.h"
#include "audio_backend.h"

static char test_ini[MAX_FILENAME];

// Provided by the game's main.cpp
const char * config_file(bool)
{
  return(test_ini);
}

static int failures = 0;

#define CHECK(cond) \
  do { if(!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } } while(0)

#define CHECK_STR(a, b) \
  do { if(strcmp((a), (b))) { printf("FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); failures++; } } while(0)

static void write_file(const char *p_file, const char *p_text)
{
  SDL_IOStream *p_io = SDL_IOFromFile(p_file, "wb");
  if(p_io) {
    SDL_WriteIO(p_io, p_text, strlen(p_text));
    SDL_CloseIO(p_io);
  }
}

static std::string read_file(const char *p_file)
{
  size_t size = 0;
  void  *p_data = SDL_LoadFile(p_file, &size);
  std::string s = p_data ? std::string((char *)p_data, size) : std::string();
  SDL_free(p_data);
  return(s);
}

/* -----------------------------------------------------------------------
   A backend that records the calls
   ----------------------------------------------------------------------- */
class recording_backend : public audio_backend {

public:

  struct play { int voice; int samples; int rate; };

  std::vector<play> plays;
  std::vector<int>  stops;
  int   music_plays = 0;
  int   music_stops = 0;
  int   music_volume = -1;
  float gain = -1;
  bool  closed = false;

  bool open(void) { return(TRUE); }
  void close(void) { closed = TRUE; }
  void voice_play(int voice, const short *p_pcm, int samples, int rate)
  {
    plays.push_back({ voice, samples, rate });
  }
  void voice_stop(int voice) { stops.push_back(voice); }
  void voice_gain_set(float g) { gain = g; }
  bool music_play(const void *p_data, size_t size) { music_plays++; return(size > 0); }
  void music_stop(void) { music_stops++; }
  void music_volume_set(int volume) { music_volume = volume; }
};

static const int level_tracks[26] = { 0, 1, 2, 4, 3, 5, 6, 7, 8, 9, 10, 11, 12,
                                      13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                      23, 24, 25 };

static bool in_list(int track, const int *p_list, int num)
{
  for(int i = 0; i < num; i++)
    if(p_list[i] == track)
      return(TRUE);
  return(FALSE);
}

/* -----------------------------------------------------------------------
   Languages
   ----------------------------------------------------------------------- */
static void test_languages(void)
{
  // English is the texts of the code
  lang_set(LANGUAGE_EN);
  CHECK_STR(_("play"), "play");
  CHECK_STR(_("steps: %d"), "steps: %d");
  CHECK_STR(_("no such text"), "no such text");

  // Czech - the wording of the DOS game
  lang_set(LANGUAGE_CS);
  CHECK(lang_get() == LANGUAGE_CS);
  CHECK_STR(_("play"), "Nová hra");
  CHECK_STR(_("quit"), "Konec");
  CHECK_STR(_("sound"), "Zvuky");
  CHECK_STR(_("music"), "Hudba");
  CHECK_STR(_("training"), "Trénink");
  CHECK_STR(_("impossible"), "Nemožná");
  CHECK_STR(_("steps: %d"), "celkem kroků: %d");
  CHECK_STR(_("restart level (CTRL+R)"), "Restart levelu (CTRL+R)");
  // not translated -> the English text
  CHECK_STR(_("no such text"), "no such text");
  CHECK_STR(_("F1"), "F1");

  // and back
  lang_set(LANGUAGE_EN);
  CHECK_STR(_("play"), "play");

  // N_() only marks, the text is translated where it's used
  static const char *p_table[] = { N_("easy") };
  lang_set(LANGUAGE_CS);
  CHECK_STR(_(p_table[0]), "Lehká");
  lang_set(LANGUAGE_EN);

  // codes / names
  LANGUAGE l;
  CHECK(lang_from_code("en", &l) && l == LANGUAGE_EN);
  CHECK(lang_from_code("EN", &l) && l == LANGUAGE_EN);
  CHECK(lang_from_code("cs", &l) && l == LANGUAGE_CS);
  CHECK(lang_from_code("cz", &l) && l == LANGUAGE_CS);
  CHECK(lang_from_code("czech", &l) && l == LANGUAGE_CS);
  CHECK(lang_from_code("čeština", &l) && l == LANGUAGE_CS);
  CHECK(!lang_from_code("auto", &l));
  CHECK(!lang_from_code("de", &l));
  CHECK(!lang_from_code(NULL, &l));
  CHECK_STR(lang_code(LANGUAGE_CS), "cs");
  CHECK_STR(lang_name(LANGUAGE_CS), "čeština");
  CHECK_STR(lang_name(LANGUAGE_EN), "english");
}

/* The table: valid UTF-8, only letters the font can draw, no duplicates */
static void test_czech_table(void)
{
  int num = lang_table_size(LANGUAGE_CS);
  CHECK(num > 200);
  CHECK(lang_table_size(LANGUAGE_EN) == 0);

  for(int i = 0; i < num; i++) {
    const char *p_en, *p_cs;
    CHECK(lang_table_entry(LANGUAGE_CS, i, &p_en, &p_cs));

    const char *p = p_cs;
    unsigned ch;
    while((ch = utf8_next(&p))) {
      ACCENT a;
      if(glyph_decompose(ch, &a) == '?' && ch != '?') {
        printf("FAIL entry %d (%s): character U+%04X can't be drawn\n", i, p_en, ch);
        failures++;
      }
    }

    for(int j = 0; j < i; j++) {
      const char *p_en2, *p_cs2;
      lang_table_entry(LANGUAGE_CS, j, &p_en2, &p_cs2);
      if(!strcmp(p_en, p_en2)) {
        printf("FAIL duplicate entry %s\n", p_en);
        failures++;
      }
    }
  }
}

/* UTF-8 and the Czech letters of the font (base letter + accent) */
static void test_utf8(void)
{
  const char *p_letters = "áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ";
  const char *p_bases   = "acdeeinorstuuyzACDEEINORSTUUYZ";
  const ACCENT accents[15] = { ACCENT_ACUTE, ACCENT_CARON, ACCENT_CARON, ACCENT_ACUTE,
                               ACCENT_CARON, ACCENT_ACUTE, ACCENT_CARON, ACCENT_ACUTE,
                               ACCENT_CARON, ACCENT_CARON, ACCENT_CARON, ACCENT_ACUTE,
                               ACCENT_RING, ACCENT_ACUTE, ACCENT_CARON };

  const char *p = p_letters;
  for(int i = 0; i < 30; i++) {
    unsigned ch = utf8_next(&p);
    ACCENT a;
    int base = glyph_decompose(ch, &a);
    CHECK(base == p_bases[i]);
    CHECK(a == accents[i % 15]);
  }
  CHECK(utf8_next(&p) == 0);

  // ASCII as it is, no accent
  ACCENT a;
  CHECK(glyph_decompose('A', &a) == 'A' && a == ACCENT_NONE);
  // something the font doesn't have
  CHECK(glyph_decompose(0x4e2d, &a) == '?');

  // multi-byte sequences
  const char *p_text = "Ž\xe2\x82\xac" "A";  // Ž, euro sign, A
  CHECK(utf8_next(&p_text) == 0x17d);
  CHECK(utf8_next(&p_text) == 0x20ac);
  CHECK(utf8_next(&p_text) == 'A');
  CHECK(utf8_next(&p_text) == 0);

  // broken sequences don't read past the end
  const char *p_broken = "\xc5";
  CHECK(utf8_next(&p_broken) == 0xc5);
  CHECK(utf8_next(&p_broken) == 0);
  const char *p_broken2 = "\xc5" "A";
  CHECK(utf8_next(&p_broken2) == 0xc5);
  CHECK(utf8_next(&p_broken2) == 'A');
}

/* language = ... in the config file */
static void test_language_config(const char *p_dir)
{
  char ini[MAX_FILENAME];
  snprintf(ini, sizeof(ini), "%s/lang.ini", p_dir);

  write_file(ini, "# a config of an older version\nfullscreen = 0\n");
  LANGUAGE l = lang_config_load(ini);
  CHECK(l == lang_system());        // missing key = auto

  CHECK(lang_config_save(ini, LANGUAGE_CS));
  CHECK(lang_config_load(ini) == LANGUAGE_CS);
  CHECK(read_file(ini).find("fullscreen = 0") != std::string::npos);   // the rest is kept

  CHECK(lang_config_save(ini, LANGUAGE_EN));
  CHECK(lang_config_load(ini) == LANGUAGE_EN);

  write_file(ini, "language = cz\n");
  CHECK(lang_config_load(ini) == LANGUAGE_CS);
  write_file(ini, "language = klingon\n");
  CHECK(lang_config_load(ini) == lang_system());

  // the environment wins
  SDL_setenv_unsafe("BERUSKY_LANGUAGE", "cs", 1);
  write_file(ini, "language = en\n");
  CHECK(lang_config_load(ini) == LANGUAGE_CS);
  SDL_unsetenv_unsafe("BERUSKY_LANGUAGE");
  CHECK(lang_config_load(ini) == LANGUAGE_EN);

  lang_set(LANGUAGE_EN);
}

/* The Czech data files: found for Czech, English otherwise, valid UTF-8 */
static void test_data_files(const char *p_data)
{
  char gamedata[MAX_FILENAME];
  char file[MAX_FILENAME];
  snprintf(gamedata, sizeof(gamedata), "%s/GameData", p_data);

  lang_set(LANGUAGE_EN);
  CHECK_STR(lang_data_file(gamedata, "hints.dat", file, sizeof(file)), "hints.dat");

  lang_set(LANGUAGE_CS);
  CHECK_STR(lang_data_file(gamedata, "hints.dat", file, sizeof(file)), "cs/hints.dat");
  CHECK_STR(lang_data_file(gamedata, "credits.dat", file, sizeof(file)), "cs/credits.dat");
  for(int i = 0; i < 5; i++) {
    char name[20];
    snprintf(name, sizeof(name), "end%d.dat", i);
    char expected[30];
    snprintf(expected, sizeof(expected), "cs/%s", name);
    CHECK_STR(lang_data_file(gamedata, name, file, sizeof(file)), expected);
  }
  // no Czech variant -> the English file
  CHECK_STR(lang_data_file(gamedata, "items.dat", file, sizeof(file)), "items.dat");
  lang_set(LANGUAGE_EN);

  // The hints of all 120 levels, in UTF-8 (the DOS TEXTY.TXT)
  char path[MAX_FILENAME];
  snprintf(path, sizeof(path), "%s/cs/hints.dat", gamedata);
  std::string hints = read_file(path);
  CHECK(!hints.empty());
  int marks = 0;
  for(size_t pos = 0; (pos = hints.find("\n~", pos)) != std::string::npos; pos++)
    marks++;
  CHECK(marks >= 120);
  CHECK(hints.find("První úroveň") != std::string::npos);
  CHECK(hints.find("Beruškách") != std::string::npos);

  const char *p = hints.c_str();
  unsigned ch;
  int czech = 0;
  while((ch = utf8_next(&p))) {
    ACCENT a;
    int base = glyph_decompose(ch, &a);
    if(ch > 127) {
      CHECK(base != '?');
      czech++;
    }
  }
  CHECK(czech > 1000);

  snprintf(path, sizeof(path), "%s/cs/credits.dat", gamedata);
  std::string credits = read_file(path);
  CHECK(credits.find("Stránský") != std::string::npos);
  CHECK(credits.find("hudba a zvuky") != std::string::npos);
}

/* -----------------------------------------------------------------------
   Audio settings
   ----------------------------------------------------------------------- */
static void test_audio_settings(const char *p_dir, const char *p_data)
{
  char ini[MAX_FILENAME], sound[MAX_FILENAME], music[MAX_FILENAME];
  snprintf(ini, sizeof(ini), "%s/audio.ini", p_dir);
  snprintf(sound, sizeof(sound), "%s/Sound", p_data);
  snprintf(music, sizeof(music), "%s/Music", p_data);

  // An old config without the new keys: the DOS defaults
  write_file(ini, "fullscreen = 0\nlog = 0\n");
  GAME_AUDIO a;
  recording_backend *p_b = new recording_backend;
  a.init(ini, sound, music, p_b);
  CHECK(a.sound_enabled());
  CHECK(a.music_enabled());
  CHECK(a.sound_volume() == 86);     // 55 of 64
  CHECK(a.music_volume() == 23);     // 15 of 64
  CHECK(p_b->music_volume == 23);
  CHECK(p_b->gain > 0.85f && p_b->gain < 0.87f);

  // Values out of range are clamped
  write_file(ini, "fullscreen = 0\nsound = no\nmusic = off\nsound_volume = 150\nmusic_volume = -20\n");
  a.settings_load(ini);
  CHECK(!a.sound_enabled());
  CHECK(!a.music_enabled());
  CHECK(a.sound_volume() == 100);
  CHECK(a.music_volume() == 0);

  a.sound_volume_set(1000);
  CHECK(a.sound_volume() == 100);
  a.music_volume_set(-1);
  CHECK(a.music_volume() == 0);

  // Round trip
  a.sound_enable(TRUE);
  a.music_enable(FALSE);
  a.sound_volume_set(40);
  a.music_volume_set(70);
  a.settings_save(ini);
  std::string text = read_file(ini);
  CHECK(text.find("fullscreen = 0") != std::string::npos);
  a.settings_load(ini);
  CHECK(a.sound_enabled());
  CHECK(!a.music_enabled());
  CHECK(a.sound_volume() == 40);
  CHECK(a.music_volume() == 70);

  write_file(ini, "sound = yes\nmusic = yes\nsound_volume = abc\n");
  a.settings_load(ini);
  CHECK(a.sound_volume() >= 0 && a.sound_volume() <= 100);

  a.shutdown();                 // (deletes the backend)
}

/* -----------------------------------------------------------------------
   Track lists and the shuffle bag of SOUND.C
   ----------------------------------------------------------------------- */
static void test_music_bags(void)
{
  GAME_AUDIO a;

  music_bag *p_level = a.music_bag_get(MUSIC_LEVEL);
  CHECK(p_level->size() == 26);
  for(int i = 0; i < 26; i++)
    CHECK(p_level->track(i) == level_tracks[i]);

  music_bag *p_menu = a.music_bag_get(MUSIC_MENU);
  CHECK(p_menu->size() == 5);
  for(int i = 0; i < 5; i++)
    CHECK(p_menu->track(i) == 4 + i);

  music_bag *p_credits = a.music_bag_get(MUSIC_CREDITS);
  CHECK(p_credits->size() == 4);
  for(int i = 0; i < 4; i++)
    CHECK(p_credits->track(i) == i);

  CHECK(GAME_AUDIO::music_outro_track(0) == 27);
  CHECK(GAME_AUDIO::music_outro_track(1) == 28);
  CHECK(GAME_AUDIO::music_outro_track(2) == 27);
  CHECK(GAME_AUDIO::music_outro_track(3) == 28);
  CHECK(GAME_AUDIO::music_outro_track(4) == 29);
  CHECK(GAME_AUDIO::music_outro_track(5) == MUSIC_NO_TRACK);

  // Every track once, then the first one of the list (the DOS loop)
  static const int tracks[] = { 10, 11, 12, 13, 14 };
  music_bag bag(tracks, 5);
  bool seen[5] = { false };
  unsigned r = 12345;
  for(int i = 0; i < 5; i++) {
    r = r * 1103515245 + 12345;
    int t = bag.next(r >> 8);
    CHECK(t >= 10 && t <= 14);
    CHECK(!seen[t - 10]);
    seen[t - 10] = true;
  }
  CHECK(bag.next(3) == 10);
  // the same random number takes the next unplayed one
  CHECK(bag.next(3) == 13);
  CHECK(bag.next(3) == 14);
}

/* -----------------------------------------------------------------------
   The audio layer
   ----------------------------------------------------------------------- */
static void test_audio_layer(const char *p_dir, const char *p_data)
{
  char ini[MAX_FILENAME], sound[MAX_FILENAME], music[MAX_FILENAME];
  snprintf(ini, sizeof(ini), "%s/layer.ini", p_dir);
  snprintf(sound, sizeof(sound), "%s/Sound", p_data);
  snprintf(music, sizeof(music), "%s/Music", p_data);
  write_file(ini, "sound = yes\nmusic = yes\n");

  GAME_AUDIO a;
  recording_backend *p_b = new recording_backend;
  a.init(ini, sound, music, p_b);
  a.log_enable(TRUE);
  a.random_seed(7);

  // 8 voices, round robin like the MIDAS auto channels, the sample at 20050 Hz
  for(int i = 0; i < 10; i++)
    a.sound(SOUND_PICKUP, SOUND_TICKS_SECOND, SOUND_PRIORITY_PICKUP);
  CHECK(p_b->plays.size() == 10);
  for(int i = 0; i < 10; i++)
    CHECK(p_b->plays[i].voice == i % AUDIO_VOICES);
  CHECK(p_b->plays[0].rate == 20050);
  CHECK(p_b->plays[0].samples == 34176 / 2);      // smp_014.raw

  // the ten sounds are cut after a second
  for(int i = 0; i < SOUND_TICKS_SECOND; i++)
    a.tick();
  CHECK(p_b->stops.size() == AUDIO_VOICES);

  // cut after the length (game ticks): a step lasts as long as the move
  p_b->plays.clear();
  p_b->stops.clear();
  a.sound(SOUND_STEPS_MUD, SOUND_TICKS_STEP, SOUND_PRIORITY_STEPS);
  int voice = p_b->plays.back().voice;
  for(int i = 0; i < SOUND_TICKS_STEP - 1; i++)
    a.tick();
  CHECK(p_b->stops.empty());
  a.tick();
  CHECK(p_b->stops.size() == 1 && p_b->stops[0] == voice);

  // length 0 = the whole sample, nothing cuts it
  p_b->stops.clear();
  a.sound(SOUND_BUMP, 0, 0);
  for(int i = 0; i < 200; i++)
    a.tick();
  CHECK(p_b->stops.empty());

  // sound off
  p_b->plays.clear();
  a.sound_enable(FALSE);
  a.sound(SOUND_EXPLOSION, SOUND_TICKS_EXPLOSION, SOUND_PRIORITY_PICKUP);
  CHECK(p_b->plays.empty());
  a.sound_enable(TRUE);
  a.sound(SOUND_EXPLOSION, SOUND_TICKS_EXPLOSION, SOUND_PRIORITY_PICKUP);
  CHECK(p_b->plays.size() == 1);

  // an invalid id is ignored
  a.sound((SOUND_ID)99);
  CHECK(p_b->plays.size() == 1);

  // Music: the menu, it goes on while the menu stays
  a.music_menu();
  CHECK(a.music_context() == MUSIC_MENU);
  int menu_track = a.music_track();
  CHECK(menu_track >= 4 && menu_track <= 8);
  CHECK(p_b->music_plays == 1);
  a.music_menu();
  CHECK(a.music_track() == menu_track);
  CHECK(p_b->music_plays == 1);

  // A module start silences the effects (the MIDAS channels were reallocated)
  p_b->stops.clear();
  a.music_level();
  CHECK(a.music_context() == MUSIC_LEVEL);
  CHECK(in_list(a.music_track(), level_tracks, 26));
  CHECK(p_b->music_plays == 2);
  CHECK(p_b->stops.size() == AUDIO_VOICES);

  // N / every level: another track until all 26 were played
  bool played[30] = { false };
  played[a.music_track()] = true;
  for(int i = 1; i < 26; i++) {
    a.music_level();
    CHECK(!played[a.music_track()]);
    played[a.music_track()] = true;
  }
  for(int i = 0; i < 26; i++)
    CHECK(played[level_tracks[i]]);
  a.music_level();
  CHECK(a.music_track() == 0);     // the bag starts again at its first track

  // A solved level: the menu music comes MUSIC_TICKS_AFTER_LEVEL later
  a.music_stop();
  CHECK(a.music_context() == MUSIC_NONE);
  a.music_menu_after_level();
  CHECK(a.music_context() == MUSIC_MENU);   // pending
  CHECK(a.music_track() == MUSIC_NO_TRACK);
  for(int i = 0; i < MUSIC_TICKS_AFTER_LEVEL - 1; i++)
    a.tick();
  CHECK(a.music_track() == MUSIC_NO_TRACK);
  a.tick();
  CHECK(a.music_track() >= 4 && a.music_track() <= 8);

  // ... unless the next level starts first
  a.music_stop();
  a.music_menu_after_level();
  a.music_level();
  CHECK(a.music_context() == MUSIC_LEVEL);
  for(int i = 0; i < MUSIC_TICKS_AFTER_LEVEL + 5; i++)
    a.tick();
  CHECK(a.music_context() == MUSIC_LEVEL);

  // Episode end: its own track after the wait, not for user levels
  a.music_outro(3);
  for(int i = 0; i < MUSIC_TICKS_AFTER_LEVEL; i++)
    a.tick();
  CHECK(a.music_context() == MUSIC_OUTRO);
  CHECK(a.music_track() == 28);
  a.music_outro(5);
  CHECK(a.music_context() == MUSIC_OUTRO && a.music_track() == 28);

  // Credits
  a.music_credits();
  CHECK(a.music_context() == MUSIC_CREDITS);
  CHECK(a.music_track() >= 0 && a.music_track() <= 3);

  // Music off: nothing is loaded, the track is still chosen; switching it
  // on plays the music of the situation
  int plays = p_b->music_plays;
  a.music_enable(FALSE);
  CHECK(p_b->music_stops > 0);
  a.music_menu();
  CHECK(a.music_context() == MUSIC_MENU);
  CHECK(p_b->music_plays == plays);
  a.music_enable(TRUE);
  CHECK(p_b->music_plays == plays + 1);

  // SN_ events
  p_b->plays.clear();
  LEVEL_EVENT ev(SN_PLAY_SAMPLE, (int)SOUND_SWITCH, SOUND_TICKS_SECOND, SOUND_PRIORITY_SWITCH);
  CHECK(a.event_process(&ev));
  CHECK(p_b->plays.size() == 1);
  LEVEL_EVENT stop(SN_STOP_MUSIC);
  CHECK(a.event_process(&stop));
  CHECK(a.music_context() == MUSIC_NONE);
  LEVEL_EVENT level(SN_PLAY_MUSIC, (int)MUSIC_LEVEL);
  CHECK(a.event_process(&level));
  CHECK(a.music_context() == MUSIC_LEVEL);
  LEVEL_EVENT other(GC_MENU_START);
  CHECK(!a.event_process(&other));

  // The log says what was asked for
  CHECK(a.log_get().find("sound switch voice") != std::string::npos);
  CHECK(a.log_get().find("music level track") != std::string::npos);

  a.shutdown();
}

/* Missing sound / music files: silence, no crash */
static void test_missing_files(const char *p_dir)
{
  char ini[MAX_FILENAME], empty[MAX_FILENAME];
  snprintf(ini, sizeof(ini), "%s/missing.ini", p_dir);
  snprintf(empty, sizeof(empty), "%s/no_such_dir", p_dir);
  write_file(ini, "");

  GAME_AUDIO a;
  recording_backend *p_b = new recording_backend;
  a.init(ini, empty, empty, p_b);
  a.log_enable(TRUE);

  for(int i = 0; i < SOUND_NUM; i++)
    a.sound((SOUND_ID)i);
  CHECK(p_b->plays.empty());
  CHECK(a.log_get().find("sound pickup missing") != std::string::npos);

  a.music_menu();
  CHECK(a.music_context() == MUSIC_MENU);
  CHECK(p_b->music_plays == 0);
  a.music_level();
  CHECK(p_b->music_plays == 0);
  for(int i = 0; i < 100; i++)
    a.tick();

  a.shutdown();

  // No backend at all (sounds before init / after shutdown)
  GAME_AUDIO b;
  b.sound(SOUND_PICKUP);
  b.music_menu();
  b.tick();
  b.music_stop();
}

int main(int argc, char *argv[])
{
  const char *p_data = BERUSKY_TEST_DATA;
  char dir[MAX_FILENAME];
  snprintf(dir, sizeof(dir), "%s", BERUSKY_TEST_TMP);
  SDL_CreateDirectory(dir);
  snprintf(test_ini, sizeof(test_ini), "%s/berusky.ini", dir);
  write_file(test_ini, "");

  test_languages();
  test_czech_table();
  test_utf8();
  test_language_config(dir);
  test_data_files(p_data);
  test_audio_settings(dir, p_data);
  test_music_bags();
  test_audio_layer(dir, p_data);
  test_missing_files(dir);

  if(failures) {
    printf("%d check(s) failed\n", failures);
    return(1);
  }
  printf("audio + language test ok\n");
  return(0);
}
