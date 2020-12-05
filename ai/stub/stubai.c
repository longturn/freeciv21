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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "ai.h"
#include "player.h"

const char *fc_ai_stub_capstr(void);
bool fc_ai_stub_setup(struct ai_type *ai);

/**********************************************************************/ /**
   Return module capability string
 **************************************************************************/
const char *fc_ai_stub_capstr(void) { return FC_AI_MOD_CAPSTR; }

/**********************************************************************/ /**
   Set phase done
 **************************************************************************/
static void stub_end_turn(struct player *pplayer)
{
  pplayer->ai_phase_done = TRUE;
}

/**********************************************************************/ /**
   Setup player ai_funcs function pointers.
 **************************************************************************/
bool fc_ai_stub_setup(struct ai_type *ai)
{
  strncpy(ai->name, "stub", sizeof(ai->name));

  ai->funcs.first_activities = stub_end_turn;
  ai->funcs.restart_phase = stub_end_turn;

  return TRUE;
}
