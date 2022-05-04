/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

#include "packets.h"

#include "luaconsole_common.h"

void luaconsole_dialog_popup(bool raise);
bool luaconsole_dialog_is_open();
void real_luaconsole_dialog_update();

void real_luaconsole_append(const char *astring,
                            const struct text_tag_list *tags);
