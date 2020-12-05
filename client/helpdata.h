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

#include <stddef.h> /* size_t */

/* common */
#include "improvement.h" /* Impr_type_id */

#include "helpdlg_g.h" /* enum help_page_type */

struct help_item {
  char *topic, *text;
  enum help_page_type type;
};

void boot_help_texts(void);
void free_help_texts(void);

const struct help_item *get_help_item(int pos);
const struct help_item *
get_help_item_spec(const char *name, enum help_page_type htype, int *pos);

char *helptext_building(char *buf, size_t bufsz, struct player *pplayer,
                        const char *user_text,
                        const struct impr_type *pimprove);
char *helptext_unit(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, const struct unit_type *utype);
void helptext_advance(char *buf, size_t bufsz, struct player *pplayer,
                      const char *user_text, int i);
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
