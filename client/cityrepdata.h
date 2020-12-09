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

#include "support.h" /* bool type */

#include "fc_types.h"

/* Number of city report columns: have to set this manually now... */
#define NUM_CREPORT_COLS (num_city_report_spec())

struct city_report_spec {
  bool show;               /* modify this to customize */
  int width;               /* 0 means variable; rightmost only */
  int space;               /* number of leading spaces (see below) */
  const char *title1;      /* already translated or NULL */
  const char *title2;      /* already translated or NULL */
  const char *explanation; /* already translated */
  void *data;
  const char *(*func)(const struct city *pcity, const void *data);
  const char *tagname; /* for save_options */
};

extern struct city_report_spec *city_report_specs;

/* Use tagname rather than index for load/save, because later
   additions won't necessarily be at the end.
*/

/* Note on space: you can do spacing and alignment in various ways;
   you can avoid explicit space between columns if they are bracketted,
   but the problem is that with a configurable report you don't know
   what's going to be next to what.

   Here specify width, and leading space, although different clients
   may interpret these differently (gui-gtk and gui-mui ignore space
   field, handling columns without additional spacing).
   For some clients negative width means left justified (gui-gtk
   always treats width as negative; gui-mui ignores width field).
*/

/* Following are wanted to save/load options; use wrappers rather
   than expose the grotty details of the city_report_spec:
   (well, the details are exposed now too, but still keep
   this "clean" interface...)
*/
int num_city_report_spec(void);
bool *city_report_spec_show_ptr(int i);
const char *city_report_spec_tagname(int i);

void init_city_report_game_data(void);

int cityrepfield_compare(const char *field1, const char *field2);

bool can_city_sell_universal(const struct city *pcity,
                             const struct universal *target);


