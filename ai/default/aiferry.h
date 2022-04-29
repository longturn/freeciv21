/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "support.h" // bool type

#include "fc_types.h"

class PFPath;
struct pft_amphibious;

bool dai_is_ferry_type(const struct unit_type *pferry, struct ai_type *ait);
bool dai_is_ferry(struct unit *pferry, struct ai_type *ait);

/*
 * Initialize ferrybaot-related statistics in the ai data.
 */
void aiferry_init_stats(struct ai_type *ait, struct player *pplayer);

/*
 * Find the nearest boat.  Can be called from inside the continents too
 */
int aiferry_find_boat(struct ai_type *ait, struct unit *punit, int cap,
                      PFPath *path);

/*
 * How many boats are available
 */
int aiferry_avail_boats(struct ai_type *ait, struct player *pplayer);

/*
 * Initializes aiferry stats for a new unit
 */
void dai_ferry_init_ferry(struct ai_type *ait, struct unit *ferry);
void dai_ferry_lost(struct ai_type *ait, struct unit *punit);
void dai_ferry_transformed(struct ai_type *ait, struct unit *ferry,
                           const struct unit_type *old);

/*
 * Release the boat reserved in punit's ai.ferryboat field.
 */
void aiferry_clear_boat(struct ai_type *ait, struct unit *punit);

/*
 * Go to the destination by hitching a ride on a boat.  Will try to find
 * a beachhead but it works better if dst_tile is on the coast.
 * Loads a bodyguard too, if necessary.
 */
bool aiferry_gobyboat(struct ai_type *ait, struct player *pplayer,
                      struct unit *punit, struct tile *dst_tile,
                      bool with_bodyguard);
/*
 * Go to the destination on a particular boat.  Will try to find
 * a beachhead but it works better if ptile is on the coast.
 */
bool aiferry_goto_amphibious(struct ai_type *ait, struct unit *ferry,
                             struct unit *passenger, struct tile *ptile);

bool dai_amphibious_goto_constrained(struct ai_type *ait, struct unit *ferry,
                                     struct unit *passenger,
                                     struct tile *ptile,
                                     struct pft_amphibious *parameter);

bool is_boat_free(struct ai_type *ait, struct unit *boat, struct unit *punit,
                  int cap);
bool is_boss_of_boat(struct ai_type *ait, struct unit *punit);

/*
 * Main boat managing function.  Gets units on board to where they want to
 * go and then looks for new passengers or (if it fails) for a city which
 * will build a passenger soon.
 */
void dai_manage_ferryboat(struct ai_type *ait, struct player *pplayer,
                          struct unit *punit);
