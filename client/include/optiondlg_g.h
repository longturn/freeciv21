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
#ifndef FC__OPTIONDLG_G_H
#define FC__OPTIONDLG_G_H

#include "gui_proto_constructor.h"

struct option;
struct option_set;

GUI_FUNC_PROTO(void, option_dialog_popup, const char *name,
               const struct option_set *poptset)
GUI_FUNC_PROTO(void, option_dialog_popdown, const struct option_set *poptset)

GUI_FUNC_PROTO(void, option_gui_update, struct option *poption)
GUI_FUNC_PROTO(void, option_gui_add, struct option *poption)
GUI_FUNC_PROTO(void, option_gui_remove, struct option *poption)

#endif /* FC__OPTIONDLG_G_H */
