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
 * Audio output behind the audio layer (audio.h). It knows nothing about
 * the game: voices play PCM, the music plays a module in a loop.
 */
#ifndef __AUDIO_BACKEND_H__
#define __AUDIO_BACKEND_H__

#include <stddef.h>

typedef class audio_backend {

public:

  virtual ~audio_backend(void) {}

  // Opens the output. false = no audio (the game runs silent).
  virtual bool open(void) = 0;
  virtual void close(void) = 0;

  // A voice plays 16-bit signed mono PCM once; play() replaces what the
  // voice was playing. gain 0.0 - 1.0 for all voices.
  virtual void voice_play(int voice, const short *p_pcm, int samples, int rate) = 0;
  virtual void voice_stop(int voice) = 0;
  virtual void voice_gain_set(float gain) = 0;

  // Loads a module from memory (the data may be freed then) and plays it
  // in a loop; false when it can't be played. volume 0 - 100.
  virtual bool music_play(const void *p_data, size_t size) = 0;
  virtual void music_stop(void) = 0;
  virtual void music_volume_set(int volume) = 0;

} AUDIO_BACKEND;

// SDL3 audio streams + libxmp (audio_sdl.cpp)
AUDIO_BACKEND * audio_backend_sdl_create(void);

// No output (tests, no audio device)
AUDIO_BACKEND * audio_backend_null_create(void);

#endif // __AUDIO_BACKEND_H__
