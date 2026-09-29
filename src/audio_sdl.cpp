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
 * Audio output with SDL3 and libxmp.
 *
 * One playback device, the SDL mixer of bound audio streams does all the
 * mixing: every effect voice is an audio stream (16-bit mono, 20050 Hz,
 * converted by SDL), the music is one more stream that asks for data on the
 * audio thread and gets it from libxmp (the original FastTracker II modules
 * are played directly).
 */
#include <xmp.h>

#include "berusky.h"
#include "audio.h"
#include "audio_backend.h"

#define MUSIC_RATE      44100
#define MUSIC_CHANNELS  2

class audio_backend_sdl : public audio_backend {

  SDL_AudioDeviceID device;
  SDL_AudioStream  *p_voice[AUDIO_VOICES];
  int               voice_rate[AUDIO_VOICES];
  SDL_AudioStream  *p_music;

  // Used by the audio thread - changed only with p_music locked
  xmp_context       xmp;
  bool              xmp_playing;
  int               music_volume;

  bool              subsystem;

  static void SDLCALL music_feed(void *p_user, SDL_AudioStream *p_stream,
                                 int additional, int total);

  // SDL may already be shut down when the game exits
  bool active(void)
  {
    return(device != 0 && SDL_WasInit(SDL_INIT_AUDIO));
  }

  void music_release(xmp_context c, bool playing)
  {
    if(c) {
      if(playing) {
        xmp_end_player(c);
        xmp_release_module(c);
      }
      xmp_free_context(c);
    }
  }

public:

  audio_backend_sdl(void)
  : device(0), p_music(NULL), xmp(NULL), xmp_playing(FALSE), music_volume(100), subsystem(FALSE)
  {
    for(int i = 0; i < AUDIO_VOICES; i++) {
      p_voice[i] = NULL;
      voice_rate[i] = SOUND_RATE;
    }
  }

  ~audio_backend_sdl(void)
  {
    close();
  }

  bool open(void)
  {
    if(!SDL_WasInit(SDL_INIT_AUDIO)) {
      if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        bprintf("Audio: SDL audio init failed: %s", SDL_GetError());
        return(FALSE);
      }
      subsystem = TRUE;
    }

    device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if(!device) {
      bprintf("Audio: can't open the audio device: %s", SDL_GetError());
      close();
      return(FALSE);
    }

    SDL_AudioSpec device_spec;
    if(!SDL_GetAudioDeviceFormat(device, &device_spec, NULL)) {
      device_spec.format = SDL_AUDIO_F32;
      device_spec.channels = 2;
      device_spec.freq = 48000;
    }

    SDL_AudioSpec voice_spec = { SDL_AUDIO_S16, 1, SOUND_RATE };
    for(int i = 0; i < AUDIO_VOICES; i++) {
      p_voice[i] = SDL_CreateAudioStream(&voice_spec, &device_spec);
      if(!p_voice[i] || !SDL_BindAudioStream(device, p_voice[i])) {
        bprintf("Audio: can't create a sound stream: %s", SDL_GetError());
        close();
        return(FALSE);
      }
    }

    SDL_AudioSpec music_spec = { SDL_AUDIO_S16, MUSIC_CHANNELS, MUSIC_RATE };
    p_music = SDL_CreateAudioStream(&music_spec, &device_spec);
    if(!p_music ||
       !SDL_SetAudioStreamGetCallback(p_music, music_feed, this) ||
       !SDL_BindAudioStream(device, p_music)) {
      bprintf("Audio: can't create the music stream: %s", SDL_GetError());
      close();
      return(FALSE);
    }

    SDL_ResumeAudioDevice(device);

    bprintf("Audio: %s, %d Hz, %d channels, libxmp %s",
            SDL_GetAudioDeviceName(device), device_spec.freq, device_spec.channels,
            xmp_version);
    return(TRUE);
  }

  void close(void)
  {
    if(SDL_WasInit(SDL_INIT_AUDIO)) {
      if(p_music) {
        SDL_DestroyAudioStream(p_music);
      }
      for(int i = 0; i < AUDIO_VOICES; i++) {
        if(p_voice[i])
          SDL_DestroyAudioStream(p_voice[i]);
      }
      if(device)
        SDL_CloseAudioDevice(device);
      if(subsystem)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    p_music = NULL;
    for(int i = 0; i < AUDIO_VOICES; i++)
      p_voice[i] = NULL;
    device = 0;
    subsystem = FALSE;

    // nothing runs on the audio thread any more
    music_release(xmp, xmp_playing);
    xmp = NULL;
    xmp_playing = FALSE;
  }

  void voice_play(int voice, const short *p_pcm, int samples, int rate)
  {
    if(!active() || voice < 0 || voice >= AUDIO_VOICES || !p_voice[voice])
      return;

    SDL_AudioStream *p_stream = p_voice[voice];
    SDL_ClearAudioStream(p_stream);

    if(rate != voice_rate[voice]) {
      SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, rate };
      SDL_SetAudioStreamFormat(p_stream, &spec, NULL);
      voice_rate[voice] = rate;
    }

    SDL_PutAudioStreamData(p_stream, p_pcm, samples * (int)sizeof(short));
    // no more data comes - convert the end of the sound too
    SDL_FlushAudioStream(p_stream);
  }

  void voice_stop(int voice)
  {
    if(active() && voice >= 0 && voice < AUDIO_VOICES && p_voice[voice])
      SDL_ClearAudioStream(p_voice[voice]);
  }

  void voice_gain_set(float gain)
  {
    if(!active())
      return;
    for(int i = 0; i < AUDIO_VOICES; i++) {
      if(p_voice[i])
        SDL_SetAudioStreamGain(p_voice[i], gain);
    }
  }

  bool music_play(const void *p_data, size_t size)
  {
    if(!active() || !p_music)
      return(FALSE);

    // Load and start it before it's handed to the audio thread
    xmp_context c = xmp_create_context();
    if(!c)
      return(FALSE);

    if(xmp_load_module_from_memory(c, p_data, (long)size) != 0) {
      xmp_free_context(c);
      return(FALSE);
    }
    if(xmp_start_player(c, MUSIC_RATE, 0) != 0) {
      xmp_release_module(c);
      xmp_free_context(c);
      return(FALSE);
    }

    SDL_LockAudioStream(p_music);
    xmp_set_player(c, XMP_PLAYER_VOLUME, music_volume);
    xmp_context old = xmp;
    bool        old_playing = xmp_playing;
    xmp = c;
    xmp_playing = TRUE;
    SDL_ClearAudioStream(p_music);
    SDL_UnlockAudioStream(p_music);

    music_release(old, old_playing);
    return(TRUE);
  }

  void music_stop(void)
  {
    if(!active() || !p_music)
      return;

    SDL_LockAudioStream(p_music);
    xmp_context old = xmp;
    bool        old_playing = xmp_playing;
    xmp = NULL;
    xmp_playing = FALSE;
    SDL_ClearAudioStream(p_music);
    SDL_UnlockAudioStream(p_music);

    music_release(old, old_playing);
  }

  void music_volume_set(int volume)
  {
    music_volume = volume;
    if(!active() || !p_music)
      return;

    // the module's master volume - it's applied before the mix is clipped
    // (MIDASsetMusicVolume() did the same)
    SDL_LockAudioStream(p_music);
    if(xmp && xmp_playing)
      xmp_set_player(xmp, XMP_PLAYER_VOLUME, music_volume);
    SDL_UnlockAudioStream(p_music);
  }
};

// Audio thread, the stream is locked
void SDLCALL audio_backend_sdl::music_feed(void *p_user, SDL_AudioStream *p_stream,
                                           int additional, int total)
{
  audio_backend_sdl *p_this = (audio_backend_sdl *)p_user;

  if(!p_this->xmp || !p_this->xmp_playing || additional <= 0)
    return;

  #define FRAME_BYTES  (MUSIC_CHANNELS*(int)sizeof(short))
  Sint16 buffer[4096];

  while(additional > 0) {
    int bytes = additional < (int)sizeof(buffer) ? additional : (int)sizeof(buffer);
    bytes = ((bytes + FRAME_BYTES - 1) / FRAME_BYTES) * FRAME_BYTES;
    if(bytes > (int)sizeof(buffer))
      bytes = (int)sizeof(buffer);

    // loop 0 = the module plays forever (MIDASplayModule(module, TRUE))
    if(xmp_play_buffer(p_this->xmp, buffer, bytes, 0) != 0)
      break;
    SDL_PutAudioStreamData(p_stream, buffer, bytes);
    additional -= bytes;
  }
}

AUDIO_BACKEND * audio_backend_sdl_create(void)
{
  return(new audio_backend_sdl);
}
