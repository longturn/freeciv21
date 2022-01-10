/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "fc_client.h"
// Qt
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QStackedLayout>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTcpSocket>
#include <QTextBlock>
#include <QTextCodec>
// utility
#include "fcintl.h"
// common
#include "climisc.h"
#include "game.h"
// client
#include "chatline_common.h"
#include "chatline_g.h"
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"
// gui-qt
#include "colors.h"
#include "fonts.h"
#include "gui_main.h"
#include "icons.h"
#include "mapview.h"
#include "messagewin.h"
#include "minimap.h"
#include "optiondlg.h"
#include "page_game.h"
#include "page_load.h"
#include "page_main.h"
#include "page_network.h"
#include "page_pregame.h"
#include "page_scenario.h"
#include "sidebar.h"
#include "sprite.h"
#include "tileset_debugger.h"
#include "voteinfo_bar.h"

fcFont *fcFont::m_instance = 0;
extern "C" void real_science_report_dialog_update(void *);

/**
   Constructor
 */
fc_client::fc_client() : QMainWindow(), current_file(QLatin1String(""))
{
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
  status_bar_queue.clear();
  for (int i = 0; i <= PAGE_GAME; i++) {
    pages_layout[i] = NULL;
    pages[i] = NULL;
  }
  fcFont::instance()->initFonts();
  read_settings();
  QApplication::setFont(fcFont::instance()->getFont(fonts::default_font));
  QString path;

  central_wdg = new QWidget;
  central_layout = new QStackedLayout;

  menu_bar = new mr_menu();
  corner_wid = new fc_corner(this);
  if (!gui_options.gui_qt_show_titlebar) {
    menu_bar->setCornerWidget(corner_wid);
  }
  setMenuBar(menu_bar);
  status_bar = statusBar();
  status_bar_label = new QLabel;
  status_bar_label->setAlignment(Qt::AlignCenter);
  status_bar->addWidget(status_bar_label, 1);
  set_status_bar(_("Welcome to Freeciv21"));
  create_cursors();
  // fake color init for research diagram
  research_color::i()->setFixedSize(1, 1);
  research_color::i()->hide();
  pages[PAGE_MAIN] = new page_main(central_wdg, this);
  page = PAGE_MAIN;
  pages[PAGE_START] = new page_pregame(central_wdg, this);
  pages[PAGE_SCENARIO] = new page_scenario(central_wdg, this);
  pages[PAGE_LOAD] = new page_load(central_wdg, this);
  pages[PAGE_NETWORK] = new page_network(central_wdg, this);
  pages[PAGE_NETWORK]->setVisible(false);
  gui_options.gui_qt_allied_chat_only = true;
  path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (!path.isEmpty()) {
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, path);
  }
  init_mapcanvas_and_overview();
  pages[PAGE_GAME] = new pageGame(central_wdg);
  pages[PAGE_GAME + 1] = new QWidget(central_wdg);
  create_loading_page();
  pages[PAGE_GAME + 1]->setLayout(pages_layout[PAGE_GAME + 1]);
  central_layout->addWidget(pages[PAGE_MAIN]);
  central_layout->addWidget(pages[PAGE_NETWORK]);
  central_layout->addWidget(pages[PAGE_LOAD]);
  central_layout->addWidget(pages[PAGE_SCENARIO]);
  central_layout->addWidget(pages[PAGE_START]);
  central_layout->addWidget(pages[PAGE_GAME]);
  central_layout->addWidget(pages[PAGE_GAME + 1]);
  central_wdg->setLayout(central_layout);
  setCentralWidget(central_wdg);
  resize(pages[PAGE_MAIN]->minimumSizeHint());
  setVisible(true);
  QPixmapCache::setCacheLimit(80000);
}

/**
   Destructor
 */
fc_client::~fc_client()
{
  status_bar_queue.clear();
  if (fc_shortcuts::sc()) {
    delete fc_shortcuts::sc();
  }
  mrIdle::idlecb()->drop();
  delete_cursors();
}

/**
   Main part of gui-qt.
   This is not called simply 'fc_client::main()', since SDL includes
   ould sometimes cause 'main' to be considered an macro that expands to
   'SDL_main'
 */
void fc_client::fc_main(QApplication *qapp)
{
  qRegisterMetaType<QTextCursor>("QTextCursor");
  qRegisterMetaType<QTextBlock>("QTextBlock");
  fc_allocate_ow_mutex();
  real_output_window_append(_("This is the client for Freeciv21."), NULL,
                            -1);
  fc_release_ow_mutex();
  chat_welcome_message(true);

  set_client_state(C_S_DISCONNECTED);

  startTimer(TIMER_INTERVAL);
  connect(qapp, &QCoreApplication::aboutToQuit, this, &fc_client::closing);
  qapp->exec();

  free_mapcanvas_and_overview();
  tileset_free_tiles(tileset);
}

/**
   Returns status if fc_client is being closed
 */
bool fc_client::is_closing() { return quitting; }

/**
   Called when fc_client is going to quit
 */
void fc_client::closing() { quitting = true; }

/**
   Switch from one client page to another.
   Argument is int cause QSignalMapper doesn't want to work with enum
   Because chat widget is in 2 layouts we need to switch between them here
   (addWidget removes it from prevoius layout automatically)
 */
void fc_client::switch_page(int new_pg)
{
  enum client_pages new_page;
  int i_page;

  new_page = static_cast<client_pages>(new_pg);

  if ((new_page == PAGE_SCENARIO || new_page == PAGE_LOAD)
      && !is_server_running()) {
    current_file = QLatin1String("");
    client_start_server_and_set_page(new_page);
    return;
  }

  if (page == PAGE_NETWORK) {
    qobject_cast<page_network *>(pages[PAGE_NETWORK])
        ->destroy_server_scans();
  }
  menuBar()->setVisible(false);
  if (status_bar != nullptr) {
    status_bar->setVisible(true);
  }
  QApplication::alert(king()->central_wdg);
  central_layout->setCurrentWidget(pages[new_pg]);
  page = new_page;
  i_page = new_page;
  switch (i_page) {
  case PAGE_MAIN:
    break;
  case PAGE_START:
    voteinfo_gui_update();
    break;
  case PAGE_LOAD:
    qobject_cast<page_load *>(pages[PAGE_LOAD])->update_load_page();
    break;
  case PAGE_GAME:
    tileset_changed();
    if (!gui_options.gui_qt_show_titlebar) {
      setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    }
    showMaximized();
    // Uncommenting will fix some resizing errors but will cause some
    // problems with no focus caused by update_queue or something
    // QCoreApplication::processEvents();
    // For MS Windows, it might ingore first
    showMaximized();
    queen()->infotab->chtwdg->update_widgets();
    status_bar->setVisible(false);
    if (gui_options.gui_qt_fullscreen) {
      king()->showFullScreen();
      queen()->game_tab_widget->showFullScreen();
    }
    menuBar()->setVisible(true);
    queen()->mapview_wdg->setFocus();
    center_on_something();
    voteinfo_gui_update();
    update_info_label();
    queen()->minimapview_wdg->reset();
    update_minimap();
    queen()->updateSidebarTooltips();
    real_science_report_dialog_update(nullptr);
    show_new_turn_info();
    break;
  case PAGE_SCENARIO:
    qobject_cast<page_scenario *>(pages[PAGE_SCENARIO])
        ->update_scenarios_page();
    break;
  case PAGE_NETWORK:
    qobject_cast<page_network *>(pages[PAGE_NETWORK])
        ->update_network_lists();
    qobject_cast<page_network *>(pages[PAGE_NETWORK])
        ->set_connection_state(LOGIN_TYPE);
    break;
  case (PAGE_GAME + 1):
    break;
  default:
    if (client.conn.used) {
      disconnect_from_server();
    }
    set_client_page(PAGE_MAIN);
    break;
  }

  // Maybe popdown the tileset debugger
  if (page != PAGE_GAME) {
    queen()->mapview_wdg->hide_debugger();
  }
}

/**
   Returns currently open page
 */
enum client_pages fc_client::current_page() { return page; }

/**
   Add notifier for server input
 */
void fc_client::add_server_source(QTcpSocket *sock)
{
  connect(sock, &QIODevice::readyRead, this, &fc_client::server_input);

  // By the time we reach this function, the socket may already have received
  // data. Make sure that it's processed as well.
  input_from_server(sock);
}

/**
   Closes main window
 */
void fc_client::closeEvent(QCloseEvent *event)
{
  popup_quit_dialog();
  event->ignore();
}

/**
   There is input from server
 */
void fc_client::server_input()
{
  if (auto *socket = dynamic_cast<QTcpSocket *>(sender())) {
    input_from_server(socket);
  }
}

/**
   Timer event handling
 */
void fc_client::timerEvent(QTimerEvent *event)
{
  // Prevent current timer from repeating with possibly wrong interval
  killTimer(event->timerId());

  // Call timer callback in client common code and
  // start new timer with correct interval
  startTimer(real_timer_callback() * 1000);
}

/**
   Quit App
 */
void fc_client::quit()
{
  QApplication *qapp = current_app();

  if (qapp != nullptr) {
    qapp->quit();
  }
}

/**
   Disconnect from server and return to MAIN PAGE
 */
void fc_client::slot_disconnect()
{
  if (client.conn.used) {
    disconnect_from_server();
  }

  switch_page(PAGE_MAIN);
}

/****************************************************************************
  Deletes cursors
****************************************************************************/
void fc_client::delete_cursors()
{
  int frame;
  int cursor;

  for (cursor = 0; cursor < CURSOR_LAST; cursor++) {
    for (frame = 0; frame < NUM_CURSOR_FRAMES; frame++) {
      delete fc_cursors[cursor][frame];
    }
  }
}

/**
   Loads qt-specific options
 */
void fc_client::read_settings()
{
  QSettings s(QSettings::IniFormat, QSettings::UserScope,
              QStringLiteral("freeciv21-client"));
  if (s.contains(QStringLiteral("Chat-fx-size"))) {
    qt_settings.chat_fwidth =
        s.value(QStringLiteral("Chat-fx-size")).toFloat();
  } else {
    qt_settings.chat_fwidth = 0.2;
  }
  if (s.contains(QStringLiteral("Chat-fy-size"))) {
    qt_settings.chat_fheight =
        s.value(QStringLiteral("Chat-fy-size")).toFloat();
  } else {
    qt_settings.chat_fheight = 0.4;
  }
  if (s.contains(QStringLiteral("Chat-fx-pos"))) {
    qt_settings.chat_fx_pos =
        s.value(QStringLiteral("Chat-fx-pos")).toFloat();
  } else {
    qt_settings.chat_fx_pos = 0.0;
  }
  if (s.contains(QStringLiteral("Chat-fy-pos"))) {
    qt_settings.chat_fy_pos =
        s.value(QStringLiteral("Chat-fy-pos")).toFloat();
  } else {
    qt_settings.chat_fy_pos = 0.6;
  }
  if (s.contains(QStringLiteral("unit_fx"))) {
    qt_settings.unit_info_pos_fx =
        s.value(QStringLiteral("unit_fx")).toFloat();
  } else {
    qt_settings.unit_info_pos_fx = 0.33;
  }
  if (s.contains(QStringLiteral("unit_fy"))) {
    qt_settings.unit_info_pos_fy =
        s.value(QStringLiteral("unit_fy")).toFloat();
  } else {
    qt_settings.unit_info_pos_fy = 0.88;
  }
  if (s.contains(QStringLiteral("minimap_x"))) {
    qt_settings.minimap_x = s.value(QStringLiteral("minimap_x")).toFloat();
  } else {
    qt_settings.minimap_x = 0.84;
  }
  if (s.contains(QStringLiteral("minimap_y"))) {
    qt_settings.minimap_y = s.value(QStringLiteral("minimap_y")).toFloat();
  } else {
    qt_settings.minimap_y = 0.79;
  }
  if (s.contains(QStringLiteral("minimap_width"))) {
    qt_settings.minimap_width =
        s.value(QStringLiteral("minimap_width")).toFloat();
  } else {
    qt_settings.minimap_width = 0.15;
  }
  if (s.contains(QStringLiteral("minimap_height"))) {
    qt_settings.minimap_height =
        s.value(QStringLiteral("minimap_height")).toFloat();
  } else {
    qt_settings.minimap_height = 0.2;
  }
  if (s.contains(QStringLiteral("battlelog_scale"))) {
    qt_settings.battlelog_scale =
        s.value(QStringLiteral("battlelog_scale")).toFloat();
  } else {
    qt_settings.battlelog_scale = 1;
  }

  if (s.contains(QStringLiteral("help-dialog"))) {
    qt_settings.help_geometry =
        s.value(QStringLiteral("help-dialog")).toByteArray();
  }
  if (s.contains(QStringLiteral("help_splitter1"))) {
    qt_settings.help_splitter1 =
        s.value(QStringLiteral("help_splitter1")).toByteArray();
  }
  if (s.contains(QStringLiteral("new_turn_text"))) {
    qt_settings.show_new_turn_text =
        s.value(QStringLiteral("new_turn_text")).toBool();
  } else {
    qt_settings.show_new_turn_text = true;
  }
  if (s.contains(QStringLiteral("show_battle_log"))) {
    qt_settings.show_battle_log =
        s.value(QStringLiteral("show_battle_log")).toBool();
  } else {
    qt_settings.show_battle_log = false;
  }
  if (s.contains(QStringLiteral("battlelog_x"))) {
    qt_settings.battlelog_x =
        s.value(QStringLiteral("battlelog_x")).toFloat();
  } else {
    qt_settings.battlelog_x = 0.0;
  }
  if (s.contains(QStringLiteral("battlelog_y"))) {
    qt_settings.battlelog_y =
        s.value(QStringLiteral("battlelog_y")).toFloat();
  } else {
    qt_settings.battlelog_y = 0.0;
  }
  if (s.contains(QStringLiteral("civstatus_x"))) {
    qt_settings.civstatus_x =
        s.value(QStringLiteral("civstatus_x")).toFloat();
  } else {
    qt_settings.civstatus_x = 0.0;
  }
  if (s.contains(QStringLiteral("civstatus_y"))) {
    qt_settings.civstatus_y =
        s.value(QStringLiteral("civstatus_y")).toFloat();
  } else {
    qt_settings.civstatus_y = 0.0;
  }
  qt_settings.player_repo_sort_col = -1;
  qt_settings.city_repo_sort_col = -1;

  if (qt_settings.chat_fx_pos < 0 || qt_settings.chat_fx_pos >= 1) {
    qt_settings.chat_fx_pos = 0.0;
  }
  if (qt_settings.chat_fy_pos >= 1 || qt_settings.chat_fy_pos < 0) {
    qt_settings.chat_fy_pos = 0.6;
  }
  if (qt_settings.chat_fwidth < 0.05 || qt_settings.chat_fwidth > 0.9) {
    qt_settings.chat_fwidth = 0.2;
  }
  if (qt_settings.chat_fheight < 0.05 || qt_settings.chat_fheight > 0.9) {
    qt_settings.chat_fheight = 0.33;
  }
  if (qt_settings.battlelog_x < 0.0) {
    qt_settings.battlelog_x = 0.33;
  }
  if (qt_settings.battlelog_y < 0.0) {
    qt_settings.battlelog_y = 0.0;
  }
  if (qt_settings.battlelog_scale > 5.0) {
    qt_settings.battlelog_y = 5.0;
  }
  if (qt_settings.minimap_x < 0 || qt_settings.minimap_x > 1) {
    qt_settings.chat_fx_pos = 0.84;
  }
  if (qt_settings.minimap_y < 0 || qt_settings.minimap_y > 1) {
    qt_settings.chat_fx_pos = 0.79;
  }
}

/**
   Save qt-specific options
 */
void fc_client::write_settings()
{
  QSettings s(QSettings::IniFormat, QSettings::UserScope,
              QStringLiteral("freeciv21-client"));
  s.setValue(QStringLiteral("Fonts-set"), true);
  s.setValue(QStringLiteral("Chat-fx-size"), qt_settings.chat_fwidth);
  s.setValue(QStringLiteral("Chat-fy-size"), qt_settings.chat_fheight);
  s.setValue(QStringLiteral("Chat-fx-pos"), qt_settings.chat_fx_pos);
  s.setValue(QStringLiteral("Chat-fy-pos"), qt_settings.chat_fy_pos);
  s.setValue(QStringLiteral("help-dialog"), qt_settings.help_geometry);
  s.setValue(QStringLiteral("help_splitter1"), qt_settings.help_splitter1);
  s.setValue(QStringLiteral("unit_fx"), qt_settings.unit_info_pos_fx);
  s.setValue(QStringLiteral("unit_fy"), qt_settings.unit_info_pos_fy);
  s.setValue(QStringLiteral("minimap_x"), qt_settings.minimap_x);
  s.setValue(QStringLiteral("minimap_y"), qt_settings.minimap_y);
  s.setValue(QStringLiteral("minimap_width"), qt_settings.minimap_width);
  s.setValue(QStringLiteral("minimap_height"), qt_settings.minimap_height);
  s.setValue(QStringLiteral("battlelog_scale"), qt_settings.battlelog_scale);
  s.setValue(QStringLiteral("battlelog_x"), qt_settings.battlelog_x);
  s.setValue(QStringLiteral("battlelog_y"), qt_settings.battlelog_y);
  s.setValue(QStringLiteral("civstatus_x"), qt_settings.civstatus_x);
  s.setValue(QStringLiteral("civstatus_y"), qt_settings.civstatus_y);
  s.setValue(QStringLiteral("new_turn_text"),
             qt_settings.show_new_turn_text);
  s.setValue(QStringLiteral("show_battle_log"), qt_settings.show_battle_log);
  write_shortcuts();
}

/**
   Popups client options
 */
void popup_client_options()
{
  option_dialog_popup(_("Set local options"), client_optset);
}

/**
   Setup cursors
 */
void fc_client::create_cursors()
{
  for (int cursor = 0; cursor < CURSOR_LAST; cursor++) {
    for (int frame = 0; frame < NUM_CURSOR_FRAMES; frame++) {
      auto curs = static_cast<cursor_type>(cursor);
      int hot_x, hot_y;
      auto sprite = get_cursor_sprite(tileset, curs, &hot_x, &hot_y, frame);
      auto c = new QCursor(*sprite, hot_x, hot_y);
      fc_cursors[cursor][frame] = c;
    }
  }
}

/**
   Sets the "page" that the client should show.  See also pages_g.h.
 */
void qtg_real_set_client_page(enum client_pages page)
{
  king()->switch_page(page);
}

/**
   Set the list of available rulesets.  The default ruleset should be
   "default", and if the user changes this then set_ruleset() should be
   called.
 */
void qtg_set_rulesets(int num_rulesets, QStringList rulesets)
{
  qobject_cast<page_pregame *>(king()->pages[PAGE_START])
      ->set_rulesets(num_rulesets, rulesets);
}

/**
   Returns current client page
 */
enum client_pages qtg_get_current_client_page()
{
  return king()->current_page();
}

/**
   Update the start page.
 */
void update_start_page(void)
{
  qobject_cast<page_pregame *>(king()->pages[PAGE_START])
      ->update_start_page();
}

/**
   Sets application status bar for given time in miliseconds
 */
void fc_client::set_status_bar(const QString &message, int timeout)
{
  if (status_bar_label->text().isEmpty()) {
    status_bar_label->setText(message);
    QTimer::singleShot(timeout, this, &fc_client::clear_status_bar);
  } else {
    status_bar_queue.append(message);
    while (status_bar_queue.count() > 3) {
      status_bar_queue.removeFirst();
    }
  }
}

/**
   Clears status bar or shows next message in queue if exists
 */
void fc_client::clear_status_bar()
{
  QString str;

  if (!status_bar_queue.isEmpty()) {
    str = status_bar_queue.takeFirst();
    status_bar_label->setText(str);
    QTimer::singleShot(2000, this, &fc_client::clear_status_bar);
  } else {
    status_bar_label->setText(QLatin1String(""));
  }
}

/**
   Creates page LOADING, showing label with Loading text
 */
void fc_client::create_loading_page()
{
  QLabel *label = new QLabel(_("Loading..."));

  pages_layout[PAGE_GAME + 1] = new QGridLayout;
  pages_layout[PAGE_GAME + 1]->addWidget(label, 0, 0, 1, 1,
                                         Qt::AlignHCenter);
}

/**
   spawn a server, if there isn't one, using the default settings.
 */
void fc_client::start_new_game()
{
  if (is_server_running() || client_start_server(client_url().userName())) {
    /* saved settings are sent in client/options.c load_settable_options() */
  }
}

/**
   Contructor for corner widget (used for menubar)
 */
fc_corner::fc_corner(QMainWindow *qmw) : QWidget()
{
  QHBoxLayout *hb;
  QPushButton *qpb;
  int h;
  QFont f = fcFont::instance()->getFont(fonts::default_font);

  if (f.pointSize() > 0) {
    h = f.pointSize();
  } else {
    h = f.pixelSize();
  }
  mw = qmw;
  hb = new QHBoxLayout();
  qpb = new QPushButton(fcIcons::instance()->getIcon(QStringLiteral("cmin")),
                        QLatin1String(""));
  qpb->setFixedSize(h, h);
  connect(qpb, &QAbstractButton::clicked, this, &fc_corner::minimize);
  hb->addWidget(qpb);
  qpb = new QPushButton(fcIcons::instance()->getIcon(QStringLiteral("cmax")),
                        QLatin1String(""));
  qpb->setFixedSize(h, h);
  connect(qpb, &QAbstractButton::clicked, this, &fc_corner::maximize);
  hb->addWidget(qpb);
  qpb =
      new QPushButton(fcIcons::instance()->getIcon(QStringLiteral("cclose")),
                      QLatin1String(""));
  qpb->setFixedSize(h, h);
  connect(qpb, &QAbstractButton::clicked, this, &fc_corner::close_fc);
  hb->addWidget(qpb);
  setLayout(hb);
}

/**
   Slot for closing freeciv via corner widget
 */
void fc_corner::close_fc() { mw->close(); }

/**
   Slot for maximizing freeciv window via corner widget
 */
void fc_corner::maximize()
{
  if (!mw->isMaximized()) {
    mw->showMaximized();
  } else {
    mw->showNormal();
  }
}

/**
   Slot for minimizing freeciv window via corner widget
 */
void fc_corner::minimize() { mw->showMinimized(); }
