// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer.h"
#include "layer_darkness.h"

#include <QPixmap>

#include <array>
#include <memory>

namespace freeciv {

#define SPECENUM_NAME fog_style
// Fog is automatically appended by the code.
#define SPECENUM_VALUE0 FOG_AUTO
#define SPECENUM_VALUE0NAME "Auto"
// A single fog sprite is provided by the tileset (tx.fog).
#define SPECENUM_VALUE1 FOG_SPRITE
#define SPECENUM_VALUE1NAME "Sprite"
// No fog, or fog derived from darkness style.
#define SPECENUM_VALUE2 FOG_DARKNESS
#define SPECENUM_VALUE2NAME "Darkness"
#include "specenum_gen.h"

class layer_fog : public layer {
public:
  explicit layer_fog(struct tileset *ts, fog_style style,
                     darkness_style darkness);
  virtual ~layer_fog() = default;

  void load_sprites() override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

private:
  fog_style m_style;
  darkness_style m_darkness;
  QPixmap *m_fog_sprite;
  std::vector<QPixmap *> m_darkness_sprites; //< For darkness style = CORNER
};

} // namespace freeciv
