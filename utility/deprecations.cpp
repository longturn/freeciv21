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
