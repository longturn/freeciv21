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
#pragma once

#include "fc_types.h"

#include "map.h"
#include "packets.h"
#include "terrain.h"
#include "vision.h"

#include "hand_gen.h"

struct section_file;
struct conn_list;

struct player_tile {
  std::unique_ptr<vision_site> site; // nullptr for no vision site
  struct extra_type *resource;       // nullptr for no resource
  struct terrain *terrain;           // nullptr for unknown tiles
  struct player *owner;              // nullptr for unowned
  struct player *extras_owner;
  bv_extras extras;

  /* If you build a city with an unknown square within city radius
     the square stays unknown. However, we still have to keep count
     of the seen points, so they are kept in here. When the tile
     then becomes known they are moved to seen. */
  v_radius_t own_seen;
  v_radius_t seen_count;
  short last_updated;
};

void global_warming(int effect);
void nuclear_winter(int effect);
void climate_change(bool warming, int effect);
bool upgrade_city_extras(struct city *pcity, struct extra_type **gained);
void upgrade_all_city_extras(struct player *pplayer, bool discovery);

void give_map_from_player_to_player(struct player *pfrom,
                                    struct player *pdest);
void give_seamap_from_player_to_player(struct player *pfrom,
                                       struct player *pdest);
void give_citymap_from_player_to_player(struct city *pcity,
                                        struct player *pfrom,
                                        struct player *pdest);
void send_all_known_tiles(struct conn_list *dest);

bool send_tile_suppression(bool now);
void send_tile_info(struct conn_list *dest, struct tile *ptile,
                    bool send_unknown);

void send_map_info(struct conn_list *dest);

void map_show_tile(struct player *pplayer, struct tile *ptile);
void map_hide_tile(struct player *pplayer, struct tile *ptile);
void map_show_circle(struct player *pplayer, struct tile *ptile,
                     int radius_sq);
void map_vision_update(struct player *pplayer, struct tile *ptile,
                       const v_radius_t old_radius_sq,
                       const v_radius_t new_radius_sq,
                       bool can_reveal_tiles);
void map_set_border_vision(struct player *pplayer, const bool is_enabled);
void map_show_all(struct player *pplayer);

bool map_is_known_and_seen(const struct tile *ptile,
                           const struct player *pplayer,
                           enum vision_layer vlayer);
bool map_is_known(const struct tile *ptile, const struct player *pplayer);
void map_set_known(struct tile *ptile, struct player *pplayer);
void map_clear_known(struct tile *ptile, struct player *pplayer);
void map_know_and_see_all(struct player *pplayer);
void show_map_to_all();

void player_map_init(struct player *pplayer);
void player_map_free(struct player *pplayer);
void remove_player_from_maps(struct player *pplayer);

struct vision_site *map_get_player_city(const struct tile *ptile,
                                        const struct player *pplayer);
struct vision_site *map_get_player_site(const struct tile *ptile,
                                        const struct player *pplayer);
struct player_tile *map_get_player_tile(const struct tile *ptile,
                                        const struct player *pplayer);
bool update_player_tile_knowledge(struct player *pplayer,
                                  struct tile *ptile);
void update_tile_knowledge(struct tile *ptile);
void update_player_tile_last_seen(struct player *pplayer,
                                  struct tile *ptile);

void give_shared_vision(struct player *pfrom, struct player *pto);
void remove_shared_vision(struct player *pfrom, struct player *pto);
bool really_gives_vision(struct player *me, struct player *them);

void enable_fog_of_war();
void disable_fog_of_war();
void enable_fog_of_war_player(struct player *pplayer);
void disable_fog_of_war_player(struct player *pplayer);

void map_calculate_borders();
void map_claim_border(struct tile *ptile, struct player *powner,
                      int radius_sq);
void map_claim_ownership(struct tile *ptile, struct player *powner,
                         struct tile *psource, bool claim_bases);
void map_clear_border(struct tile *ptile);
void map_update_border(struct tile *ptile, struct player *owner,
                       int old_radius_sq, int new_radius_sq);

void tile_claim_bases(struct tile *ptile, struct player *powner);
void map_claim_base(struct tile *ptile, const extra_type *pextra,
                    struct player *powner, struct player *ploser);

void terrain_changed(struct tile *ptile);
void check_terrain_change(struct tile *ptile, struct terrain *oldter);
void fix_tile_on_terrain_change(struct tile *ptile, struct terrain *oldter,
                                bool extend_rivers);
bool need_to_reassign_continents(const struct terrain *oldter,
                                 const struct terrain *newter);
void bounce_units_on_terrain_change(struct tile *ptile);

void vision_change_sight(struct vision *vision, const v_radius_t radius_sq);
void vision_clear_sight(struct vision *vision);

void change_playertile_site(struct player_tile *ptile,
                            struct vision_site *new_site);

void create_extra(struct tile *ptile, const extra_type *pextra,
                  struct player *pplayer);
void destroy_extra(struct tile *ptile, struct extra_type *pextra);

void give_distorted_map(struct player *pfrom, struct player *pto, int prob,
                        bool reveal_cities);
