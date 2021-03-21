/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "shared.h"

// common
#include "player.h"
#include "tile.h"
#include "vision.h"

#include "fc_interface.h"

/* Struct with functions pointers; the functions are defined in
   ./client/client_main.c:init_client_functions() and
   ./server/srv_main.c:init_server_functions(). */
struct functions fc_functions;

// The functions are accessed via this pointer.
const struct functions *fc_funcs = NULL;
/* After this is set to TRUE (in interface_init()), the functions are
   available via fc_funcs. */
bool fc_funcs_defined = false;

/**
   Return the function pointer. Only possible before interface_init() was
   called (fc_funcs_defined == FALSE).
 */
struct functions *fc_interface_funcs()
{
  fc_assert_exit(fc_funcs_defined == false);

  return &fc_functions;
}

/**
   Test and initialize the functions. The existence of all functions should
   be checked!
 */
void fc_interface_init()
{
  fc_funcs = &fc_functions;

  // Test the existence of each required function here!
  fc_assert_exit(fc_funcs->server_setting_by_name);
  fc_assert_exit(fc_funcs->server_setting_name_get);
  fc_assert_exit(fc_funcs->server_setting_type_get);
  fc_assert_exit(fc_funcs->server_setting_val_bool_get);
  fc_assert_exit(fc_funcs->server_setting_val_int_get);
  fc_assert_exit(fc_funcs->server_setting_val_bitwise_get);
  fc_assert_exit(fc_funcs->player_tile_vision_get);
  fc_assert_exit(fc_funcs->player_tile_city_id_get);
  fc_assert_exit(fc_funcs->gui_color_free);

  fc_funcs_defined = true;

  setup_real_activities_array();
}

/**
   Free misc resources allocated for libfreeciv.
 */
void free_libfreeciv()
{
  diplrel_mess_close();
  free_multicast_group();
}
