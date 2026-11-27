// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "advspace.h"

// common
#include "actions.h"
#include "player.h"
#include "spaceship.h"

// server
#include "spacerace.h"

/**
   Place all available spaceship components.

   Returns TRUE iff at least one part was placed.
 */
bool adv_spaceship_autoplace(struct player *pplayer,
                             struct player_spaceship *ship)
{
  struct spaceship_component place;
  bool retval = false;
  bool placed;

  do {
    placed = next_spaceship_component(pplayer, ship, &place);

    if (placed) {
      if (do_spaceship_place(pplayer, ACT_REQ_SS_AGENT, place.type,
                             place.num)) {
        /* A part was placed. It was placed even if the placement of future
         * parts will fail. */
        retval = true;
      } else {
        // Unable to place this part. Don't try to place it again.
        break;
      }
    }
  } while (placed);

  return retval;
}
