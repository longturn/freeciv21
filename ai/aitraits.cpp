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

/* utility */
#include "rand.h"

/* common */
#include "game.h"
#include "player.h"
#include "traits.h"

#include "aitraits.h"

/**********************************************************************/ /**
   Initialize ai traits for player
 **************************************************************************/
void ai_traits_init(struct player *pplayer)
{
  enum trait tr;

  pplayer->ai_common.traits = static_cast<ai_trait *>(fc_realloc(
      pplayer->ai_common.traits, sizeof(struct ai_trait) * TRAIT_COUNT));

  for (tr = trait_begin(); tr != trait_end(); tr = trait_next(tr)) {
    int min = pplayer->nation->server.traits[tr].min;
    int max = pplayer->nation->server.traits[tr].max;

    switch (game.server.trait_dist) {
    case TDM_FIXED:
      pplayer->ai_common.traits[tr].val =
          pplayer->nation->server.traits[tr].fixed;
      break;
    case TDM_EVEN:
      pplayer->ai_common.traits[tr].val = fc_rand(max + 1 - min) + min;
      break;
    }
    pplayer->ai_common.traits[tr].mod = 0;
  }
}

/**********************************************************************/ /**
   Free resources associated with player ai traits.
 **************************************************************************/
void ai_traits_close(struct player *pplayer)
{
  free(pplayer->ai_common.traits); //realloc

  pplayer->ai_common.traits = NULL;
}

/**********************************************************************/ /**
   Get current value of player trait
 **************************************************************************/
int ai_trait_get_value(enum trait tr, struct player *pplayer)
{
  int val =
      pplayer->ai_common.traits[tr].val + pplayer->ai_common.traits[tr].mod;

  /* Clip so that value is at least 1, and maximum is
   * TRAIT_DEFAULT_VALUE as many times as TRAIT_DEFAULT value is
   * minimum value of 1 ->
   * minimum is default / TRAIT_DEFAULT_VALUE,
   * maximum is default * TRAIT_DEFAULT_VALUE */
  val = CLIP(1, val, TRAIT_MAX_VALUE);

  return val;
}
