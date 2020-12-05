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
#include "government.h"
#include "packets.h"
#include "spaceship.h"

/* server */
#include "spacerace.h"

#include "advspace.h"

/************************************************************************/ /**
   Place all available spaceship components.

   Returns TRUE iff at least one part was placed.
 ****************************************************************************/
bool adv_spaceship_autoplace(struct player *pplayer,
                             struct player_spaceship *ship)
{
  struct spaceship_component place;
  bool retval = FALSE;
  bool placed;

  do {
    placed = next_spaceship_component(pplayer, ship, &place);

    if (placed) {
      if (do_spaceship_place(pplayer, ACT_REQ_SS_AGENT, place.type,
                             place.num)) {
        /* A part was placed. It was placed even if the placement of future
         * parts will fail. */
        retval = TRUE;
      } else {
        /* Unable to place this part. Don't try to place it again. */
        break;
      }
    }
  } while (placed);

  return retval;
}
