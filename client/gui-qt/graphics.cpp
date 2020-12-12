/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// client
#include "tilespec.h"
// gui-qt
#include "graphics.h"
#include "qtg_cxxside.h"

static struct sprite *intro_gfx_sprite = NULL;

/************************************************************************/ /**
   Return whether the client supports given view type.
 ****************************************************************************/
bool qtg_is_view_supported(enum ts_type type)
{
  switch (type) {
  case TS_ISOMETRIC:
  case TS_OVERHEAD:
    return true;
  case TS_3D:
    return false;
  }

  return false;
}

/************************************************************************/ /**
   Loading tileset of the specified type
 ****************************************************************************/
void qtg_tileset_type_set(enum ts_type type) {Q_UNUSED(type)}

/************************************************************************/ /**
   Frees the introductory sprites.
 ****************************************************************************/
void qtg_free_intro_radar_sprites()
{
  if (intro_gfx_sprite) {
    free_sprite(intro_gfx_sprite);
    intro_gfx_sprite = NULL;
  }
}
