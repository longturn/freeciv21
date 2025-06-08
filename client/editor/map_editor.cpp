// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// editor
#include "editor/map_editor.h"

// common
#include "chatline_common.h" //map_editor::player_changed

// client                     ::Where Needed::
#include "citydlg.h"        //map_editor::showEvent
#include "client_main.h"    //map_editor::check_open
#include "editor.h"         //map_editor::showEvent
#include "fc_client.h"      //map_editor::check_open
#include "minimap_panel.h"  //map_editor::showEvent
#include "page_game.h"      //map_editor::showEvent
#include "views/view_map.h" //map_editor::showEvent

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

  // Prevent mouse events from going through the panels to the main map
  for (auto child : findChildren<QWidget *>()) {
    child->setAttribute(Qt::WA_NoMousePropagation);
  }

  // initialize in constructor
  ett_wdg = new editor_tool_tile(nullptr);

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
  ui.tbut_tool_tile->setText("");
  ui.tbut_tool_tile->setToolTip(_("Open Tile Tool"));
  ui.tbut_tool_tile->setMinimumSize(32, 32);
  ui.tbut_tool_tile->setIcon(
      QIcon::fromTheme(QStringLiteral("editor-tile")));
  connect(ui.tbut_tool_tile, &QAbstractButton::clicked, this,
          &map_editor::select_tool_tile);

  // Slot to handle selections from the players list
  QObject::connect(ui.combo_players,
                   QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &map_editor::player_changed);
}

/**
 *  \brief Destructor for map_editor
 */
map_editor::~map_editor() { delete ett_wdg; }

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
  editor_init();
}

/**
 * \brief Check that we can go into edit mode
 */
void map_editor::check_open()
{
  struct connection *my_conn = &client.conn;
  if (can_conn_edit(my_conn) || can_conn_enable_editing(my_conn)) {
    // Notify the server we are going into edit mode
    dsend_packet_edit_mode(my_conn, true);

    queen()->map_editor_wdg->show();
  } else {
    hud_message_box *ask = new hud_message_box(king()->central_wdg);
    ask->set_text_title(_("Cannot enable edit mode! You do not have the "
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
  // Notify the server we are exiting edit mode and show all floating widgets
  struct connection *my_conn = &client.conn;
  dsend_packet_edit_mode(my_conn, false);

  queen()->mapview_wdg->show_all_fcwidgets();
  queen()->unitinfo_wdg->show();
  ett_wdg->close_tool();
  ett_wdg->hide();
  editor_free();

  setVisible(false);
}

/**
 * \brief Populate a combo box with the current list of players by nation
 * name
 */
void map_editor::update_players()
{
  if (!players_done) {
    players_iterate(pplayer)
    {
      if (!is_barbarian(pplayer)) {
        auto sprite =
            *get_nation_flag_sprite(tileset, nation_of_player(pplayer));
        ui.combo_players->addItem(
            sprite, _(qUtf8Printable(nation_adjective_for_player(pplayer))));
      }
    }
    players_iterate_end;

    ui.combo_players->model()->sort(0, Qt::AscendingOrder);
    ui.combo_players->setCurrentText(
        _(qUtf8Printable(nation_adjective_for_player(client.conn.playing))));
    players_done = true;
  }
}

/**
 * \brief Slot to change the active player for editing
 */
void map_editor::player_changed(int index)
{
  ui.combo_players->setCurrentIndex(index);

  players_iterate(pplayer)
  {
    if (ui.combo_players->currentText().toUtf8()
        == nation_adjective_for_player(pplayer)) {
      send_chat_printf("/take \"%s\"", pplayer->name);
    }
  }
  players_iterate_end;
  if (ett_wdg_active) {
    ett_wdg->set_default_values();
  }
}

/**
 * \brief Activate the Tile Tool when clicked
 */
void map_editor::select_tool_tile()
{
  if (ett_wdg_active) {
    ui.tbut_tool_tile->setToolTip(_("Open Tile Tool"));
    ett_wdg->close_tool();
    ett_wdg->hide();
    ett_wdg_active = false;
  } else {
    ui.tbut_tool_tile->setToolTip(_("Close Tile Tool"));
    ui.sw_tools->addWidget(ett_wdg);
    editor_set_tool(ETT_TERRAIN);
    ett_wdg->show();
    ett_wdg->update();
    ett_wdg_active = true;
  }
}

/**
 * \brief Capture tile selected and pass tile structure to the editor tool
 * tile widget
 */
void map_editor::tile_selected(struct tile *ptile)
{
  // Take the tile handed to us from mapctrl.cpp and pass it to the tile tool
  // widget, which will do the real work.
  if (can_edit_tile_properties(ptile)) {
    if (ett_wdg_active) {
      ett_wdg->update_ett(ptile);
    }
  }
}
