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
#ifndef FC__MAP_TYPES_H
#define FC__MAP_TYPES_H

#include <QHash>

/* common */
#include "fc_types.h"

/****************************************************************
  Miscellaneous terrain information
*****************************************************************/
#define terrain_misc packet_ruleset_terrain_control

/* Some types used below. */
struct nation_hash;
struct nation_type;
struct packet_edit_startpos_full;

struct startpos {
  struct tile *location;
  bool exclude;
  QSet<const struct nation_type *> *nations;
};

enum mapsize_type {
  MAPSIZE_FULLSIZE = 0, /* Using the number of tiles / 1000. */
  MAPSIZE_PLAYER,       /* Define the number of (land) tiles per player;
                         * the setting 'landmass' and the number of players
                         * are used to calculate the map size. */
  MAPSIZE_XYSIZE        /* 'xsize' and 'ysize' are defined. */
};

enum map_generator {
  MAPGEN_SCENARIO = 0,
  MAPGEN_RANDOM,
  MAPGEN_FRACTAL,
  MAPGEN_ISLAND,
  MAPGEN_FAIR,
  MAPGEN_FRACTURE
};

enum map_startpos {
  MAPSTARTPOS_DEFAULT = 0, /* Generator's choice. */
  MAPSTARTPOS_SINGLE,      /* One player per continent. */
  MAPSTARTPOS_2or3,        /* Two on three players per continent. */
  MAPSTARTPOS_ALL,         /* All players on a single continent. */
  MAPSTARTPOS_VARIABLE,    /* Depending on size of continents. */
};

#define SPECENUM_NAME team_placement
#define SPECENUM_VALUE0 TEAM_PLACEMENT_DISABLED
#define SPECENUM_VALUE1 TEAM_PLACEMENT_CLOSEST
#define SPECENUM_VALUE2 TEAM_PLACEMENT_CONTINENT
#define SPECENUM_VALUE3 TEAM_PLACEMENT_HORIZONTAL
#define SPECENUM_VALUE4 TEAM_PLACEMENT_VERTICAL
#include "specenum_gen.h"

struct civ_map {
  int topology_id;
  enum direction8 valid_dirs[8], cardinal_dirs[8];
  int num_valid_dirs, num_cardinal_dirs;
  struct iter_index *iterate_outwards_indices;
  int num_iterate_outwards_indices;
  int xsize, ysize; /* native dimensions */
  int num_continents;
  int num_oceans; /* not updated at the client */
  struct tile *tiles;
  QHash<struct tile *, struct startpos *> *startpos_table;

  union {
    struct {
      enum mapsize_type mapsize; /* how the map size is defined */
      int size;                  /* used to calculate [xy]size */
      int tilesperplayer; /* tiles per player; used to calculate size */
      int seed_setting;
      int seed;
      int riches;
      int huts;
      int huts_absolute; /* For compatibility conversion from pre-2.6
                            savegames */
      int animals;
      int landpercent;
      enum map_generator generator;
      enum map_startpos startpos;
      bool tinyisles;
      bool separatepoles;
      int flatpoles;
      bool single_pole;
      bool alltemperate;
      int temperature;
      int wetness;
      int steepness;
      bool ocean_resources; /* Resources in the middle of the ocean */
      bool have_huts;
      bool have_resources;
      enum team_placement team_placement;
    } server;

    /* Add client side when needed */
  };
};


#endif /* FC__MAP_H */
