/***********************************************************************
Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, gui_clear_theme, void)

void gui_load_theme(QString &directory, QString &theme_name);
QStringList get_useable_themes_in_directory(QString &directory, int *count);
QStringList get_gui_specific_themes_directories(int *count);
