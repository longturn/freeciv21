/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#include "deprecations.h"

Q_LOGGING_CATEGORY(deprecations_category, "freeciv.depr")

static deprecation_warn_callback depr_cb = NULL;

/********************************************************************/ /**
   Set callback to call when deprecation warnings are issued
 ************************************************************************/
void deprecation_warn_cb_set(deprecation_warn_callback new_cb)
{
  // FIXME Not used at the moment
  depr_cb = new_cb;
}
