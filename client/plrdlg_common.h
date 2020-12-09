/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once
// Qt
#include <QString>

/* utility */
#include "support.h" /* bool type */

/* common */
#include "fc_types.h"
#include "player.h"

enum player_dlg_column_type {
  COL_FLAG,
  COL_COLOR,
  COL_BOOLEAN,
  COL_TEXT,
  COL_RIGHT_TEXT /* right aligned text */
};

typedef int (*plr_dlg_sort_func)(const struct player *p1,
                                 const struct player *p2);

struct player_dlg_column {
  bool show;
  enum player_dlg_column_type type;
  const char *title;                        /* already translated */
  QString (*func)(const struct player *);   // if type = COL_*TEXT
  bool (*bool_func)(const struct player *); /* if type = COL_BOOLEAN */
  plr_dlg_sort_func sort_func;
  const char *tagname; /* for save_options */
};

extern struct player_dlg_column player_dlg_columns[];
extern const int num_player_dlg_columns;

QString plrdlg_col_state(const struct player *plr);

void init_player_dlg_common(void);
int player_dlg_default_sort_column(void);

QString player_addr_hack(const struct player *pplayer);

