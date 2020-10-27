/******  ,   ,  **********************************************************
         \\  \\             Copyright (c) 1996-2020 Freeciv21 and Freeciv
         ) \\ \\    I--      contributors. This file is part of Freeciv21.
         )  )) ))  / * \ Freeciv21 is free software: you can redistribute
         \  || || / /^="   in and/or modify it under the terms of the GNU
 ,__     _\ \\ --/ /     General Public License  as published by the Free
<  \\___/         '  Software Foundation, either version 3 of the License,
    '===\    ___, )                 or (at your option) any later version.
         \  )___/\\                You should have received a copy of the
         / /      '"      GNU General Public License along with Freeciv21.
         \ \                    If not, see https://www.gnu.org/licenses/.
******    '"   **********************************************************/

#ifndef FC__PAGE_GAME_H
#define FC__PAGE_GAME_H

#include <QStackedWidget>
#include <QWidget>
#include <QMap>

class fc_client;
class map_view;
class fc_sidebar;
class minimap_view;
class fc_sidewidget;
class hud_units;
class info_tab;
class hud_battle_log;
class goto_dialog;
class unitinfo_wdg;
class units_select;
class xvote;

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

class page_game : public QWidget {
  Q_OBJECT
public:
  page_game(QWidget *);
  ~page_game();
  void update_sidebar_position();
  void reload_sidebar_icons();
  void update_sidebar_tooltips();
  int add_game_tab(QWidget *widget);
  void rm_game_tab(int index); /* doesn't delete widget */
  void gimme_place(QWidget *widget, const QString &str);
  int gimme_index_of(const QString &str);
  void remove_repo_dlg(const QString &str);
  bool is_repo_dlg_open(const QString &str);

  QWidget *game_main_widget;
  fc_game_tab_widget *game_tab_widget;
  fc_sidebar *sidebar_wdg;
  goto_dialog *gtd;
  units_select *unit_selector;
  hud_battle_log *battlelog_wdg;
  hud_units *unitinfo_wdg;
  info_tab *infotab;
  map_view *mapview_wdg;
  minimap_view *minimapview_wdg;
  xvote *x_vote;
  fc_sidewidget *sw_diplo;
  fc_sidewidget *sw_indicators;
  fc_sidewidget *sw_endturn;
  fc_sidewidget *sw_science;
public slots:
  void update_info_label();
private slots:
private:
  QMap<QString, QWidget *> opened_repo_dlgs;
  QTimer *update_info_timer;
  fc_sidewidget *sw_cities;
  fc_sidewidget *sw_cunit;
  fc_sidewidget *sw_economy;
  fc_sidewidget *sw_map;
  fc_sidewidget *sw_tax;
};

/**********************************************************************/ /**
   Return game instandce
 **************************************************************************/
page_game *queen();

#endif /* FC__PAGE_GAME_H */