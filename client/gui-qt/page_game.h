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
#pragma once

#include <QMap>
#include <QStackedWidget>
#include <QWidget>

class city_dialog;
class fc_client;
class map_view;
class sidebar;
class minimap_view;
class sidebarWidget;
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

class pageGame : public QWidget {
  Q_OBJECT
public:
  pageGame(QWidget *);
  ~pageGame();
  void updateSidebarPosition();
  void reloadSidebarIcons();
  void updateSidebarTooltips();
  int addGameTab(QWidget *widget);
  void rmGameTab(int index); /* doesn't delete widget */
  void gimmePlace(QWidget *widget, const QString &str);
  int gimmeIndexOf(const QString &str);
  void removeRepoDlg(const QString &str);
  bool isRepoDlgOpen(const QString &str);
  void updateInfoLabel();
  QWidget *game_main_widget;
  fc_game_tab_widget *game_tab_widget;
  sidebar *sidebar_wdg;
  goto_dialog *gtd;
  units_select *unit_selector;
  hud_battle_log *battlelog_wdg;
  hud_units *unitinfo_wdg;
  info_tab *infotab;
  map_view *mapview_wdg;
  minimap_view *minimapview_wdg;
  city_dialog *city_overlay;
  xvote *x_vote;
  sidebarWidget *sw_diplo;
  sidebarWidget *sw_indicators;
  sidebarWidget *sw_endturn;
  sidebarWidget *sw_science;
public slots:
private slots:
  void updateInfoLabelTimeout();

private:
  QMap<QString, QWidget *> opened_repo_dlgs;
  QTimer *update_info_timer;
  sidebarWidget *sw_cities;
  sidebarWidget *sw_cunit;
  sidebarWidget *sw_economy;
  sidebarWidget *sw_map;
  sidebarWidget *sw_tax;
};

/**********************************************************************/ /**
   Return game instandce
 **************************************************************************/
pageGame *queen();
