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

#include "map.h"

#define SINGLE_MOVE (terrain_control.move_fragments)
#define MOVE_COST_IGTER (terrain_control.igter_cost)
// packets.def MOVEFRAGS
#define MAX_MOVE_FRAGS 65535

struct unit_type;
struct terrain;

enum unit_move_result {
  MR_OK,
  MR_DEATH,
  MR_PAUSE,
  MR_NO_WAR, // Can't move here without declaring war.
  MR_PEACE,  // Can't move here because of a peace treaty.
  MR_ZOC,
  MR_BAD_ACTIVITY,
  MR_BAD_DESTINATION,
  MR_BAD_MAP_POSITION,
  MR_DESTINATION_OCCUPIED_BY_NON_ALLIED_UNIT,
  MR_NO_TRANSPORTER_CAPACITY,
  MR_TRIREME,
  MR_CANNOT_DISEMBARK,
  MR_NON_NATIVE_MOVE, // Usually RMM_RELAXED road diagonally without link
  MR_ANIMAL_DISALLOWED,
  MR_UNIT_STAY,
  MR_NOT_ALLOWED
};

int utype_move_rate(const struct unit_type *utype, const struct tile *ptile,
                    const struct player *pplayer, int veteran_level,
                    int hitpoints);
int unit_move_rate(const struct unit *punit);
int utype_unknown_move_cost(const struct unit_type *utype);

bool unit_can_defend_here(const struct civ_map *nmap,
                          const struct unit *punit);
bool can_attack_non_native(const struct unit_type *utype);
bool can_attack_from_non_native(const struct unit_type *utype);

bool is_city_channel_tile(const struct unit_class *punitclass,
                          const struct tile *ptile,
                          const struct tile *pexclude);

bool is_native_tile(const struct unit_type *punittype,
                    const struct tile *ptile);

bool is_native_to_class(const struct unit_class *punitclass,
                        const struct terrain *pterrain,
                        const bv_extras *extras);

/****************************************************************************
  Check if this tile is native to given unit class.

  See is_native_to_class()
****************************************************************************/
static inline bool
is_native_tile_to_class(const struct unit_class *punitclass,
                        const struct tile *ptile)
{
  return is_native_to_class(punitclass, tile_terrain(ptile),
                            tile_extras(ptile));
}

bool is_native_move(const struct unit_class *punitclass,
                    const struct tile *src_tile,
                    const struct tile *dst_tile);
bool is_native_near_tile(const struct civ_map *nmap,
                         const struct unit_class *uclass,
                         const struct tile *ptile);
bool can_exist_at_tile(const struct civ_map *nmap,
                       const struct unit_type *utype,
                       const struct tile *ptile);
bool can_unit_exist_at_tile(const struct civ_map *nmap,
                            const struct unit *punit,
                            const struct tile *ptile);
bool can_unit_survive_at_tile(const struct civ_map *nmap,
                              const struct unit *punit,
                              const struct tile *ptile);
bool can_step_taken_wrt_to_zoc(const struct unit_type *punittype,
                               const struct player *unit_owner,
                               const struct tile *src_tile,
                               const struct tile *dst_tile,
                               const struct civ_map *zmap);
bool unit_can_move_to_tile(const struct civ_map *nmap,
                           const struct unit *punit,
                           const struct tile *ptile, bool igzoc,
                           bool enter_enemy_city);
enum unit_move_result
unit_move_to_tile_test(const struct civ_map *nmap, const struct unit *punit,
                       enum unit_activity activity,
                       const struct tile *src_tile,
                       const struct tile *dst_tile, bool igzoc,
                       struct unit *embark_to, bool enter_enemy_city);
bool can_unit_transport(const struct unit *transporter,
                        const struct unit *transported);
bool can_unit_type_transport(const struct unit_type *transporter,
                             const struct unit_class *transported);
bool unit_can_load(const struct unit *punit);
bool unit_could_load_at(const struct unit *punit, const struct tile *ptile);

bool is_unit_being_refueled(const struct unit *punit);
bool is_airunit_refuel_point(const struct tile *ptile,
                             const struct player *pplayer,
                             const struct unit *punit);

void init_move_fragments();
const char *move_points_text_full(int mp, bool reduce, const char *prefix,
                                  const char *none, bool align);
const char *move_points_text(int mp, bool reduce);
