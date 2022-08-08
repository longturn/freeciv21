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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QMenuBar>
// client
#include "control.h"
#include "menu_g.h"

class QLabel;
class QPushButton;
class QScrollArea;
struct fc_shortcut;

/** used for indicating menu about current option - for renaming
 * and enabling, disabling */
enum munit {
  STANDARD,
  EXPLORE,
  LOAD,
  UNLOAD,
  TRANSPORTER,
  DISBAND,
  RENAME,
  CONVERT,
  MINE,
  PLANT,
  IRRIGATION,
  CULTIVATE,
  TRANSFORM,
  PILLAGE,
  BUILD,
  ROAD,
  FORTIFY,
  FORTRESS,
  AIRBASE,
  POLLUTION,
  FALLOUT,
  SENTRY,
  HOMECITY,
  WAKEUP,
  AUTOSETTLER,
  CONNECT_ROAD,
  CONNECT_RAIL,
  CONNECT_IRRIGATION,
  GOTO_CITY,
  AIRLIFT,
  BUILD_WONDER,
  AUTOTRADEROUTE,
  ORDER_TRADEROUTE,
  ORDER_DIPLOMAT_DLG,
  UPGRADE,
  NOT_4_OBS,
  MULTIPLIERS,
  ENDGAME,
  SAVE
};

enum delay_order { D_GOTO, D_NUKE, D_PARADROP, D_FORT };

/**************************************************************************
  Class holding city list for rally points
**************************************************************************/
class qfc_rally_list {
public:
  qfc_rally_list()
  {
    rally_city = nullptr;
    hover_tile = false;
    hover_city = false;
  }
  bool hover_tile;
  bool hover_city;
  struct city *rally_city;
};

void multiairlift(struct city *acity, Unit_type_id ut);

/**************************************************************************
  Class representing one unit for delayed goto
**************************************************************************/
class qfc_delayed_unit_item {
public:
  qfc_delayed_unit_item(delay_order dg, int i)
  {
    order = dg;
    id = i;
    ptile = nullptr;
  }
  delay_order order;
  int id;
  struct tile *ptile;
};

/****************************************************************************
  Instantiable government menu.
****************************************************************************/
class gov_menu : public QMenu {
  Q_OBJECT
  static QSet<gov_menu *> instances;

  QVector<QAction *> actions;

public:
  gov_menu(QWidget *parent = 0);
  ~gov_menu() override;

  static void create_all();
  static void update_all();

public slots:
  void revolution();
  void change_gov(int target_gov);

  void create();
  void update();
};

/****************************************************************************
  Go to and... menu.
****************************************************************************/
class go_act_menu : public QMenu {
  Q_OBJECT
  static QSet<go_act_menu *> instances;

  QMap<QAction *, int> items;

public:
  go_act_menu(QWidget *parent = 0);
  ~go_act_menu() override;

  static void reset_all();
  static void update_all();

public slots:
  void start_go_act(int act_id, int sub_tgt_id);

  void reset();
  void create();
  void update();
};

/**************************************************************************
  Class representing global menus in gameview
**************************************************************************/
class mr_menu : public QMenuBar {
  Q_OBJECT
  QMenu *airlift_menu = nullptr;
  QMenu *bases_menu = nullptr;
  QMenu *menu = nullptr;
  QMenu *multiplayer_menu = nullptr;
  QMenu *roads_menu = nullptr;
  QMenu *citybar_submenu = nullptr;
  QActionGroup *airlift_type = nullptr;
  QActionGroup *action_vs_city = nullptr;
  QActionGroup *action_vs_unit = nullptr;
  QActionGroup *action_citybar = nullptr;
  QMenu *action_unit_menu = nullptr;
  QMenu *action_city_menu = nullptr;
  QMultiHash<munit, QAction *> menu_list;
  std::vector<qfc_delayed_unit_item> units_list;
  bool initialized = false;

public:
  mr_menu();
  void setup_menus();
  void menus_sensitive();
  void update_airlift_menu();
  void update_roads_menu();
  void update_bases_menu();
  void set_tile_for_order(struct tile *ptile);
  void execute_shortcut(int sid);
  bool shortcut_exists(const fc_shortcut &fcs, QString &where);
  QString shortcut_2_menustring(int sid);
  QAction *minimap_status = nullptr;
  QAction *scale_fonts_status = nullptr;
  QAction *lock_status = nullptr;
  QAction *osd_status = nullptr;
  QAction *btlog_status = nullptr;
  QAction *chat_status = nullptr;
  QAction *messages_status = nullptr;
  bool delayed_order = false;
  bool quick_airlifting = false;
  Unit_type_id airlift_type_id = 0;
private slots:
  // game menu
  void local_options();
  void shortcut_options();
  void server_options();
  void messages_options();
  void save_options_now();
  void save_game();
  void save_game_as();
  void save_image();
  void tileset_custom_load();
  void load_new_tileset();
  void back_to_menu();
  void quit_game();

  // help menu
  void slot_help(const QString &topic);

  /*used by work menu*/
  void slot_build_path(int id);
  void slot_build_base(int id);
  void slot_build_city();
  void slot_auto_settler();
  void slot_build_road();
  void slot_build_irrigation();
  void slot_cultivate();
  void slot_build_mine();
  void slot_plant();
  void slot_conn_road();
  void slot_conn_rail();
  void slot_conn_irrigation();
  void slot_transform();
  void slot_clean_pollution();
  void slot_clean_fallout();

  /*used by unit menu */
  void slot_unit_sentry();
  void slot_unit_explore();
  void slot_unit_goto();
  void slot_airlift();
  void slot_patrol();
  void slot_unsentry();
  void slot_load();
  void slot_unload();
  void slot_unload_all();
  void slot_set_home();
  void slot_upgrade();
  void slot_convert();
  void slot_disband();
  void slot_rename();

  /*used by combat menu*/
  void slot_unit_fortify();
  void slot_unit_fortress();
  void slot_unit_airbase();
  void slot_pillage();
  void slot_action();

  /*used by view menu*/
  void slot_set_citybar();
  void slot_center_view();
  void slot_show_new_turn_text();
  void slot_battlelog();
  void slot_fullscreen();
  void slot_lock();
  void slot_city_outlines();
  void slot_city_output();
  void slot_map_grid();
  void slot_borders();
  void slot_native_tiles();
  void slot_city_growth();
  void slot_city_production();
  void slot_city_buycost();
  void slot_city_traderoutes();
  void slot_city_names();
  void zoom_scale_fonts();

  /*used by select menu */
  void slot_select_one();
  void slot_select_all_tile();
  void slot_select_same_tile();
  void slot_select_same_continent();
  void slot_select_same_everywhere();
  void slot_done_moving();
  void slot_wait();
  void slot_unit_filter();

  // used by multiplayer menu
  void slot_orders_clear();
  void slot_execute_orders();
  void slot_delayed_goto();
  void slot_trade_add_all();
  void slot_trade_city();
  void slot_calculate();
  void slot_clear_trade();
  void slot_autocaravan();
  void slot_rally();
  void slot_quickairlift_set();
  void slot_quickairlift();
  void slot_action_vs_unit();
  void slot_action_vs_city();

  /*used by civilization menu */
  void slot_show_map();
  void slot_popup_tax_rates();
  void slot_popup_mult_rates();
  void slot_show_research_tab();
  void slot_spaceship();
  void slot_demographics();
  void slot_achievements();
  void slot_endgame();
  void slot_top_five();
  void slot_traveler();

private:
  void clear_menus();
  void nonunit_sensitivity();
  struct tile *find_last_unit_pos(struct unit *punit, int pos);
};
