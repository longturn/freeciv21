// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv and Freeciv21 Contributors

#pragma once

// common
#include "fc_types.h"
#include "name_translation.h" // struct name_translation
#include "requirements.h"     // struct requirement_vector

// Qt
class QString;

struct nation_style {
  int id;
  struct name_translation name;
  bool ruledit_disabled;
};

struct music_style {
  int id;
  QString music_peaceful;
  QString music_combat;
  struct requirement_vector reqs;
};

void styles_alloc(int count);
void styles_free();
int style_number(const struct nation_style *pstyle);
int style_index(const struct nation_style *pstyle);
struct nation_style *style_by_number(int id);
const char *style_name_translation(const struct nation_style *pstyle);
const char *style_rule_name(const struct nation_style *pstyle);
struct nation_style *style_by_rule_name(const char *name);

#define styles_iterate(_p)                                                  \
  {                                                                         \
    int _i_;                                                                \
    for (_i_ = 0; _i_ < game.control.num_styles; _i_++) {                   \
      struct nation_style *_p = style_by_number(_i_);

#define styles_iterate_end                                                  \
  }                                                                         \
  }

#define styles_re_active_iterate(_p)                                        \
  styles_iterate(_p)                                                        \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define styles_re_active_iterate_end                                        \
  }                                                                         \
  }                                                                         \
  styles_iterate_end;

void music_styles_alloc(int count);
void music_styles_free();

int music_style_number(const struct music_style *pms);
struct music_style *music_style_by_number(int id);

struct music_style *player_music_style(struct player *plr);

#define music_styles_iterate(_p)                                            \
  {                                                                         \
    int _i_;                                                                \
    for (_i_ = 0; _i_ < game.control.num_music_styles; _i_++) {             \
      struct music_style *_p = music_style_by_number(_i_);                  \
      if (_p != nullptr) {

#define music_styles_iterate_end                                            \
  }                                                                         \
  }                                                                         \
  }

int style_of_city(const struct city *pcity);

int basic_city_style_for_style(struct nation_style *pstyle);

int city_style(struct city *pcity);
