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

#include <QHash>
// utility
#include "iterator.h"
#include "shared.h"
// common
#include "fc_types.h"
#include "name_translation.h"
#include "requirements.h"

struct ruler_title; // Opaque type.

/* G_LAST is a value guaranteed to be larger than any valid
 * Government_type_id. It defines the maximum number of governments
 * (so can also be used to size static arrays indexed by governments);
 * it is sometimes used as a sentinel value (but not in the network
 * protocol, which generally uses government_count()). */
#define G_LAST (127)

/* This is struct government itself.  All information about a form of
 * government is contained inhere. -- SKi */
struct government {
  Government_type_id item_number;
  struct name_translation name;
  bool ruledit_disabled;
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  struct requirement_vector reqs;
  QHash<const struct nation_type *, struct ruler_title *> *ruler_titles;
  int changed_to_times;
  QVector<QString> *helptext;

  // AI cached data for this government.
  struct {
    struct government *better; // hint: a better government (or nullptr)
  } ai;
  government();
  ~government();
};

extern std::vector<government> governments;
// General government accessor functions.
Government_type_id government_count();
Government_type_id government_index(const struct government *pgovern);
Government_type_id government_number(const struct government *pgovern);

struct government *government_by_number(const Government_type_id gov);
struct government *government_of_player(const struct player *pplayer);
struct government *government_of_city(const struct city *pcity);

struct government *government_by_rule_name(const char *name);
struct government *government_by_translated_name(const char *name);

const char *government_rule_name(const struct government *pgovern);
const char *government_name_translation(const struct government *pgovern);
const char *government_name_for_player(const struct player *pplayer);

// Ruler titles.
QHash<const struct nation_type *, struct ruler_title *> *
government_ruler_titles(const struct government *pgovern);
struct ruler_title *government_ruler_title_new(
    struct government *pgovern, const struct nation_type *pnation,
    const char *ruler_male_title, const char *ruler_female_title);

const struct nation_type *
ruler_title_nation(const struct ruler_title *pruler_title);
const char *
ruler_title_male_untranslated_name(const struct ruler_title *pruler_title);
const char *
ruler_title_female_untranslated_name(const struct ruler_title *pruler_title);

const char *ruler_title_for_player(const struct player *pplayer, char *buf,
                                   size_t buf_len);

// Ancillary routines
bool can_change_to_government(struct player *pplayer,
                              const struct government *pgovern);

// Initialization and iteration
void governments_alloc(int num);
void governments_free();

bool untargeted_revolution_allowed();
