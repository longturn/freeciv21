// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#include "editor/map_editor.h"

#include "icons.h"
#include "minimap_panel.h"
#include "page_game.h"
#include "views/view_map.h"

/**
 *  \class map_editor
 *  \brief A widget that allows a user to create custom maps and scenarios.
 *
 *  As of March 2025, this is a brand new widget.
 */

/**
 *  \brief Constructor for map_editor, sets layouts, policies ...
 */
map_editor::map_editor(QWidget *parent)
{
  ui.setupUi(this);
  setParent(parent);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
  setAutoFillBackground(true);
  setVisible(false);

  ui.label_status->setText(_("MAP EDITOR<br/>This is WIP!"));
  ui.label_status->setStyleSheet("background-color:red;");
  ui.label_status->setAlignment(Qt::AlignCenter);
  ui.but_close->setText("");
  ui.but_close->setToolTip(_("Close"));
  ui.but_close->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("city-close")));
  connect(ui.but_close, &QAbstractButton::clicked, this, &map_editor::close);
}

/**
 *  \brief Destructor for map_editor
 */
map_editor::~map_editor() {}

/**
 *  \brief Show event
 */
void map_editor::showEvent(QShowEvent *event)
{
  queen()->mapview_wdg->hide_all_fcwidgets();
  queen()->unitinfo_wdg->hide();
  queen()->minimap_panel->show();
  auto height = queen()->mapview_wdg->height();
  this->setFixedHeight(height);
  setVisible(true);
}

/**
 * \brief Close the dialog, show the wigets hidden at open/showEvent
 */
void map_editor::close()
{
  setVisible(false);
  queen()->mapview_wdg->show_all_fcwidgets();
  queen()->unitinfo_wdg->show();
}
