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

/* common */
#include "fc_types.h"
#include "game.h"
#include "player.h"

#include "mood.h"

/**********************************************************************/ /**
   What is the player mood?
 **************************************************************************/
enum mood_type player_mood(struct player *pplayer)
{
  if (pplayer->last_war_action >= 0
      && pplayer->last_war_action + WAR_MOOD_LASTS >= game.info.turn) {
    players_iterate(other)
    {
      struct player_diplstate *us, *them;

      us = player_diplstate_get(pplayer, other);
      them = player_diplstate_get(other, pplayer);

      if (us->type == DS_WAR || us->has_reason_to_cancel > 0
          || them->has_reason_to_cancel > 0) {
        return MOOD_COMBAT;
      }
    }
    players_iterate_end;
  }

  return MOOD_PEACEFUL;
}
