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

/* common/scriptcore */
#include "luascript.h"

/* client */
#include "chatline_common.h"

#include "api_client_base.h"

/*************************************************************************/ /**
   Print a message in the chat window.
 *****************************************************************************/
void api_client_chat_base(lua_State *L, const char *msg)
{
  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, msg, 2, string);

  output_window_printf(ftc_chat_luaconsole, "%s", msg);
}
