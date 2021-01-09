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

#include "fc_types.h"

// Number of city report columns: have to set this manually now...
#define NUM_CREPORT_COLS (num_city_report_spec())

struct city_report_spec {
  bool show;               // modify this to customize
  int width;               // 0 means variable; rightmost only
  int space;               // number of leading spaces (see below)
  const char *title1;      // already translated or NULL
  const char *title2;      // already translated or NULL
  const char *explanation; // already translated
  void *data;
  const char *(*func)(const struct city *pcity, const void *data);
  const char *tagname; // for save_options
};

extern struct city_report_spec *city_report_specs;

int num_city_report_spec();
bool *city_report_spec_show_ptr(int i);
const char *city_report_spec_tagname(int i);

void init_city_report_game_data();

int cityrepfield_compare(const char *field1, const char *field2);

bool can_city_sell_universal(const struct city *pcity,
                             const struct universal *target);
