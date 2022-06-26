/***********************************************************************
Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/

// utility
#include "log.h"
#include "rand.h"

// common
#include "city.h"
#include "game.h"
#include "player.h"

#include "citizens.h"

/**
   Initialise citizens data.
 */
void citizens_init(struct city *pcity)
{
  fc_assert_ret(pcity);

  if (!game.info.citizen_nationality) {
    return;
  }

  /* For the nationality of the citizens the informations for all player
   * slots are allocated as once. Considering a size of citizens (= char)
   * this results in an allocation of 2 * 128 * 1 bytes for the citizens
   * per nation as well as the timer for a nationality change. */
  if (pcity->nationality == nullptr) {
    /* Allocate the memory*/
    pcity->nationality = new citizens[MAX_NUM_PLAYER_SLOTS]();
  } else {
    // Reset the nationality information.
    memset(pcity->nationality, 0,
           MAX_NUM_PLAYER_SLOTS * sizeof(*pcity->nationality));
  }
}

/**
   Free citizens data.
 */
void citizens_free(struct city *pcity)
{
  fc_assert_ret(pcity);

  delete[] pcity->nationality;
  pcity->nationality = nullptr;
}

/**
   Get the number of citizens with the given nationality.

   The player_slot has to be used as the client does not has the exact
   knowledge about the players at certain points (especially at connecting).
 */
citizens citizens_nation_get(const struct city *pcity,
                             const struct player_slot *pslot)
{
  if (!game.info.citizen_nationality) {
    return 0;
  }

  fc_assert_ret_val(pslot != nullptr, 0);
  fc_assert_ret_val(pcity != nullptr, 0);
  fc_assert_ret_val(pcity->nationality != nullptr, 0);

  return *(pcity->nationality + player_slot_index(pslot));
}

/**
   Get the number of foreign citizens.
 */
citizens citizens_nation_foreign(const struct city *pcity)
{
  return citizens_count(pcity)
         - citizens_nation_get(pcity, city_owner(pcity)->slot);
}

/**
   Add a (positive or negative) value to the citizens of the given
 nationality. As citizens is an unsigned value use int for the parameter
 'add'.

   The player_slot has to be used as the client does not has the exact
   knowledge about the players at certain points (especially at connecting).
 */
void citizens_nation_add(struct city *pcity, const struct player_slot *pslot,
                         int add)
{
  citizens nationality = citizens_nation_get(pcity, pslot);

  if (!game.info.citizen_nationality) {
    return;
  }

  fc_assert_ret(pslot != nullptr);
  fc_assert_ret(pcity != nullptr);
  fc_assert_ret(pcity->nationality != nullptr);

  fc_assert_ret(MAX_CITY_SIZE - nationality > add);
  fc_assert_ret(nationality >= -add);

  citizens_nation_set(pcity, pslot, nationality + add);
}

/**
   Convert a (positive or negative) value to the citizens from one nation to
   another. As citizens is an unsigned value use int for the parameter
 'move'.

   The player_slot has to be used as the client does not has the exact
   knowledge about the players at certain points (especially at connecting).
 */
void citizens_nation_move(struct city *pcity,
                          const struct player_slot *pslot_from,
                          const struct player_slot *pslot_to, int move)
{
  citizens_nation_add(pcity, pslot_from, -move);
  citizens_nation_add(pcity, pslot_to, move);
}

/**
   Set the number of citizens with the given nationality.

   The player_slot has to be used as the client does not has the exact
   knowledge about the players at certain points (especially at connecting).
 */
void citizens_nation_set(struct city *pcity, const struct player_slot *pslot,
                         citizens count)
{
  if (!game.info.citizen_nationality) {
    return;
  }

  fc_assert_ret(pslot != nullptr);
  fc_assert_ret(pcity != nullptr);
  fc_assert_ret(pcity->nationality != nullptr);

  *(pcity->nationality + player_slot_index(pslot)) = count;
}

/**
   Return the number of citizens in a city.
 */
citizens citizens_count(const struct city *pcity)
{
  // Use int here to check for an possible overflow at the end.
  int count = 0;

  if (!game.info.citizen_nationality) {
    return city_size_get(pcity);
  }

  citizens_iterate(pcity, pslot, nationality)
  {
    /* If the citizens of a nation is greater than 0 there should be a player
     * for this nation. This test should only be done on the server as the
     * client does not has the knowledge about all players all the time. */
    fc_assert_ret_val(!is_server()
                          || player_slot_get_player(pslot) != nullptr,
                      city_size_get(pcity));

    count += nationality;
  }
  citizens_iterate_end;

  fc_assert_ret_val(count >= 0 && count <= MAX_CITY_SIZE,
                    city_size_get(pcity));

  return static_cast<citizens>(count);
}

/**
   Return random citizen from city.
 */
struct player_slot *citizens_random(const struct city *pcity)
{
  int total = citizens_count(pcity);
  int chocen = fc_rand(total);

  citizens_iterate(pcity, pslot, nationality)
  {
    chocen -= nationality;
    if (chocen <= 0) {
      return pslot;
    }
  }
  citizens_iterate_end;

  fc_assert(false);

  return nullptr;
}
