/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 2021 Freeciv21 contributors.
\_   \        /  __/                        This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include "layer_background.h"

#include "client_main.h"
#include "colors_common.h"
#include "control.h" // unit_is_in_focus
#include "game.h"
#include "sprite_g.h"
#include "tilespec.h"

namespace freeciv {

layer_background::layer_background(struct tileset *ts)
    : freeciv::layer(ts, LAYER_BACKGROUND)
{
}

std::vector<drawn_sprite> layer_background::fill_sprite_array(
    const tile *ptile, const tile_edge *pedge, const tile_corner *pcorner,
    const unit *punit, const city *pcity, const unit_type *putype) const
{
  // Set up background color.
  player *owner = nullptr;
  if (gui_options.solid_color_behind_units) {
    /* Unit drawing is disabled when the view options are turned off,
     * but only where we're drawing on the mapview. */
    bool do_draw_unit =
        (punit
         && (gui_options.draw_units || !ptile
             || (gui_options.draw_focus_unit && unit_is_in_focus(punit))));
    if (do_draw_unit) {
      owner = unit_owner(punit);
    } else if (pcity && gui_options.draw_cities) {
      owner = city_owner(pcity);
    }
  }
  if (owner) {
    return {drawn_sprite(tileset(),
                         m_player_background[player_index(owner)].get())};
  } else {
    return {};
  }
}

void layer_background::initialize_player(const player *player)
{
  // Get the color. In pregame, players have no color so we just use red.
  const auto color = player_has_color(tileset(), player)
                         ? *get_player_color(tileset(), player)
                         : Qt::red;

  auto sprite = create_player_sprite(color);

  const auto id = player_index(player);
  m_player_background.at(id) = std::unique_ptr<QPixmap>(crop_sprite(
      sprite.get(), 0, 0, tileset_tile_width(tileset()),
      tileset_tile_height(tileset()), get_mask_sprite(tileset()), 0, 0));
}

void layer_background::free_player(int player_id)
{
  m_player_background.at(player_id) = nullptr;
}

/**
 * Create a sprite with the given color.
 */
std::unique_ptr<QPixmap>
layer_background::create_player_sprite(const QColor &pcolor) const
{
  return std::unique_ptr<QPixmap>(
      create_sprite(tileset_full_tile_width(tileset()),
                    tileset_full_tile_height(tileset()), &pcolor));
}

} // namespace freeciv
