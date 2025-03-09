// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// Qt
#include <QPainter>

// Editor
#include "editor/tool_tile.h"

// Client
#include "fcintl.h" //editor_tool_tile
#include "fonts.h"  //editor_tool_tile

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
  ui.value_x->setText(_("N/A"));
  ui.value_x->setFont(value_font);
  ui.value_y->setText(_("N/A"));
  ui.value_y->setFont(value_font);
  ui.value_nat_x->setText(_("N/A"));
  ui.value_nat_x->setFont(value_font);
  ui.value_nat_y->setText(_("N/A"));
  ui.value_nat_y->setFont(value_font);

  // Set the tool button or Tile Tool
  ui.tbut_select_tile->setText("");
  ui.tbut_select_tile->setToolTip(_("Select a Tile to Edit"));
  ui.tbut_select_tile->setMinimumSize(32, 32);
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
  // TODO: have me do something real
  return;
}
