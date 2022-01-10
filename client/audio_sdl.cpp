/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstring>

#ifdef AUDIO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif
// utility
#include "log.h"
#include "support.h"

// client
#include "audio.h"

#include "audio_sdl.h"

#include <array>

struct sample {
  Mix_Chunk *wave = nullptr;
  QString tag;
};

/* Sounds don't sound good on Windows unless the buffer size is 4k,
 * but this seems to cause strange behaviour on other systems,
 * such as a delay before playing the sound. */
#ifdef FREECIV_MSWINDOWS
const size_t buf_size = 4096;
#else
const size_t buf_size = 1024;
#endif

static Mix_Music *mus = NULL;
static std::array<sample, MIX_CHANNELS> samples;
static double sdl_audio_volume;

/**
   Set the volume.
 */
static void sdl_audio_set_volume(double volume)
{
  Mix_VolumeMusic(volume * MIX_MAX_VOLUME);
  Mix_Volume(-1, volume * MIX_MAX_VOLUME);
  sdl_audio_volume = volume;
}

/**
   Get the volume.
 */
static double sdl_audio_get_volume() { return sdl_audio_volume; }

/**
   Play sound
 */
static bool sdl_audio_play(const QString &tag, const QString &fullpath,
                           bool repeat, audio_finished_callback cb)
{
  Mix_Chunk *wave = NULL;

  if (fullpath.isEmpty()) {
    return false;
  }

  if (repeat) {
    // unload previous
    Mix_HaltMusic();
    Mix_FreeMusic(mus);

    // load music file
    mus = Mix_LoadMUS(qUtf8Printable(fullpath));
    if (mus == NULL) {
      qCritical("Can't open file \"%s\"", qUtf8Printable(fullpath));
      return false;
    }

    if (cb == NULL) {
      Mix_PlayMusic(mus, -1); // -1 means loop forever
    } else {
      Mix_PlayMusic(mus, 0);
      Mix_HookMusicFinished(cb);
    }
    qDebug("Playing file \"%s\" on music channel", qUtf8Printable(fullpath));
    /* in case we did a sdl_audio_stop() recently; add volume controls later
     */
    Mix_VolumeMusic(sdl_audio_volume * MIX_MAX_VOLUME);

  } else {
    // see if we can cache on this one
    for (auto sample : samples) {
      if (sample.wave != nullptr && sample.tag == tag) {
        log_debug("Playing file \"%s\" from cache",
                  qUtf8Printable(fullpath));
        Mix_PlayChannel(-1, sample.wave, 0);
        return true;
      }
    } // guess not

    // load wave
    wave = Mix_LoadWAV(qUtf8Printable(fullpath));
    if (wave == NULL) {
      qCritical("Can't open file \"%s\"", qUtf8Printable(fullpath));
      return false;
    }

    /* play sound sample on first available channel, returns -1 if no
       channel found */
    int i = Mix_PlayChannel(-1, wave, 0);
    if (i < 0) {
      qDebug("No available sound channel to play %s.", qUtf8Printable(tag));
      Mix_FreeChunk(wave);
      return false;
    }
    qDebug("Playing file \"%s\" on channel %d", qUtf8Printable(fullpath), i);
    /* free previous sample on this channel. it will by definition no
       longer be playing by the time we get here */
    if (samples[i].wave) {
      Mix_FreeChunk(samples[i].wave);
      samples[i].wave = NULL;
    }
    // remember for cacheing
    samples[i].wave = wave;
    samples[i].tag = tag;
  }
  return true;
}

/**
   Stop music
 */
static void sdl_audio_stop()
{
  // fade out over 2 sec
  Mix_FadeOutMusic(2000);
}

/**
   Wait for audio to die on all channels.
   WARNING: If a channel is looping, it will NEVER exit! Always call
   music_stop() first!
 */
static void sdl_audio_wait()
{
  while (Mix_Playing(-1) != 0) {
    SDL_Delay(100);
  }
}

/**
   Quit SDL.  If the video is still in use (by gui-sdl), just quit the
   subsystem.

   This will need to be changed if SDL is used elsewhere.
 */
static void quit_sdl_audio()
{
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  } else {
    SDL_Quit();
  }
}

/**
   Init SDL.  If the video is already in use (by gui-sdl), just init the
   subsystem.

   This will need to be changed if SDL is used elsewhere.
 */
static int init_sdl_audio()
{
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    return SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
  } else {
    return SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
  }
}

/**
   Clean up.
 */
static void sdl_audio_shutdown()
{
  int i;

  sdl_audio_stop();
  sdl_audio_wait();

  // remove all buffers
  for (i = 0; i < MIX_CHANNELS; i++) {
    if (samples[i].wave) {
      Mix_FreeChunk(samples[i].wave);
    }
  }
  Mix_HaltMusic();
  if (mus != nullptr) {
    Mix_FreeMusic(mus);
  }

  Mix_CloseAudio();
  quit_sdl_audio();
}

/**
   Initialize.
 */
static bool sdl_audio_init()
{
  // Initialize variables
  const int audio_rate = MIX_DEFAULT_FREQUENCY;
  const int audio_format = MIX_DEFAULT_FORMAT;
  const int audio_channels = 2;
  int i;

  if (init_sdl_audio() < 0) {
    return false;
  }

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, buf_size)
      < 0) {
    qCritical("Error calling Mix_OpenAudio");
    // try something else
    quit_sdl_audio();
    return false;
  }

  Mix_AllocateChannels(MIX_CHANNELS);
  for (i = 0; i < MIX_CHANNELS; i++) {
    samples[i].wave = NULL;
  }
  // sanity check, for now; add volume controls later
  sdl_audio_set_volume(sdl_audio_volume);
  return true;
}

/**
   Initialize. Note that this function is called very early at the
   client startup. So for example logging isn't available.
 */
void audio_sdl_init()
{
  struct audio_plugin self;

  self.name = QStringLiteral("sdl");
  self.descr =
      QStringLiteral("Simple DirectMedia Library (SDL) mixer plugin");
  self.init = sdl_audio_init;
  self.shutdown = sdl_audio_shutdown;
  self.stop = sdl_audio_stop;
  self.wait = sdl_audio_wait;
  self.play = sdl_audio_play;
  self.set_volume = sdl_audio_set_volume;
  self.get_volume = sdl_audio_get_volume;
  audio_add_plugin(&self);
  sdl_audio_volume = 1.0;
}
