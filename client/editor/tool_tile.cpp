// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// Qt
#include <QPainter>

// editor
#include "editor/tool_tile.h"

// client
#include "editor.h"         //editor_tool_tile::update_ett
#include "fcintl.h"         //editor_tool_tile::editor_tool_tile
#include "fonts.h"          //editor_tool_tile::editor_tool_tile
#include "mapctrl_common.h" //editor_tool_tile::select_tile

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
  // chatline font is fixed width
  auto value_font = fcFont::instance()->getFont(fonts::chatline);
  ui.setupUi(this);

  // Set default values and the font we want.
  ui.value_continent->setText(_("-"));
  ui.value_continent->setFont(value_font);
  ui.value_x->setText(_("-"));
  ui.value_x->setFont(value_font);
  ui.value_y->setText(_("-"));
  ui.value_y->setFont(value_font);
  ui.value_nat_x->setText(_("-"));
  ui.value_nat_x->setFont(value_font);
  ui.value_nat_y->setText(_("-"));
  ui.value_nat_y->setFont(value_font);

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
 *  \brief Select a tile for editor_tool_tile
 */
void editor_tool_tile::select_tile()
{
  set_hover_state({}, HOVER_EDIT_TILE, ACTIVITY_LAST, nullptr, NO_TARGET,
                  NO_TARGET, ACTION_NONE, ORDER_LAST);
  return;
}

/**
 * \bried Close the Tile Tool
 */
void editor_tool_tile::close_tool() { void clear_hover_state(); }

/**
 * \bried Update the editor tool widget UI elements
 */
void editor_tool_tile::update_ett(struct tile *ptile)
{
  editor_set_current_tile(ptile);
  ui.value_continent->setNum(ptile->continent);
  ui.value_x->setNum(index_to_map_pos_x(ptile->index));
  ui.value_y->setNum(index_to_map_pos_y(ptile->index));
  ui.value_nat_x->setNum(index_to_native_pos_x(ptile->index));
  ui.value_nat_y->setNum(index_to_native_pos_y(ptile->index));

  void clear_hover_state();
}
