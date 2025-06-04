// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/**
 * \file
 * idex = ident index: a lookup table for quick mapping of unit and city
 * id values to unit and city pointers.
 *
 * Method: use separate hash tables for each type.
 * Means code duplication for city/unit cases, but simplicity advantages.
 * Don't have to manage memory at all: store pointers to unit and city
 * structs allocated elsewhere, and keys are pointers to id values inside
 * the structs.
 *
 * Note id values should probably be unsigned int: here leave as plain int
 * so can use pointers to pcity->id etc.
 */

// self
#include "idex.h"

// utility
#include "log.h"

// common
#include "city.h"
#include "fc_types.h"
#include "unittype.h"
#include "world_object.h"

// Qt
#include <QHash>

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
