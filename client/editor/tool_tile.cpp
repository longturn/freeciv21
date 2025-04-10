// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// self
#include "editor/tool_tile.h"

// utility
#include "astring.h"

// common
#include "nation.h"

// client
#include "editor.h"
#include "fcintl.h"
#include "fonts.h"
#include "mapctrl_common.h"
#include "tileset/tilespec.h"
#include "views/view_map_common.h"

// Qt
#include <QPainter>
#include <QPoint>

/**
 *  \class editor_tool_tile
 *  \brief A widget that allows a user to edit tiles on maps and scenarios.
 *
 *  As of March 2025, this is a brand new widget.
 */

/**
 *  \brief Constructor for editor_tool_tile
 */
editor_tool_tile::editor_tool_tile(QWidget *parent)
{
  ui.setupUi(this);
  set_default_values();

  // Set the tool button or Tile Tool
  ui.tbut_select_tile->setText("");
  ui.tbut_select_tile->setToolTip(_("Select a Tile to Edit"));
  ui.tbut_select_tile->setMinimumSize(32, 32);
  ui.tbut_select_tile->setEnabled(true);
  ui.tbut_select_tile->setIcon(
      QIcon::fromTheme(QStringLiteral("pencil-ruler")));

  connect(ui.tbut_select_tile, &QAbstractButton::clicked, this,
          &editor_tool_tile::select_tile);
}

/**
 *  \brief Destructor for editor_tool_tile
 */
editor_tool_tile::~editor_tool_tile() {}

/**
 * \brief Function that can be used to set the default values for the widget
 */
void editor_tool_tile::set_default_values()
{
  // Set default values and the font we want.
  ui.value_continent->setText(_("-"));
  ui.value_continent->setAlignment(Qt::AlignLeft);

  ui.value_owner->setText(_("-"));
  ui.value_owner->setAlignment(Qt::AlignLeft);

  ui.value_x->setText(_("-"));
  ui.value_x->setAlignment(Qt::AlignLeft);

  ui.value_y->setText(_("-"));
  ui.value_y->setAlignment(Qt::AlignLeft);

  ui.value_nat_x->setText(_("-"));
  ui.value_nat_x->setAlignment(Qt::AlignLeft);

  ui.value_nat_y->setText(_("-"));
  ui.value_nat_y->setAlignment(Qt::AlignLeft);

  ui.value_terrain->setText(_("-"));
  ui.value_terrain->setAlignment(Qt::AlignLeft);

  ui.pixmap_terrain->setText(_("-"));
  ui.pixmap_terrain->setAlignment(Qt::AlignLeft);

  ui.value_resource->setText(_("-"));
  ui.value_resource->setAlignment(Qt::AlignLeft);

  ui.value_road->setText(_("-"));
  ui.value_road->setAlignment(Qt::AlignLeft);

  ui.value_infra->setText(_("-"));
  ui.value_infra->setAlignment(Qt::AlignLeft);

  ui.value_base->setText(_("-"));
  ui.value_base->setAlignment(Qt::AlignLeft);

  ui.value_hut->setText(_("-"));
  ui.value_hut->setAlignment(Qt::AlignLeft);

  ui.value_nuisance->setText(_("-"));
  ui.value_nuisance->setAlignment(Qt::AlignLeft);
}

/**
 *  \brief Select a tile for editor_tool_tile
 */
void editor_tool_tile::select_tile()
{
  set_hover_state({}, HOVER_EDIT_TILE, ACTIVITY_LAST, nullptr, NO_TARGET,
                  NO_TARGET, ACTION_NONE, ORDER_LAST);
}

/**
 * \brief Close the Tile Tool
 */
void editor_tool_tile::close_tool()
{
  clear_hover_state();
  set_default_values();
  editor_clear();
}

/**
 * \brief Update the editor tool widget UI elements based on the tile
 * selected
 */
void editor_tool_tile::update_ett(struct tile *ptile)
{
  if (ptile != nullptr) {
    clear_hover_state();
    editor_set_current_tile(ptile);

    // tile terrain w/ image
    ui.value_terrain->setText(
        QString(terrain_name_translation(tile_terrain(ptile))));
    QPixmap pm = get_tile_sprites(ptile);
    ui.pixmap_terrain->setPixmap(pm);

    // tile continent number
    ui.value_continent->setNum(ptile->continent);

    // tile owner
    struct player *owner = tile_owner(ptile);
    if (owner != nullptr) {
      ui.value_owner->setText(nation_adjective_for_player(owner));
    } else {
      ui.value_owner->setText(Q_("?owner:None"));
    }

    // tile coordinates
    ui.value_x->setNum(index_to_map_pos_x(ptile->index));
    ui.value_y->setNum(index_to_map_pos_y(ptile->index));
    ui.value_nat_x->setNum(index_to_native_pos_x(ptile->index));
    ui.value_nat_y->setNum(index_to_native_pos_y(ptile->index));

    // tile resource
    if (ptile->resource) {
      struct extra_type *res = ptile->resource;
      ui.value_resource->setText(extra_name_translation(res));
    } else {
      ui.value_resource->setText(Q_("?resource:None"));
    }

    // tile road (highest level)
    ui.value_road->setText(get_tile_road_name(ptile));
    // tile infrastructure
    ui.value_infra->setText(get_tile_infra_name(ptile));
    // tile base
    ui.value_base->setText(get_tile_base_name(ptile));
    // tile hut
    ui.value_hut->setText(get_tile_hut_name(ptile));
    // tile nuisance (pollution, fallout)
    ui.value_nuisance->setText(get_tile_nuisance_name(ptile));
  }
}

/**
 * \brief Return a string of the final road on a tile
 */
QString editor_tool_tile::get_tile_road_name(const struct tile *ptile) const
{
  QVector<QString> roads;
  extra_type_by_cause_iterate(EC_ROAD, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      roads.push_back(extra_name_translation(pextra));
    }
  }
  extra_type_by_cause_iterate_end;

  if (!roads.isEmpty()) {
    return strvec_to_and_list(roads);
  } else {
    return Q_("?road:None");
  }
}

/**
 * \brief Return a string of the added infrastructure on a tile
 */
QString editor_tool_tile::get_tile_infra_name(const struct tile *ptile) const
{
  QString sinfra;
  extra_type_by_cause_iterate(EC_IRRIGATION, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      sinfra = extra_name_translation(pextra);
    }
  }
  extra_type_by_cause_iterate_end;

  extra_type_by_cause_iterate(EC_MINE, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      sinfra + extra_name_translation(pextra);
    }
  }
  extra_type_by_cause_iterate_end;

  if (!sinfra.isEmpty()) {
    return sinfra;
  } else {
    return Q_("?infrastructure:None");
  }
}

/**
 * \brief Return a string of the any nuisances on a tile
 */
QString
editor_tool_tile::get_tile_nuisance_name(const struct tile *ptile) const
{
  QString snuisance;
  extra_type_by_cause_iterate(EC_POLLUTION, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      snuisance = extra_name_translation(pextra);
    }
  }
  extra_type_by_cause_iterate_end;

  extra_type_by_cause_iterate(EC_FALLOUT, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      snuisance + extra_name_translation(pextra);
    }
  }
  extra_type_by_cause_iterate_end;

  if (!snuisance.isEmpty()) {
    return snuisance;
  } else {
    return Q_("?nuisance:None");
  }
}

/**
 * \brief Return a string of any huts on a tile
 */
QString editor_tool_tile::get_tile_hut_name(const struct tile *ptile) const
{
  QString shut;
  extra_type_by_cause_iterate(EC_HUT, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      shut + extra_name_translation(pextra);
    }
  }
  extra_type_by_cause_iterate_end;

  if (!shut.isEmpty()) {
    return shut;
  } else {
    return Q_("?hut:None");
  }
}

/**
 * \brief Return a string of any bases on a tile
 */
QString editor_tool_tile::get_tile_base_name(const struct tile *ptile) const
{
  QString sbase;
  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      sbase + extra_name_translation(pextra);
    }
  }
  extra_type_by_cause_iterate_end;

  if (!sbase.isEmpty()) {
    return sbase;
  } else {
    return Q_("?base:None");
  }
}

/**
 * \brief Return a pixmap of the selected tile.
 * TODO: Add more layers over time, right now we show the base tile and any
 * resource We don't show any other infrastructure extras, such as
 * roads/rivers, mines, irrigation, bases, or pollution.
 */
QPixmap editor_tool_tile::get_tile_sprites(const struct tile *ptile) const
{
  int width = tileset_full_tile_width(tileset);
  int height = tileset_full_tile_height(tileset);
  int canvas_y = height - tileset_tile_height(tileset);

  QPixmap tile_pixmap(width, height);
  tile_pixmap.fill(Qt::transparent);

  // Get base level of sprites for the terrain. Does not give us specials,
  // roads, etc.
  for (int i = 0; i < 3; ++i) {
    auto sprites = fill_basic_terrain_layer_sprite_array(
        tileset, i, tile_terrain(ptile));
    put_drawn_sprites(&tile_pixmap, QPoint(0, canvas_y), sprites, false);
  }

  // Add any resource on the tile
  // FIXME: fill_basic_extra_sprite_array() does not work with all EC_* cause
  // codes.
  if (tile_terrain(ptile)->resources) {
    struct extra_type *tres = nullptr;
    extra_type_by_cause_iterate(EC_RESOURCE, pextra)
    {
      if (tile_has_extra(ptile, pextra)) {
        tres = pextra;
        break;
      }
    }
    extra_type_by_cause_iterate_end;
    if (tres != nullptr) {
      auto sprites = fill_basic_extra_sprite_array(tileset, tres);
      put_drawn_sprites(&tile_pixmap, QPoint(0, canvas_y), sprites, false);
    }
  }
  return tile_pixmap;
}
