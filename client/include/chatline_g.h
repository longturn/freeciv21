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
#ifndef FC__CHATLINE_G_H
#define FC__CHATLINE_G_H

#include "chatline_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, real_output_window_append, const QString& astring,
               const struct text_tag_list *tags, int conn_id)
GUI_FUNC_PROTO(void, version_message, const char *vertext)

#endif /* FC__CHATLINE_G_H */
