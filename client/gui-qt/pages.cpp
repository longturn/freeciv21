/***********************************************************************
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>

// utility
#include "fcintl.h"

// common
#include "game.h"
#include "version.h"

#include "cityrep_g.h"
#include "repodlgs_g.h"
// client
#include "client_main.h"
#include "colors_common.h"
#include "connectdlg_common.h"
#include "mapview_common.h"
#include "text.h"
#include "tilespec.h"

// gui-qt
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"
#include "icons.h"
#include "minimap.h"
#include "pages.h"
#include "plrdlg.h"
#include "qtg_cxxside.h"
#include "sidebar.h"
#include "sprite.h"
#include "voteinfo_bar.h"

int last_center_capital = 0;
int last_center_player_city = 0;
int last_center_enemy_city = 0;
extern void toggle_units_report(bool);
extern void popup_shortcuts_dialog();
static void center_next_enemy_city();
static void center_next_player_city();
static void center_next_player_capital();
static struct server_scan *meta_scan, *lan_scan;
static bool holding_srv_list_mutex = false;
static enum connection_state connection_status;
static struct terrain *char2terrain(char ch);
static void cycle_enemy_units();
int last_center_enemy = 0;

/**********************************************************************/ /**
   Helper function for drawing map of savegames. Converts stored map char in
   savefile to proper terrain.
 **************************************************************************/
static struct terrain *char2terrain(char ch)
{
  if (ch == TERRAIN_UNKNOWN_IDENTIFIER) {
    return T_UNKNOWN;
  }
  terrain_type_iterate(pterrain)
  {
    if (pterrain->identifier_load == ch) {
      return pterrain;
    }
  }
  terrain_type_iterate_end;
  return nullptr;
}

/**********************************************************************/ /**
   Sets the "page" that the client should show.  See also pages_g.h.
 **************************************************************************/
void qtg_real_set_client_page(enum client_pages page)
{
  gui()->switch_page(page);
}

/**********************************************************************/ /**
   Set the list of available rulesets.  The default ruleset should be
   "default", and if the user changes this then set_ruleset() should be
   called.
 **************************************************************************/
void qtg_set_rulesets(int num_rulesets, char **rulesets)
{
  gui()->pr_options->set_rulesets(num_rulesets, rulesets);
}

/**********************************************************************/ /**
   Returns current client page
 **************************************************************************/
enum client_pages qtg_get_current_client_page()
{
  return gui()->current_page();
}

/**********************************************************************/ /**
   Update the start page.
 **************************************************************************/
void update_start_page(void) { gui()->update_start_page(); }

/**********************************************************************/ /**
   Update network page connection state.
 **************************************************************************/
void fc_client::set_connection_state(enum connection_state state)
{
  switch (state) {
  case LOGIN_TYPE:
    set_status_bar("");
    connect_password_edit->setText("");
    connect_password_edit->setDisabled(true);
    connect_confirm_password_edit->setText("");
    connect_confirm_password_edit->setDisabled(true);
    break;
  case NEW_PASSWORD_TYPE:
    connect_password_edit->setText("");
    connect_confirm_password_edit->setText("");
    connect_password_edit->setDisabled(false);
    connect_confirm_password_edit->setDisabled(false);
    connect_password_edit->setFocus(Qt::OtherFocusReason);
    break;
  case ENTER_PASSWORD_TYPE:
    connect_password_edit->setText("");
    connect_confirm_password_edit->setText("");
    connect_password_edit->setDisabled(false);
    connect_confirm_password_edit->setDisabled(true);
    connect_password_edit->setFocus(Qt::OtherFocusReason);

    break;
  case WAITING_TYPE:
    set_status_bar("");
    break;
  }

  connection_status = state;
}

/**********************************************************************/ /**
   Creates buttons and layouts for network page.
 **************************************************************************/
void fc_client::create_network_page(void)
{
  QHeaderView *header;
  QLabel *connect_msg;
  QLabel *lan_label;
  QPushButton *network_button;

  pages_layout[PAGE_NETWORK] = new QGridLayout;
  QVBoxLayout *page_network_layout = new QVBoxLayout;
  QGridLayout *page_network_grid_layout = new QGridLayout;
  QHBoxLayout *page_network_lan_layout = new QHBoxLayout;
  QHBoxLayout *page_network_wan_layout = new QHBoxLayout;

  connect_host_edit = new QLineEdit;
  connect_port_edit = new QLineEdit;
  connect_login_edit = new QLineEdit;
  connect_password_edit = new QLineEdit;
  connect_confirm_password_edit = new QLineEdit;

  connect_password_edit->setEchoMode(QLineEdit::Password);
  connect_confirm_password_edit->setEchoMode(QLineEdit::Password);

  connect_password_edit->setDisabled(true);
  connect_confirm_password_edit->setDisabled(true);
  connect_lan = new QWidget;
  connect_metaserver = new QWidget;
  lan_widget = new QTableWidget;
  wan_widget = new QTableWidget;
  info_widget = new QTableWidget;

  QStringList servers_list;
  servers_list << _("Server Name") << _("Port") << _("Version")
               << _("Status") << Q_("?count:Players") << _("Comment");
  QStringList server_info;
  server_info << _("Name") << _("Type") << _("Host") << _("Nation");

  lan_widget->setRowCount(0);
  lan_widget->setColumnCount(servers_list.count());
  lan_widget->verticalHeader()->setVisible(false);
  lan_widget->setAutoScroll(false);

  wan_widget->setRowCount(0);
  wan_widget->setColumnCount(servers_list.count());
  wan_widget->verticalHeader()->setVisible(false);
  wan_widget->setAutoScroll(false);

  info_widget->setRowCount(0);
  info_widget->setColumnCount(server_info.count());
  info_widget->verticalHeader()->setVisible(false);

  lan_widget->setHorizontalHeaderLabels(servers_list);
  lan_widget->setProperty("showGrid", "false");
  lan_widget->setProperty("selectionBehavior", "SelectRows");
  lan_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  lan_widget->setSelectionMode(QAbstractItemView::SingleSelection);

  wan_widget->setHorizontalHeaderLabels(servers_list);
  wan_widget->setProperty("showGrid", "false");
  wan_widget->setProperty("selectionBehavior", "SelectRows");
  wan_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  wan_widget->setSelectionMode(QAbstractItemView::SingleSelection);

  connect(wan_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &fc_client::slot_selection_changed);

  connect(lan_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &fc_client::slot_selection_changed);
  connect(wan_widget, &QTableWidget::itemDoubleClicked, this,
          &fc_client::slot_connect);
  connect(lan_widget, &QTableWidget::itemDoubleClicked, this,
          &fc_client::slot_connect);

  info_widget->setHorizontalHeaderLabels(server_info);
  info_widget->setProperty("selectionBehavior", "SelectRows");
  info_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  info_widget->setSelectionMode(QAbstractItemView::SingleSelection);
  info_widget->setProperty("showGrid", "false");
  info_widget->setAlternatingRowColors(true);

  header = lan_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  header = wan_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  header = info_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);

  QStringList label_names;
  label_names << _("Connect") << _("Port") << _("Login") << _("Password")
              << _("Confirm Password");

  for (int i = 0; i < label_names.count(); i++) {
    connect_msg = new QLabel;
    connect_msg->setText(label_names[i]);
    page_network_grid_layout->addWidget(connect_msg, i, 0, Qt::AlignHCenter);
  }

  page_network_grid_layout->addWidget(connect_host_edit, 0, 1);
  page_network_grid_layout->addWidget(connect_port_edit, 1, 1);
  page_network_grid_layout->addWidget(connect_login_edit, 2, 1);
  page_network_grid_layout->addWidget(connect_password_edit, 3, 1);
  page_network_grid_layout->addWidget(connect_confirm_password_edit, 4, 1);
  page_network_grid_layout->addWidget(info_widget, 0, 2, 5, 4);

  network_button = new QPushButton(_("Refresh"));
  QObject::connect(network_button, &QAbstractButton::clicked, this,
                   &fc_client::update_network_lists);
  page_network_grid_layout->addWidget(network_button, 5, 0);

  network_button = new QPushButton(_("Cancel"));
  QObject::connect(network_button, &QPushButton::clicked,
                   [this]() { switch_page(PAGE_MAIN); });
  page_network_grid_layout->addWidget(network_button, 5, 2, 1, 1);

  network_button = new QPushButton(_("Connect"));
  page_network_grid_layout->addWidget(network_button, 5, 5, 1, 1);
  connect(network_button, &QAbstractButton::clicked, this,
          &fc_client::slot_connect);
  connect(connect_login_edit, &QLineEdit::returnPressed, this,
          &fc_client::slot_connect);
  connect(connect_password_edit, &QLineEdit::returnPressed, this,
          &fc_client::slot_connect);
  connect(connect_confirm_password_edit, &QLineEdit::returnPressed, this,
          &fc_client::slot_connect);

  connect_lan->setLayout(page_network_lan_layout);
  connect_metaserver->setLayout(page_network_wan_layout);
  page_network_lan_layout->addWidget(lan_widget, 0);
  page_network_wan_layout->addWidget(wan_widget, 1);
  lan_label = new QLabel(_("Internet servers:"));
  page_network_layout->addWidget(lan_label, 1);
  page_network_layout->addWidget(wan_widget, 10);
  lan_label = new QLabel(_("Local servers:"));
  page_network_layout->addWidget(lan_label, 1);
  page_network_layout->addWidget(lan_widget, 1);
  page_network_grid_layout->setColumnStretch(3, 4);
  pages_layout[PAGE_NETWORK]->addLayout(page_network_layout, 1, 1);
  pages_layout[PAGE_NETWORK]->addLayout(page_network_grid_layout, 2, 1);
}

/**********************************************************************/ /**
   Sets application status bar for given time in miliseconds
 **************************************************************************/
void fc_client::set_status_bar(QString message, int timeout)
{
  if (status_bar_label->text().isEmpty()) {
    status_bar_label->setText(message);
    QTimer::singleShot(timeout, this, SLOT(clear_status_bar()));
  } else {
    status_bar_queue.append(message);
    while (status_bar_queue.count() > 3) {
      status_bar_queue.removeFirst();
    }
  }
}

/**********************************************************************/ /**
   Clears status bar or shows next message in queue if exists
 **************************************************************************/
void fc_client::clear_status_bar()
{
  QString str;

  if (!status_bar_queue.isEmpty()) {
    str = status_bar_queue.takeFirst();
    status_bar_label->setText(str);
    QTimer::singleShot(2000, this, SLOT(clear_status_bar()));
  } else {
    status_bar_label->setText("");
  }
}

/**********************************************************************/ /**
   Creates page LOADING, showing label with Loading text
 **************************************************************************/
void fc_client::create_loading_page()
{
  QLabel *label = new QLabel(_("Loading..."));

  pages_layout[PAGE_GAME + 1] = new QGridLayout;
  pages_layout[PAGE_GAME + 1]->addWidget(label, 0, 0, 1, 1,
                                         Qt::AlignHCenter);
}


/**********************************************************************/ /**
   Creates buttons and layouts for game page.
 **************************************************************************/
void fc_client::create_game_page()
{
  QGridLayout *game_layout;

  pages_layout[PAGE_GAME] = new QGridLayout;
  game_main_widget = new QWidget;
  game_layout = new QGridLayout;
  game_layout->setContentsMargins(0, 0, 0, 0);
  game_layout->setSpacing(0);
  mapview_wdg = new map_view();
  mapview_wdg->setFocusPolicy(Qt::WheelFocus);
  sidebar_wdg = new fc_sidebar();

  sw_map = new fc_sidewidget(fc_icons::instance()->get_pixmap("view"),
                             Q_("?noun:View"), "MAP", side_show_map);
  sw_tax = new fc_sidewidget(nullptr, nullptr, "", side_rates_wdg, SW_TAX);
  sw_indicators =
      new fc_sidewidget(nullptr, nullptr, "", side_show_map, SW_INDICATORS);
  sw_indicators->set_right_click(side_indicators_menu);
  sw_cunit = new fc_sidewidget(fc_icons::instance()->get_pixmap("units"),
                               _("Units"), "", toggle_units_report);
  sw_cities =
      new fc_sidewidget(fc_icons::instance()->get_pixmap("cities"),
                        _("Cities"), "CTS", city_report_dialog_popup);
  sw_cities->set_wheel_up(center_next_enemy_city);
  sw_cities->set_wheel_down(center_next_player_city);
  sw_diplo = new fc_sidewidget(fc_icons::instance()->get_pixmap("nations"),
                               _("Nations"), "PLR", popup_players_dialog);
  sw_diplo->set_wheel_up(center_next_player_capital);
  sw_diplo->set_wheel_down(key_center_capital);
  sw_science =
      new fc_sidewidget(fc_icons::instance()->get_pixmap("research"),
                        _("Research"), "SCI", side_left_click_science);
  sw_economy =
      new fc_sidewidget(fc_icons::instance()->get_pixmap("economy"),
                        _("Economy"), "ECO", economy_report_dialog_popup);
  sw_endturn = new fc_sidewidget(fc_icons::instance()->get_pixmap("endturn"),
                                 _("Turn Done"), "", side_finish_turn);
  sw_cunit->set_right_click(side_center_unit);
  sw_cunit->set_wheel_up(cycle_enemy_units);
  sw_cunit->set_wheel_down(key_unit_wait);
  sw_diplo->set_right_click(side_right_click_diplomacy);
  sw_science->set_right_click(side_right_click_science);

  sidebar_wdg->add_widget(sw_map);
  sidebar_wdg->add_widget(sw_cunit);
  sidebar_wdg->add_widget(sw_cities);
  sidebar_wdg->add_widget(sw_diplo);
  sidebar_wdg->add_widget(sw_science);
  sidebar_wdg->add_widget(sw_economy);
  sidebar_wdg->add_widget(sw_tax);
  sidebar_wdg->add_widget(sw_indicators);
  sidebar_wdg->add_widget(sw_endturn);

  minimapview_wdg = new minimap_view(mapview_wdg);
  minimapview_wdg->show();
  unitinfo_wdg = new hud_units(mapview_wdg);
  battlelog_wdg = new hud_battle_log(mapview_wdg);
  battlelog_wdg->hide();
  infotab = new info_tab(mapview_wdg);
  infotab->show();
  x_vote = new xvote(mapview_wdg);
  x_vote->hide();
  gtd = new goto_dialog(mapview_wdg);
  gtd->hide();

  game_layout->addWidget(mapview_wdg, 1, 0);
  game_main_widget->setLayout(game_layout);
  game_tab_widget = new fc_game_tab_widget;
  game_tab_widget->setMinimumSize(600, 400);
  game_tab_widget->setContentsMargins(0, 0, 0, 0);
  add_game_tab(game_main_widget);
  if (gui_options.gui_qt_sidebar_left) {
    pages_layout[PAGE_GAME]->addWidget(sidebar_wdg, 1, 0);
  } else {
    pages_layout[PAGE_GAME]->addWidget(sidebar_wdg, 1, 2);
  }
  pages_layout[PAGE_GAME]->addWidget(game_tab_widget, 1, 1);
  pages_layout[PAGE_GAME]->setContentsMargins(0, 0, 0, 0);
  pages_layout[PAGE_GAME]->setSpacing(0);
}

/**********************************************************************/ /**
   Inserts tab widget to game view page
 **************************************************************************/
int fc_client::add_game_tab(QWidget *widget)
{
  int i;

  i = game_tab_widget->addWidget(widget);
  game_tab_widget->setCurrentWidget(widget);
  return i;
}

/**********************************************************************/ /**
   Removes given tab widget from game page
 **************************************************************************/
void fc_client::rm_game_tab(int index)
{
  game_tab_widget->removeWidget(game_tab_widget->widget(index));
}


/**********************************************************************/ /**
   Updates list of servers in network page in proper QTableViews
 **************************************************************************/
void fc_client::update_server_list(enum server_scan_type sstype,
                                   const struct server_list *list)
{
  QTableWidget *sel = NULL;
  QString host, portstr;
  int port;
  int row;
  int old_row_count;

  switch (sstype) {
  case SERVER_SCAN_LOCAL:
    sel = lan_widget;
    break;
  case SERVER_SCAN_GLOBAL:
    sel = wan_widget;
    break;
  default:
    break;
  }

  if (!sel) {
    return;
  }

  if (!list) {
    return;
  }

  host = connect_host_edit->text();
  portstr = connect_port_edit->text();
  port = portstr.toInt();
  old_row_count = sel->rowCount();
  sel->clearContents();
  row = 0;
  server_list_iterate(list, pserver)
  {
    char buf[20];
    int tmp;
    QString tstring;

    if (old_row_count <= row) {
      sel->insertRow(row);
    }

    if (pserver->humans >= 0) {
      fc_snprintf(buf, sizeof(buf), "%d", pserver->humans);
    } else {
      strncpy(buf, _("Unknown"), sizeof(buf) - 1);
    }

    tmp = pserver->port;
    tstring = QString::number(tmp);

    for (int col = 0; col < 6; col++) {
      QTableWidgetItem *item;

      item = new QTableWidgetItem();

      switch (col) {
      case 0:
        item->setText(pserver->host);
        break;
      case 1:
        item->setText(tstring);
        break;
      case 2:
        item->setText(pserver->version);
        break;
      case 3:
        item->setText(_(pserver->state));
        break;
      case 4:
        item->setText(buf);
        break;
      case 5:
        item->setText(pserver->message);
        break;
      default:
        break;
      }
      sel->setItem(row, col, item);
    }

    if (host == pserver->host && port == pserver->port) {
      sel->selectRow(row);
    }

    row++;
  }
  server_list_iterate_end;

  /* Remove unneeded rows, if there are any */
  while (old_row_count - row > 0) {
    sel->removeRow(old_row_count - 1);
    old_row_count--;
  }
}

/**********************************************************************/ /**
   Callback function for when there's an error in the server scan.
 **************************************************************************/
void server_scan_error(struct server_scan *scan, const char *message)
{
  qtg_version_message(message);
  log_error("%s", message);

  /* Main thread will finalize the scan later (or even concurrently) -
   * do not do anything here to cause double free or raze condition. */
}

/**********************************************************************/ /**
   Free the server scans.
 **************************************************************************/
void fc_client::destroy_server_scans(void)
{
  if (meta_scan) {
    server_scan_finish(meta_scan);
    meta_scan = NULL;
  }

  if (meta_scan_timer != NULL) {
    meta_scan_timer->stop();
    meta_scan_timer->disconnect();
    delete meta_scan_timer;
    meta_scan_timer = NULL;
  }

  if (lan_scan) {
    server_scan_finish(lan_scan);
    lan_scan = NULL;
  }

  if (lan_scan_timer != NULL) {
    lan_scan_timer->stop();
    lan_scan_timer->disconnect();
    delete lan_scan_timer;
    lan_scan_timer = NULL;
  }
}

/**********************************************************************/ /**
   Stop and restart the metaserver and lan server scans.
 **************************************************************************/
void fc_client::update_network_lists(void)
{
  destroy_server_scans();

  lan_scan_timer = new QTimer(this);
  lan_scan = server_scan_begin(SERVER_SCAN_LOCAL, server_scan_error);
  connect(lan_scan_timer, &QTimer::timeout, this, &fc_client::slot_lan_scan);
  lan_scan_timer->start(500);

  meta_scan_timer = new QTimer(this);
  meta_scan = server_scan_begin(SERVER_SCAN_GLOBAL, server_scan_error);
  connect(meta_scan_timer, &QTimer::timeout, this,
          &fc_client::slot_meta_scan);
  meta_scan_timer->start(800);
}

/**********************************************************************/ /**
   This function updates the list of servers every so often.
 **************************************************************************/
bool fc_client::check_server_scan(server_scan *scan_data)
{
  struct server_scan *scan = scan_data;
  enum server_scan_status stat;

  if (!scan) {
    return false;
  }

  stat = server_scan_poll(scan);

  if (stat >= SCAN_STATUS_PARTIAL) {
    enum server_scan_type type;
    struct srv_list *srvrs;

    type = server_scan_get_type(scan);
    srvrs = server_scan_get_list(scan);
    fc_allocate_mutex(&srvrs->mutex);
    holding_srv_list_mutex = true;
    update_server_list(type, srvrs->servers);
    holding_srv_list_mutex = false;
    fc_release_mutex(&srvrs->mutex);
  }

  if (stat == SCAN_STATUS_ERROR || stat == SCAN_STATUS_DONE) {
    return false;
  }

  return true;
}

/**********************************************************************/ /**
   Executes lan scan network
 **************************************************************************/
void fc_client::slot_lan_scan()
{
  if (lan_scan_timer == NULL) {
    return;
  }
  check_server_scan(lan_scan);
}

/**********************************************************************/ /**
   Executes metaserver scan network
 **************************************************************************/
void fc_client::slot_meta_scan()
{
  if (meta_scan_timer == NULL) {
    return;
  }
  check_server_scan(meta_scan);
}

/**********************************************************************/ /**
   spawn a server, if there isn't one, using the default settings.
 **************************************************************************/
void fc_client::start_new_game()
{
  if (is_server_running() || client_start_server()) {
    /* saved settings are sent in client/options.c load_settable_options() */
  }
}


/**********************************************************************/ /**
   Selection chnaged in some tableview on some page
 **************************************************************************/
void fc_client::slot_selection_changed(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{

  QModelIndexList indexes = selected.indexes();
  QStringList sl;
  QModelIndex index;
  QTableWidgetItem *item;
  QItemSelectionModel *tw;
  QVariant qvar;
  QString str_pixmap;

  client_pages i = current_page();
  const char *terr_name;
  const struct server *pserver = NULL;
  int ii = 0;
  int k, col, n, nat_y, nat_x;
  struct section_file *sf;
  struct srv_list *srvrs;
  QByteArray fn_bytes;

  if (indexes.isEmpty()) {
    return;
  }

  switch (i) {
  case PAGE_NETWORK:
    index = indexes.at(0);
    connect_host_edit->setText(index.data().toString());
    index = indexes.at(1);
    connect_port_edit->setText(index.data().toString());

    tw = qobject_cast<QItemSelectionModel *>(sender());

    if (tw == lan_widget->selectionModel()) {
      wan_widget->clearSelection();
    } else {
      lan_widget->clearSelection();
    }

    srvrs = server_scan_get_list(meta_scan);
    if (!holding_srv_list_mutex) {
      fc_allocate_mutex(&srvrs->mutex);
    }
    if (srvrs->servers) {
      pserver = server_list_get(srvrs->servers, index.row());
    }
    if (!holding_srv_list_mutex) {
      fc_release_mutex(&srvrs->mutex);
    }
    if (!pserver || !pserver->players) {
      return;
    }
    n = pserver->nplayers;
    info_widget->clearContents();
    info_widget->setRowCount(0);
    for (k = 0; k < n; k++) {
      info_widget->insertRow(k);
      for (col = 0; col < 4; col++) {
        item = new QTableWidgetItem();
        switch (col) {
        case 0:
          item->setText(pserver->players[k].name);
          break;
        case 1:
          item->setText(pserver->players[k].type);
          break;
        case 2:
          item->setText(pserver->players[k].host);
          break;
        case 3:
          item->setText(pserver->players[k].nation);
          break;
        default:
          break;
        }
        info_widget->setItem(k, col, item);
      }
    }
    break;
  case PAGE_SCENARIO:
    index = indexes.at(0);
    qvar = index.data(Qt::UserRole);
    sl = qvar.toStringList();
    scenarios_text->setText(sl.at(0));
    if (sl.count() > 1) {
      scenarios_view->setText(sl.at(2));
      current_file = sl.at(1);
    }
    break;
  case PAGE_LOAD:
    index = indexes.at(0);
    qvar = index.data(Qt::UserRole);
    current_file = qvar.toString();
    if (show_preview->checkState() == Qt::Unchecked) {
      load_pix->setPixmap(*(new QPixmap));
      load_save_text->setText("");
      break;
    }
    fn_bytes = current_file.toLocal8Bit();
    if ((sf = secfile_load_section(fn_bytes.data(), "game", TRUE))) {
      const char *sname;
      bool sbool;
      int integer;
      QString final_str;
      QString pl_str = nullptr;
      int num_players = 0;
      int curr_player = 0;
      QByteArray pl_bytes;

      integer = secfile_lookup_int_default(sf, -1, "game.turn");
      if (integer >= 0) {
        final_str = QString("<b>") + _("Turn") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      if ((sf = secfile_load_section(fn_bytes.data(), "players", TRUE))) {
        integer = secfile_lookup_int_default(sf, -1, "players.nplayers");
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Players") + ":</b>" + " "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        num_players = integer;
      }
      for (int i = 0; i < num_players; i++) {
        pl_str = QString("player") + QString::number(i);
        pl_bytes = pl_str.toLocal8Bit();
        if ((sf = secfile_load_section(fn_bytes.data(), pl_bytes.data(),
                                       true))) {
          if (!(sbool = secfile_lookup_bool_default(
                    sf, true, "player%d.unassigned_user", i))) {
            curr_player = i;
            break;
          }
        }
      }
      /* Break case (and return) if no human player found */
      if (pl_str == nullptr) {
        load_save_text->setText(final_str);
        break;
      }

      /* Information about human player */
      pl_bytes = pl_str.toLocal8Bit();
      if ((sf = secfile_load_section(fn_bytes.data(), pl_bytes.data(),
                                     true))) {
        sname = secfile_lookup_str_default(sf, nullptr, "player%d.nation",
                                           curr_player);
        if (sname) {
          final_str = final_str + "<b>" + _("Nation") + ":</b> "
                      + QString(sname).toHtmlEscaped() + "<br>";
        }
        integer = secfile_lookup_int_default(sf, -1, "player%d.ncities",
                                             curr_player);
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Cities") + ":</b> "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        integer = secfile_lookup_int_default(sf, -1, "player%d.nunits",
                                             curr_player);
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Units") + ":</b> "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        integer =
            secfile_lookup_int_default(sf, -1, "player%d.gold", curr_player);
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Gold") + ":</b> "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        nat_x = 0;
        for (nat_y = 0; nat_y > -1; nat_y++) {
          const char *line = secfile_lookup_str_default(
              sf, nullptr, "player%d.map_t%04d", curr_player, nat_y);
          if (line == nullptr) {
            break;
          }
          nat_x = strlen(line);
          str_pixmap = str_pixmap + line;
        }

        /* Reset terrain information */
        terrain_type_iterate(pterr) { pterr->identifier_load = '\0'; }
        terrain_type_iterate_end;

        /* Load possible terrains and their identifiers (chars) */
        if ((sf = secfile_load_section(fn_bytes.data(), "savefile", true)))
          while ((terr_name = secfile_lookup_str_default(
                      sf, NULL, "savefile.terrident%d.name", ii))
                 != NULL) {
            struct terrain *pterr = terrain_by_rule_name(terr_name);
            if (pterr != NULL) {
              const char *iptr = secfile_lookup_str_default(
                  sf, NULL, "savefile.terrident%d.identifier", ii);
              pterr->identifier_load = *iptr;
            }
            ii++;
          }

        /* Create image */
        QImage img(nat_x, nat_y, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::black);
        for (int a = 0; a < nat_x; a++) {
          for (int b = 0; b < nat_y; b++) {
            struct terrain *tr;
            struct rgbcolor *rgb;
            tr = char2terrain(str_pixmap.at(b * nat_x + a).toLatin1());
            if (tr != nullptr) {
              rgb = tr->rgb;
              QColor col;
              col.setRgb(rgb->r, rgb->g, rgb->b);
              img.setPixel(a, b, col.rgb());
            }
          }
        }
        if (img.width() > 1) {
          load_pix->setPixmap(QPixmap::fromImage(img).scaledToHeight(200));
        } else {
          load_pix->setPixmap(*(new QPixmap));
        }
        load_pix->setFixedSize(load_pix->pixmap()->width(),
                               load_pix->pixmap()->height());
        if ((sf = secfile_load_section(fn_bytes.data(), "research", TRUE))) {
          sname = secfile_lookup_str_default(
              sf, nullptr, "research.r%d.now_name", curr_player);
          if (sname) {
            final_str = final_str + "<b>" + _("Researching") + ":</b> "
                        + QString(sname).toHtmlEscaped();
          }
        }
      }
      load_save_text->setText(final_str);
    }
    break;
  default:
    break;
  }
}



/**********************************************************************/ /**
   Configure the dialog depending on what type of authentication request the
   server is making.
 **************************************************************************/
void fc_client::handle_authentication_req(enum authentication_type type,
                                          const char *message)
{
  set_status_bar(QString::fromUtf8(message));
  destroy_server_scans();

  switch (type) {
  case AUTH_NEWUSER_FIRST:
  case AUTH_NEWUSER_RETRY:
    set_connection_state(NEW_PASSWORD_TYPE);
    return;
  case AUTH_LOGIN_FIRST:
    /* if we magically have a password already present in 'password'
     * then, use that and skip the password entry dialog */
    if (password[0] != '\0') {
      struct packet_authentication_reply reply;

      sz_strlcpy(reply.password, password);
      send_packet_authentication_reply(&client.conn, &reply);
      return;
    } else {
      set_connection_state(ENTER_PASSWORD_TYPE);
    }

    return;
  case AUTH_LOGIN_RETRY:
    set_connection_state(ENTER_PASSWORD_TYPE);
    return;
  }

  log_error("Unsupported authentication type %d: %s.", type, message);
}

/**********************************************************************/ /**
   If on the network page, switch page to the login page (with new server
   and port). if on the login page, send connect and/or authentication
   requests to the server.
 **************************************************************************/
void fc_client::slot_connect()
{
  char errbuf[512];
  struct packet_authentication_reply reply;
  QByteArray ba_bytes;

  switch (connection_status) {
  case LOGIN_TYPE:
    ba_bytes = connect_login_edit->text().toLocal8Bit();
    sz_strlcpy(user_name, ba_bytes.data());
    ba_bytes = connect_host_edit->text().toLocal8Bit();
    sz_strlcpy(server_host, ba_bytes.data());
    server_port = connect_port_edit->text().toInt();

    if (connect_to_server(user_name, server_host, server_port, errbuf,
                          sizeof(errbuf))
        != -1) {
    } else {
      set_status_bar(QString::fromUtf8(errbuf));
      output_window_append(ftc_client, errbuf);
    }

    return;
  case NEW_PASSWORD_TYPE:
    ba_bytes = connect_password_edit->text().toLatin1();
    sz_strlcpy(password, ba_bytes.data());
    ba_bytes = connect_confirm_password_edit->text().toLatin1();
    sz_strlcpy(reply.password, ba_bytes.data());

    if (strncmp(reply.password, password, MAX_LEN_NAME) == 0) {
      password[0] = '\0';
      send_packet_authentication_reply(&client.conn, &reply);
      set_connection_state(WAITING_TYPE);
    } else {
      set_status_bar(_("Passwords don't match, enter password."));
      set_connection_state(NEW_PASSWORD_TYPE);
    }

    return;
  case ENTER_PASSWORD_TYPE:
    ba_bytes = connect_password_edit->text().toLatin1();
    sz_strlcpy(reply.password, ba_bytes.data());
    send_packet_authentication_reply(&client.conn, &reply);
    set_connection_state(WAITING_TYPE);
    return;
  case WAITING_TYPE:
    return;
  }

  log_error("Unsupported connection status: %d", connection_status);
}

/**********************************************************************/ /**
   Calls dialg selecting nations
 **************************************************************************/
void fc_client::slot_pick_nation() { popup_races_dialog(client_player()); }

/**********************************************************************/ /**
   Reloads sidebar icons (useful on theme change)
 **************************************************************************/
void fc_client::reload_sidebar_icons()
{
  sw_map->set_pixmap(fc_icons::instance()->get_pixmap("view"));
  sw_cunit->set_pixmap(fc_icons::instance()->get_pixmap("units"));
  sw_cities->set_pixmap(fc_icons::instance()->get_pixmap("cities"));
  sw_diplo->set_pixmap(fc_icons::instance()->get_pixmap("nations"));
  sw_science->set_pixmap(fc_icons::instance()->get_pixmap("research"));
  sw_economy->set_pixmap(fc_icons::instance()->get_pixmap("economy"));
  sw_endturn->set_pixmap(fc_icons::instance()->get_pixmap("endturn"));
  sidebar_wdg->resize_me(game_tab_widget->height(), true);
}

/**********************************************************************/ /**
   Updates sidebar tooltips
 **************************************************************************/
void fc_client::update_sidebar_tooltips()
{
  QString str;
  int max;
  int entries_used, building_total, unit_total, tax;
  char buf[256];

  struct improvement_entry building_entries[B_LAST];
  struct unit_entry unit_entries[U_LAST];

  if (current_page() != PAGE_GAME) {
    return;
  }

  if (NULL != client.conn.playing) {
    max = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  } else {
    max = 100;
  }

  if (!client_is_global_observer()) {
    sw_science->set_tooltip(science_dialog_text());
    str = QString(nation_plural_for_player(client_player()));
    str = str + '\n' + get_info_label_text(false);
    sw_map->set_tooltip(str);
    str = QString(_("Tax: %1% Science: %2% Luxury: %3%\n"))
              .arg(client.conn.playing->economic.tax)
              .arg(client.conn.playing->economic.luxury)
              .arg(client.conn.playing->economic.science);

    str += QString(_("%1 - max rate: %2%"))
               .arg(government_name_for_player(client.conn.playing),
                    QString::number(max));

    get_economy_report_units_data(unit_entries, &entries_used, &unit_total);
    get_economy_report_data(building_entries, &entries_used, &building_total,
                            &tax);
    fc_snprintf(buf, sizeof(buf), _("Income: %d    Total Costs: %d"), tax,
                building_total + unit_total);
    sw_economy->set_tooltip(buf);
    if (player_primary_capital(client_player())) {
      sw_cities->set_tooltip(
          text_happiness_cities(player_primary_capital(client_player())));
    }
  } else {
    sw_tax->set_tooltip("");
    sw_science->set_tooltip("");
    sw_map->set_tooltip("");
    sw_economy->set_tooltip("");
  }
  sw_indicators->set_tooltip(QString(get_info_label_text_popup()));
}

/**********************************************************************/ /**
   Centers next enemy city on view
 **************************************************************************/
void center_next_enemy_city()
{
  bool center_next = false;
  bool first_tile = false;
  int first_id;
  struct tile *ptile = nullptr;

  players_iterate(pplayer)
  {
    if (pplayer != client_player()) {
      city_list_iterate(pplayer->cities, pcity)
      {
        if (!first_tile) {
          first_tile = true;
          ptile = pcity->tile;
          first_id = pcity->id;
        }
        if ((last_center_enemy_city == 0) || center_next) {
          last_center_enemy_city = pcity->id;
          center_tile_mapcanvas(pcity->tile);
          return;
        }
        if (pcity->id == last_center_enemy_city) {
          center_next = true;
        }
      }
      city_list_iterate_end;
    }
  }
  players_iterate_end;

  if (ptile != nullptr) {
    center_tile_mapcanvas(ptile);
    last_center_enemy_city = first_id;
  }
}

/**********************************************************************/ /**
   Centers next player city on view
 **************************************************************************/
void center_next_player_city()
{
  bool center_next = false;
  bool first_tile = false;
  int first_id;
  struct tile *ptile = nullptr;

  players_iterate(pplayer)
  {
    if (pplayer == client_player()) {
      city_list_iterate(pplayer->cities, pcity)
      {
        if (!first_tile) {
          first_tile = true;
          ptile = pcity->tile;
          first_id = pcity->id;
        }
        if ((last_center_player_city == 0) || center_next) {
          last_center_player_city = pcity->id;
          center_tile_mapcanvas(pcity->tile);
          return;
        }
        if (pcity->id == last_center_player_city) {
          center_next = true;
        }
      }
      city_list_iterate_end;
    }
  }
  players_iterate_end;

  if (ptile != nullptr) {
    center_tile_mapcanvas(ptile);
    last_center_player_city = first_id;
  }
}

/**********************************************************************/ /**
   Centers next enemy capital
 **************************************************************************/
void center_next_player_capital()
{
  struct city *capital;
  bool center_next = false;
  bool first_tile = false;
  int first_id;
  struct tile *ptile = nullptr;

  players_iterate(pplayer)
  {
    if (pplayer != client_player()) {
      capital = player_primary_capital(pplayer);
      if (capital == nullptr) {
        continue;
      }
      if (!first_tile) {
        first_tile = true;
        ptile = capital->tile;
        first_id = capital->id;
      }
      if ((last_center_player_city == 0) || center_next) {
        last_center_player_city = capital->id;
        center_tile_mapcanvas(capital->tile);
        put_cross_overlay_tile(capital->tile);
        return;
      }
      if (capital->id == last_center_player_city) {
        center_next = true;
      }
    }
  }
  players_iterate_end;

  if (ptile != nullptr) {
    center_tile_mapcanvas(ptile);
    put_cross_overlay_tile(ptile);
    last_center_player_city = first_id;
  }
}

/**********************************************************************/ /**
   Update position
 **************************************************************************/
void fc_client::update_sidebar_position()
{
  pages_layout[PAGE_GAME]->removeWidget(gui()->sidebar_wdg);
  if (gui_options.gui_qt_sidebar_left) {
    pages_layout[PAGE_GAME]->addWidget(sidebar_wdg, 1, 0);
  } else {
    pages_layout[PAGE_GAME]->addWidget(sidebar_wdg, 1, 2);
  }
}

/**********************************************************************/ /**
   Center on next enemy unit
 **************************************************************************/
void cycle_enemy_units()
{
  bool center_next = false;
  bool first_tile = false;
  int first_id;
  struct tile *ptile = nullptr;

  players_iterate(pplayer)
  {
    if (pplayer != client_player()) {
      unit_list_iterate(pplayer->units, punit)
      {
        if (!first_tile) {
          first_tile = true;
          ptile = punit->tile;
          first_id = punit->id;
        }
        if ((last_center_enemy == 0) || center_next) {
          last_center_enemy = punit->id;
          center_tile_mapcanvas(punit->tile);
          return;
        }
        if (punit->id == last_center_enemy) {
          center_next = true;
        }
      }
      unit_list_iterate_end;
    }
  }
  players_iterate_end;

  if (ptile != nullptr) {
    center_tile_mapcanvas(ptile);
    last_center_enemy = first_id;
  }
}
