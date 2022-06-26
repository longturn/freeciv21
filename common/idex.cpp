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

/***********************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.

   Method: use separate hash tables for each type.
   Means code duplication for city/unit cases, but simplicity advantages.
   Don't have to manage memory at all: store pointers to unit and city
   structs allocated elsewhere, and keys are pointers to id values inside
   the structs.

   Note id values should probably be unsigned int: here leave as plain int
   so can use pointers to pcity->id etc.
***********************************************************************/

// utility
#include "log.h"

// common
#include "city.h"
#include "unit.h"

#include "idex.h"

/**
    Initialize.  Should call this at the start before use.
 */
void idex_init(struct world *iworld)
{
  iworld->cities = new QHash<int, const struct city *>;
  iworld->units = new QHash<int, const struct unit *>;
}

/**
    Free the hashs.
 */
void idex_free(struct world *iworld)
{
  delete iworld->cities;
  delete iworld->units;
  iworld->cities = nullptr;
  iworld->units = nullptr;
}

/**
    Register a city into idex, with current pcity->id.
    Call this when pcity created.
 */
void idex_register_city(struct world *iworld, struct city *pcity)
{
  const struct city *old;

  if (iworld->cities->contains(pcity->id)) {
    old = iworld->cities->value(pcity->id);
    fc_assert_ret_msg(nullptr == old,
                      "IDEX: city collision: new %d %p %s, old %d %p %s",
                      pcity->id, (void *) pcity, city_name_get(pcity),
                      old->id, (void *) old, city_name_get(old));
  }
  iworld->cities->insert(pcity->id, pcity);
}

/**
    Register a unit into idex, with current punit->id.
    Call this when punit created.
 */
void idex_register_unit(struct world *iworld, struct unit *punit)
{
  const struct unit *old;

  if (iworld->units->contains(punit->id)) {
    old = iworld->units->value(punit->id);
    fc_assert_ret_msg(nullptr == old,
                      "IDEX: unit collision: new %d %p %s, old %d %p %s",
                      punit->id, (void *) punit, unit_rule_name(punit),
                      old->id, (void *) old, unit_rule_name(old));
  }
  iworld->units->insert(punit->id, punit);
}

/**
    Remove a city from idex, with current pcity->id.
    Call this when pcity deleted.
 */
void idex_unregister_city(struct world *iworld, struct city *pcity)
{
  const struct city *old;

  if (!iworld->units->contains(pcity->id)) {
    old = pcity;
    fc_assert_ret_msg(nullptr != old, "IDEX: city unreg missing: %d %p %s",
                      pcity->id, (void *) pcity, city_name_get(pcity));
    fc_assert_ret_msg(old == pcity,
                      "IDEX: city unreg mismatch: "
                      "unreg %d %p %s, old %d %p %s",
                      pcity->id, (void *) pcity, city_name_get(pcity),
                      old->id, (void *) old, city_name_get(old));
  }
  iworld->cities->remove(pcity->id);
}

/**
    Remove a unit from idex, with current punit->id.
    Call this when punit deleted.
 */
void idex_unregister_unit(struct world *iworld, struct unit *punit)
{
  const struct unit *old;

  if (!iworld->units->contains(punit->id)) {
    old = punit;
    fc_assert_ret_msg(nullptr != old, "IDEX: unit unreg missing: %d %p %s",
                      punit->id, (void *) punit, unit_rule_name(punit));
    fc_assert_ret_msg(old == punit,
                      "IDEX: unit unreg mismatch: "
                      "unreg %d %p %s, old %d %p %s",
                      punit->id, (void *) punit, unit_rule_name(punit),
                      old->id, (void *) old, unit_rule_name(old));
  }
  iworld->units->remove(punit->id);
}

/**
    Lookup city with given id.
    Returns nullptr if the city is not registered (which is not an error).
 */
struct city *idex_lookup_city(struct world *iworld, int id)
{
  const struct city *pcity;

  pcity = iworld->cities->value(id);

  return const_cast<struct city *>(pcity);
}

/**
    Lookup unit with given id.
    Returns nullptr if the unit is not registered (which is not an error).
 */
struct unit *idex_lookup_unit(struct world *iworld, int id)
{
  const struct unit *punit;

  punit = iworld->units->value(id);

  return const_cast<struct unit *>(punit);
}
