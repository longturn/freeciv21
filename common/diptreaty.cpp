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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "log.h"

// common
#include "diptreaty.h"
#include "game.h"
#include "nation.h"
#include "player.h"

static struct clause_info clause_infos[CLAUSE_COUNT];

/**
   Returns TRUE iff pplayer could do diplomancy in the game at all.
 */
bool diplomacy_possible(const struct player *pplayer1,
                        const struct player *pplayer2)
{
  switch (game.info.diplomacy) {
  case DIPLO_FOR_ALL:
    return true;
  case DIPLO_FOR_HUMANS:
    return (is_human(pplayer1) && is_human(pplayer2));
  case DIPLO_FOR_AIS:
    return (is_ai(pplayer1) && is_ai(pplayer2));
  case DIPLO_NO_AIS:
    return (!is_ai(pplayer1) || !is_ai(pplayer2));
  case DIPLO_NO_MIXED:
    return ((is_human(pplayer1) && is_human(pplayer2))
            || (is_ai(pplayer1) && is_ai(pplayer2)));
  case DIPLO_FOR_TEAMS:
    return players_on_same_team(pplayer1, pplayer2);
  case DIPLO_DISABLED:
    return false;
  }
  qCritical("%s(): Unsupported diplomacy variant %d.", __FUNCTION__,
            game.info.diplomacy);
  return false;
}

/**
   Returns TRUE iff pplayer could do diplomatic meetings with aplayer.
 */
bool could_meet_with_player(const struct player *pplayer,
                            const struct player *aplayer)
{
  return (
      pplayer->is_alive && aplayer->is_alive && pplayer != aplayer
      && diplomacy_possible(pplayer, aplayer)
      && get_player_bonus(pplayer, EFT_NO_DIPLOMACY) <= 0
      && get_player_bonus(aplayer, EFT_NO_DIPLOMACY) <= 0
      && (player_has_embassy(aplayer, pplayer)
          || player_has_embassy(pplayer, aplayer)
          || player_diplstate_get(pplayer, aplayer)->contact_turns_left > 0
          || player_diplstate_get(aplayer, pplayer)->contact_turns_left
                 > 0));
}

/**
   Returns TRUE iff pplayer can get intelligence about aplayer.
 */
bool could_intel_with_player(const struct player *pplayer,
                             const struct player *aplayer)
{
  return (
      pplayer->is_alive && aplayer->is_alive && pplayer != aplayer
      && (player_diplstate_get(pplayer, aplayer)->contact_turns_left > 0
          || player_diplstate_get(aplayer, pplayer)->contact_turns_left > 0
          || player_has_embassy(pplayer, aplayer)));
}

/**
   Initialize treaty structure between two players.
 */
void init_treaty(struct Treaty *ptreaty, struct player *plr0,
                 struct player *plr1)
{
  ptreaty->plr0 = plr0;
  ptreaty->plr1 = plr1;
  ptreaty->accept0 = false;
  ptreaty->accept1 = false;
  ptreaty->clauses = clause_list_new();
}

/**
   Free the clauses of a treaty.
 */
void clear_treaty(struct Treaty *ptreaty)
{
  clause_list_iterate(ptreaty->clauses, pclause) { delete (pclause); }
  clause_list_iterate_end;
  clause_list_destroy(ptreaty->clauses);
}

/**
   Remove clause from treaty
 */
bool remove_clause(struct Treaty *ptreaty, struct player *pfrom,
                   enum clause_type type, int val)
{
  clause_list_iterate(ptreaty->clauses, pclause)
  {
    if (pclause->type == type && pclause->from == pfrom
        && pclause->value == val) {
      clause_list_remove(ptreaty->clauses, pclause);
      delete pclause;

      ptreaty->accept0 = false;
      ptreaty->accept1 = false;

      return true;
    }
  }
  clause_list_iterate_end;

  return false;
}

/**
   Add clause to treaty.
 */
bool add_clause(struct Treaty *ptreaty, struct player *pfrom,
                enum clause_type type, int val)
{
  struct player *pto =
      (pfrom == ptreaty->plr0 ? ptreaty->plr1 : ptreaty->plr0);
  struct Clause *pclause;
  enum diplstate_type ds =
      player_diplstate_get(ptreaty->plr0, ptreaty->plr1)->type;

  if (!clause_type_is_valid(type)) {
    qCritical("Illegal clause type encountered.");
    return false;
  }

  if (type == CLAUSE_ADVANCE && !valid_advance_by_number(val)) {
    qCritical("Illegal tech value %i in clause.", val);
    return false;
  }

  if (is_pact_clause(type)
      && ((ds == DS_PEACE && type == CLAUSE_PEACE)
          || (ds == DS_ARMISTICE && type == CLAUSE_PEACE)
          || (ds == DS_ALLIANCE && type == CLAUSE_ALLIANCE)
          || (ds == DS_CEASEFIRE && type == CLAUSE_CEASEFIRE))) {
    // we already have this diplomatic state
    qCritical("Illegal treaty suggested between %s and %s - they "
              "already have this treaty level.",
              nation_rule_name(nation_of_player(ptreaty->plr0)),
              nation_rule_name(nation_of_player(ptreaty->plr1)));
    return false;
  }

  if (type == CLAUSE_EMBASSY && player_has_real_embassy(pto, pfrom)) {
    // we already have embassy
    qCritical("Illegal embassy clause: %s already have embassy with %s.",
              nation_rule_name(nation_of_player(pto)),
              nation_rule_name(nation_of_player(pfrom)));
    return false;
  }

  if (!clause_enabled(type, pfrom, pto)) {
    return false;
  }

  if (!are_reqs_active(pfrom, pto, nullptr, nullptr, nullptr, nullptr,
                       nullptr, nullptr, nullptr, nullptr,
                       &clause_infos[type].giver_reqs, RPT_POSSIBLE)
      || !are_reqs_active(pto, pfrom, nullptr, nullptr, nullptr, nullptr,
                          nullptr, nullptr, nullptr, nullptr,
                          &clause_infos[type].receiver_reqs, RPT_POSSIBLE)) {
    return false;
  }

  clause_list_iterate(ptreaty->clauses, old_clause)
  {
    if (old_clause->type == type && old_clause->from == pfrom
        && old_clause->value == val) {
      // same clause already there
      return false;
    }
    if (is_pact_clause(type) && is_pact_clause(old_clause->type)) {
      // pact clause already there
      ptreaty->accept0 = false;
      ptreaty->accept1 = false;
      old_clause->type = type;
      return true;
    }
    if (type == CLAUSE_GOLD && old_clause->type == CLAUSE_GOLD
        && old_clause->from == pfrom) {
      // gold clause there, different value
      ptreaty->accept0 = false;
      ptreaty->accept1 = false;
      old_clause->value = val;
      return true;
    }
  }
  clause_list_iterate_end;

  pclause = new Clause;

  pclause->type = type;
  pclause->from = pfrom;
  pclause->value = val;

  clause_list_append(ptreaty->clauses, pclause);

  ptreaty->accept0 = false;
  ptreaty->accept1 = false;

  return true;
}

/**
   Initialize clause info structures.
 */
void clause_infos_init()
{
  int i;

  for (i = 0; i < CLAUSE_COUNT; i++) {
    clause_infos[i].type = clause_type(i);
    clause_infos[i].enabled = false;
    requirement_vector_init(&(clause_infos[i].giver_reqs));
    requirement_vector_init(&(clause_infos[i].receiver_reqs));
  }
}

/**
   Free memory associated with clause infos.
 */
void clause_infos_free()
{
  int i;

  for (i = 0; i < CLAUSE_COUNT; i++) {
    requirement_vector_free(&(clause_infos[i].giver_reqs));
    requirement_vector_free(&(clause_infos[i].receiver_reqs));
  }
}

/**
   Free memory associated with clause infos.
 */
struct clause_info *clause_info_get(enum clause_type type)
{
  fc_assert(type >= 0 && type < CLAUSE_COUNT);

  return &clause_infos[type];
}

/**
   Is clause enabled in this game?
   Currently this does not consider clause requirements that may change
   during the game, but returned value is constant for the given clause type
   thought the game. Try not to rely on that, though, as the goal is to
   change this so that also non-constant requirements will be considered
   in the future.
 */
bool clause_enabled(enum clause_type type, struct player *from,
                    struct player *to)
{
  Q_UNUSED(from)
  Q_UNUSED(to)
  struct clause_info *info = &clause_infos[type];

  if (!info->enabled) {
    return false;
  }

  if (!game.info.trading_gold && type == CLAUSE_GOLD) {
    return false;
  }
  if (!game.info.trading_tech && type == CLAUSE_ADVANCE) {
    return false;
  }
  if (!game.info.trading_city && type == CLAUSE_CITY) {
    return false;
  }

  return true;
}
