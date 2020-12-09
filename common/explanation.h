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

#pragma once

/* A category of reasons why an action isn't enabled. */
enum ane_kind {
  /* Explanation: wrong actor unit. */
  ANEK_ACTOR_UNIT,
  /* Explanation: no action target. */
  ANEK_MISSING_TARGET,
  /* Explanation: the action is redundant vs this target. */
  ANEK_BAD_TARGET,
  /* Explanation: bad actor terrain. */
  ANEK_BAD_TERRAIN_ACT,
  /* Explanation: bad target terrain. */
  ANEK_BAD_TERRAIN_TGT,
  /* Explanation: being transported. */
  ANEK_IS_TRANSPORTED,
  /* Explanation: not being transported. */
  ANEK_IS_NOT_TRANSPORTED,
  /* Explanation: transports a cargo unit. */
  ANEK_IS_TRANSPORTING,
  /* Explanation: doesn't transport a cargo unit. */
  ANEK_IS_NOT_TRANSPORTING,
  /* Explanation: actor unit has a home city. */
  ANEK_ACTOR_HAS_HOME_CITY,
  /* Explanation: actor unit has no a home city. */
  ANEK_ACTOR_HAS_NO_HOME_CITY,
  /* Explanation: must declare war first. */
  ANEK_NO_WAR,
  /* Explanation: must break peace first. */
  ANEK_PEACE,
  /* Explanation: can't be done to domestic targets. */
  ANEK_DOMESTIC,
  /* Explanation: can't be done to foreign targets. */
  ANEK_FOREIGN,
  /* Explanation: this nation can't act. */
  ANEK_NATION_ACT,
  /* Explanation: this nation can't be targeted. */
  ANEK_NATION_TGT,
  /* Explanation: not enough MP left. */
  ANEK_LOW_MP,
  /* Explanation: can't be done to city centers. */
  ANEK_IS_CITY_CENTER,
  /* Explanation: can't be done to non city centers. */
  ANEK_IS_NOT_CITY_CENTER,
  /* Explanation: can't be done to claimed target tiles. */
  ANEK_TGT_IS_CLAIMED,
  /* Explanation: can't be done to unclaimed target tiles. */
  ANEK_TGT_IS_UNCLAIMED,
  /* Explanation: can't be done because target is too near. */
  ANEK_DISTANCE_NEAR,
  /* Explanation: can't be done because target is too far away. */
  ANEK_DISTANCE_FAR,
  /* Explanation: can't be done to targets that far from the coast line. */
  ANEK_TRIREME_MOVE,
  /* Explanation: can't be done because the actor can't exit its
   * transport. */
  ANEK_DISEMBARK_ACT,
  /* Explanation: actor can't reach unit at target. */
  ANEK_TGT_UNREACHABLE,
  /* Explanation: the action is disabled in this scenario. */
  ANEK_SCENARIO_DISABLED,
  /* Explanation: too close to a city. */
  ANEK_CITY_TOO_CLOSE_TGT,
  /* Explanation: the target city is too big. */
  ANEK_CITY_TOO_BIG,
  /* Explanation: the target city's population limit banned the action. */
  ANEK_CITY_POP_LIMIT,
  /* Explanation: the specified city don't have the needed capacity. */
  ANEK_CITY_NO_CAPACITY,
  /* Explanation: the target unit can't switch sides because it is unique
   * and the actor player already has one. */
  ANEK_TGT_IS_UNIQUE_ACT_HAS,
  /* Explanation: the target tile is unknown. */
  ANEK_TGT_TILE_UNKNOWN,
  /* Explanation: the actor player can't afford performing this action. */
  ANEK_ACT_NOT_ENOUGH_MONEY,
  /* Explanation: the action is blocked by another action. */
  ANEK_ACTION_BLOCKS,
  /* Explanation not detected. */
  ANEK_UNKNOWN,
};


