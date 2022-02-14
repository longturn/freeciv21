/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// common
#include "fc_interface.h"

// server
#include "srv_main.h"

#include "tools_fc_interface.h"

/**
   Unused but required by fc_interface_init()
 */
static bool tool_player_tile_vision_get(const struct tile *ptile,
                                        const struct player *pplayer,
                                        enum vision_layer vision)
{
  qCritical("Assumed unused function %s called.", __FUNCTION__);
  return false;
}

/**
   Unused but required by fc_interface_init()
 */
static int tool_player_tile_city_id_get(const struct tile *ptile,
                                        const struct player *pplayer)
{
  qCritical("Assumed unused function %s called.", __FUNCTION__);
  return IDENTITY_NUMBER_ZERO;
}

/**
   Initialize tool specific functions.
 */
void fc_interface_init_tool()
{
  struct functions *funcs = fc_interface_funcs();

  // May be used when generating help texts
  funcs->server_setting_by_name = server_ss_by_name;
  funcs->server_setting_name_get = server_ss_name_get;
  funcs->server_setting_type_get = server_ss_type_get;
  funcs->server_setting_val_bool_get = server_ss_val_bool_get;
  funcs->server_setting_val_int_get = server_ss_val_int_get;
  funcs->server_setting_val_bitwise_get = server_ss_val_bitwise_get;

  // Not used. Set to dummy functions.
  funcs->player_tile_vision_get = tool_player_tile_vision_get;
  funcs->player_tile_city_id_get = tool_player_tile_city_id_get;

  /* Keep this function call at the end. It checks if all required functions
     are defined. */
  fc_interface_init();
}
