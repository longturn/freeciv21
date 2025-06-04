// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/****************************************************************************
  Map images:

  * Basic functions:

    mapimg_init()       Initialise the map images.
    mapimg_reset()      Reset the map images.
    mapimg_free()       Free all memory needed for map images.
    mapimg_count()      Return the number of map image definitions.
    mapimg_error()      Return the last error message.
    mapimg_help()       Return a help text.

  * Advanced functions:

    mapimg_define()     Define a new map image.
    mapimg_delete()     Delete a map image definition.
    mapimg_show()       Show the map image definition.
    mapimg_id2str()     Convert the map image definition to a string. Usefull
                        to save the definitions.
    mapimg_create()     ...
    mapimg_colortest()  ...

    These functions return TRUE on success and FALSE on error. In the later
    case the error message is available with mapimg_error().

  * ...

    mapimg_isvalid()    Check if the map image is valid. This is only
                        possible after the game is started or a savegame was
                        loaded. For a valid map image definition the
                        definition is returned. The struct is freed by
                        mapimg_reset() or mapimg_free().

    mapimg_get_format_list()      ...

****************************************************************************/

#pragma once

// common
#include "fc_types.h"
#include "rgbcolor.h" // struct rgbcolor

// std
#include <cstddef> // size_t

/* If you change this enum, the default values for the client have to be
 * adapted (see options.c). */

typedef enum known_type (*mapimg_tile_known_func)(
    const struct tile *ptile, const struct player *pplayer, bool knowledge);
typedef struct terrain *(*mapimg_tile_terrain_func)(
    const struct tile *ptile, const struct player *pplayer, bool knowledge);
typedef struct player *(*mapimg_tile_player_func)(
    const struct tile *ptile, const struct player *pplayer, bool knowledge);

typedef int (*mapimg_plrcolor_count_func)();
typedef struct rgbcolor *(*mapimg_plrcolor_get_func)(int);

// map definition
struct mapdef;

void mapimg_init(mapimg_tile_known_func mapimg_tile_known,
                 mapimg_tile_terrain_func mapimg_tile_terrain,
                 mapimg_tile_player_func mapimg_tile_owner,
                 mapimg_tile_player_func mapimg_tile_city,
                 mapimg_tile_player_func mapimg_tile_unit,
                 mapimg_plrcolor_count_func mapimg_plrcolor_count,
                 mapimg_plrcolor_get_func mapimg_plrcolor_get);
void mapimg_reset();
void mapimg_free();
int mapimg_count();
char *mapimg_help(const char *cmdname);
const char *mapimg_error();

bool mapimg_define(const char *maparg, bool check);
bool mapimg_delete(int id);
bool mapimg_show(int id, char *str, size_t str_len, bool detail);
bool mapimg_id2str(int id, char *str, size_t str_len);
bool mapimg_create(struct mapdef *pmapdef, bool force, const char *savename,
                   const char *path);
bool mapimg_colortest(const char *savename, const char *path);

struct mapdef *mapimg_isvalid(int id);
