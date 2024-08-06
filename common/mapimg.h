/***********************************************************************
Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/

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

#include "tile.h"
#define MAX_LEN_MAPDEF 256

// map image layers
#define SPECENUM_NAME mapimg_layer
#define SPECENUM_VALUE0 MAPIMG_LAYER_AREA
#define SPECENUM_VALUE0NAME "a"
#define SPECENUM_VALUE1 MAPIMG_LAYER_BORDERS
#define SPECENUM_VALUE1NAME "b"
#define SPECENUM_VALUE2 MAPIMG_LAYER_CITIES
#define SPECENUM_VALUE2NAME "c"
#define SPECENUM_VALUE3 MAPIMG_LAYER_FOGOFWAR
#define SPECENUM_VALUE3NAME "f"
#define SPECENUM_VALUE4 MAPIMG_LAYER_KNOWLEDGE
#define SPECENUM_VALUE4NAME "k"
#define SPECENUM_VALUE5 MAPIMG_LAYER_TERRAIN
#define SPECENUM_VALUE5NAME "t"
#define SPECENUM_VALUE6 MAPIMG_LAYER_UNITS
#define SPECENUM_VALUE6NAME "u"
// used a possible dummy value
#define SPECENUM_COUNT MAPIMG_LAYER_COUNT
#define SPECENUM_COUNTNAME "-"
#include "specenum_gen.h"
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
