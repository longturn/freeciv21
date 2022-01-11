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
#include <map>
#include <memory>

namespace freeciv {

class layer_terrain : public layer {
public:
  /// Indicates how many sprites are used to draw a tile.
  enum sprite_type {
    CELL_WHOLE, ///< One sprite for the entire tile.
    CELL_CORNER ///< One sprite for each corner of the tile.
  };

private:
  struct matching_group {
    int number;
    QString name;
  };

  enum match_style {
    MATCH_NONE,
    MATCH_SAME, // "boolean" match
    MATCH_PAIR,
    MATCH_FULL
  };

  struct terrain_info {
    sprite_type type = CELL_WHOLE;
    QString sprite_name;
    int offset_x = 0, offset_y = 0;

    match_style style = MATCH_NONE;
    matching_group *group = nullptr;
    std::vector<matching_group *> matches_with;

    std::vector<QPixmap *> sprites = {};

    bool blend = false;
    std::array<QPixmap *, 4> blend_sprites = {nullptr};
  };

public:
  constexpr static auto MAX_NUM_MATCH_WITH = 8;

  explicit layer_terrain(struct tileset *ts, int number);
  virtual ~layer_terrain() = default;

  bool create_matching_group(const QString &name);

  bool add_tag(const QString &tag, const QString &sprite_name);
  bool set_tag_sprite_type(const QString &tag, sprite_type type);
  bool set_tag_offsets(const QString &tag, int offset_x, int offset_y);
  bool set_tag_matching_group(const QString &tag, const QString &group_name);
  bool set_tag_matches_with(const QString &tag, const QString &group_name);
  void enable_blending(const QString &tag);

  void initialize_terrain(const terrain *terrain) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner, const unit *punit,
                    const city *pcity,
                    const unit_type *putype) const override;

private:
  matching_group *group(const QString &name);

  void initialize_cell_whole_match_none(const terrain *terrain,
                                        terrain_info &info);
  void initialize_cell_whole_match_same(const terrain *terrain,
                                        terrain_info &info);
  void initialize_cell_corner_match_none(const terrain *terrain,
                                         terrain_info &info);
  void initialize_cell_corner_match_same(const terrain *terrain,
                                         terrain_info &info);
  void initialize_cell_corner_match_pair(const terrain *terrain,
                                         terrain_info &info);
  void initialize_cell_corner_match_full(const terrain *terrain,
                                         terrain_info &info);
  void initialize_blending(const terrain *terrain, terrain_info &info);

  int terrain_group(const terrain *pterrain) const;
  void fill_terrain_sprite_array(std::vector<drawn_sprite> &sprs,
                                 const tile *ptile, const terrain *pterrain,
                                 terrain **tterrain_near) const;
  void fill_blending_sprite_array(std::vector<drawn_sprite> &sprs,
                                  const tile *ptile, const terrain *pterrain,
                                  terrain **tterrain_near) const;

  int m_number = 0;

  /**
   * List of those sprites in 'cells' that are allocated by some other
   * means than load_sprite() and thus are not freed by unload_all_sprites().
   */
  std::vector<std::unique_ptr<QPixmap>> m_allocated;

  std::map<QChar, matching_group> m_matching_groups;

  /**
   * Before terrains are loaded, this contains the list of available terrain
   * tags.
   */
  std::map<QString, terrain_info> m_terrain_tag_info;

  /// Every terrain drawn in this layer appears here
  std::map<int, terrain_info> m_terrain_info;
};

} // namespace freeciv
