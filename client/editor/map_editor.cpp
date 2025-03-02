// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#include "editor/map_editor.h"

// client
#include "citydlg.h"
#include "client_main.h"
#include "fc_client.h"
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

  ui.label_title->setText(_("MAP EDITOR"));
  ui.label_title->setAlignment(Qt::AlignCenter);
  ui.label_status->setText(_("This is WIP!"));
  ui.label_status->setStyleSheet("background-color:red;");
  ui.label_status->setAlignment(Qt::AlignCenter);
  ui.but_close->setText("");
  ui.but_close->setToolTip(_("Close"));
  ui.but_close->setIcon(QIcon::fromTheme(QStringLiteral("close")));
  connect(ui.but_close, &QAbstractButton::clicked, this, &map_editor::close);

  // Tile Tools
  ui.tbut_inspect_tile->setText("");
  ui.tbut_inspect_tile->setToolTip(_("Inspect Tile"));
  ui.tbut_inspect_tile->setIcon(
      QIcon::fromTheme(QStringLiteral("editor-inspect")));

  ui.tbut_edit_tile->setText("");
  ui.tbut_edit_tile->setToolTip(_("Edit Tile"));
  ui.tbut_edit_tile->setIcon(
      QIcon::fromTheme(QStringLiteral("editor-tile")));
}

/**
 *  \brief Destructor for map_editor
 */
map_editor::~map_editor() {}

/**
 *  \brief Show event, enable edit mode
 */
void map_editor::showEvent(QShowEvent *event)
{
  // hide the city dialog if its open
  if (queen()->city_overlay->isVisible()) {
    queen()->city_overlay->hide();
  }

  // clear the map of all widget except minimap
  queen()->mapview_wdg->hide_all_fcwidgets();
  queen()->unitinfo_wdg->hide();
  queen()->minimap_panel->show();

  // set the height of the map editor to the height of the game
  auto height = queen()->mapview_wdg->height();
  this->setFixedHeight(height);
  setVisible(true);
}

/**
 * \brief Check that we can go into edit mode
 */
void map_editor::check_open()
{
  struct connection *my_conn = &client.conn;
  if (can_conn_edit(my_conn) || can_conn_enable_editing(my_conn)) {
    dsend_packet_edit_mode(my_conn, true);
    queen()->map_editor_wdg->show();
  } else {
    hud_message_box *ask = new hud_message_box(king()->central_wdg);
    ask->set_text_title(_("Cannot enable edit mode. You do not have the "
                          "correct access level."),
                        _("Map Editor"));
    ask->setStandardButtons(QMessageBox::Ok);
    ask->setDefaultButton(QMessageBox::Ok);
    ask->setAttribute(Qt::WA_DeleteOnClose);
    ask->show();
  }
}

/**
 * \brief Close the dialog, show the wigets hidden at open/showEvent
 */
void map_editor::close()
{
  // Notify the server we are exiting edit mode
  struct connection *my_conn = &client.conn;
  dsend_packet_edit_mode(my_conn, false);

  setVisible(false);
  queen()->mapview_wdg->show_all_fcwidgets();
  queen()->unitinfo_wdg->show();
}
