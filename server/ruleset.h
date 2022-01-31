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

#include <QLoggingCategory>

#define CAP_EFT_HP_REGEN_MIN "HP_Regen_Min"
#define RULESET_CAPABILITIES                                                \
  "+Freeciv-ruleset-Devel-2017.Jan.02 " CAP_EFT_HP_REGEN_MIN
/*
 * Ruleset capabilities acceptable to this program:
 *
 * +Freeciv-3.1-ruleset
 *    - basic ruleset format for Freeciv versions 3.1.x; required
 *
 * +Freeciv-ruleset-Devel-YYYY.MMM.DD
 *    - ruleset of the development version at the given data
 *
 * HP_Regen_Min (optional)
 *    - Hard-coded HP regeneration rules in cities moved to effects.ruleset
 */

Q_DECLARE_LOGGING_CATEGORY(ruleset_category)

struct conn_list;

typedef void (*rs_conversion_logger)(const char *msg);

// functions
bool load_rulesets(const char *restore, const char *alt, bool compat_mode,
                   rs_conversion_logger logger, bool act, bool buffer_script,
                   bool load_luadata);
bool reload_rulesets_settings();
void send_rulesets(struct conn_list *dest);

void rulesets_deinit();

char *get_script_buffer();
char *get_parser_buffer();

// Default ruleset values that are not settings (in game.h)

#define GAME_DEFAULT_ADDTOSIZE 9
#define GAME_DEFAULT_CHANGABLE_TAX true
#define GAME_DEFAULT_VISION_REVEAL_TILES false
#define GAME_DEFAULT_NATIONALITY false
#define GAME_DEFAULT_CONVERT_SPEED 50
#define GAME_DEFAULT_DISASTER_FREQ 10
#define GAME_DEFAULT_ACH_UNIQUE true
#define GAME_DEFAULT_ACH_VALUE 1
#define RS_DEFAULT_MUUK_FOOD_WIPE true
#define RS_DEFAULT_MUUK_GOLD_WIPE true
#define RS_DEFAULT_MUUK_SHIELD_WIPE false
#define RS_DEFAULT_TECH_STEAL_HOLES true
#define RS_DEFAULT_TECH_TRADE_HOLES true
#define RS_DEFAULT_TECH_TRADE_LOSS_HOLES true
#define RS_DEFAULT_TECH_PARASITE_HOLES true
#define RS_DEFAULT_TECH_LOSS_HOLES true
#define RS_DEFAULT_PYTHAGOREAN_DIAGONAL false

#define RS_DEFAULT_GOLD_UPKEEP_STYLE "City"
#define RS_DEFAULT_TECH_COST_STYLE "Civ I|II"
#define RS_DEFAULT_TECH_LEAKAGE "None"
#define RS_DEFAULT_TECH_UPKEEP_STYLE "None"

#define RS_DEFAULT_CULTURE_VIC_POINTS 1000
#define RS_DEFAULT_CULTURE_VIC_LEAD 300
#define RS_DEFAULT_CULTURE_MIGRATION_PML 50
#define RS_DEFAULT_HISTORY_INTEREST_PML 0

#define RS_DEFAULT_EXTRA_APPEARANCE 15
#define RS_DEFAULT_EXTRA_DISAPPEARANCE 15
