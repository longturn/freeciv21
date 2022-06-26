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
// common
#include "map.h"

// server
#include "hand_gen.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"

/**
   Handle player_place_infra packet
 */
void handle_player_place_infra(struct player *pplayer, int tile, int extra)
{
  struct tile *ptile;
  struct extra_type *pextra;

  if (!terrain_control.infrapoints) {
    return;
  }

  ptile = index_to_tile(&(wld.map), tile);
  pextra = extra_by_number(extra);

  if (ptile == nullptr || pextra == nullptr) {
    return;
  }

  if (!map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
    notify_player(pplayer, nullptr, E_LOW_ON_FUNDS, ftc_server,
                  _("Cannot place %s to unseen tile."),
                  extra_name_translation(pextra));
    return;
  }

  if (pplayer->economic.infra_points < pextra->infracost) {
    notify_player(pplayer, nullptr, E_LOW_ON_FUNDS, ftc_server,
                  _("Cannot place %s for lack of infrapoints."),
                  extra_name_translation(pextra));
    return;
  }

  if (!player_can_place_extra(pextra, pplayer, ptile)) {
    notify_player(pplayer, nullptr, E_LOW_ON_FUNDS, ftc_server,
                  _("Cannot place unbuildable %s."),
                  extra_name_translation(pextra));
    return;
  }

  pplayer->economic.infra_points -= pextra->infracost;
  send_player_info_c(pplayer, pplayer->connections);

  ptile->placing = pextra;

  if (pextra->build_time > 0) {
    ptile->infra_turns = pextra->build_time;
  } else {
    ptile->infra_turns =
        tile_terrain(ptile)->placing_time * pextra->build_time_factor;
  }

  /* update_tile_knowledge() would not know to send the tile
   * when only placing has changed, so send it explicitly. */
  send_tile_info(pplayer->connections, ptile, false);
}
