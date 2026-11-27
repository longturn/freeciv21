// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "api_server_luadata.h"

// utility
#include "registry_ini.h"

// common
#include "game.h"
#include "luascript.h"

// dependencies/lua
extern "C" {
#include "lua.h"
}

/**
   Get string value from luadata.
 */
const char *api_luadata_get_str(lua_State *L, const char *field)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);

  if (game.server.luadata == nullptr) {
    return nullptr;
  }

  return secfile_lookup_str_default(game.server.luadata, nullptr, "%s",
                                    field);
}
