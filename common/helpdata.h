// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "fcintl.h"

// common
#include "extras.h" // struct extra_type, goods_type
#include "fc_types.h"

// std
#include <cstddef> // size_t

struct nation_set;

// This must be in same order as names in helpdata.cpp
enum help_page_type {
  HELP_ANY,
  HELP_TEXT,
  HELP_UNIT,
  HELP_IMPROVEMENT,
  HELP_WONDER,
  HELP_TECH,
  HELP_TERRAIN,
  HELP_EXTRA,
  HELP_GOODS,
  HELP_SPECIALIST,
  HELP_GOVERNMENT,
  HELP_RULESET,
  HELP_TILESET,
  HELP_NATIONS,
  HELP_MULTIPLIER,
  HELP_EFFECT,
  HELP_LAST
};

// TRANS: "Overview" topic in built-in help
#define HELP_OVERVIEW_ITEM N_("?help:Overview")
#define HELP_PLAYING_ITEM N_("Strategy and Tactics")
#define HELP_LANGUAGES_ITEM N_("Languages")
#define HELP_CONNECTING_ITEM N_("Connecting")
#define HELP_CHATLINE_ITEM N_("Chatline")
#define HELP_WORKLIST_EDITOR_ITEM N_("Worklist Editor")
#define HELP_CMA_ITEM N_("Citizen Governor")
#define HELP_CONTROLS_ITEM N_("Controls")
#define HELP_RULESET_ITEM N_("About Current Ruleset")
#define HELP_TILESET_ITEM N_("About Current Tileset")
#define HELP_NATIONS_ITEM N_("About Nations")
#define HELP_ECONOMY_ITEM N_("Economy")
#define HELP_CITIES_ITEM N_("Cities")
#define HELP_IMPROVEMENTS_ITEM N_("City Improvements")
#define HELP_UNITS_ITEM N_("Units")
#define HELP_COMBAT_ITEM N_("Combat")
#define HELP_ZOC_ITEM N_("Zones of Control")
#define HELP_TECHS_ITEM N_("Technology")
#define HELP_EXTRAS_ITEM N_("Extras")
#define HELP_TERRAIN_ITEM N_("Terrain")
#define HELP_WONDERS_ITEM N_("Wonders of the World")
#define HELP_GOVERNMENT_ITEM N_("Government")
#define HELP_EFFECTS_ITEM N_("Effects")
#define HELP_DIPLOMACY_ITEM N_("Diplomacy")
#define HELP_SPACE_RACE_ITEM N_("Space Race")
#define HELP_COPYING_ITEM N_("Copying")
#define HELP_ABOUT_ITEM N_("About Freeciv21")
#define HELP_MULTIPLIER_ITEM N_("Policies")

struct help_item {
  char *topic, *text;
  enum help_page_type type;
};

void boot_help_texts(const nation_set *nations_to_show,
                     help_item *tileset_help);
void free_help_texts();

struct help_item *new_help_item(help_page_type type);
const struct help_item *
get_help_item_spec(const char *name, enum help_page_type htype, int *pos);

char *helptext_building(char *buf, size_t bufsz, struct player *pplayer,
                        const char *user_text,
                        const struct impr_type *pimprove,
                        const nation_set *nations_to_show);
char *helptext_unit(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, const struct unit_type *utype,
                    const nation_set *nations_to_show);
void helptext_advance(char *buf, size_t bufsz, struct player *pplayer,
                      const char *user_text, int i,
                      const nation_set *nations_to_show);
void helptext_terrain(char *buf, size_t bufsz, struct player *pplayer,
                      const char *user_text, struct terrain *pterrain);
void helptext_extra(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, struct extra_type *pextra);
void helptext_goods(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, struct goods_type *pgood);
void helptext_specialist(char *buf, size_t bufsz, struct player *pplayer,
                         const char *user_text, struct specialist *pspec);
void helptext_government(char *buf, size_t bufsz, struct player *pplayer,
                         const char *user_text, struct government *gov);
void helptext_nation(char *buf, size_t bufsz, struct nation_type *pnation,
                     const char *user_text);

char *helptext_unit_upkeep_str(const struct unit_type *punittype);
const char *helptext_road_bonus_str(const struct terrain *pterrain,
                                    const struct road_type *proad);
const char *helptext_extra_for_terrain_str(struct extra_type *pextra,
                                           struct terrain *pterrain,
                                           enum unit_activity act);
