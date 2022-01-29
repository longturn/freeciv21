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
#pragma once

#include "fc_types.h"
#include "layer.h"
#include "tilespec.h"

#include <QPixmap>

#include <array>

namespace freeciv {

#define SPECENUM_NAME darkness_style
// No darkness sprites are drawn.
#define SPECENUM_VALUE0 DARKNESS_NONE
#define SPECENUM_VALUE0NAME "None"
/* 1 sprite that is split into 4 parts and treated as a darkness4.  Only
 * works in iso-view. */
#define SPECENUM_VALUE1 DARKNESS_ISORECT
#define SPECENUM_VALUE1NAME "IsoRect"
/* 4 sprites, one per direction.  More than one sprite per tile may be
 * drawn. */
#define SPECENUM_VALUE2 DARKNESS_CARD_SINGLE
#define SPECENUM_VALUE2NAME "CardinalSingle"
/* 15=2^4-1 sprites.  A single sprite is drawn, chosen based on whether
 * there's darkness in _each_ of the cardinal directions. */
#define SPECENUM_VALUE3 DARKNESS_CARD_FULL
#define SPECENUM_VALUE3NAME "CardinalFull"
// Corner darkness & fog.  3^4 = 81 sprites.
#define SPECENUM_VALUE4 DARKNESS_CORNER
#define SPECENUM_VALUE4NAME "Corner"
#include "specenum_gen.h"

class layer_darkness : public layer {
public:
  explicit layer_darkness(struct tileset *ts);
  virtual ~layer_darkness() = default;

  /**
   * Sets the way in which the darkness is drawn.
   */
  void set_darkness_style(darkness_style style) { m_style = style; }

  /**
   * Gets the way in which the darkness is drawn.
   */
  darkness_style style() const { return m_style; }

  /**
   * Sets one of the sprites used to draw the darkness.
   */
  void set_sprite(std::size_t index, const QPixmap &p)
  {
    m_sprites[index] = p;
  }

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner, const unit *punit,
                    const city *pcity,
                    const unit_type *putype) const override;

private:
  darkness_style m_style;

  // First unused
  std::array<QPixmap, MAX_INDEX_CARDINAL> m_sprites;
};

} // namespace freeciv
