/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__CLIMAP_H
#define FC__CLIMAP_H



/* common */
#include "fc_types.h" /* enum direction8, struct tile */
#include "tile.h"     /* enum known_type */

enum known_type client_tile_get_known(const struct tile *ptile);

enum direction8 gui_to_map_dir(enum direction8 gui_dir);
enum direction8 map_to_gui_dir(enum direction8 map_dir);

struct tile *client_city_tile(const struct city *pcity);
bool client_city_can_work_tile(const struct city *pcity,
                               const struct tile *ptile);



#endif /* FC__CLIMAP_H */
