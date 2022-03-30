/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

// utility
#include "connection.h"
#include "log.h"
#include "nation.h"
#include "style.h"
// client
#include "audio.h"
#include "client_main.h"
#include "options.h"

#include "music.h"

/**
   Start music suitable for current game situation
 */
void start_style_music()
{
  if (client_state() != C_S_RUNNING) {
    // Style music plays in running game only
    return;
  }

  if (client.conn.playing == nullptr) {
    /* Detached connections and global observers currently not
     * supported */
    return;
  }

  if (gui_options.sound_enable_game_music) {
    struct music_style *pms;

    stop_style_music();

    pms = music_style_by_number(client.conn.playing->music_style);

    if (pms != nullptr) {
      QString tag;

      switch (client.conn.playing->client.mood) {
      case MOOD_COUNT:
        fc_assert(client.conn.playing->client.mood != MOOD_COUNT);
        fc__fallthrough; // No break but use default tag
      case MOOD_PEACEFUL:
        tag = pms->music_peaceful;
        break;
      case MOOD_COMBAT:
        tag = pms->music_combat;
        break;
      }

      if (!tag.isEmpty()) {
        log_debug("Play %s", qUtf8Printable(tag));
        audio_play_music(tag, nullptr, MU_INGAME);
      }
    }
  }
}

/**
   Stop style music completely.
 */
void stop_style_music() { audio_stop_usage(); }

/**
   Start menu music.
 */
void start_menu_music(const QString &tag, const QString &alt_tag)
{
  if (gui_options.sound_enable_menu_music) {
    audio_play_music(tag, alt_tag, MU_MENU);
  }
}

/**
   Stop menu music completely.
 */
void stop_menu_music() { audio_stop_usage(); }

/**
   Play single track before continuing normal style music
 */
void play_single_track(const QString &tag)
{
  if (client_state() != C_S_RUNNING) {
    // Only in running game
    return;
  }

  audio_play_track(tag, nullptr);
}

/**
   Musicset changed in options.
 */
void musicspec_reread_callback(struct option *poption)
{
  const char *musicset_name = option_str_get(poption);

  audio_restart(sound_set_name, musicset_name);

  // Start suitable music from the new set
  if (client_state() != C_S_RUNNING) {
    start_menu_music(QStringLiteral("music_menu"), nullptr);
  } else {
    start_style_music();
  }
}
