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
#include "unittype.h"

#include "aiactions.h"

/**********************************************************************/ /**
   Returns TRUE if the specified unit type is able to perform diplomatic
   actions against cities.
 **************************************************************************/
bool aia_utype_is_considered_spy_vs_city(const struct unit_type *putype)
{
  return (
      utype_can_do_action_result(putype, ACTRES_SPY_POISON)
      || utype_can_do_action_result(putype, ACTRES_SPY_SPREAD_PLAGUE)
      || utype_can_do_action_result(putype, ACTRES_SPY_SABOTAGE_CITY)
      || utype_can_do_action_result(putype,
                                    ACTRES_SPY_TARGETED_SABOTAGE_CITY)
      || utype_can_do_action_result(putype,
                                    ACTRES_SPY_SABOTAGE_CITY_PRODUCTION)
      || utype_can_do_action_result(putype, ACTRES_SPY_INCITE_CITY)
      || utype_can_do_action_result(putype, ACTRES_ESTABLISH_EMBASSY)
      || utype_can_do_action_result(putype, ACTRES_SPY_STEAL_TECH)
      || utype_can_do_action_result(putype, ACTRES_SPY_TARGETED_STEAL_TECH)
      || utype_can_do_action_result(putype, ACTRES_SPY_INVESTIGATE_CITY)
      || utype_can_do_action_result(putype, ACTRES_SPY_STEAL_GOLD)
      || utype_can_do_action_result(putype, ACTRES_STEAL_MAPS)
      || utype_can_do_action_result(putype, ACTRES_SPY_NUKE));
}

/**********************************************************************/ /**
   Returns TRUE if the specified unit type is able to perform diplomatic
   actions.
 **************************************************************************/
bool aia_utype_is_considered_spy(const struct unit_type *putype)
{
  return (aia_utype_is_considered_spy_vs_city(putype)
          || utype_can_do_action_result(putype, ACTRES_SPY_ATTACK)
          || utype_can_do_action_result(putype, ACTRES_SPY_BRIBE_UNIT)
          || utype_can_do_action_result(putype, ACTRES_SPY_SABOTAGE_UNIT));
}

/**********************************************************************/ /**
   Returns TRUE if the specified unit type is able to perform trade
   caravan actions.
 **************************************************************************/
bool aia_utype_is_considered_caravan_trade(const struct unit_type *putype)
{
  return (utype_can_do_action_result(putype, ACTRES_TRADE_ROUTE)
          || utype_can_do_action_result(putype, ACTRES_MARKETPLACE));
}

/**********************************************************************/ /**
   Returns TRUE if the specified unit type is able to perform caravan
   actions.
 **************************************************************************/
bool aia_utype_is_considered_caravan(const struct unit_type *putype)
{
  return (aia_utype_is_considered_caravan_trade(putype)
          || utype_can_do_action_result(putype, ACTRES_HELP_WONDER));
}

/**********************************************************************/ /**
   Returns TRUE if the specified unit type is able to perform worker
   actions.
 **************************************************************************/
bool aia_utype_is_considered_worker(const struct unit_type *putype)
{
  return (utype_can_do_action_result(putype, ACTRES_TRANSFORM_TERRAIN)
          || utype_can_do_action_result(putype, ACTRES_CULTIVATE)
          || utype_can_do_action_result(putype, ACTRES_PLANT)
          || utype_can_do_action_result(putype, ACTRES_ROAD)
          || utype_can_do_action_result(putype, ACTRES_BASE)
          || utype_can_do_action_result(putype, ACTRES_MINE)
          || utype_can_do_action_result(putype, ACTRES_IRRIGATE)
          || utype_can_do_action_result(putype, ACTRES_CLEAN_POLLUTION)
          || utype_can_do_action_result(putype, ACTRES_CLEAN_FALLOUT));
}
