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
#include "log.h"

/* common */
#include "connection.h"

/* common/scriptcore */
#include "luascript.h"

/* server */
#include "auth.h"

/* server/scripting */
#include "script_fcdb.h"

#include "api_fcdb_auth.h"

/*************************************************************************/ /**
   Get the username.
 *****************************************************************************/
const char *api_auth_get_username(lua_State *L, Connection *pconn)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, pconn, NULL);
  fc_assert_ret_val(conn_is_valid(pconn), NULL);

  return auth_get_username(pconn);
}

/*************************************************************************/ /**
   Get the ip address.
 *****************************************************************************/
const char *api_auth_get_ipaddr(lua_State *L, Connection *pconn)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, pconn, NULL);
  fc_assert_ret_val(conn_is_valid(pconn), NULL);

  return auth_get_ipaddr(pconn);
}
