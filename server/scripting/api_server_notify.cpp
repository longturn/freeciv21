/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "featured_text.h"
#include "research.h"

/* common/scriptcore */
#include "luascript.h"

/* server */
#include "notify.h"

#include "api_server_notify.h"

/*************************************************************************/ /**
   Notify players which have embassies with pplayer with the given message.
 *****************************************************************************/
void api_notify_embassies_msg(lua_State *L, Player *pplayer, Tile *ptile,
                              int event, const char *message)
{
  LUASCRIPT_CHECK_STATE(L);

  notify_embassies(pplayer, ptile, static_cast<event_type>(event), ftc_any,
                   "%s", message);
}

/*************************************************************************/ /**
   Notify pplayer of a complex event.
 *****************************************************************************/
void api_notify_event_msg(lua_State *L, Player *pplayer, Tile *ptile,
                          int event, const char *message)
{
  LUASCRIPT_CHECK_STATE(L);

  notify_player(pplayer, ptile, static_cast<event_type>(event), ftc_any,
                "%s", message);
}

/*************************************************************************/ /**
   Notify players sharing research with the player.
 *****************************************************************************/
void api_notify_research_msg(lua_State *L, Player *pplayer, bool include_plr,
                             int event, const char *message)
{
  struct research *pres;

  LUASCRIPT_CHECK_STATE(L);

  pres = research_get(pplayer);

  notify_research(pres, include_plr ? NULL : pplayer,
                  static_cast<event_type>(event), ftc_any, "%s", message);
}

/*************************************************************************/ /**
   Notify players sharing research with the player.
 *****************************************************************************/
void api_notify_research_embassies_msg(lua_State *L, Player *pplayer,
                                       int event, const char *message)
{
  struct research *pres;

  LUASCRIPT_CHECK_STATE(L);

  pres = research_get(pplayer);

  notify_research_embassies(pres, NULL, static_cast<event_type>(event),
                            ftc_any, "%s", message);
}
