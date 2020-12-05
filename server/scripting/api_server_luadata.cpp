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

/* utility */
#include "registry_ini.h"

/* common */
#include "game.h"

/* common/scriptcore */
#include "luascript.h"

/* server/scripting */
#include "script_server.h"

#include "api_server_luadata.h"

/*************************************************************************/ /**
   Get string value from luadata.
 *****************************************************************************/
const char *api_luadata_get_str(lua_State *L, const char *field)
{
  LUASCRIPT_CHECK_STATE(L, NULL);

  if (game.server.luadata == NULL) {
    return NULL;
  }

  return secfile_lookup_str_default(game.server.luadata, NULL, "%s", field);
}
