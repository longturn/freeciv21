/**************************************************************************
\^~~~~\   )  (   /~~~~^/ *     _      Copyright (c) 1996-2020 Freeciv21 and
 ) *** \  {**}  / *** (  *  _ {o} _      Freeciv contributors. This file is
  ) *** \_ ^^ _/ *** (   * {o}{o}{o}   part of Freeciv21. Freeciv21 is free
  ) ****   vv   **** (   *  ~\ | /~software: you can redistribute it and/or
   )_****      ****_(    *    OoO      modify it under the terms of the GNU
     )*** m  m ***(      *    /|\      General Public License  as published
       by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version. You should have received  a copy of
                        the GNU General Public License along with Freeciv21.
                                 If not, see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "ai.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "player.h"
#include "research.h"
#include "tech.h"
#include "tile.h"
#include "unittype.h"

/* server */
#include "aiiface.h"
#include "plrhand.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "techtools.h"
#include "unittools.h"

/* ai */
#include "difficulty.h"

#include "animals.h"

/************************************************************************/ /**
   Return suitable animal type for the terrain
 ****************************************************************************/
static const struct unit_type *animal_for_terrain(struct terrain *pterr)
{
  return pterr->animal;
}

/************************************************************************/ /**
   Try to add one animal to the map.
 ****************************************************************************/
static void place_animal(struct player *plr)
{
  struct tile *ptile = rand_map_pos(&(wld.map));
  const struct unit_type *ptype;

  extra_type_by_rmcause_iterate(ERM_ENTER, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      /* Animals should not displace huts */
      /* FIXME: might HUT_NOTHING animals appear here? */
      return;
    }
  }
  extra_type_by_rmcause_iterate_end;

  if (unit_list_size(ptile->units) > 0 || tile_city(ptile)) {
    return;
  }
  adjc_iterate(&(wld.map), ptile, padj)
  {
    if (unit_list_size(padj->units) > 0 || tile_city(padj)) {
      /* No animals next to start units or start city */
      return;
    }
  }
  adjc_iterate_end;

  ptype = animal_for_terrain(tile_terrain(ptile));

  if (ptype != NULL) {
    struct unit *punit;

    fc_assert_ret(can_exist_at_tile(&(wld.map), ptype, ptile));

    punit = create_unit(plr, ptile, ptype, 0, 0, -1);

    send_unit_info(NULL, punit);
  }
}

/************************************************************************/ /**
   Create animal kingdom player and his units.
 ****************************************************************************/
void create_animals()
{
  struct nation_type *anination;
  struct player *plr;
  struct research *presearch;
  int i;

  if (wld.map.server.animals <= 0) {
    return;
  }

  anination = pick_a_nation(NULL, false, true, ANIMAL_BARBARIAN);

  if (anination == NO_NATION_SELECTED) {
    return;
  }

  plr = server_create_player(-1, default_ai_type_name(), NULL, false);
  if (plr == NULL) {
    return;
  }
  server_player_init(plr, true, true);

  player_set_nation(plr, anination);
  player_nation_defaults(plr, anination, true);

  assign_player_colors();

  server.nbarbarians++;

  sz_strlcpy(plr->username, _(ANON_USER_NAME));
  plr->unassigned_user = true;
  plr->is_connected = false;
  plr->government = init_government_of_nation(anination);
  plr->economic.gold = 100;

  plr->phase_done = true;

  set_as_ai(plr);
  plr->ai_common.barbarian_type = ANIMAL_BARBARIAN;
  set_ai_level_directer(plr, ai_level(game.info.skill_level));

  presearch = research_get(plr);
  init_tech(presearch, true);
  give_initial_techs(presearch, 0);

  /* Ensure that we are at war with everyone else */
  players_iterate(pplayer)
  {
    if (pplayer != plr) {
      player_diplstate_get(pplayer, plr)->type = DS_WAR;
      player_diplstate_get(plr, pplayer)->type = DS_WAR;
    }
  }
  players_iterate_end;

  CALL_PLR_AI_FUNC(gained_control, plr, plr);

  send_player_all_c(plr, NULL);
  /* Send research info after player info, else the client will complain
   * about invalid team. */
  send_research_info(presearch, NULL);

  for (i = 0;
       i < wld.map.xsize * wld.map.ysize * wld.map.server.animals / 1000;
       i++) {
    place_animal(plr);
  }
}
