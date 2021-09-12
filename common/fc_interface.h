/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#pragma once

// common
#include "vision.h"

struct player;
struct tile;
struct color;
struct extra_type;

// The existence of each function should be checked in interface_init()!
struct functions {
  server_setting_id (*server_setting_by_name)(const char *name);
  const char *(*server_setting_name_get)(server_setting_id id);
  enum sset_type (*server_setting_type_get)(server_setting_id id);
  bool (*server_setting_val_bool_get)(server_setting_id id);
  int (*server_setting_val_int_get)(server_setting_id id);
  unsigned int (*server_setting_val_bitwise_get)(server_setting_id id);
  void (*create_extra)(struct tile *ptile, const extra_type *pextra,
                       struct player *pplayer);
  void (*destroy_extra)(struct tile *ptile, struct extra_type *pextra);
  /* Returns iff the player 'pplayer' has the vision in the layer
     'vision' at tile given by 'ptile'. */
  bool (*player_tile_vision_get)(const struct tile *ptile,
                                 const struct player *pplayer,
                                 enum vision_layer vision);
  /* Returns the id of the city 'pplayer' believes exists at 'ptile' or
   * IDENTITY_NUMBER_ZERO when the player is unaware of a city at that
   * location. */
  int (*player_tile_city_id_get)(const struct tile *ptile,
                                 const struct player *pplayer);
  void (*gui_color_free)(QColor *pcolor);
};

extern const struct functions *fc_funcs;

struct functions *fc_interface_funcs();
void fc_interface_init();
void free_libfreeciv();
