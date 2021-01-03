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
#include "base.h"
#include "effects.h"
#include "map.h"
#include "movement.h"
#include "unittype.h"

/* server/advisors */
#include "autosettlers.h"

#include "advruleset.h"

/**********************************************************************/ /**
   Initialise the unit data from the ruleset for the advisors.
 **************************************************************************/
void adv_units_ruleset_init()
{
  unit_class_iterate(pclass)
  {
    bool move_land_enabled = false;  /* Can move at some land terrains */
    bool move_land_disabled = false; /* Cannot move at some land terrains */
    bool move_sea_enabled = false;   /* Can move at some ocean terrains */
    bool move_sea_disabled = false;  /* Cannot move at some ocean terrains */

    terrain_type_iterate(pterrain)
    {
      if (is_native_to_class(pclass, pterrain, NULL)) {
        /* Can move at terrain */
        if (is_ocean(pterrain)) {
          move_sea_enabled = true;
        } else {
          move_land_enabled = true;
        }
      } else {
        /* Cannot move at terrain */
        if (is_ocean(pterrain)) {
          move_sea_disabled = true;
        } else {
          move_land_disabled = true;
        }
      }
    }
    terrain_type_iterate_end;

    if (move_land_enabled && !move_land_disabled) {
      pclass->adv.land_move = MOVE_FULL;
    } else if (move_land_enabled && move_land_disabled) {
      pclass->adv.land_move = MOVE_PARTIAL;
    } else {
      fc_assert(!move_land_enabled);
      pclass->adv.land_move = MOVE_NONE;
    }

    if (move_sea_enabled && !move_sea_disabled) {
      pclass->adv.sea_move = MOVE_FULL;
    } else if (move_sea_enabled && move_sea_disabled) {
      pclass->adv.sea_move = MOVE_PARTIAL;
    } else {
      fc_assert(!move_sea_enabled);
      pclass->adv.sea_move = MOVE_NONE;
    }
  }
  unit_class_iterate_end;

  unit_type_iterate(ptype)
  {
    ptype->adv.igwall = true;

    effect_list_iterate(get_effects(EFT_DEFEND_BONUS), peffect)
    {
      if (peffect->value > 0) {
        requirement_vector_iterate(&peffect->reqs, preq)
        {
          if (!is_req_active(NULL, NULL, NULL, NULL, NULL, NULL, ptype, NULL,
                             NULL, NULL, preq, RPT_POSSIBLE)) {
            ptype->adv.igwall = false;
            break;
          }
        }
        requirement_vector_iterate_end;
      }
      if (!ptype->adv.igwall) {
        break;
      }
    }
    effect_list_iterate_end;

    ptype->adv.worker = utype_has_flag(ptype, UTYF_SETTLERS);
  }
  unit_type_iterate_end;

  /* Initialize autosettlers actions */
  auto_settlers_ruleset_init();
}
