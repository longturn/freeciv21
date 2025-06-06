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

#include <QBitArray>

// utility
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "support.h"

// common
#include "actions.h"
#include "ai.h"
#include "base.h"
#include "borders.h"
#include "events.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "road.h"
#include "unit.h"
#include "unitlist.h"
#include "vision.h"

// server
#include "citytools.h"
#include "cityturn.h"
#include "notify.h"
#include "plrhand.h"
#include "sanitycheck.h"
#include "sernet.h"
#include "srv_main.h"
#include "unithand.h"
#include "unittools.h"

/* server/generator */
#include "mapgen_utils.h"

#include "maphand.h"

#define MAXIMUM_CLAIMED_OCEAN_SIZE (20)

// Suppress send_tile_info() during game_load()
static bool send_tile_suppressed = false;

static void player_tile_init(struct tile *ptile, struct player *pplayer);
static void player_tile_free(struct tile *ptile, struct player *pplayer);
static void give_tile_info_from_player_to_player(struct player *pfrom,
                                                 struct player *pdest,
                                                 struct tile *ptile);
static void shared_vision_change_seen(struct player *pplayer,
                                      struct tile *ptile,
                                      const v_radius_t change,
                                      bool can_reveal_tiles);
static void map_change_seen(struct player *pplayer, struct tile *ptile,
                            const v_radius_t change, bool can_reveal_tiles);
static void map_change_own_seen(struct player *pplayer, struct tile *ptile,
                                const v_radius_t change);
static inline int map_get_seen(const struct player *pplayer,
                               const struct tile *ptile,
                               enum vision_layer vlayer);
static inline int map_get_own_seen(const struct player *pplayer,
                                   const struct tile *ptile,
                                   enum vision_layer vlayer);

static bool is_claimable_ocean(struct tile *ptile, struct tile *source,
                               struct player *pplayer);

/**
   Used only in global_warming() and nuclear_winter() below.
 */
static bool is_terrain_ecologically_wet(struct tile *ptile)
{
  return (is_terrain_class_near_tile(ptile, TC_OCEAN)
          || tile_has_river(ptile)
          || count_river_near_tile(ptile, nullptr) > 0);
}

/**
   Wrapper for climate_change().
 */
void global_warming(int effect)
{
  climate_change(true, effect);
  notify_player(nullptr, nullptr, E_GLOBAL_ECO, ftc_server,
                _("Global warming has occurred!"));
  notify_player(nullptr, nullptr, E_GLOBAL_ECO, ftc_server,
                _("Coastlines have been flooded and vast "
                  "ranges of grassland have become deserts."));
}

/**
   Wrapper for climate_change().
 */
void nuclear_winter(int effect)
{
  climate_change(false, effect);
  notify_player(nullptr, nullptr, E_GLOBAL_ECO, ftc_server,
                _("Nuclear winter has occurred!"));
  notify_player(nullptr, nullptr, E_GLOBAL_ECO, ftc_server,
                _("Wetlands have dried up and vast "
                  "ranges of grassland have become tundra."));
}

/**
   Do a climate change. Global warming occurred if 'warming' is TRUE, else
   there is a nuclear winter.
 */
void climate_change(bool warming, int effect)
{
  int k = map_num_tiles();
  bool used[k];
  memset(used, 0, sizeof(used));

  qDebug("Climate change: %s (%d)",
         warming ? "Global warming" : "Nuclear winter", effect);

  while (effect > 0 && (k--) > 0) {
    struct terrain *old, *candidates[2], *tnew;
    struct tile *ptile;
    int i;

    do {
      // We want to transform a tile at most once due to a climate change.
      ptile = rand_map_pos(&(wld.map));
    } while (used[tile_index(ptile)]);
    used[tile_index(ptile)] = true;

    old = tile_terrain(ptile);
    /* Prefer the transformation that's appropriate to the ambient moisture,
     * but be prepared to fall back in exceptional circumstances */
    {
      struct terrain *wetter, *drier;
      wetter =
          warming ? old->warmer_wetter_result : old->cooler_wetter_result;
      drier = warming ? old->warmer_drier_result : old->cooler_drier_result;
      if (is_terrain_ecologically_wet(ptile)) {
        candidates[0] = wetter;
        candidates[1] = drier;
      } else {
        candidates[0] = drier;
        candidates[1] = wetter;
      }
    }

    /* If the preferred transformation is ruled out for some exceptional
     * reason specific to this tile, fall back to the other, rather than
     * letting this tile be immune to change. */
    for (i = 0; i < 2; i++) {
      tnew = candidates[i];

      /* If the preferred transformation simply hasn't been specified
       * for this terrain at all, don't fall back to the other. */
      if (tnew == T_NONE) {
        break;
      }

      if (tile_city(ptile) != nullptr
          && terrain_has_flag(tnew, TER_NO_CITIES)) {
        /* do not change to a terrain with the flag TER_NO_CITIES if the tile
         * has a city */
        continue;
      }

      /* Only change between water and land at coastlines, and between
       * frozen and unfrozen at ice margin */
      if (!terrain_surroundings_allow_change(ptile, tnew)) {
        continue;
      }

      // OK!
      break;
    }
    if (i == 2) {
      // Neither transformation was permitted. Give up.
      continue;
    }

    if (tnew != T_NONE && old != tnew) {
      effect--;

      // Really change the terrain.
      tile_change_terrain(ptile, tnew);
      check_terrain_change(ptile, old);
      update_tile_knowledge(ptile);

      // Check the unit activities.
      unit_activities_cancel_all_illegal_area(ptile);
    } else if (old == tnew) {
      // This counts toward a climate change although nothing is changed.
      effect--;
    }
  }
}

/**
   Check city for extra upgrade. Returns whether anything was done.
   *gained will be set if there's exactly one kind of extra added.
 */
bool upgrade_city_extras(struct city *pcity, struct extra_type **gained)
{
  struct tile *ptile = pcity->tile;
  struct player *pplayer = city_owner(pcity);
  bool upgradet = false;

  extra_type_iterate(pextra)
  {
    if (!tile_has_extra(ptile, pextra)) {
      if (extra_has_flag(pextra, EF_ALWAYS_ON_CITY_CENTER)
          || (extra_has_flag(pextra, EF_AUTO_ON_CITY_CENTER)
              && player_can_build_extra(pextra, pplayer, ptile)
              && !tile_has_conflicting_extra(ptile, pextra))) {
        tile_add_extra(pcity->tile, pextra);
        if (gained != nullptr) {
          if (upgradet) {
            *gained = nullptr;
          } else {
            *gained = pextra;
          }
        }
        upgradet = true;
      }
    }
  }
  extra_type_iterate_end;

  return upgradet;
}

/**
   To be called when a player gains some better extra building tech
   for the first time.  Sends a message, and upgrades all city
   squares to new extras.  "discovery" just affects the message: set to
      1 if the tech is a "discovery",
      0 if otherwise acquired (conquer/trade/GLib).        --dwp
 */
void upgrade_all_city_extras(struct player *pplayer, bool discovery)
{
  int cities_upgradet = 0;
  struct extra_type *upgradet = nullptr;
  bool multiple_types = false;
  int cities_total = city_list_size(pplayer->cities);
  int percent;

  conn_list_do_buffer(pplayer->connections);

  city_list_iterate(pplayer->cities, pcity)
  {
    struct extra_type *new_upgrade;

    if (upgrade_city_extras(pcity, &new_upgrade)) {
      update_tile_knowledge(pcity->tile);
      cities_upgradet++;
      if (new_upgrade == nullptr) {
        // This single city alone had multiple types
        multiple_types = true;
      } else if (upgradet == nullptr) {
        // First gained
        upgradet = new_upgrade;
      } else if (upgradet != new_upgrade) {
        // Different type from what another city got.
        multiple_types = true;
      }
    }
  }
  city_list_iterate_end;

  if (cities_total > 0) {
    percent = cities_upgradet * 100 / cities_total;
  } else {
    percent = 0;
  }

  if (cities_upgradet > 0) {
    if (discovery) {
      if (percent >= 75) {
        notify_player(
            pplayer, nullptr, E_TECH_GAIN, ftc_server,
            _("New hope sweeps like fire through the country as "
              "the discovery of new infrastructure building technology "
              "is announced."));
      }
    } else {
      if (percent >= 75) {
        notify_player(
            pplayer, nullptr, E_TECH_GAIN, ftc_server,
            _("The people are pleased to hear that your "
              "scientists finally know about new infrastructure building "
              "technology."));
      }
    }

    if (multiple_types) {
      notify_player(pplayer, nullptr, E_TECH_GAIN, ftc_server,
                    _("Workers spontaneously gather and upgrade all "
                      "possible cities with better infrastructure."));
    } else {
      notify_player(pplayer, nullptr, E_TECH_GAIN, ftc_server,
                    _("Workers spontaneously gather and upgrade all "
                      "possible cities with %s."),
                    extra_name_translation(upgradet));
    }
  }

  conn_list_do_unbuffer(pplayer->connections);
}

/**
   Return TRUE iff the player me really gives shared vision to player them.
 */
bool really_gives_vision(struct player *me, struct player *them)
{
  return BV_ISSET(me->server.really_gives_vision, player_index(them));
}

/**
   Start buffering shared vision
 */
static void buffer_shared_vision(struct player *pplayer)
{
  players_iterate(pplayer2)
  {
    if (really_gives_vision(pplayer, pplayer2)) {
      conn_list_compression_freeze(pplayer2->connections);
      conn_list_do_buffer(pplayer2->connections);
    }
  }
  players_iterate_end;
  conn_list_compression_freeze(pplayer->connections);
  conn_list_do_buffer(pplayer->connections);
}

/**
   Stop buffering shared vision
 */
static void unbuffer_shared_vision(struct player *pplayer)
{
  players_iterate(pplayer2)
  {
    if (really_gives_vision(pplayer, pplayer2)) {
      conn_list_do_unbuffer(pplayer2->connections);
      conn_list_compression_thaw(pplayer2->connections);
    }
  }
  players_iterate_end;
  conn_list_do_unbuffer(pplayer->connections);
  conn_list_compression_thaw(pplayer->connections);
}

/**
   Give information about whole map (all tiles) from player to player.
   Takes care of shared vision chains.
 */
void give_map_from_player_to_player(struct player *pfrom,
                                    struct player *pdest)
{
  buffer_shared_vision(pdest);

  whole_map_iterate(&(wld.map), ptile)
  {
    give_tile_info_from_player_to_player(pfrom, pdest, ptile);
  }
  whole_map_iterate_end;

  unbuffer_shared_vision(pdest);
  city_thaw_workers_queue();
  sync_cities();
}

/**
   Give information about all oceanic tiles from player to player
 */
void give_seamap_from_player_to_player(struct player *pfrom,
                                       struct player *pdest)
{
  buffer_shared_vision(pdest);

  whole_map_iterate(&(wld.map), ptile)
  {
    if (is_ocean_tile(ptile)) {
      give_tile_info_from_player_to_player(pfrom, pdest, ptile);
    }
  }
  whole_map_iterate_end;

  unbuffer_shared_vision(pdest);
  city_thaw_workers_queue();
  sync_cities();
}

/**
   Give information about tiles within city radius from player to player
 */
void give_citymap_from_player_to_player(struct city *pcity,
                                        struct player *pfrom,
                                        struct player *pdest)
{
  struct tile *pcenter = city_tile(pcity);

  buffer_shared_vision(pdest);

  city_tile_iterate(city_map_radius_sq_get(pcity), pcenter, ptile)
  {
    give_tile_info_from_player_to_player(pfrom, pdest, ptile);
  }
  city_tile_iterate_end;

  unbuffer_shared_vision(pdest);
  city_thaw_workers_queue();
  sync_cities();
}

/**
   Send all tiles known to specified clients.
   If dest is nullptr means game.est_connections.

   Note for multiple connections this may change "sent" multiple times
   for single player.  This is ok, because "sent" data is just optimised
   calculations, so it will be correct before this, for each connection
   during this, and at end.
 */
void send_all_known_tiles(struct conn_list *dest)
{
  int tiles_sent;

  if (!dest) {
    dest = game.est_connections;
  }

  /* send whole map piece by piece to each player to balance the load
     of the send buffers better */
  tiles_sent = 0;
  conn_list_do_buffer(dest);

  whole_map_iterate(&(wld.map), ptile)
  {
    tiles_sent++;
    if ((tiles_sent % wld.map.xsize) == 0) {
      conn_list_do_unbuffer(dest);
      flush_packets();
      conn_list_do_buffer(dest);
    }

    send_tile_info(dest, ptile, false);
  }
  whole_map_iterate_end;

  conn_list_do_unbuffer(dest);
  flush_packets();
}

/**
   Suppress send_tile_info() during game_load()
 */
bool send_tile_suppression(bool now)
{
  bool formerly = send_tile_suppressed;

  send_tile_suppressed = now;
  return formerly;
}

/**
   Send tile information to all the clients in dest which know and see
   the tile. If dest is nullptr, sends to all clients (game.est_connections)
   which know and see tile.

   Note that this function does not update the playermap.  For that call
   update_tile_knowledge().
 */
void send_tile_info(struct conn_list *dest, struct tile *ptile,
                    bool send_unknown)
{
  struct packet_tile_info info;
  const struct player *owner;
  const struct player *eowner;

  if (dest == nullptr) {
    CALL_FUNC_EACH_AI(tile_info, ptile);
  }

  if (send_tile_suppressed) {
    return;
  }

  if (!dest) {
    dest = game.est_connections;
  }

  info.tile = tile_index(ptile);

  if (ptile->spec_sprite) {
    sz_strlcpy(info.spec_sprite, ptile->spec_sprite);
  } else {
    info.spec_sprite[0] = '\0';
  }

  conn_list_iterate(dest, pconn)
  {
    struct player *pplayer = pconn->playing;

    if (nullptr == pplayer && !pconn->observer) {
      continue;
    }

    if (!pplayer || map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
      info.known = TILE_KNOWN_SEEN;
      info.continent = tile_continent(ptile);
      owner = tile_owner(ptile);
      eowner = extra_owner(ptile);
      info.owner = (owner ? player_number(owner) : MAP_TILE_OWNER_NULL);
      info.extras_owner =
          (eowner ? player_number(eowner) : MAP_TILE_OWNER_NULL);
      info.worked = (nullptr != tile_worked(ptile)) ? tile_worked(ptile)->id
                                                    : IDENTITY_NUMBER_ZERO;

      info.terrain = (nullptr != tile_terrain(ptile))
                         ? terrain_number(tile_terrain(ptile))
                         : terrain_count();
      info.resource = (nullptr != tile_resource(ptile))
                          ? extra_number(tile_resource(ptile))
                          : MAX_EXTRA_TYPES;
      info.placing =
          (nullptr != ptile->placing) ? extra_number(ptile->placing) : -1;
      info.place_turn = (nullptr != ptile->placing)
                            ? game.info.turn + ptile->infra_turns
                            : 0;

      if (pplayer != nullptr) {
        info.extras = map_get_player_tile(ptile, pplayer)->extras;
      } else {
        info.extras = ptile->extras;
      }

      if (ptile->label != nullptr) {
        // Always leave final '/* Always leave final '\0' in place */' in
        // place
        qstrncpy(info.label, ptile->label, sizeof(info.label) - 1);
      } else {
        info.label[0] = '\0';
      }

      send_packet_tile_info(pconn, &info);
    } else if (pplayer && map_is_known(ptile, pplayer)) {
      struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);
      const vision_site *psite = map_get_player_site(ptile, pplayer);

      info.known = TILE_KNOWN_UNSEEN;
      info.continent = tile_continent(ptile);
      owner =
          (game.server.foggedborders ? plrtile->owner : tile_owner(ptile));
      eowner = plrtile->extras_owner;
      info.owner = (owner ? player_number(owner) : MAP_TILE_OWNER_NULL);
      info.extras_owner =
          (eowner ? player_number(eowner) : MAP_TILE_OWNER_NULL);
      info.worked =
          (nullptr != psite) ? psite->identity : IDENTITY_NUMBER_ZERO;

      info.terrain = (nullptr != plrtile->terrain)
                         ? terrain_number(plrtile->terrain)
                         : terrain_count();
      info.resource = (nullptr != plrtile->resource)
                          ? extra_number(plrtile->resource)
                          : MAX_EXTRA_TYPES;
      info.placing = -1;
      info.place_turn = 0;

      info.extras = plrtile->extras;

      // Labels never change, so they are not subject to fog of war
      if (ptile->label != nullptr) {
        sz_strlcpy(info.label, ptile->label);
      } else {
        info.label[0] = '\0';
      }

      send_packet_tile_info(pconn, &info);
    } else if (send_unknown) {
      info.known = TILE_UNKNOWN;
      info.continent = 0;
      info.owner = MAP_TILE_OWNER_NULL;
      info.extras_owner = MAP_TILE_OWNER_NULL;
      info.worked = IDENTITY_NUMBER_ZERO;

      info.terrain = terrain_count();
      info.resource = MAX_EXTRA_TYPES;
      info.placing = -1;
      info.place_turn = 0;

      BV_CLR_ALL(info.extras);

      info.label[0] = '\0';

      send_packet_tile_info(pconn, &info);
    }
  }
  conn_list_iterate_end;
}

/**
   Send basic map information: map size, topology, and is_earth.
 */
void send_map_info(struct conn_list *dest)
{
  struct packet_map_info minfo;

  minfo.xsize = wld.map.xsize;
  minfo.ysize = wld.map.ysize;
  minfo.topology_id = wld.map.topology_id;

  lsend_packet_map_info(dest, &minfo);
}

/**
   Change the seen count of a tile for a pplayer. It will automatically
   handle the shared visions.
 */
static void shared_vision_change_seen(struct player *pplayer,
                                      struct tile *ptile,
                                      const v_radius_t change,
                                      bool can_reveal_tiles)
{
  map_change_own_seen(pplayer, ptile, change);
  map_change_seen(pplayer, ptile, change, can_reveal_tiles);

  players_iterate(pplayer2)
  {
    if (really_gives_vision(pplayer, pplayer2)) {
      map_change_seen(pplayer2, ptile, change, can_reveal_tiles);
    }
  }
  players_iterate_end;
}

/**
   There doesn't have to be a city.
 */
void map_vision_update(struct player *pplayer, struct tile *ptile,
                       const v_radius_t old_radius_sq,
                       const v_radius_t new_radius_sq, bool can_reveal_tiles)
{
  v_radius_t change;
  int max_radius;

  if (old_radius_sq[V_MAIN] == new_radius_sq[V_MAIN]
      && old_radius_sq[V_INVIS] == new_radius_sq[V_INVIS]
      && old_radius_sq[V_SUBSURFACE] == new_radius_sq[V_SUBSURFACE]) {
    return;
  }

  // Determines 'max_radius' value.
  max_radius = 0;
  vision_layer_iterate(v)
  {
    if (max_radius < old_radius_sq[v]) {
      max_radius = old_radius_sq[v];
    }
    if (max_radius < new_radius_sq[v]) {
      max_radius = new_radius_sq[v];
    }
  }
  vision_layer_iterate_end;

#ifdef FREECIV_DEBUG
  log_debug("Updating vision at (%d, %d) in a radius of %d.", TILE_XY(ptile),
            max_radius);
  vision_layer_iterate(v)
  {
    log_debug("  vision layer %d is changing from %d to %d.", v,
              old_radius_sq[v], new_radius_sq[v]);
  }
  vision_layer_iterate_end;
#endif // FREECIV_DEBUG

  buffer_shared_vision(pplayer);
  circle_dxyr_iterate(&(wld.map), ptile, max_radius, tile1, dx, dy, dr)
  {
    vision_layer_iterate(v)
    {
      if (dr > old_radius_sq[v] && dr <= new_radius_sq[v]) {
        change[v] = 1;
      } else if (dr > new_radius_sq[v] && dr <= old_radius_sq[v]) {
        change[v] = -1;
      } else {
        change[v] = 0;
      }
    }
    vision_layer_iterate_end;
    shared_vision_change_seen(pplayer, tile1, change, can_reveal_tiles);
  }
  circle_dxyr_iterate_end;
  unbuffer_shared_vision(pplayer);
}

/**
   Turn a players ability to see inside his borders on or off.

   It is safe to set the current value.
 */
void map_set_border_vision(struct player *pplayer, const bool is_enabled)
{
  const v_radius_t radius_sq = V_RADIUS(is_enabled ? 1 : -1, 0, 0);

  if (pplayer->server.border_vision == is_enabled) {
    /* No change. Changing the seen count beyond what already exists would
     * be a bug. */
    return;
  }

  // Set the new border seer value.
  pplayer->server.border_vision = is_enabled;

  whole_map_iterate(&(wld.map), ptile)
  {
    if (pplayer == ptile->owner) {
      // The tile is within the player's borders.
      shared_vision_change_seen(pplayer, ptile, radius_sq, true);
    }
  }
  whole_map_iterate_end;
}

/**
   Shows the area to the player.  Unless the tile is "seen", it will remain
   fogged and units will be hidden.

   Callers may wish to buffer_shared_vision before calling this function.
 */
void map_show_tile(struct player *src_player, struct tile *ptile)
{
  static int recurse = 0;

  log_debug("Showing %i,%i to %s", TILE_XY(ptile), player_name(src_player));

  fc_assert(recurse == 0);
  recurse++;

  players_iterate(pplayer)
  {
    if (pplayer == src_player || really_gives_vision(src_player, pplayer)) {
      struct city *pcity;

      if (!map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
        map_set_known(ptile, pplayer);

        /* as the tile may be fogged send_tile_info won't always do this for
         * us */
        update_player_tile_knowledge(pplayer, ptile);
        update_player_tile_last_seen(pplayer, ptile);

        send_tile_info(pplayer->connections, ptile, false);

        // remove old cities that exist no more
        reality_check_city(pplayer, ptile);

        if ((pcity = tile_city(ptile))) {
          // as the tile may be fogged send_city_info won't do this for us
          update_dumb_city(pplayer, pcity);
          send_city_info(pplayer, pcity);
        }

        vision_layer_iterate(v)
        {
          if (0 < map_get_seen(pplayer, ptile, v)) {
            unit_list_iterate(ptile->units, punit)
            {
              if (v == unit_type_get(punit)->vlayer) {
                send_unit_info(pplayer->connections, punit);
              }
            }
            unit_list_iterate_end;
          }
        }
        vision_layer_iterate_end;
      }
    }
  }
  players_iterate_end;

  recurse--;
}

/**
   Hides the area to the player.

   Callers may wish to buffer_shared_vision before calling this function.
 */
void map_hide_tile(struct player *src_player, struct tile *ptile)
{
  static int recurse = 0;

  log_debug("Hiding %d,%d to %s", TILE_XY(ptile), player_name(src_player));

  fc_assert(recurse == 0);
  recurse++;

  players_iterate(pplayer)
  {
    if (pplayer == src_player || really_gives_vision(src_player, pplayer)) {
      if (map_is_known(ptile, pplayer)) {
        if (0 < map_get_seen(pplayer, ptile, V_MAIN)) {
          update_player_tile_last_seen(pplayer, ptile);
        }

        // Remove city.
        remove_dumb_city(pplayer, ptile);

        // Remove units.
        vision_layer_iterate(v)
        {
          if (0 < map_get_seen(pplayer, ptile, v)) {
            unit_list_iterate(ptile->units, punit)
            {
              if (v == unit_type_get(punit)->vlayer) {
                unit_goes_out_of_sight(pplayer, punit);
              }
            }
            unit_list_iterate_end;
          }
        }
        vision_layer_iterate_end;
      }

      map_clear_known(ptile, pplayer);

      send_tile_info(pplayer->connections, ptile, true);
    }
  }
  players_iterate_end;

  recurse--;
}

/**
   Shows the area to the player.  Unless the tile is "seen", it will remain
   fogged and units will be hidden.
 */
void map_show_circle(struct player *pplayer, struct tile *ptile,
                     int radius_sq)
{
  buffer_shared_vision(pplayer);

  circle_iterate(&(wld.map), ptile, radius_sq, tile1)
  {
    map_show_tile(pplayer, tile1);
  }
  circle_iterate_end;

  unbuffer_shared_vision(pplayer);
}

/**
   Shows the area to the player.  Unless the tile is "seen", it will remain
   fogged and units will be hidden.
 */
void map_show_all(struct player *pplayer)
{
  buffer_shared_vision(pplayer);

  whole_map_iterate(&(wld.map), ptile) { map_show_tile(pplayer, ptile); }
  whole_map_iterate_end;

  unbuffer_shared_vision(pplayer);
}

/**
   Return whether the player knows the tile.  Knowing a tile means you've
   seen it once (as opposed to seeing a tile which means you can see it now).
 */
bool map_is_known(const struct tile *ptile, const struct player *pplayer)
{
  return !pplayer->tile_known->isEmpty()
         && pplayer->tile_known->at(tile_index(ptile));
}

/**
   Returns whether the layer 'vlayer' of the tile 'ptile' is known and seen
   by the player 'pplayer'.
 */
bool map_is_known_and_seen(const struct tile *ptile,
                           const struct player *pplayer,
                           enum vision_layer vlayer)
{
  return (map_is_known(ptile, pplayer)
          && 0 < map_get_seen(pplayer, ptile, vlayer));
}

/**
   Return whether the player can see the tile.  Seeing a tile means you have
   vision of it now (as opposed to knowing a tile which means you've seen it
   before).  Note that a tile can be seen but not known (currently this only
   happens when a city is founded with some unknown tiles in its radius); in
   this case the tile is unknown (but map_get_seen will still return TRUE).
 */
static inline int map_get_seen(const struct player *pplayer,
                               const struct tile *ptile,
                               enum vision_layer vlayer)
{
  return map_get_player_tile(ptile, pplayer)->seen_count[vlayer];
}

/**
   This function changes the seen state of one player for all vision layers
   of a tile. It reveals the tiles if needed and controls the fog of war.

   See also map_change_own_seen(), shared_vision_change_seen().
 */
void map_change_seen(struct player *pplayer, struct tile *ptile,
                     const v_radius_t change, bool can_reveal_tiles)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);
  bool revealing_tile = false;

#ifdef FREECIV_DEBUG
  log_debug("%s() for player %s (nb %d) at (%d, %d).", __FUNCTION__,
            player_name(pplayer), player_number(pplayer), TILE_XY(ptile));
  vision_layer_iterate(v)
  {
    log_debug("  vision layer %d is changing from %d to %d.", v,
              plrtile->seen_count[v], plrtile->seen_count[v] + change[v]);
  }
  vision_layer_iterate_end;
#endif // FREECIV_DEBUG

  /* Removes units out of vision. First, check invisible layers because
   * we must remove all units before fog of war because clients expect
   * the tile is empty when it is fogged. */
  if (0 > change[V_INVIS]
      && plrtile->seen_count[V_INVIS] == -change[V_INVIS]) {
    log_debug("(%d, %d): hiding invisible units to player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));

    unit_list_iterate(ptile->units, punit)
    {
      if (V_INVIS == unit_type_get(punit)->vlayer
          && can_player_see_unit(pplayer, punit)) {
        unit_goes_out_of_sight(pplayer, punit);
      }
    }
    unit_list_iterate_end;
  }
  if (0 > change[V_SUBSURFACE]
      && plrtile->seen_count[V_SUBSURFACE] == -change[V_SUBSURFACE]) {
    log_debug("(%d, %d): hiding subsurface units to player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));

    unit_list_iterate(ptile->units, punit)
    {
      if (V_SUBSURFACE == unit_type_get(punit)->vlayer
          && can_player_see_unit(pplayer, punit)) {
        unit_goes_out_of_sight(pplayer, punit);
      }
    }
    unit_list_iterate_end;
  }

  if (0 > change[V_MAIN] && plrtile->seen_count[V_MAIN] == -change[V_MAIN]) {
    log_debug("(%d, %d): hiding visible units to player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));

    unit_list_iterate(ptile->units, punit)
    {
      if (V_MAIN == unit_type_get(punit)->vlayer
          && can_player_see_unit(pplayer, punit)) {
        unit_goes_out_of_sight(pplayer, punit);
      }
    }
    unit_list_iterate_end;
  }

  vision_layer_iterate(v)
  {
    // Avoid underflow.
    fc_assert(0 <= change[v] || -change[v] <= plrtile->seen_count[v]);
    plrtile->seen_count[v] += change[v];
  }
  vision_layer_iterate_end;

  /* V_MAIN vision ranges must always be more than invisible ranges
   * (see comment in common/vision.h), so we assume that the V_MAIN
   * seen count cannot be inferior to V_INVIS or V_SUBSURFACE seen count.
   * Moreover, when the fog of war is disabled, V_MAIN has an extra
   * seen count point. */
  fc_assert(plrtile->seen_count[V_INVIS] + !game.info.fogofwar
            <= plrtile->seen_count[V_MAIN]);
  fc_assert(plrtile->seen_count[V_SUBSURFACE] + !game.info.fogofwar
            <= plrtile->seen_count[V_MAIN]);

  if (!map_is_known(ptile, pplayer)) {
    if (0 < plrtile->seen_count[V_MAIN] && can_reveal_tiles) {
      log_debug("(%d, %d): revealing tile to player %s (nb %d).",
                TILE_XY(ptile), player_name(pplayer),
                player_number(pplayer));

      map_set_known(ptile, pplayer);
      revealing_tile = true;
    } else {
      return;
    }
  }

  // Fog the tile.
  if (0 > change[V_MAIN] && 0 == plrtile->seen_count[V_MAIN]) {
    log_debug("(%d, %d): fogging tile for player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));

    update_player_tile_last_seen(pplayer, ptile);
    if (game.server.foggedborders) {
      plrtile->owner = tile_owner(ptile);
    }
    plrtile->extras_owner = extra_owner(ptile);
    send_tile_info(pplayer->connections, ptile, false);
  }

  if ((revealing_tile && 0 < plrtile->seen_count[V_MAIN])
      || (0 < change[V_MAIN]
          /* plrtile->seen_count[V_MAIN] Always set to 1
           * when the fog of war is disabled. */
          && (change[V_MAIN] + !game.info.fogofwar
              == (plrtile->seen_count[V_MAIN])))) {
    struct city *pcity;

    log_debug("(%d, %d): unfogging tile for player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));

    /* Send info about the tile itself.
     * It has to be sent first because the client needs correct
     * continent number before it can handle following packets
     */
    update_player_tile_knowledge(pplayer, ptile);
    send_tile_info(pplayer->connections, ptile, false);

    // Discover units.
    unit_list_iterate(ptile->units, punit)
    {
      if (V_MAIN == unit_type_get(punit)->vlayer) {
        send_unit_info(pplayer->connections, punit);
      }
    }
    unit_list_iterate_end;

    // Discover cities.
    reality_check_city(pplayer, ptile);

    if (nullptr != (pcity = tile_city(ptile))) {
      send_city_info(pplayer, pcity);
    }
  }

  if ((revealing_tile && 0 < plrtile->seen_count[V_INVIS])
      || (0 < change[V_INVIS]
          && change[V_INVIS] == plrtile->seen_count[V_INVIS])) {
    log_debug("(%d, %d): revealing invisible units to player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));
    // Discover units.
    unit_list_iterate(ptile->units, punit)
    {
      if (V_INVIS == unit_type_get(punit)->vlayer) {
        send_unit_info(pplayer->connections, punit);
      }
    }
    unit_list_iterate_end;
  }
  if ((revealing_tile && 0 < plrtile->seen_count[V_SUBSURFACE])
      || (0 < change[V_SUBSURFACE]
          && change[V_SUBSURFACE] == plrtile->seen_count[V_SUBSURFACE])) {
    log_debug("(%d, %d): revealing subsurface units to player %s (nb %d).",
              TILE_XY(ptile), player_name(pplayer), player_number(pplayer));
    // Discover units.
    unit_list_iterate(ptile->units, punit)
    {
      if (V_SUBSURFACE == unit_type_get(punit)->vlayer) {
        send_unit_info(pplayer->connections, punit);
      }
    }
    unit_list_iterate_end;
  }
}

/**
   Returns the own seen count of a tile for a player. It doesn't count the
   shared vision.

   See also map_get_seen().
 */
static inline int map_get_own_seen(const struct player *pplayer,
                                   const struct tile *ptile,
                                   enum vision_layer vlayer)
{
  return map_get_player_tile(ptile, pplayer)->own_seen[vlayer];
}

/**
   Changes the own seen count of a tile for a player.
 */
static void map_change_own_seen(struct player *pplayer, struct tile *ptile,
                                const v_radius_t change)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

  vision_layer_iterate(v) { plrtile->own_seen[v] += change[v]; }
  vision_layer_iterate_end;
}

/**
  Changes site information for player tile.
 */
void change_playertile_site(struct player_tile *ptile,
                            struct vision_site *new_site)
{
  if (ptile->site.get() != new_site) {
    ptile->site.reset(new_site);
  }
}

/**
   Set known status of the tile.
 */
void map_set_known(struct tile *ptile, struct player *pplayer)
{
  pplayer->tile_known->setBit(tile_index(ptile));
}

/**
   Clear known status of the tile.
 */
void map_clear_known(struct tile *ptile, struct player *pplayer)
{
  pplayer->tile_known->setBit(tile_index(ptile), false);
}

/**
   Call this function to unfog all tiles.  This should only be called when
   a player dies or at the end of the game as it will result in permanent
   vision of the whole map.
 */
void map_know_and_see_all(struct player *pplayer)
{
  const v_radius_t radius_sq = V_RADIUS(1, 1, 1);

  buffer_shared_vision(pplayer);
  whole_map_iterate(&(wld.map), ptile)
  {
    map_change_seen(pplayer, ptile, radius_sq, true);
  }
  whole_map_iterate_end;
  unbuffer_shared_vision(pplayer);
}

/**
   Unfogs all tiles for all players.  See map_know_and_see_all.
 */
void show_map_to_all()
{
  players_iterate(pplayer) { map_know_and_see_all(pplayer); }
  players_iterate_end;
}

/**
   Allocate space for map, and initialise the tiles.
   Uses current map.xsize and map.ysize.
 */
void player_map_init(struct player *pplayer)
{
  delete[] pplayer->server.private_map;
  pplayer->server.private_map = new player_tile[MAP_INDEX_SIZE];

  whole_map_iterate(&(wld.map), ptile) { player_tile_init(ptile, pplayer); }
  whole_map_iterate_end;

  pplayer->tile_known->resize(MAP_INDEX_SIZE);
}

/**
   Free a player's private map.
 */
void player_map_free(struct player *pplayer)
{
  if (!pplayer->server.private_map) {
    return;
  }

  whole_map_iterate(&(wld.map), ptile) { player_tile_free(ptile, pplayer); }
  whole_map_iterate_end;

  delete[] pplayer->server.private_map;
  pplayer->server.private_map = nullptr;
  pplayer->tile_known->clear();
}

/**
   Remove all knowledge of a player from main map and other players'
   private maps, and send updates to connected clients.
   Frees all vision_sites associated with that player.
 */
void remove_player_from_maps(struct player *pplayer)
{
  // only after removing borders!
  conn_list_do_buffer(game.est_connections);
  whole_map_iterate(&(wld.map), ptile)
  {
    /* Clear all players' knowledge about the removed player, and free
     * data structures (including those in removed player's player map). */
    bool reality_changed = false;

    players_iterate(aplayer)
    {
      struct player_tile *aplrtile;
      bool changed = false;

      if (!aplayer->server.private_map) {
        continue;
      }
      aplrtile = map_get_player_tile(ptile, aplayer);

      // Free vision sites (cities) for removed and other players
      if (aplrtile && aplrtile->site
          && vision_site_owner(aplrtile->site) == pplayer) {
        change_playertile_site(aplrtile, nullptr);
        changed = true;
      }

      // Remove references to player from others' maps
      if (aplrtile->owner == pplayer) {
        aplrtile->owner = nullptr;
        changed = true;
      }
      if (aplrtile->extras_owner == pplayer) {
        aplrtile->extras_owner = nullptr;
        changed = true;
      }

      /* Must ensure references to dying player are gone from clients
       * before player is destroyed */
      if (changed) {
        // This will use player tile if fogged
        send_tile_info(pplayer->connections, ptile, false);
      }
    }
    players_iterate_end;

    // Clear removed player's knowledge
    if (pplayer->tile_known->size()) {
      map_clear_known(ptile, pplayer);
    }

    // Free all claimed tiles.
    if (tile_owner(ptile) == pplayer) {
      tile_set_owner(ptile, nullptr, nullptr);
      reality_changed = true;
    }
    if (extra_owner(ptile) == pplayer) {
      ptile->extras_owner = nullptr;
      reality_changed = true;
    }

    if (reality_changed) {
      // Update anyone who can see the tile (e.g. global observers)
      send_tile_info(nullptr, ptile, false);
    }
  }
  whole_map_iterate_end;
  conn_list_do_unbuffer(game.est_connections);
}

/**
   We need to use fogofwar_old here, so the player's tiles get
   in the same state as the other players' tiles.
 */
static void player_tile_init(struct tile *ptile, struct player *pplayer)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

  plrtile->terrain = T_UNKNOWN;
  plrtile->resource = nullptr;
  plrtile->owner = nullptr;
  plrtile->extras_owner = nullptr;
  plrtile->site = nullptr;
  BV_CLR_ALL(plrtile->extras);
  if (!game.server.last_updated_year) {
    plrtile->last_updated = game.info.turn;
  } else {
    plrtile->last_updated = game.info.year;
  }

  plrtile->seen_count[V_MAIN] = !game.server.fogofwar_old;
  plrtile->seen_count[V_INVIS] = 0;
  plrtile->seen_count[V_SUBSURFACE] = 0;
  memcpy(plrtile->own_seen, plrtile->seen_count, sizeof(v_radius_t));
}

/**
   Free the memory stored into the player tile.
 */
static void player_tile_free(struct tile *ptile, struct player *pplayer)
{
  map_get_player_tile(ptile, pplayer)->site = nullptr;
}

/**
   Returns city located at given tile from player map.
 */
struct vision_site *map_get_player_city(const struct tile *ptile,
                                        const struct player *pplayer)
{
  struct vision_site *psite = map_get_player_site(ptile, pplayer);

  fc_assert_ret_val(psite == nullptr || psite->location == ptile, nullptr);

  return psite;
}

/**
   Returns site located at given tile from player map.
 */
struct vision_site *map_get_player_site(const struct tile *ptile,
                                        const struct player *pplayer)
{
  return map_get_player_tile(ptile, pplayer)->site.get();
}

/**
   Players' information of tiles is tracked so that fogged area can be kept
   consistent even when the client disconnects.  This function returns the
   player tile information for the given tile and player.
 */
struct player_tile *map_get_player_tile(const struct tile *ptile,
                                        const struct player *pplayer)
{
  fc_assert_ret_val(pplayer->server.private_map, nullptr);

  return pplayer->server.private_map + tile_index(ptile);
}

/**
   Give pplayer the correct knowledge about tile; return TRUE iff
   knowledge changed.

   Note that unlike update_tile_knowledge, this function will not send any
   packets to the client.  Callers may want to call send_tile_info() if this
   function returns TRUE.
 */
bool update_player_tile_knowledge(struct player *pplayer, struct tile *ptile)
{
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

  if (plrtile->terrain != ptile->terrain
      || !BV_ARE_EQUAL(plrtile->extras, ptile->extras)
      || plrtile->resource != ptile->resource
      || plrtile->owner != tile_owner(ptile)
      || plrtile->extras_owner != extra_owner(ptile)) {
    plrtile->terrain = ptile->terrain;
    extra_type_iterate(pextra)
    {
      if (player_knows_extra_exist(pplayer, pextra, ptile)) {
        BV_SET(plrtile->extras, extra_number(pextra));
      } else {
        BV_CLR(plrtile->extras, extra_number(pextra));
      }
    }
    extra_type_iterate_end;
    plrtile->resource = ptile->resource;
    plrtile->owner = tile_owner(ptile);
    plrtile->extras_owner = extra_owner(ptile);

    return true;
  }

  return false;
}

/**
   Update playermap knowledge for everybody who sees the tile, and send a
   packet to everyone whose info is changed.

   Note this only checks for changing of the terrain, special, or resource
   for the tile, since these are the only values held in the playermap.

   A tile's owner always can see terrain changes in his or her territory.
 */
void update_tile_knowledge(struct tile *ptile)
{
  if (server_state() == S_S_INITIAL) {
    return;
  }

  // Players
  players_iterate(pplayer)
  {
    if (map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
      if (update_player_tile_knowledge(pplayer, ptile)) {
        send_tile_info(pplayer->connections, ptile, false);
      }
    }
  }
  players_iterate_end;

  // Global observers
  conn_list_iterate(game.est_connections, pconn)
  {
    struct player *pplayer = pconn->playing;

    if (nullptr == pplayer && pconn->observer) {
      send_tile_info(pconn->self, ptile, false);
    }
  }
  conn_list_iterate_end;
}

/**
   Remember that tile was last seen this year.
 */
void update_player_tile_last_seen(struct player *pplayer, struct tile *ptile)
{
  if (!game.server.last_updated_year) {
    map_get_player_tile(ptile, pplayer)->last_updated = game.info.turn;
  } else {
    map_get_player_tile(ptile, pplayer)->last_updated = game.info.year;
  }
}

/**
   Give tile information from one player to one player.
 */
static void really_give_tile_info_from_player_to_player(struct player *pfrom,
                                                        struct player *pdest,
                                                        struct tile *ptile)
{
  // No need to transfer if pdest can see the tile
  if (map_is_known_and_seen(ptile, pdest, V_MAIN)) {
    return;
  }

  // Nothing to transfer if the pfrom doesn't know the tile
  if (!map_is_known(ptile, pfrom)) {
    return;
  }

  // If pdest knows the tile, check that pfrom has more recent info
  if (map_is_known(ptile, pdest)
      && !map_is_known_and_seen(ptile, pfrom, V_MAIN)
      && map_get_player_tile(ptile, pfrom)->last_updated
             <= map_get_player_tile(ptile, pdest)->last_updated) {
    return;
  }

  // Transfer knowldege
  auto from_tile = map_get_player_tile(ptile, pfrom);
  auto dest_tile = map_get_player_tile(ptile, pdest);

  // Update and send tile knowledge
  map_set_known(ptile, pdest);
  dest_tile->terrain = from_tile->terrain;
  dest_tile->extras = from_tile->extras;
  dest_tile->resource = from_tile->resource;
  dest_tile->owner = from_tile->owner;
  dest_tile->extras_owner = from_tile->extras_owner;
  dest_tile->last_updated = from_tile->last_updated;
  send_tile_info(pdest->connections, ptile, false);

  // Set and send latest city info
  if (from_tile->site) {
    change_playertile_site(dest_tile, new vision_site(*from_tile->site));
    // Note that we don't care if receiver knows vision source city or not.
    send_city_info_at_tile(pdest, pdest->connections, nullptr, ptile);
  }

  city_map_update_tile_frozen(ptile);
}

/**
   Give information about whole map (all tiles) from player to player.
   Does not take care of shared vision; caller is assumed to do that.
 */
static void really_give_map_from_player_to_player(struct player *pfrom,
                                                  struct player *pdest)
{
  whole_map_iterate(&(wld.map), ptile)
  {
    really_give_tile_info_from_player_to_player(pfrom, pdest, ptile);
  }
  whole_map_iterate_end;

  city_thaw_workers_queue();
  sync_cities();
}

/**
   Give tile information from player to player. Handles chains of
   shared vision so that receiver may give information forward.
 */
static void give_tile_info_from_player_to_player(struct player *pfrom,
                                                 struct player *pdest,
                                                 struct tile *ptile)
{
  really_give_tile_info_from_player_to_player(pfrom, pdest, ptile);

  players_iterate(pplayer2)
  {
    if (really_gives_vision(pdest, pplayer2)) {
      really_give_tile_info_from_player_to_player(pfrom, pplayer2, ptile);
    }
  }
  players_iterate_end;
}

/**
   This updates all players' really_gives_vision field.
   If p1 gives p2 shared vision and p2 gives p3 shared vision p1
   should also give p3 shared vision.
 */
static void create_vision_dependencies()
{
  int added;

  players_iterate(pplayer)
  {
    pplayer->server.really_gives_vision = pplayer->gives_shared_vision;
  }
  players_iterate_end;

  /* In words: This terminates when it has run a round without adding
     a dependency. One loop only propagates dependencies one level deep,
     which is why we keep doing it as long as changes occur. */
  do {
    added = 0;
    players_iterate(pplayer)
    {
      players_iterate(pplayer2)
      {
        if (really_gives_vision(pplayer, pplayer2) && pplayer != pplayer2) {
          players_iterate(pplayer3)
          {
            if (really_gives_vision(pplayer2, pplayer3)
                && !really_gives_vision(pplayer, pplayer3)
                && pplayer != pplayer3) {
              BV_SET(pplayer->server.really_gives_vision,
                     player_index(pplayer3));
              added++;
            }
          }
          players_iterate_end;
        }
      }
      players_iterate_end;
    }
    players_iterate_end;
  } while (added > 0);
}

/**
   Starts shared vision between two players.
 */
void give_shared_vision(struct player *pfrom, struct player *pto)
{
  bv_player save_vision[MAX_NUM_PLAYER_SLOTS];
  if (pfrom == pto) {
    return;
  }
  if (gives_shared_vision(pfrom, pto)) {
    qCritical("Trying to give shared vision from %s to %s, "
              "but that vision is already given!",
              player_name(pfrom), player_name(pto));
    return;
  }

  players_iterate(pplayer)
  {
    save_vision[player_index(pplayer)] = pplayer->server.really_gives_vision;
  }
  players_iterate_end;

  BV_SET(pfrom->gives_shared_vision, player_index(pto));
  create_vision_dependencies();
  log_debug("giving shared vision from %s to %s", player_name(pfrom),
            player_name(pto));

  players_iterate(pplayer)
  {
    buffer_shared_vision(pplayer);
    players_iterate(pplayer2)
    {
      if (really_gives_vision(pplayer, pplayer2)
          && !BV_ISSET(save_vision[player_index(pplayer)],
                       player_index(pplayer2))) {
        log_debug("really giving shared vision from %s to %s",
                  player_name(pplayer), player_name(pplayer2));
        whole_map_iterate(&(wld.map), ptile)
        {
          const v_radius_t change =
              V_RADIUS(map_get_own_seen(pplayer, ptile, V_MAIN),
                       map_get_own_seen(pplayer, ptile, V_INVIS),
                       map_get_own_seen(pplayer, ptile, V_SUBSURFACE));

          if (0 < change[V_MAIN] || 0 < change[V_INVIS]) {
            map_change_seen(pplayer2, ptile, change,
                            map_is_known(ptile, pplayer));
          }
        }
        whole_map_iterate_end;

        /* squares that are not seen, but which pfrom may have more recent
           knowledge of */
        really_give_map_from_player_to_player(pplayer, pplayer2);
      }
    }
    players_iterate_end;
    unbuffer_shared_vision(pplayer);
  }
  players_iterate_end;

  if (S_S_RUNNING == server_state()) {
    send_player_info_c(pfrom, nullptr);
  }
}

/**
   Removes shared vision from between two players.
 */
void remove_shared_vision(struct player *pfrom, struct player *pto)
{
  bv_player save_vision[MAX_NUM_PLAYER_SLOTS];

  fc_assert_ret(pfrom != pto);
  if (!gives_shared_vision(pfrom, pto)) {
    qCritical("Tried removing the shared vision from %s to %s, "
              "but it did not exist in the first place!",
              player_name(pfrom), player_name(pto));
    return;
  }

  players_iterate(pplayer)
  {
    save_vision[player_index(pplayer)] = pplayer->server.really_gives_vision;
  }
  players_iterate_end;

  log_debug("removing shared vision from %s to %s", player_name(pfrom),
            player_name(pto));

  BV_CLR(pfrom->gives_shared_vision, player_index(pto));
  create_vision_dependencies();

  players_iterate(pplayer)
  {
    buffer_shared_vision(pplayer);
    players_iterate(pplayer2)
    {
      if (!really_gives_vision(pplayer, pplayer2)
          && BV_ISSET(save_vision[player_index(pplayer)],
                      player_index(pplayer2))) {
        log_debug("really removing shared vision from %s to %s",
                  player_name(pplayer), player_name(pplayer2));
        whole_map_iterate(&(wld.map), ptile)
        {
          const v_radius_t change =
              V_RADIUS(-map_get_own_seen(pplayer, ptile, V_MAIN),
                       -map_get_own_seen(pplayer, ptile, V_INVIS),
                       -map_get_own_seen(pplayer, ptile, V_SUBSURFACE));

          if (0 > change[V_MAIN] || 0 > change[V_INVIS]) {
            map_change_seen(pplayer2, ptile, change, false);
          }
        }
        whole_map_iterate_end;
      }
    }
    players_iterate_end;
    unbuffer_shared_vision(pplayer);
  }
  players_iterate_end;

  if (S_S_RUNNING == server_state()) {
    send_player_info_c(pfrom, nullptr);
  }
}

/**
   Turns FoW on for player
 */
void enable_fog_of_war_player(struct player *pplayer)
{
  const v_radius_t radius_sq = V_RADIUS(-1, 0, 0);

  buffer_shared_vision(pplayer);
  whole_map_iterate(&(wld.map), ptile)
  {
    map_change_seen(pplayer, ptile, radius_sq, false);
  }
  whole_map_iterate_end;
  unbuffer_shared_vision(pplayer);
}

/**
   Turns FoW on for everyone.
 */
void enable_fog_of_war()
{
  players_iterate(pplayer) { enable_fog_of_war_player(pplayer); }
  players_iterate_end;
}

/**
   Turns FoW off for player
 */
void disable_fog_of_war_player(struct player *pplayer)
{
  const v_radius_t radius_sq = V_RADIUS(1, 0, 0);

  buffer_shared_vision(pplayer);
  whole_map_iterate(&(wld.map), ptile)
  {
    map_change_seen(pplayer, ptile, radius_sq, false);
  }
  whole_map_iterate_end;
  unbuffer_shared_vision(pplayer);
}

/**
   Turns FoW off for everyone
 */
void disable_fog_of_war()
{
  players_iterate(pplayer) { disable_fog_of_war_player(pplayer); }
  players_iterate_end;
}

/**
   Set the tile to be a river if required.
   It's required if one of the tiles nearby would otherwise be part of a
   river to nowhere.
   (Note that rivers-to-nowhere can still occur if a single-tile lake is
   transformed away, but this is relatively unlikely.)
   For simplicity, I'm assuming that this is the only exit of the river,
   so I don't need to trace it across the continent.  --CJM
 */
static void ocean_to_land_fix_rivers(struct tile *ptile)
{
  cardinal_adjc_iterate(&(wld.map), ptile, tile1)
  {
    bool ocean_near = false;

    cardinal_adjc_iterate(&(wld.map), tile1, tile2)
    {
      if (is_ocean_tile(tile2)) {
        ocean_near = true;
      }
    }
    cardinal_adjc_iterate_end;

    if (!ocean_near) {
      /* If ruleset has several river types defined, this
       * may cause same tile to contain more than one river. */
      extra_type_by_cause_iterate(EC_ROAD, priver)
      {
        if (tile_has_extra(tile1, priver)
            && road_has_flag(extra_road_get(priver), RF_RIVER)) {
          tile_add_extra(ptile, priver);
        }
      }
      extra_type_by_cause_iterate_end;
    }
  }
  cardinal_adjc_iterate_end;
}

/**
   Helper function for bounce_units_on_terrain_change() that checks units
   on a single tile.
 */
static void check_units_single_tile(struct tile *ptile)
{
  unit_list_iterate_safe(ptile->units, punit)
  {
    if (unit_tile(punit) == ptile && !unit_transported(punit)
        && !can_unit_exist_at_tile(&(wld.map), punit, ptile)) {
      bounce_unit(
          punit, 1,
          [](struct bounce_event bevent) -> void {
            notify_player(unit_owner(bevent.bunit), bevent.to_tile,
                          E_UNIT_RELOCATED, ftc_server,
                          _("Moved your %s due to changing terrain."),
                          unit_link(bevent.bunit));
          },
          [](struct bounce_disband_event bevent) -> void {
            notify_player(unit_owner(bevent.bunit), unit_tile(bevent.bunit),
                          E_UNIT_LOST_MISC, ftc_server,
                          _("Disbanded your %s due to changing terrain."),
                          unit_tile_link(bevent.bunit));
          });
    }
  }
  unit_list_iterate_safe_end;
}

/**
   Check ptile and nearby tiles to see if all units can remain at their
   current locations, and move or disband any that cannot. Call this after
   terrain or specials change on ptile.
 */
void bounce_units_on_terrain_change(struct tile *ptile)
{
  // Check this tile for direct effect on its units
  check_units_single_tile(ptile);
  /* We have to check adjacent tiles too, in case units in cities are now
   * illegal (e.g., boat in a city that has become landlocked). */
  adjc_iterate(&(wld.map), ptile, ptile2)
  {
    check_units_single_tile(ptile2);
  }
  adjc_iterate_end;
}

/**
   Returns TRUE if the terrain change from 'oldter' to 'newter' may require
   expensive reassignment of continents.
 */
bool need_to_reassign_continents(const struct terrain *oldter,
                                 const struct terrain *newter)
{
  bool old_is_ocean, new_is_ocean;

  if (!oldter || !newter) {
    return false;
  }

  old_is_ocean = is_ocean(oldter);
  new_is_ocean = is_ocean(newter);

  return (old_is_ocean && !new_is_ocean) || (!old_is_ocean && new_is_ocean);
}

/**
   Handle local side effects for a terrain change.
 */
void terrain_changed(struct tile *ptile)
{
  struct city *pcity = tile_city(ptile);

  if (pcity != nullptr) {
    // Tile is city center and new terrain may support better extras.
    upgrade_city_extras(pcity, nullptr);
  }

  bounce_units_on_terrain_change(ptile);
}

/**
   Handles local side effects for a terrain change (tile and its
   surroundings). Does *not* handle global side effects (such as reassigning
   continents).
   For in-game terrain changes 'extend_rivers' should be TRUE; for edits it
   should be FALSE.
 */
void fix_tile_on_terrain_change(struct tile *ptile, struct terrain *oldter,
                                bool extend_rivers)
{
  if (is_ocean(oldter) && !is_ocean_tile(ptile)) {
    if (extend_rivers) {
      ocean_to_land_fix_rivers(ptile);
    }
    city_landlocked_sell_coastal_improvements(ptile);
  }

  terrain_changed(ptile);
}

/**
   Handles local and global side effects for a terrain change for a single
   tile.
   Call this in the server immediately after calling tile_change_terrain.
   Assumes an in-game terrain change (e.g., by workers/engineers).
 */
void check_terrain_change(struct tile *ptile, struct terrain *oldter)
{
  struct terrain *newter = tile_terrain(ptile);
  struct tile *claimer;

  /* Check if new terrain is a freshwater terrain next to non-freshwater.
   * In that case, the new terrain is *changed*. */
  if (is_ocean(newter) && terrain_has_flag(newter, TER_FRESHWATER)) {
    bool nonfresh = false;

    adjc_iterate(&(wld.map), ptile, atile)
    {
      if (is_ocean(tile_terrain(atile))
          && !terrain_has_flag(tile_terrain(atile), TER_FRESHWATER)) {
        nonfresh = true;
        break;
      }
    }
    adjc_iterate_end;
    if (nonfresh) {
      /* Need to pick a new, non-freshwater ocean type for this tile.
       * We don't want e.g. Deep Ocean to be propagated to this tile
       * and then to a whole lake by the flooding below, so we pick
       * the shallowest non-fresh oceanic type.
       * Prefer terrain that matches the frozenness of the target. */
      newter = most_shallow_ocean(terrain_has_flag(newter, TER_FROZEN));
      tile_change_terrain(ptile, newter);
    }
  }

  fix_tile_on_terrain_change(ptile, oldter, true);

  // Check for saltwater filling freshwater lake
  if (game.scenario.lake_flooding && is_ocean(newter)
      && !terrain_has_flag(newter, TER_FRESHWATER)) {
    adjc_iterate(&(wld.map), ptile, atile)
    {
      if (terrain_has_flag(tile_terrain(atile), TER_FRESHWATER)) {
        struct terrain *aold = tile_terrain(atile);

        tile_change_terrain(
            atile, most_shallow_ocean(terrain_has_flag(aold, TER_FROZEN)));

        /* Recursive, but as lakes are of limited size, this
         * won't recurse so much as to cause stack problems. */
        check_terrain_change(atile, aold);
        update_tile_knowledge(atile);
      }
    }
    adjc_iterate_end;
  }

  if (need_to_reassign_continents(oldter, newter)) {
    assign_continent_numbers();
    send_all_known_tiles(nullptr);
  }

  claimer = tile_claimer(ptile);
  if (claimer != nullptr && is_ocean_tile(ptile)) {
    if (!is_claimable_ocean(ptile, claimer, tile_owner(ptile))) {
      map_clear_border(ptile);
    }
  }

  sanity_check_tile(ptile);
}

/**
   Ocean tile can be claimed iff one of the following conditions stands:
   a) it is an inland lake not larger than MAXIMUM_OCEAN_SIZE
   b) it is adjacent to only one continent and not more than two ocean tiles
   c) It is one tile away from a border source
   d) Player knows tech with Claim_Ocean flag
   e) Source itself is Oceanic tile and player knows tech with
 Claim_Ocean_Limited flag The source which claims the ocean has to be placed
 on the correct continent. in case a) The continent which surrounds the
 inland lake in case b) The only continent which is adjacent to the tile
 */
static bool is_claimable_ocean(struct tile *ptile, struct tile *source,
                               struct player *pplayer)
{
  Continent_id cont = tile_continent(ptile);
  Continent_id cont1 = tile_continent(source);
  Continent_id cont2;
  int ocean_tiles;
  bool other_continent;

  if (get_ocean_size(-cont) <= MAXIMUM_CLAIMED_OCEAN_SIZE
      && get_lake_surrounders(cont) == cont1) {
    return true;
  }

  if (ptile == source) {
    // Source itself is always claimable.
    return true;
  }

  if (num_known_tech_with_flag(pplayer, TF_CLAIM_OCEAN) > 0
      || (cont1 < 0
          && num_known_tech_with_flag(pplayer, TF_CLAIM_OCEAN_LIMITED)
                 > 0)) {
    return true;
  }

  ocean_tiles = 0;
  other_continent = false;
  adjc_iterate(&(wld.map), ptile, tile2)
  {
    cont2 = tile_continent(tile2);
    if (tile2 == source) {
      // Water next to border source is always claimable
      return true;
    }
    if (cont2 == cont) {
      ocean_tiles++;
    } else if (cont1 <= 0) {
      // First adjacent land (only if border source is not on land)
      cont1 = cont2;
    } else if (cont2 != cont1) {
      /* This water has two land continents adjacent, or land adjacent
       * that is of a different continent from the border source */
      other_continent = true;
    }
  }
  adjc_iterate_end;
  return !other_continent && ocean_tiles <= 2;
}

/**
   For each unit at the tile, queue any unique home city.
 */
static void map_unit_homecity_enqueue(struct tile *ptile)
{
  unit_list_iterate(ptile->units, punit)
  {
    struct city *phome = game_city_by_number(punit->homecity);

    if (nullptr == phome) {
      continue;
    }

    city_refresh_queue_add(phome);
  }
  unit_list_iterate_end;
}

/**
   Claim ownership of a single tile.
 */
static void map_claim_border_ownership(struct tile *ptile,
                                       struct player *powner,
                                       struct tile *psource)
{
  struct player *ploser = tile_owner(ptile);

  if ((ploser != powner && ploser != nullptr)
      && (BORDERS_SEE_INSIDE == game.info.borders
          || BORDERS_EXPAND == game.info.borders
          || ploser->server.border_vision)) {
    const v_radius_t radius_sq = V_RADIUS(-1, 0, 0);

    shared_vision_change_seen(ploser, ptile, radius_sq, false);
  }

  if (powner != nullptr
      && (BORDERS_SEE_INSIDE == game.info.borders
          || BORDERS_EXPAND == game.info.borders
          || powner->server.border_vision)) {
    const v_radius_t radius_sq = V_RADIUS(1, 0, 0);

    shared_vision_change_seen(powner, ptile, radius_sq, true);
  }

  tile_set_owner(ptile, powner, psource);

  /* Needed only when foggedborders enabled, but we do it unconditionally
   * in case foggedborders ever gets enabled later. Better to have correct
   * information in player map just in case. */
  update_tile_knowledge(ptile);

  if (ploser != powner) {
    if (S_S_RUNNING == server_state()
        && game.info.happyborders != HB_DISABLED) {
      map_unit_homecity_enqueue(ptile);
    }

    if (!city_map_update_tile_frozen(ptile)) {
      send_tile_info(nullptr, ptile, false);
    }
  }
}

/**
   Claim ownership of a single tile.
 */
void map_claim_ownership(struct tile *ptile, struct player *powner,
                         struct tile *psource, bool claim_bases)
{
  map_claim_border_ownership(ptile, powner, psource);

  if (claim_bases) {
    tile_claim_bases(ptile, powner);
  }
}

/**
   Claim ownership of bases on single tile.
 */
void tile_claim_bases(struct tile *ptile, struct player *powner)
{
  struct player *base_loser = extra_owner(ptile);

  /* This MUST be before potentially recursive call to map_claim_base(),
   * so that the recursive call will get new owner == base_loser and
   * abort recursion. */
  ptile->extras_owner = powner;

  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    map_claim_base(ptile, pextra, powner, base_loser);
  }
  extra_type_by_cause_iterate_end;
}

/**
   Remove border for this source.
 */
void map_clear_border(struct tile *ptile)
{
  int radius_sq = tile_border_source_radius_sq(ptile);

  circle_dxyr_iterate(&(wld.map), ptile, radius_sq, dtile, dx, dy, dr)
  {
    struct tile *claimer = tile_claimer(dtile);

    if (claimer == ptile) {
      map_claim_ownership(dtile, nullptr, nullptr, false);
    }
  }
  circle_dxyr_iterate_end;
}

/**
   Update borders for this source. Changes the radius without temporary
   clearing.
 */
void map_update_border(struct tile *ptile, struct player *owner,
                       int old_radius_sq, int new_radius_sq)
{
  if (old_radius_sq == new_radius_sq) {
    // No change
    return;
  }

  if (BORDERS_DISABLED == game.info.borders) {
    return;
  }

  if (old_radius_sq < new_radius_sq) {
    map_claim_border(ptile, owner, new_radius_sq);
  } else {
    circle_dxyr_iterate(&(wld.map), ptile, old_radius_sq, dtile, dx, dy, dr)
    {
      if (dr > new_radius_sq) {
        struct tile *claimer = tile_claimer(dtile);

        if (claimer == ptile) {
          map_claim_ownership(dtile, nullptr, nullptr, false);
        }
      }
    }
    circle_dxyr_iterate_end;
  }
}

/**
   Update borders for this source. Call this for each new source.

   If radius_sq is -1, get value from the border source on tile.
 */
void map_claim_border(struct tile *ptile, struct player *owner,
                      int radius_sq)
{
  if (BORDERS_DISABLED == game.info.borders) {
    return;
  }

  if (owner == nullptr) {
    /* Clear the border instead of claiming. Code below this block
     * cannot handle nullptr owner. */
    map_clear_border(ptile);

    return;
  }

  if (radius_sq < 0) {
    radius_sq = tile_border_source_radius_sq(ptile);
  }

  circle_dxyr_iterate(&(wld.map), ptile, radius_sq, dtile, dx, dy, dr)
  {
    struct tile *dclaimer = tile_claimer(dtile);

    if (dclaimer == ptile) {
      // Already claimed by the ptile
      continue;
    }

    if (dr != 0 && is_border_source(dtile)) {
      // Do not claim border sources other than self
      /* Note that this is extremely important at the moment for
       * base claiming to work correctly in case there's two
       * fortresses near each other. There could be infinite
       * recursion in them claiming each other. */
      continue;
    }

    if (!map_is_known(dtile, owner) && game.info.borders < BORDERS_EXPAND) {
      continue;
    }

    // Always claim source itself (distance, dr, to it 0)
    if (dr != 0 && nullptr != dclaimer && dclaimer != ptile) {
      struct city *ccity = tile_city(dclaimer);
      int strength_old, strength_new;

      if (ccity != nullptr) {
        // Previously claimed by city
        int city_x, city_y;

        map_distance_vector(&city_x, &city_y, ccity->tile, dtile);

        if (map_vector_to_sq_distance(city_x, city_y)
            <= city_map_radius_sq_get(ccity)
                   + game.info.border_city_permanent_radius_sq) {
          // Tile is within region permanently claimed by city
          continue;
        }
      }

      strength_old = tile_border_strength(dtile, dclaimer);
      strength_new = tile_border_strength(dtile, ptile);

      if (strength_new <= strength_old) {
        /* Stronger shall prevail,
         * in case of equal strength older shall prevail */
        continue;
      }
    }

    if (is_ocean_tile(dtile)) {
      // Only certain water tiles are claimable
      if (is_claimable_ocean(dtile, ptile, owner)) {
        map_claim_ownership(dtile, owner, ptile, dr == 0);
      }
    } else {
      /* Only land tiles on the same island as the border source
       * are claimable */
      if (tile_continent(dtile) == tile_continent(ptile)) {
        map_claim_ownership(dtile, owner, ptile, dr == 0);
      }
    }
  }
  circle_dxyr_iterate_end;
}

/**
   Update borders for all sources. Call this on turn end.
 */
void map_calculate_borders()
{
  if (BORDERS_DISABLED == game.info.borders) {
    return;
  }

  if (wld.map.tiles == nullptr) {
    // Map not yet initialized
    return;
  }

  qDebug("map_calculate_borders()");

  whole_map_iterate(&(wld.map), ptile)
  {
    if (is_border_source(ptile)) {
      map_claim_border(ptile, ptile->owner, -1);
    }
  }
  whole_map_iterate_end;

  qDebug("map_calculate_borders() workers");
  city_thaw_workers_queue();
  city_refresh_queue_processing();
}

/**
   Claim base to player's ownership.
 */
void map_claim_base(struct tile *ptile, const extra_type *pextra,
                    struct player *powner, struct player *ploser)
{
  struct base_type *pbase;
  int units_num;
  int i;

  if (!tile_has_extra(ptile, pextra)) {
    return;
  }

  units_num = unit_list_size(ptile->units);
  auto could_see_unit =
      (units_num > 0 ? std::make_unique<bv_player[]>(units_num) : nullptr);

  i = 0;
  if (pextra->eus != EUS_NORMAL) {
    unit_list_iterate(ptile->units, aunit)
    {
      BV_CLR_ALL(could_see_unit[i]);
      players_iterate(aplayer)
      {
        if (can_player_see_unit(aplayer, aunit)) {
          BV_SET(could_see_unit[i], player_index(aplayer));
        }
      }
      players_iterate_end;
      i++;
    }
    unit_list_iterate_end;
  }

  pbase = extra_base_get(pextra);

  fc_assert_ret(pbase != nullptr);

  // Transfer base provided vision to new owner
  if (powner != nullptr) {
    const v_radius_t old_radius_sq = V_RADIUS(-1, -1, -1);
    const v_radius_t new_radius_sq =
        V_RADIUS(pbase->vision_main_sq, pbase->vision_invis_sq,
                 pbase->vision_subs_sq);

    map_vision_update(powner, ptile, old_radius_sq, new_radius_sq,
                      game.server.vision_reveal_tiles);
  }

  if (ploser != nullptr) {
    const v_radius_t old_radius_sq =
        V_RADIUS(pbase->vision_main_sq, pbase->vision_invis_sq,
                 pbase->vision_subs_sq);
    const v_radius_t new_radius_sq = V_RADIUS(-1, -1, -1);

    map_vision_update(ploser, ptile, old_radius_sq, new_radius_sq,
                      game.server.vision_reveal_tiles);
  }

  if (BORDERS_DISABLED != game.info.borders && territory_claiming_base(pbase)
      && powner != ploser) {
    /* Clear borders from old owner. New owner may not know all those
     * tiles and thus does not claim them when borders mode is less
     * than EXPAND. */
    if (ploser != nullptr) {
      /* Set this particular tile owner by nullptr so in recursion
       * both loser and owner will be nullptr. */
      map_claim_border_ownership(ptile, nullptr, ptile);
      map_clear_border(ptile);
    }

    /* We here first claim this tile ownership -> now on extra_owner()
     * will return new owner. Then we claim border, which will recursively
     * lead to this tile and base being claimed. But at that point
     * ploser == powner and above check will abort the recursion. */
    if (powner != nullptr) {
      map_claim_border_ownership(ptile, powner, ptile);
      map_claim_border(ptile, powner, -1);
    }
    city_thaw_workers_queue();
    city_refresh_queue_processing();
  }

  i = 0;
  if (pextra->eus != EUS_NORMAL) {
    unit_list_iterate(ptile->units, aunit)
    {
      players_iterate(aplayer)
      {
        if (can_player_see_unit(aplayer, aunit)) {
          if (!BV_ISSET(could_see_unit[i], player_index(aplayer))) {
            send_unit_info(aplayer->connections, aunit);
          }
        } else {
          if (BV_ISSET(could_see_unit[i], player_index(aplayer))) {
            unit_goes_out_of_sight(aplayer, aunit);
          }
        }
      }
      players_iterate_end;
      i++;
    }
    unit_list_iterate_end;
  }
}

/**
   Change the sight points for the vision source, fogging or unfogging tiles
   as needed.

   See documentation in vision.h.
 */
void vision_change_sight(struct vision *vision, const v_radius_t radius_sq)
{
  map_vision_update(vision->player, vision->tile, vision->radius_sq,
                    radius_sq, vision->can_reveal_tiles);
  memcpy(vision->radius_sq, radius_sq, sizeof(v_radius_t));
}

/**
   Clear all sight points from this vision source.

   See documentation in vision.h.
 */
void vision_clear_sight(struct vision *vision)
{
  const v_radius_t vision_radius_sq = V_RADIUS(-1, -1, -1);

  vision_change_sight(vision, vision_radius_sq);
}

/**
   Create extra to tile.
 */
void create_extra(struct tile *ptile, const extra_type *pextra,
                  struct player *pplayer)
{
  bool extras_removed = false;

  extra_type_iterate(old_extra)
  {
    if (tile_has_extra(ptile, old_extra)
        && !can_extras_coexist(old_extra, pextra)) {
      destroy_extra(ptile, old_extra);
      extras_removed = true;
    }
  }
  extra_type_iterate_end;

  if (pextra->eus != EUS_NORMAL) {
    unit_list_iterate(ptile->units, aunit)
    {
      if (is_native_extra_to_utype(pextra, unit_type_get(aunit))) {
        players_iterate(aplayer)
        {
          if (!pplayers_allied(pplayer, aplayer)
              && can_player_see_unit(aplayer, aunit)) {
            unit_goes_out_of_sight(aplayer, aunit);
          }
        }
        players_iterate_end;
      }
    }
    unit_list_iterate_end;
  }

  tile_add_extra(ptile, pextra);

  // Watchtower might become effective.
  unit_list_refresh_vision(ptile->units);

  if (pextra->data.base != nullptr) {
    // Claim bases on tile
    if (pplayer) {
      struct player *old_owner = extra_owner(ptile);

      // Created base from nullptr -> pplayer
      map_claim_base(ptile, pextra, pplayer, nullptr);

      if (old_owner != pplayer) {
        // Existing bases from old_owner -> pplayer
        extra_type_by_cause_iterate(EC_BASE, oldbase)
        {
          if (oldbase != pextra) {
            map_claim_base(ptile, oldbase, pplayer, old_owner);
          }
        }
        extra_type_by_cause_iterate_end;

        ptile->extras_owner = pplayer;
      }
    } else {
      // Player who already owns bases on tile claims new base
      map_claim_base(ptile, pextra, extra_owner(ptile), nullptr);
    }
  }

  if (extras_removed) {
    /* Maybe conflicting extra that was removed was the only thing
     * making tile native to some unit. */
    bounce_units_on_terrain_change(ptile);
  }
}

/**
   Remove extra from tile.
 */
void destroy_extra(struct tile *ptile, struct extra_type *pextra)
{
  bv_player base_seen;
  bool is_virtual = tile_virtual_check(ptile);

  // Remember what players were able to see the base.
  if (!is_virtual) {
    BV_CLR_ALL(base_seen);
    players_iterate(pplayer)
    {
      if (map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
        BV_SET(base_seen, player_index(pplayer));
      }
    }
    players_iterate_end;
  }

  if (!is_virtual && is_extra_caused_by(pextra, EC_BASE)) {
    struct base_type *pbase = extra_base_get(pextra);
    struct player *owner = extra_owner(ptile);

    if (territory_claiming_base(pbase)) {
      map_clear_border(ptile);
    }

    if (nullptr != owner
        && (0 <= pbase->vision_main_sq || 0 <= pbase->vision_invis_sq)) {
      // Base provides vision, but no borders.
      const v_radius_t old_radius_sq =
          V_RADIUS(0 <= pbase->vision_main_sq ? pbase->vision_main_sq : -1,
                   0 <= pbase->vision_invis_sq ? pbase->vision_invis_sq : -1,
                   0 <= pbase->vision_subs_sq ? pbase->vision_subs_sq : -1);
      const v_radius_t new_radius_sq = V_RADIUS(-1, -1, -1);

      map_vision_update(owner, ptile, old_radius_sq, new_radius_sq,
                        game.server.vision_reveal_tiles);
    }
  }

  tile_remove_extra(ptile, pextra);

  if (!is_virtual) {
    // Remove base from vision of players which were able to see the base.
    players_iterate(pplayer)
    {
      if (BV_ISSET(base_seen, player_index(pplayer))
          && update_player_tile_knowledge(pplayer, ptile)) {
        send_tile_info(pplayer->connections, ptile, false);
      }
    }
    players_iterate_end;

    if (pextra->eus != EUS_NORMAL) {
      struct player *eowner = extra_owner(ptile);

      unit_list_iterate(ptile->units, aunit)
      {
        if (is_native_extra_to_utype(pextra, unit_type_get(aunit))) {
          players_iterate(aplayer)
          {
            if (can_player_see_unit(aplayer, aunit)
                && !pplayers_allied(aplayer, eowner)) {
              send_unit_info(aplayer->connections, aunit);
            }
          }
          players_iterate_end;
        }
      }
      unit_list_iterate_end;
    }
  }
}

/**
   Transfer (random parts of) player pfrom's world map to pto.
   @param pfrom         player that is the source of the map
   @param pto           player that receives the map
   @param prob          probability for the transfer each known tile
   @param reveal_cities if the map of all known cities should be transferred
 */
void give_distorted_map(struct player *pfrom, struct player *pto, int prob,
                        bool reveal_cities)
{
  buffer_shared_vision(pto);

  whole_map_iterate(&(wld.map), ptile)
  {
    if (fc_rand(100) < prob) {
      give_tile_info_from_player_to_player(pfrom, pto, ptile);
    } else if (reveal_cities && nullptr != tile_city(ptile)) {
      give_tile_info_from_player_to_player(pfrom, pto, ptile);
    }
  }
  whole_map_iterate_end;

  unbuffer_shared_vision(pto);
}
