/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// Qt
#include <QMainWindow>
#include <QPixmapCache>
#include <QStackedWidget>

// common
#include "packets.h"
// client
#include "pages_g.h"
#include "servers.h"
#include "tilespec.h"
// gui-qt
#include "chatline.h"
#include "menu.h"
#include "tradecalculation.h"

class QApplication;
class QGridLayout;
class QObject;
class QPoint;
class QSocketNotifier;
class QStackedLayout;
class QStatusBar;
class QTimerEvent;
class choice_dialog;
struct server_scan;

namespace freeciv {
class tileset_debugger;
}

enum connection_state {
  LOGIN_TYPE,
  NEW_PASSWORD_TYPE,
  ENTER_PASSWORD_TYPE,
  WAITING_TYPE
};

/****************************************************************************
  Some qt-specific options like size to save between restarts
****************************************************************************/
struct fc_settings {
  float chat_fwidth;
  float chat_fheight;
  float chat_fx_pos;
  float chat_fy_pos;
  int player_repo_sort_col;
  bool show_new_turn_text;
  bool show_battle_log;
  bool show_chat;     // Only used when loading
  bool show_messages; // Only used when loading
  Qt::SortOrder player_report_sort;
  int city_repo_sort_col;
  Qt::SortOrder city_report_sort;
  QByteArray help_geometry;
  QByteArray help_splitter1;
  float unit_info_pos_fx;
  float unit_info_pos_fy;
  float minimap_x;
  float minimap_y;
  float minimap_width;
  float minimap_height;
  float battlelog_scale;
  float battlelog_x;
  float battlelog_y;
};

/****************************************************************************
  Corner widget for menu
****************************************************************************/
class fc_corner : public QWidget {
  Q_OBJECT
  QMainWindow *mw;

public:
  fc_corner(QMainWindow *qmw);
public slots:
  void maximize();
  void minimize();
  void close_fc();
};

class fc_client : public QMainWindow {
  Q_OBJECT

  enum client_pages page;
  QGridLayout *pages_layout[PAGE_COUNT];
  QLabel *status_bar_label{nullptr};
  QSocketNotifier *server_notifier{nullptr};
  QStackedLayout *central_layout{nullptr};
  QStatusBar *status_bar{nullptr};
  QString current_file;
  QStringList status_bar_queue;
  bool quitting{false};
  bool send_new_aifill_to_server;
  choice_dialog *opened_dialog{nullptr};

public:
  fc_client();
  ~fc_client() override;
  QWidget *pages[static_cast<int>(PAGE_GAME) + 2];
  void fc_main(QApplication *);
  void add_server_source(QTcpSocket *socket);

  enum client_pages current_page();

  void set_status_bar(const QString &str, int timeout = 2000);
  void set_diplo_dialog(choice_dialog *widget);
  choice_dialog *get_diplo_dialog();
  void write_settings();
  bool is_closing();
  QCursor *fc_cursors[CURSOR_LAST][NUM_CURSOR_FRAMES];
  QWidget *central_wdg{nullptr};
  bool interface_locked{false};
  fc_corner *corner_wid;
  fc_settings qt_settings;
  mr_menu *menu_bar{nullptr};
  qfc_rally_list rallies;
  trade_generator trade_gen;

private slots:
  void server_input();
  void closing();
  void clear_status_bar();

public slots:
  void slot_disconnect();
  void start_tutorial();
  void start_from_file(const QString &file);
  void start_new_game();
  void switch_page(int i);

private:
  void create_loading_page();
  void create_cursors();
  void delete_cursors();
  void read_settings();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

  void timerEvent(QTimerEvent *) override;
  void closeEvent(QCloseEvent *event) override;

signals:
  void keyCaught(QKeyEvent *e);
};

// Return fc_client instance. Implementation in gui_main.cpp
class fc_client *king();
void popup_client_options();
