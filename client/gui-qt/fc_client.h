/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__FC_CLIENT_H
#define FC__FC_CLIENT_H

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
#include "idlecallback.h"
#include "menu.h"
#include "tradecalculation.h"

class QApplication;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QCursor;
class QDialogButtonBox;
class QEvent;
class QGridLayout;
class QItemSelection;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QObject;
class QPoint;
class QPushButton;
class QResizeEvent;
class QSocketNotifier;
class QSpinBox;
class QStackedLayout;
class QStatusBar;
class QTableWidget;
class QTextEdit;
class QTimer;
class QTimerEvent;
class QTreeWidget;
class choice_dialog;
class fc_sidebar;
class fc_sidewidget;
class goto_dialog;
class hud_battle_log;
class hud_units;
class info_tab;
class info_tile;
class map_view;
class messagewdg;
class minimap_view;
class pregame_options;
class pregamevote;
class units_select;
class xvote;
struct server_scan;

enum connection_state {
  LOGIN_TYPE,
  NEW_PASSWORD_TYPE,
  ENTER_PASSWORD_TYPE,
  WAITING_TYPE
};

/****************************************************************************
  Widget holding all game tabs
****************************************************************************/
class fc_game_tab_widget : public QStackedWidget {
  Q_OBJECT
public:
  fc_game_tab_widget();
  void init();

protected:
  void resizeEvent(QResizeEvent *event);
private slots:
  void current_changed(int index);
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
  Qt::SortOrder player_report_sort;
  int city_repo_sort_col;
  Qt::SortOrder city_report_sort;
  QByteArray city_geometry;
  QByteArray city_splitter1;
  QByteArray city_splitter2;
  QByteArray city_splitter3;
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

class fc_client : public QMainWindow, private chat_listener {
  Q_OBJECT

  enum client_pages page;
  QGridLayout *game_layout;
  QGridLayout *pages_layout[PAGE_GAME + 2];
  QLabel *status_bar_label;
  QMap<QString, QWidget *> opened_repo_dlgs;
  QSocketNotifier *server_notifier;
  QStackedLayout *central_layout;
  QStatusBar *status_bar;
  QString current_file;
  QStringList status_bar_queue;
  QTimer *update_info_timer;
  QWidget *game_main_widget;
  bool quitting;
  bool send_new_aifill_to_server;
  choice_dialog *opened_dialog;
  fc_sidewidget *sw_cities;
  fc_sidewidget *sw_economy;
  fc_sidewidget *sw_map;
  fc_sidewidget *sw_tax;
  info_tile *info_tile_wdg;

public:
  fc_client();
  ~fc_client();
  QWidget *pages[(int) PAGE_GAME + 2];
  void fc_main(QApplication *);
  map_view *mapview_wdg;
  fc_sidebar *sidebar_wdg;
  minimap_view *minimapview_wdg;
  void add_server_source(int);
  void remove_server_source();
  bool event(QEvent *event);

  enum client_pages current_page();

  void set_status_bar(QString str, int timeout = 2000);
  int add_game_tab(QWidget *widget);
  void rm_game_tab(int index); /* doesn't delete widget */
  void toggle_unit_sel_widget(struct tile *ptile);
  void update_unit_sel();
  void popdown_unit_sel();
  void popup_tile_info(struct tile *ptile);
  void popdown_tile_info();
  void set_diplo_dialog(choice_dialog *widget);
  choice_dialog *get_diplo_dialog();
  void update_sidebar_position();
  void gimme_place(QWidget *widget, const QString &str);
  int gimme_index_of(const QString &str);
  void remove_repo_dlg(const QString &str);
  bool is_repo_dlg_open(const QString &str);
  void write_settings();
  bool is_closing();
  void update_sidebar_tooltips();
  void reload_sidebar_icons();
  QCursor *fc_cursors[CURSOR_LAST][NUM_CURSOR_FRAMES];
  QWidget *central_wdg;
  bool interface_locked;
  bool map_font_scale;
  fc_corner *corner_wid;
  fc_game_tab_widget *game_tab_widget;
  fc_settings qt_settings;
  fc_sidewidget *sw_cunit;
  fc_sidewidget *sw_diplo;
  fc_sidewidget *sw_endturn;
  fc_sidewidget *sw_indicators;
  fc_sidewidget *sw_science;
  float map_scale;
  goto_dialog *gtd;
  hud_battle_log *battlelog_wdg;
  hud_units *unitinfo_wdg;
  info_tab *infotab;
  messagewdg *msgwdg;
  mr_idle mr_idler;
  mr_menu *menu_bar;
  qfc_rally_list rallies;
  trade_generator trade_gen;
  units_select *unit_sel;
  xvote *x_vote;

private slots:

  void server_input(int sock);
  void closing();
  void clear_status_bar();

public slots:
  void slot_disconnect();
  void start_new_game();
  void switch_page(int i);
  void update_info_label();
  void quit();

private:
  void create_game_page();
  void create_loading_page();
  void create_cursors(void);
  void delete_cursors(void);
  void init();
  void read_settings();

protected:
  void timerEvent(QTimerEvent *);
  void closeEvent(QCloseEvent *event);

signals:
  void keyCaught(QKeyEvent *e);
};

/***************************************************************************
  Class for showing options in PAGE_START, options like ai_fill, ruleset
  etc.
***************************************************************************/

// Return fc_client instance. Implementation in gui_main.cpp
class fc_client *gui();
void popup_client_options();
#endif /* FC__FC_CLIENT_H */
