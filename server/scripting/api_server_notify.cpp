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

// Sol
#include "sol/sol.hpp"

// common
#include "featured_text.h"
#include "research.h"

/* common/scriptcore */
#include "luascript.h"

// server
#include "notify.h"

#include "api_server_notify.h"

namespace {
const std::string script = R"(
-- Notify module implementation.

function notify.all(...)
  local arg = table.pack(...);
  notify.event_msg(nil, nil, E.SCRIPT, string.format(table.unpack(arg)))
end

function notify.player(player, ...)
  local arg = table.pack(...);
  notify.event_msg(player, nil, E.SCRIPT, string.format(table.unpack(arg)))
end

function notify.event(player, tile, event, ...)
  local arg = table.pack(...);
  notify.event_msg(player, tile, event, string.format(table.unpack(arg)))
end

function notify.embassies(player, ptile, event, ...)
  local arg = table.pack(...);
  notify.embassies_msg(player, ptile, event, string.format(table.unpack(arg)))
end

function notify.research(player, selfmsg, event, ...)
  local arg = table.pack(...);
  notify.research_msg(player, selfmsg, event, string.format(table.unpack(arg)))
end

function notify.research_embassies(player, event, ...)
  local arg = table.pack(...);
  notify.research_embassies_msg(player, event, string.format(table.unpack(arg)))
end
)";

/**
   Notify players which have embassies with pplayer with the given message.
 */
void notify_embassies_msg(sol::this_state s, player *pplayer, tile *ptile,
                          int event, const char *message)
{
  notify_embassies(pplayer, ptile, static_cast<event_type>(event), ftc_any,
                   "%s", message);
}

/**
   Notify pplayer of a complex event.
 */
void notify_event_msg(sol::this_state s, player *pplayer, tile *ptile,
                      int event, const char *message)
{
  notify_player(pplayer, ptile, static_cast<event_type>(event), ftc_any,
                "%s", message);
}

/**
   Notify players sharing research with the player.
 */
void notify_research_msg(sol::this_state s, player *pplayer,
                         bool include_plr, int event, const char *message)
{
  struct research *pres = research_get(pplayer);
  notify_research(pres, include_plr ? nullptr : pplayer,
                  static_cast<event_type>(event), ftc_any, "%s", message);
}

/**
   Notify players sharing research with the player.
 */
void notify_research_embassies_msg(sol::this_state s, player *pplayer,
                                   int event, const char *message)
{
  struct research *pres = research_get(pplayer);

  notify_research_embassies(pres, nullptr, static_cast<event_type>(event),
                            ftc_any, "%s", message);
}

} // namespace

void setup_server_notify(sol::state_view lua)
{
  auto notify = lua["notify"].get_or_create<sol::table>();
  notify["embassies_msg"] = notify_embassies_msg;
  notify["research_msg"] = notify_research_msg;
  notify["research_embassies_msg"] = notify_research_embassies_msg;
  notify["event_msg"] = notify_event_msg;
}
