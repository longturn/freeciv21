// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_fog.h"

#include "climap.h"
#include "colors_common.h"
#include "tilespec.h"

namespace freeciv {

layer_fog::layer_fog(struct tileset *ts, fog_style style,
                     darkness_style darkness)
    : freeciv::layer(ts, LAYER_FOG), m_style(style), m_darkness(darkness)
{
}

void layer_fog::load_sprites()
{
  m_fog_sprite = load_sprite({"tx.fog"}, true);

  if (m_darkness == DARKNESS_CORNER) {
    for (int i = 0; i < 81 /* 3^4 */; i++) {
      // Unknown, fog, known.
      const QChar ids[] = {'u', 'f', 'k'};
      auto buf = QStringLiteral("t.fog");
      int values[4], k = i;

      for (int vi = 0; vi < 4; vi++) {
        values[vi] = k % 3;
        k /= 3;

        buf += QStringLiteral("_") + ids[values[vi]];
      }
      fc_assert(k == 0);

      m_darkness_sprites.push_back(load_sprite({buf}));
    }
  }
}

std::vector<drawn_sprite>
layer_fog::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                             const tile_corner *pcorner,
                             const unit *punit) const
{
  std::vector<drawn_sprite> sprs;

  if (m_style == FOG_SPRITE && gui_options->draw_fog_of_war && ptile
      && client_tile_get_known(ptile) == TILE_KNOWN_UNSEEN) {
    sprs.emplace_back(tileset(), m_darkness_sprites[0]);
  }

  if (m_darkness == DARKNESS_CORNER && pcorner
      && gui_options->draw_fog_of_war) {
    int tileno = 0;

    for (int i = 3; i >= 0; i--) {
      const int unknown = 0, fogged = 1, known = 2;
      int value = -1;

      if (!pcorner->tile[i]) {
        value = fogged;
      } else {
        switch (client_tile_get_known(pcorner->tile[i])) {
        case TILE_KNOWN_SEEN:
          value = known;
          break;
        case TILE_KNOWN_UNSEEN:
          value = fogged;
          break;
        case TILE_UNKNOWN:
          value = unknown;
          break;
        }
      }
      fc_assert(value >= 0 && value < 3);

      tileno = tileno * 3 + value;
    }

    if (m_darkness_sprites[tileno]) {
      sprs.emplace_back(tileset(), m_darkness_sprites[tileno]);
    }
  }

  return sprs;
}

} // namespace freeciv
