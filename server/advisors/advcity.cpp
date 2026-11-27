// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "advcity.h"

// utility
#include "shared.h"

// common
#include "city.h"
#include "fc_types.h"

// server
#include "advdata.h"

/**
  This calculates the usefulness of pcity to us. Note that you can pass
  another player's adv_data structure here for evaluation by different
  priorities.
 */
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
    // The city is unmaintainable, it can't be good
    i = MIN(i, 0);
  }

  return i;
}
