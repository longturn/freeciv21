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

// utility
#include "log.h"
#include "shared.h"

// common
#include "game.h"
#include "player.h"
#include "tile.h"
#include "vision.h"

/**
   Create a new vision source.
 */
struct vision *vision_new(struct player *pplayer, struct tile *ptile)
{
  auto *vision = new struct vision;

  vision->player = pplayer;
  vision->tile = ptile;
  vision->can_reveal_tiles = true;
  vision->radius_sq[V_MAIN] = -1;
  vision->radius_sq[V_INVIS] = -1;
  vision->radius_sq[V_SUBSURFACE] = -1;

  return vision;
}

/**
   Free the vision source.
 */
void vision_free(struct vision *vision)
{
  fc_assert(-1 == vision->radius_sq[V_MAIN]);
  fc_assert(-1 == vision->radius_sq[V_INVIS]);
  fc_assert(-1 == vision->radius_sq[V_SUBSURFACE]);
  delete vision;
}

/**
   Sets the can_reveal_tiles flag.
   Returns the old flag.
 */
bool vision_reveal_tiles(struct vision *vision, bool reveal_tiles)
{
  bool was = vision->can_reveal_tiles;

  vision->can_reveal_tiles = reveal_tiles;
  return was;
}

/**
   Returns the basic structure.
 */
struct vision_site *vision_site_new(int identity, struct tile *location,
                                    struct player *owner)
{
  vision_site *psite = new vision_site[1]();

  psite->identity = identity;
  psite->location = location;
  psite->owner = owner;

  return psite;
}

/**
   Returns the basic structure filled with initial elements.
 */
struct vision_site *vision_site_new_from_city(const struct city *pcity)
{
  struct vision_site *psite =
      vision_site_new(pcity->id, city_tile(pcity), city_owner(pcity));

  vision_site_size_set(psite, city_size_get(pcity));
  sz_strlcpy(psite->name, city_name_get(pcity));

  return psite;
}

/**
   Returns the basic structure filled with current elements.
 */
void vision_site_update_from_city(struct vision_site *psite,
                                  const struct city *pcity)
{
  // should be same identity and location
  fc_assert_ret(psite->identity == pcity->id);
  fc_assert_ret(psite->location == pcity->tile);

  psite->owner = city_owner(pcity);

  vision_site_size_set(psite, city_size_get(pcity));
  sz_strlcpy(psite->name, city_name_get(pcity));
}

/**
   Get the city size.
 */
citizens vision_site_size_get(const struct vision_site *psite)
{
  fc_assert_ret_val(psite != nullptr, 0);

  return psite->size;
}

/**
   Set the city size.
 */
void vision_site_size_set(struct vision_site *psite, citizens size)
{
  fc_assert_ret(psite != nullptr);

  psite->size = size;
}
