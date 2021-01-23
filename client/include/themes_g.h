/**********************************************************************
 Freeciv - Copyright (C) 2005 The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#pragma once

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, gui_clear_theme, void)

void gui_load_theme(QString &directory, QString &theme_name);
QStringList get_useable_themes_in_directory(QString &directory, int *count);
QStringList get_gui_specific_themes_directories(int *count);
