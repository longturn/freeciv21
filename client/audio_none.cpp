/*
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
 */

#include "support.h"

#include "audio.h"

#include "audio_none.h"

// gui-qt
#include "qtg_cxxside.h"

/**
   Clean up
 */
static void none_audio_shutdown() {}

/**
   Stop music
 */
static void none_audio_stop() {}

/**
   Wait
 */
static void none_audio_wait() {}

/**
   Play sound sample
 */
static bool none_audio_play(const QString &tag, const QString &fullpath,
                            bool repeat, audio_finished_callback cb)
{
  if (tag == QLatin1String("e_turn_bell")) {
    sound_bell();
    return true;
  }
  return false;
}

/**
   Initialize.
 */
static bool none_audio_init() { return true; }

/**
   Initialize.
 */
void audio_none_init()
{
  struct audio_plugin self;

  self.name = QStringLiteral("none");
  self.descr = QStringLiteral("/dev/null plugin");
  self.init = none_audio_init;
  self.shutdown = none_audio_shutdown;
  self.stop = none_audio_stop;
  self.wait = none_audio_wait;
  self.play = none_audio_play;
  self.get_volume = audio_get_volume;
  self.set_volume = audio_set_volume;
  audio_add_plugin(&self);
}
