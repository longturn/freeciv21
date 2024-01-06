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

// common
#include "ai.h"
#include "city.h"
#include "game.h"
#include "multipliers.h"
#include "registry_ini.h"

// server
#include "citytools.h"

/* ai/default */
#include "aidata.h"
#include "daimilitary.h"

#include "aiplayer.h"

/**
   Initialize player for use with default AI. Note that this is called
   for all players, not just for those default AI is controlling.
 */
void dai_player_alloc(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *player_data = new ai_plr{};

  player_set_ai_data(pplayer, ait, player_data);

  dai_data_init(ait, pplayer);
}

/**
   Free player from use with default AI.
 */
void dai_player_free(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *player_data = def_ai_player_data(pplayer, ait);

  dai_data_close(ait, pplayer);

  if (player_data != nullptr) {
    player_set_ai_data(pplayer, ait, nullptr);
    delete player_data;
    player_data = nullptr;
  }
}

/**
   Store player specific data to savegame
 */
void dai_player_save_relations(struct ai_type *ait, const char *aitstr,
                               struct player *pplayer, struct player *other,
                               struct section_file *file, int plrno)
{
  struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, other);
  char buf[32];

  fc_snprintf(buf, sizeof(buf), "player%d.%s%d", plrno, aitstr,
              player_index(other));

  secfile_insert_int(file, adip->spam, "%s.spam", buf);
  secfile_insert_int(file, adip->countdown, "%s.countdown", buf);
  secfile_insert_int(file, adip->war_reason, "%s.war_reason", buf);
  secfile_insert_int(file, adip->ally_patience, "%s.patience", buf);
  secfile_insert_int(file, adip->warned_about_space, "%s.warn_space", buf);
  secfile_insert_int(file, adip->asked_about_peace, "%s.ask_peace", buf);
  secfile_insert_int(file, adip->asked_about_alliance, "%s.ask_alliance",
                     buf);
  secfile_insert_int(file, adip->asked_about_ceasefire, "%s.ask_ceasefire",
                     buf);
}

/**
   Load player vs player specific data from savegame
 */
void dai_player_load_relations(struct ai_type *ait, const char *aitstr,
                               struct player *pplayer, struct player *other,
                               const struct section_file *file, int plrno)
{
  struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, other);
  char buf[32];

  fc_snprintf(buf, sizeof(buf), "player%d.%s%d", plrno, aitstr,
              player_index(other));

  adip->spam = secfile_lookup_int_default(file, 0, "%s.spam", buf);
  adip->countdown =
      secfile_lookup_int_default(file, -1, "%s.countdown", buf);
  adip->war_reason = static_cast<war_reason>(
      secfile_lookup_int_default(file, 0, "%s.war_reason", buf));
  adip->ally_patience =
      secfile_lookup_int_default(file, 0, "%s.patience", buf);
  adip->warned_about_space =
      secfile_lookup_int_default(file, 0, "%s.warn_space", buf);
  adip->asked_about_peace =
      secfile_lookup_int_default(file, 0, "%s.ask_peace", buf);
  adip->asked_about_alliance =
      secfile_lookup_int_default(file, 0, "%s.ask_alliance", buf);
  adip->asked_about_ceasefire =
      secfile_lookup_int_default(file, 0, "%s.ask_ceasefire", buf);
}

/**
   Copy default ai data from player to player
 */
void dai_player_copy(struct ai_type *ait, struct player *original,
                     struct player *created)
{
  bool close_original;
  bool close_created;
  struct ai_plr *orig_data =
      dai_plr_data_get(ait, original, &close_original);
  struct ai_plr *created_data =
      dai_plr_data_get(ait, created, &close_created);

  advance_index_iterate(A_NONE, i)
  {
    created_data->tech_want[i] = orig_data->tech_want[i];
  }
  advance_index_iterate_end;

  if (close_original) {
    dai_data_phase_finished(ait, original);
  }
  if (close_created) {
    dai_data_phase_finished(ait, created);
  }
}

/**
   Ai got control of the player.
 */
void dai_gained_control(struct ai_type *ait, struct player *pplayer)
{
  if (pplayer->ai_common.skill_level != AI_LEVEL_AWAY) {
    multipliers_iterate(pmul)
    {
      pplayer->multipliers_target[multiplier_index(pmul)] = pmul->def;
    }
    multipliers_iterate_end;

    // Clear worker tasks, some AIs (e.g. classic) does not use those
    city_list_iterate(pplayer->cities, pcity) { clear_worker_tasks(pcity); }
    city_list_iterate_end;
  }

  dai_assess_danger_player(ait, pplayer, &(wld.map));
}
