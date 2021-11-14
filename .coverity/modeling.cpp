/**************************************************************************
 Copyright (c) 2021 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

/*
 * This file contains the Coverity modeling file used by Freeciv21, as
 * submitted to the Coverity dashboard.
 */

// luaL_error never returns, but Coverity doesn't know this.
struct lua_State;
int luaL_error(lua_State *, const char *, ...) { __coverity_panic__(); }

// tolua_error never returns, but Coverity doesn't know this.
struct tolua_Error;
void tolua_error(lua_State *, const char *, tolua_Error *)
{
  __coverity_panic__();
}
