/**************************************************************************
    Copyright (c) 1996-2020 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__STYLE_H
#define FC__STYLE_H



struct nation_style {
  int id;
  struct name_translation name;
  bool ruledit_disabled;
};

struct music_style {
  int id;
  char music_peaceful[MAX_LEN_NAME];
  char music_combat[MAX_LEN_NAME];
  struct requirement_vector reqs;
};

void styles_alloc(int count);
void styles_free(void);
int style_count(void);
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
void music_styles_free(void);

int music_style_number(const struct music_style *pms);
struct music_style *music_style_by_number(int id);

struct music_style *player_music_style(struct player *plr);

#define music_styles_iterate(_p)                                            \
  {                                                                         \
    int _i_;                                                                \
    for (_i_ = 0; _i_ < game.control.num_music_styles; _i_++) {             \
      struct music_style *_p = music_style_by_number(_i_);                  \
      if (_p != NULL) {

#define music_styles_iterate_end                                            \
  }                                                                         \
  }                                                                         \
  }

int style_of_city(const struct city *pcity);

int basic_city_style_for_style(struct nation_style *pstyle);

int city_style(struct city *pcity);



#endif /* FC__STYLE_H */
