/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "city.h"
#include "fc_types.h"

/* server/advisors */
#include "advdata.h"

#include "advcity.h"

/**************************************************************************
  This calculates the usefulness of pcity to us. Note that you can pass
  another player's adv_data structure here for evaluation by different
  priorities.
**************************************************************************/
int adv_eval_calc_city(struct city *pcity, struct adv_data *adv)
{
  int i =
      (pcity->surplus[O_FOOD] * adv->food_priority
       + pcity->surplus[O_SHIELD] * adv->shield_priority
       + pcity->prod[O_LUXURY] * adv->luxury_priority
       + pcity->prod[O_GOLD] * adv->gold_priority
       + pcity->prod[O_SCIENCE] * adv->science_priority
       + pcity->feel[CITIZEN_HAPPY][FEELING_FINAL] * adv->happy_priority
       - pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] * adv->unhappy_priority
       - pcity->feel[CITIZEN_ANGRY][FEELING_FINAL] * adv->angry_priority
       - pcity->pollution * adv->pollution_priority);

  if (pcity->surplus[O_FOOD] < 0 || pcity->surplus[O_SHIELD] < 0) {
    /* The city is unmaintainable, it can't be good */
    i = MIN(i, 0);
  }

  return i;
}
