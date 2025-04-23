// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "fc_interface.h"

// utility
#include "log.h"
#include "shared.h"

// common
#include "player.h"
#include "unit.h"

/* Struct with functions pointers; the functions are defined in
   ./client/client_main.c:init_client_functions() and
   ./server/srv_main.c:init_server_functions(). */
struct functions fc_functions;

// The functions are accessed via this pointer.
const struct functions *fc_funcs = nullptr;
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
