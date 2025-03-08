// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// Editor
#include "editor/map_editor.h"

// Client
#include "citydlg.h"        //showEvent
#include "client_main.h"    //check_open
#include "fc_client.h"      //check_open
#include "minimap_panel.h"  //showEvent
#include "page_game.h"      //showEvent
#include "views/view_map.h" //showEvent

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

  // Temp label, or maybe we re-use at a later date.
  ui.label_status->setText(_("This is WIP!"));
  ui.label_status->setStyleSheet("background-color:red;");
  ui.label_status->setAlignment(Qt::AlignCenter);

  // Set the close button.
  ui.but_close->setText("");
  ui.but_close->setToolTip(_("Close"));
  ui.but_close->setIcon(QIcon::fromTheme(QStringLiteral("close")));
  connect(ui.but_close, &QAbstractButton::clicked, this, &map_editor::close);

  // Set the tool button or Tile Tool
  ui.tbut_edit_tile->setText("");
  ui.tbut_edit_tile->setToolTip(_("Open Tile Tool"));
  ui.tbut_edit_tile->setMinimumSize(32, 32);
  ui.tbut_edit_tile->setIcon(
      QIcon::fromTheme(QStringLiteral("editor-tile")));
  connect(ui.tbut_edit_tile, &QAbstractButton::clicked, this,
          &map_editor::select_tool_tile);
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
  update_players();

  // hide the city dialog if its open
  if (queen()->city_overlay->isVisible()) {
    queen()->city_overlay->hide();
  }

  // clear the map of all widget except minimap
  queen()->mapview_wdg->hide_all_fcwidgets();
  queen()->unitinfo_wdg->hide();
  queen()->minimap_panel->show();
  // TODO: Figure out how to disable the turn done button

  // initialize editor functions
  // TODO: as called, causes an assertion
  // editor_init();

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

/**
 * \brief Populate a combo box with the current list of players
 */
void map_editor::update_players()
{
  if (!players_done) {
    players_iterate(pplayer)
    {
      auto sprite =
          *get_nation_flag_sprite(tileset, nation_of_player(pplayer));
      ui.combo_players->addItem(sprite,
                                _(qUtf8Printable(player_name(pplayer))));
    }
    players_iterate_end;
    players_done = true;
  }
}

/**
 * \brief Activate the Tile Tool when clicked
 */
void map_editor::select_tool_tile()
{
  if (ett_wdg_active) {
    ui.tbut_edit_tile->setToolTip(_("Open Tile Tool"));
    ett_wdg->hide();
    ett_wdg_active = false;
  } else {
    ui.tbut_edit_tile->setToolTip(_("Close Tile Tool"));
    ett_wdg = new editor_tool_tile(0);
    ui.vlayout_tools->addWidget(ett_wdg);
    ett_wdg->show();
    ett_wdg->update();
    ett_wdg_active = true;
  }
}
