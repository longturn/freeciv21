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
#pragma once

/* The overview tile width and height are defined in terms of the base
 * size.  For iso-maps the width is twice the height since "natural"
 * coordinates are used.  For classical maps the width and height are
 * equal.  The base size may be adjusted to get the correct scale. */
extern int OVERVIEW_TILE_SIZE;
#define OVERVIEW_TILE_WIDTH ((MAP_IS_ISOMETRIC ? 2 : 1) * OVERVIEW_TILE_SIZE)
#define OVERVIEW_TILE_HEIGHT OVERVIEW_TILE_SIZE

void map_to_overview_pos(int *overview_x, int *overview_y, int map_x,
                         int map_y);
void overview_to_map_pos(int *map_x, int *map_y, int overview_x,
                         int overview_y);

void refresh_overview_canvas();
void overview_update_tile(struct tile *ptile);
void calculate_overview_dimensions();
void overview_free();

void center_tile_overviewcanvas();

void flush_dirty_overview();

void overview_redraw_callback(struct option *option);
void gui_to_natural_pos(const struct tileset *t, double *ntl_x,
                        double *ntl_y, int gui_x, int gui_y);
void gui_to_overview_pos(const struct tileset *t, int *ovr_x, int *ovr_y,
                         int gui_x, int gui_y);
