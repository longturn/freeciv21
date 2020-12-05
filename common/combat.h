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
#ifndef FC__COMBAT_H
#define FC__COMBAT_H



/* common */
#include "fc_types.h"
#include "unittype.h"

/*
 * attack_strength and defense_strength are multiplied by POWER_FACTOR
 * to yield the base of attack_power and defense_power.
 *
 * The constant may be changed since it isn't externally visible used.
 */
#define POWER_FACTOR 10

enum unit_attack_result {
  ATT_OK,
  ATT_NON_ATTACK,
  ATT_UNREACHABLE,
  ATT_NONNATIVE_SRC,
  ATT_NONNATIVE_DST
};

bool is_unit_reachable_at(const struct unit *defender,
                          const struct unit *attacker,
                          const struct tile *location);
enum unit_attack_result
unit_attack_unit_at_tile_result(const struct unit *punit,
                                const struct unit *pdefender,
                                const struct tile *dest_tile);
enum unit_attack_result
unit_attack_units_at_tile_result(const struct unit *punit,
                                 const struct tile *ptile);
bool can_unit_attack_tile(const struct unit *punit,
                          const struct tile *ptile);

double win_chance(int as, int ahp, int afp, int ds, int dhp, int dfp);

void get_modified_firepower(const struct unit *attacker,
                            const struct unit *defender, int *att_fp,
                            int *def_fp);
double unit_win_chance(const struct unit *attacker,
                       const struct unit *defender);

bool unit_really_ignores_citywalls(const struct unit *punit);
struct city *sdi_try_defend(const struct player *owner,
                            const struct tile *ptile);

int get_attack_power(const struct unit *punit);
int base_get_attack_power(const struct unit_type *punittype, int veteran,
                          int moves_left);
int base_get_defense_power(const struct unit *punit);
int get_total_defense_power(const struct unit *attacker,
                            const struct unit *defender);
int get_fortified_defense_power(const struct unit *attacker,
                                struct unit *defender);
int get_virtual_defense_power(const struct unit_type *attacker,
                              const struct unit_type *defender,
                              struct player *defending_player,
                              struct tile *ptile, bool fortified,
                              int veteran);
int get_total_attack_power(const struct unit *attacker,
                           const struct unit *defender);

struct unit *get_defender(const struct unit *attacker,
                          const struct tile *ptile);
struct unit *get_attacker(const struct unit *defender,
                          const struct tile *ptile);

struct unit *get_diplomatic_defender(const struct unit *act_unit,
                                     const struct unit *pvictim,
                                     const struct tile *tgt_tile);

bool is_stack_vulnerable(const struct tile *ptile);

int combat_bonus_against(const struct combat_bonus_list *list,
                         const struct unit_type *enemy,
                         enum combat_bonus_type type);



#endif /* FC__COMBAT_H */
