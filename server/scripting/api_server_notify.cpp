// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "api_server_notify.h"

// common
#include "fc_types.h"
#include "featured_text.h"
#include "luascript.h"
#include "luascript_types.h"
#include "research.h"

// dependencies/lua
extern "C" {
#include "lua.h"
}

// server
#include "notify.h"

/**
   Notify players which have embassies with pplayer with the given message.
 */
void api_notify_embassies_msg(lua_State *L, Player *pplayer, Tile *ptile,
                              int event, const char *message)
{
  LUASCRIPT_CHECK_STATE(L);

  notify_embassies(pplayer, ptile, static_cast<event_type>(event), ftc_any,
                   "%s", message);
}

/**
   Notify pplayer of a complex event.
 */
void api_notify_event_msg(lua_State *L, Player *pplayer, Tile *ptile,
                          int event, const char *message)
{
  LUASCRIPT_CHECK_STATE(L);

  notify_player(pplayer, ptile, static_cast<event_type>(event), ftc_any,
                "%s", message);
}

/**
   Notify players sharing research with the player.
 */
void api_notify_research_msg(lua_State *L, Player *pplayer, bool include_plr,
                             int event, const char *message)
{
  struct research *pres;

  LUASCRIPT_CHECK_STATE(L);

  pres = research_get(pplayer);

  notify_research(pres, include_plr ? nullptr : pplayer,
                  static_cast<event_type>(event), ftc_any, "%s", message);
}

/**
   Notify players sharing research with the player.
 */
void api_notify_research_embassies_msg(lua_State *L, Player *pplayer,
                                       int event, const char *message)
{
  struct research *pres;

  LUASCRIPT_CHECK_STATE(L);

  pres = research_get(pplayer);

  notify_research_embassies(pres, nullptr, static_cast<event_type>(event),
                            ftc_any, "%s", message);
}
