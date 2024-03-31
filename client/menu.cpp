/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "menu.h"

// Qt
#include <QActionGroup>
#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>
#include <QVBoxLayout>

// utility
#include "fcintl.h"
// common
#include "fc_types.h"
#include "game.h"
#include "goto.h"
#include "government.h"
#include "helpdata.h"
#include "map.h"
#include "multipliers.h"
#include "road.h"
#include "tileset_options.h"
#include "unit.h"
// client
#include "audio/audio.h"
#include "citybar.h"
#include "cityrep_g.h"
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "control.h"
#include "dialogs.h"
#include "fc_client.h"
#include "gotodlg.h"
#include "gui_main.h"
#include "helpdlg.h"
#include "hudwidget.h"
#include "mapctrl_g.h"
#include "messageoptions.h"
#include "messagewin.h"
#include "minimap.h"
#include "minimap_panel.h"
#include "page_game.h"
#include "page_pregame.h"
#include "qtg_cxxside.h"
#include "ratesdlg.h"
#include "ratesdlg_g.h"
#include "renderer.h"
#include "repodlgs_g.h"
#include "shortcuts.h"
#include "spaceshipdlg.h"
#include "tileset/sprite.h"
#include "tileset/tilespec.h"
#include "top_bar.h"
#include "unithudselector.h"
#include "views/view_map.h"
#include "views/view_map_common.h"
#include "views/view_nations.h"
#include "views/view_units.h"

extern void popup_endgame_report();
static void enable_interface(bool enable);

void option_dialog_popup(const char *name, const struct option_set *poptset);

/**
   Initialize menus (sensitivity, name, etc.) based on the
   current state and current ruleset, etc.  Call menus_update().
 */
void real_menus_init(void)
{
  if (!game.client.ruleset_ready) {
    return;
  }

  king()->menu_bar->setup_menus();

  gov_menu::create_all();

  // A new ruleset may have been loaded.
  go_act_menu::reset_all();
}

/**
   Update all of the menus (sensitivity, name, etc.) based on the
   current state.
 */
void real_menus_update(void)
{
  if (C_S_RUNNING <= client_state()) {
    king()->menuBar()->setVisible(true);
    if (!is_waiting_turn_change()) {
      king()->menu_bar->menus_sensitive();
      king()->menu_bar->update_airlift_menu();
      king()->menu_bar->update_roads_menu();
      king()->menu_bar->update_bases_menu();
      gov_menu::update_all();
      go_act_menu::update_all();
      queen()->unitinfo_wdg->update_actions();
    }
  } else {
    king()->menuBar()->setVisible(false);
  }
}

/**
   Return the text for the tile, changed by the activity.
   Should only be called for irrigation, mining, or transformation, and
   only when the activity changes the base terrain type.
 */
static const char *get_tile_change_menu_text(struct tile *ptile,
                                             enum unit_activity activity)
{
  struct tile *newtile = tile_virtual_new(ptile);
  const char *text;

  tile_apply_activity(newtile, activity, nullptr);
  text = tile_get_info_text(newtile, false, 0);
  tile_virtual_destroy(newtile);

  return text;
}

/**
   Creates a new government menu.
 */
gov_menu::gov_menu(QWidget *parent) : QMenu(_("Government"), parent)
{
  // Register ourselves to get updates for free.
  instances << this;
  setAttribute(Qt::WA_TranslucentBackground);
}

/**
   Destructor.
 */
gov_menu::~gov_menu()
{
  qDeleteAll(actions);
  instances.remove(this);
}

/**
   Creates the menu once the government list is known.
 */
void gov_menu::create()
{
  QAction *action;
  struct government *gov, *revol_gov;
  int gov_count, i;

  // Clear any content
  for (auto *action : qAsConst(actions)) {
    removeAction(action);
    action->deleteLater();
  }
  actions.clear();

  gov_count = government_count();
  actions.reserve(gov_count + 1);
  action = addAction(_("Revolution..."));
  connect(action, &QAction::triggered, this, &gov_menu::revolution);
  actions.append(action);

  addSeparator();

  // Add an action for each government. There is no icon yet.
  revol_gov = game.government_during_revolution;
  for (i = 0; i < gov_count; ++i) {
    gov = government_by_number(i);
    if (gov != revol_gov) { // Skip revolution goverment
      // Defeat keyboard shortcut mnemonics
      action = addAction(QString(government_name_translation(gov)));
      // We need to keep track of the gov <-> action mapping to be able to
      // set enabled/disabled depending on available govs.
      actions.append(action);
      QObject::connect(action, &QAction::triggered,
                       [this, i]() { change_gov(i); });
    }
  }
}

/**
   Updates the menu to take gov availability into account.
 */
void gov_menu::update()
{
  struct government *gov, *revol_gov;

  revol_gov = game.government_during_revolution;
  for (int i = 0, j = 0; i < actions.count(); ++i) {
    gov = government_by_number(i);
    if (gov != revol_gov) { // Skip revolution goverment
      auto sprite = get_government_sprite(tileset, gov);
      if (sprite != nullptr) {
        actions[j + 1]->setIcon(QIcon(*sprite));
      }
      actions[j + 1]->setEnabled(
          can_change_to_government(client.conn.playing, gov));
      ++j;
    } else {
      actions[0]->setEnabled(!client_is_observer());
    }
  }
}

/**
   Shows the dialog asking for confirmation before starting a revolution.
 */
void gov_menu::revolution()
{
  popup_revolution_dialog(game.government_during_revolution);
}

/**
   Shows the dialog asking for confirmation before starting a revolution.
 */
void gov_menu::change_gov(int target_gov)
{
  popup_revolution_dialog(government_by_number(target_gov));
}

/**
  Keeps track of all gov_menu instances.
 */
QSet<gov_menu *> gov_menu::instances = QSet<gov_menu *>();

/**
   Updates all gov_menu instances.
 */
void gov_menu::create_all()
{
  for (gov_menu *m : qAsConst(instances)) {
    m->create();
  }
}

/**
   Updates all gov_menu instances.
 */
void gov_menu::update_all()
{
  for (gov_menu *m : qAsConst(instances)) {
    m->update();
  }
}

/**
   Instantiate a new goto and act sub menu.
 */
go_act_menu::go_act_menu(QWidget *parent) : QMenu(_("Goto And..."), parent)
{
  // Will need auto updates etc.
  instances << this;
}

/**
   Destructor.
 */
go_act_menu::~go_act_menu()
{
  // Updates are no longer needed.
  instances.remove(this);
}

/**
   Empty a menu of all its items and sub menues.
 */
static void reset_menu_and_sub_menues(QMenu *menu)
{
  QList<QAction *> actions = menu->actions();
  // Delete each existing menu item.
  for (auto *action : qAsConst(actions)) {
    if (action->menu() != nullptr) {
      // Delete the sub menu
      reset_menu_and_sub_menues(action->menu());
      action->menu()->deleteLater();
    }

    menu->removeAction(action);
    action->deleteLater();
  }
}

/**
   Reset the goto and act menu so it will be recreated.
 */
void go_act_menu::reset()
{
  // Clear menu item to action ID mapping.
  items.clear();

  // Remove the menu items
  reset_menu_and_sub_menues(this);
}

/**
   Fill the new goto and act sub menu with menu items.
 */
void go_act_menu::create()
{
  QAction *item;
  int tgt_kind_group;

  // Group goto and perform action menu items by target kind.
  for (tgt_kind_group = 0; tgt_kind_group < ATK_COUNT; tgt_kind_group++) {
    action_iterate(act_id)
    {
      struct action *paction = action_by_number(act_id);
      QString action_name = (QString(action_name_translation(paction)));

      if (action_id_get_actor_kind(act_id) != AAK_UNIT) {
        // This action isn't performed by a unit.
        continue;
      }

      if (action_id_get_target_kind(act_id) != tgt_kind_group) {
        // Wrong group.
        continue;
      }

      if (action_id_has_complex_target(act_id)) {
        QMenu *sub_target_menu = addMenu(action_name);
        items.insert(sub_target_menu->menuAction(), act_id);

#define CREATE_SUB_ITEM(_menu_, _act_id_, _sub_tgt_id_, _sub_tgt_name_)     \
  {                                                                         \
    QAction *_sub_item_ = _menu_->addAction(_sub_tgt_name_);                \
    int _sub_target_id_ = _sub_tgt_id_;                                     \
    QObject::connect(_sub_item_, &QAction::triggered,                       \
                     [this, _act_id_, _sub_target_id_]() {                  \
                       start_go_act(_act_id_, _sub_target_id_);             \
                     });                                                    \
  }

        switch (action_get_sub_target_kind(paction)) {
        case ASTK_BUILDING:
          improvement_iterate(pimpr)
          {
            CREATE_SUB_ITEM(sub_target_menu, act_id,
                            improvement_number(pimpr),
                            improvement_name_translation(pimpr));
          }
          improvement_iterate_end;
          break;
        case ASTK_TECH:
          advance_iterate(A_FIRST, ptech)
          {
            CREATE_SUB_ITEM(sub_target_menu, act_id, advance_number(ptech),
                            advance_name_translation(ptech));
          }
          advance_iterate_end;
          break;
        case ASTK_EXTRA:
        case ASTK_EXTRA_NOT_THERE:
          extra_type_iterate(pextra)
          {
            if (!(action_creates_extra(paction, pextra)
                  || action_removes_extra(paction, pextra))) {
              // Not relevant
              continue;
            }

            CREATE_SUB_ITEM(sub_target_menu, act_id, extra_number(pextra),
                            extra_name_translation(pextra));
          }
          extra_type_iterate_end;
          break;
        case ASTK_NONE:
          // Should not be here.
          fc_assert(action_get_sub_target_kind(paction) != ASTK_NONE);
          break;
        case ASTK_COUNT:
          // Should not exits
          fc_assert(action_get_sub_target_kind(paction) != ASTK_COUNT);
          break;
        }
        continue;
      }

#define ADD_OLD_SHORTCUT(wanted_action_id, sc_id)                           \
  if (act_id == wanted_action_id) {                                         \
    fc_shortcuts::sc()->link_action(sc_id, item);                           \
  }

      /* Create and add the menu item. It will be hidden or shown based on
       * unit type.  */
      item = addAction(action_name);
      items.insert(item, act_id);

      /* Add the keyboard shortcuts for "Go to and..." menu items that
       * existed independently before the "Go to and..." menu arrived. */
      ADD_OLD_SHORTCUT(ACTION_FOUND_CITY, SC_GOBUILDCITY);
      ADD_OLD_SHORTCUT(ACTION_JOIN_CITY, SC_GOJOINCITY);
      ADD_OLD_SHORTCUT(ACTION_NUKE, SC_NUKE);

      QObject::connect(item, &QAction::triggered, [this, act_id]() {
        start_go_act(act_id, NO_TARGET);
      });
    }
    action_iterate_end;
  }
}

/**
   Update the goto and act menu based on the selected unit(s)
 */
void go_act_menu::update()
{
  bool can_do_something = false;

  if (!actions_are_ready()) {
    // Nothing to do.
    return;
  }

  if (items.isEmpty()) {
    // The goto and act menu needs menu items.
    create();
  }

  /* Enable a menu item if it is theoretically possible that one of the
   * selected units can perform it. Checking if the action can be performed
   * at the current tile is pointless since it should be performed at the
   * target tile. */
  QList<QAction *> keys = items.keys();
  for (QAction *item : qAsConst(keys)) {
    if (units_can_do_action(get_units_in_focus(), items.value(item), true)) {
      item->setVisible(true);
      can_do_something = true;
    } else {
      item->setVisible(false);
    }
  }

  if (can_do_something) {
    // At least one menu item is enabled for one of the selected units.
    setEnabled(true);
  } else {
    // No menu item is enabled any of the selected units.
    setEnabled(false);
  }
}

/**
   Activate the goto system
 */
void go_act_menu::start_go_act(int act_id, int sub_tgt_id)
{
  request_unit_goto(ORDER_PERFORM_ACTION, act_id, sub_tgt_id);
}

/**
  Store all goto and act menu items so they can be updated etc
 */
QSet<go_act_menu *> go_act_menu::instances;

/**
   Reset all goto and act menu instances.
 */
void go_act_menu::reset_all()
{
  for (go_act_menu *m : qAsConst(instances)) {
    m->reset();
  }
}

/**
   Update all goto and act menu instances
 */
void go_act_menu::update_all()
{
  for (go_act_menu *m : qAsConst(instances)) {
    m->update();
  }
}

/**
   Predicts last unit position
 */
struct tile *mr_menu::find_last_unit_pos(unit *punit, int pos)
{
  struct tile *ptile = nullptr;
  struct unit *zunit;
  struct unit *qunit;

  int i = 0;
  qunit = punit;
  for (const auto &fui : units_list) {
    zunit = unit_list_find(client_player()->units, fui.id);
    i++;
    if (i >= pos) {
      punit = qunit;
      return ptile;
    }
    if (zunit == nullptr) {
      continue;
    }

    if (punit == zunit) { // Unit found
      /* Unit was ordered to attack city so it might stay in
         front of that city */
      if (is_non_allied_city_tile(fui.ptile, unit_owner(punit))) {
        ptile = tile_before_end_path(punit, fui.ptile);
        if (ptile == nullptr) {
          ptile = fui.ptile;
        }
      } else {
        ptile = fui.ptile;
      }
      // unit found in tranporter
    } else if (unit_contained_in(punit, zunit)) {
      ptile = fui.ptile;
    }
  }
  return nullptr;
}

/**
   Constructor for global menubar in gameview
 */
mr_menu::mr_menu() : QMenuBar() {}

/**
   Initializes menu system, and add custom enum(munit) for most of options
   Notice that if you set option for QAction->setChecked(option) it will
   check/uncheck automatically without any intervention
 */
void mr_menu::setup_menus()
{
  clear_menus();

  QAction *act;
  QList<QMenu *> menus;
  int i;

  delayed_order = false;
  airlift_type_id = 0;
  quick_airlifting = false;

  auto shortcuts = fc_shortcuts::sc();

  // Game Menu
  menu = this->addMenu(_("Game"));
  act = menu->addAction(_("Save Game"));
  act->setShortcut(QKeySequence(tr("Ctrl+s")));
  act->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  menu_list.insert(SAVE, act);
  connect(act, &QAction::triggered, this, &mr_menu::save_game);
  act = menu->addAction(_("Save Game As..."));
  menu_list.insert(SAVE, act);
  act->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  connect(act, &QAction::triggered, this, &mr_menu::save_game_as);
  act = menu->addAction(_("Save Map to Image"));
  connect(act, &QAction::triggered, this, &mr_menu::save_image);
  menu->addSeparator();

  act = menu->addAction(_("Interface Options"));
  connect(act, &QAction::triggered, this, &mr_menu::local_options);
  act = menu->addAction(_("Game Options"));
  connect(act, &QAction::triggered, this, &mr_menu::server_options);
  act = menu->addAction(_("Message Options"));
  connect(act, &QAction::triggered, this, &mr_menu::messages_options);
  act = menu->addAction(_("Shortcut Options"));
  connect(act, &QAction::triggered, this, &mr_menu::shortcut_options);
  act = menu->addAction(_("Load Another Tileset"));
  connect(act, &QAction::triggered, this, &mr_menu::tileset_custom_load);
  act = menu->addAction(_("Add Modpacks"));
  connect(act, &QAction::triggered, this, &mr_menu::add_modpacks);
  tileset_options = menu->addAction(_("Tileset Options"));
  connect(tileset_options, &QAction::triggered, this,
          &mr_menu::show_tileset_options);
  tileset_options->setEnabled(tileset_has_options(tileset));
  act = menu->addAction(_("Tileset Debugger"));
  connect(act, &QAction::triggered, queen()->mapview_wdg,
          &map_view::show_debugger);
  act = menu->addAction(_("Save Options Now"));
  act->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  connect(act, &QAction::triggered, this, &mr_menu::save_options_now);
  act = menu->addAction(_("Save Options on Exit"));
  act->setCheckable(true);
  act->setChecked(gui_options->save_options_on_exit);
  menu->addSeparator();

  act = menu->addAction(_("Leave Game"));
  act->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
  connect(act, &QAction::triggered, this, &mr_menu::back_to_menu);
  act = menu->addAction(_("Quit"));
  act->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  connect(act, &QAction::triggered, this, &mr_menu::quit_game);

  // View Menu
  menu = this->addMenu(Q_("?verb:View"));
  act = menu->addAction(_("Center View"));
  shortcuts->link_action(SC_CENTER_VIEW, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_center_view);
  menu->addSeparator();
  act = menu->addAction(_("Fullscreen"));
  shortcuts->link_action(SC_FULLSCREEN, act);
  act->setCheckable(true);
  act->setChecked(gui_options->gui_qt_fullscreen);
  connect(act, &QAction::triggered, this, &mr_menu::slot_fullscreen);
  menu->addSeparator();
  minimap_status = menu->addAction(_("Minimap"));
  minimap_status->setCheckable(true);
  shortcuts->link_action(SC_MINIMAP, minimap_status);
  minimap_status->setChecked(true);
  connect(minimap_status, &QAction::triggered, queen()->minimap_panel,
          &minimap_panel::set_minimap_visible);
  osd_status = menu->addAction(_("Show New Turn Information"));
  osd_status->setCheckable(true);
  osd_status->setChecked(king()->qt_settings.show_new_turn_text);
  connect(osd_status, &QAction::triggered, this,
          &mr_menu::slot_show_new_turn_text);
  btlog_status = menu->addAction(_("Show Combat Detailed Information"));
  btlog_status->setCheckable(true);
  btlog_status->setChecked(king()->qt_settings.show_battle_log);
  connect(btlog_status, &QAction::triggered, this, &mr_menu::slot_battlelog);
  lock_status = menu->addAction(_("Lock Interface"));
  lock_status->setCheckable(true);
  shortcuts->link_action(SC_IFACE_LOCK, lock_status);
  lock_status->setChecked(false);
  connect(lock_status, &QAction::triggered, this, &mr_menu::slot_lock);
  menu->addSeparator();
  act = menu->addAction(_("Zoom In"));
  shortcuts->link_action(SC_ZOOM_IN, act);
  connect(act, &QAction::triggered, queen()->mapview_wdg,
          &map_view::zoom_in);
  act = menu->addAction(_("Zoom Default"));
  shortcuts->link_action(SC_ZOOM_RESET, act);
  connect(act, &QAction::triggered, queen()->mapview_wdg,
          &map_view::zoom_reset);
  act = menu->addAction(_("Zoom Out"));
  shortcuts->link_action(SC_ZOOM_OUT, act);
  connect(act, &QAction::triggered, queen()->mapview_wdg,
          &map_view::zoom_out);
  scale_fonts_status = menu->addAction(_("Scale Fonts"));
  connect(scale_fonts_status, &QAction::triggered, this,
          &mr_menu::zoom_scale_fonts);
  scale_fonts_status->setCheckable(true);
  scale_fonts_status->setChecked(true);
  menu->addSeparator();
  action_citybar = new QActionGroup(this);
  citybar_submenu = menu->addMenu(_("City Bar Style"));

  for (auto a : *citybar_painter::available_vector(nullptr)) {
    act = citybar_submenu->addAction(a);
    act->setCheckable(true);
    act->setData(a);
    action_citybar->addAction(act);
    if (a == QString(gui_options->default_city_bar_style_name)) {
      act->setChecked(true);
    }
    connect(act, &QAction::triggered, this, &mr_menu::slot_set_citybar);
  }

  act = menu->addAction(_("City Outlines"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_outlines);
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_outlines);
  act = menu->addAction(_("City Output"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_output);
  shortcuts->link_action(SC_CITY_OUTPUT, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_output);
  act = menu->addAction(_("Map Grid"));
  shortcuts->link_action(SC_MAP_GRID, act);
  act->setCheckable(true);
  act->setChecked(gui_options->draw_map_grid);
  connect(act, &QAction::triggered, this, &mr_menu::slot_map_grid);
  act = menu->addAction(_("National Borders"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_borders);
  shortcuts->link_action(SC_NAT_BORDERS, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_borders);
  act = menu->addAction(_("Units"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_units);
  connect(act, &QAction::toggled, this, [](bool checked) {
    gui_options->draw_units = checked;
    update_map_canvas_visible();
  });
  act = menu->addAction(_("Native Tiles"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_native);
  act->setShortcut(QKeySequence(tr("ctrl+shift+n")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_native_tiles);
  act = menu->addAction(_("City Names"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_names);
  shortcuts->link_action(SC_CITY_NAMES, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_names);
  act = menu->addAction(_("City Growth"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_growth);
  act->setShortcut(QKeySequence(tr("ctrl+o")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_growth);
  act = menu->addAction(_("City Production Levels"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_productions);
  shortcuts->link_action(SC_CITY_PROD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_production);
  act = menu->addAction(_("City Buy Cost"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_buycost);
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_buycost);
  act = menu->addAction(_("City Traderoutes"));
  act->setCheckable(true);
  act->setChecked(gui_options->draw_city_trade_routes);
  shortcuts->link_action(SC_TRADE_ROUTES, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_city_traderoutes);

  // Select Menu
  menu = this->addMenu(_("Select"));
  act = menu->addAction(_("Single Unit (Unselect Others)"));
  act->setShortcut(QKeySequence(tr("shift+z")));
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_select_one);
  act = menu->addAction(_("All on Tile"));
  act->setShortcut(QKeySequence(tr("v")));
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_select_all_tile);
  menu->addSeparator();
  act = menu->addAction(_("Same Type on Tile"));
  act->setShortcut(QKeySequence(tr("shift+v")));
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_select_same_tile);
  act = menu->addAction(_("Same Type on Continent"));
  act->setShortcut(QKeySequence(tr("shift+c")));
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this,
          &mr_menu::slot_select_same_continent);
  act = menu->addAction(_("Same Type Everywhere"));
  act->setShortcut(QKeySequence(tr("shift+x")));
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this,
          &mr_menu::slot_select_same_everywhere);
  menu->addSeparator();
  act = menu->addAction(_("Wait"));
  shortcuts->link_action(SC_WAIT, act);
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_wait);
  act = menu->addAction(_("Done"));
  shortcuts->link_action(SC_DONE_MOVING, act);
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_done_moving);

  act = menu->addAction(_("Advanced Unit Selection"));
  act->setShortcut(QKeySequence(tr("ctrl+e")));
  menu_list.insert(NOT_4_OBS, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_filter);

  // Unit Menu
  menu = this->addMenu(_("Unit"));
  act = menu->addAction(_("Go to Tile"));
  shortcuts->link_action(SC_GOTO, act);
  menu_list.insert(STANDARD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_goto);

  // The goto and act sub menu is handled as a separate object.
  menu->addMenu(new go_act_menu());

  act = menu->addAction(_("Go to Nearest City"));
  act->setShortcut(QKeySequence(tr("shift+g")));
  menu_list.insert(GOTO_CITY, act);
  connect(act, &QAction::triggered, this, &request_units_return);
  act = menu->addAction(_("Go to / Airlift to City..."));
  shortcuts->link_action(SC_GOTOAIRLIFT, act);
  menu_list.insert(AIRLIFT, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_airlift);
  menu->addSeparator();
  act = menu->addAction(_("Auto Explore"));
  menu_list.insert(EXPLORE, act);
  shortcuts->link_action(SC_AUTOEXPLORE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_explore);
  act = menu->addAction(_("Patrol"));
  menu_list.insert(STANDARD, act);
  act->setEnabled(false);
  shortcuts->link_action(SC_PATROL, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_patrol);
  menu->addSeparator();
  act = menu->addAction(_("Sentry"));
  shortcuts->link_action(SC_SENTRY, act);
  menu_list.insert(SENTRY, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_sentry);
  act = menu->addAction(_("Unsentry All on Tile"));
  shortcuts->link_action(SC_UNSENTRY_TILE, act);
  menu_list.insert(WAKEUP, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unsentry);
  menu->addSeparator();
  act = menu->addAction(_("Load"));
  shortcuts->link_action(SC_LOAD, act);
  menu_list.insert(LOAD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_load);
  act = menu->addAction(_("Unload"));
  shortcuts->link_action(SC_UNLOAD, act);
  menu_list.insert(UNLOAD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unload);
  act = menu->addAction(_("Unload All From Transporter"));
  act->setShortcut(QKeySequence(tr("shift+u")));
  menu_list.insert(TRANSPORTER, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unload_all);
  menu->addSeparator();
  // Defeat keyboard shortcut mnemonics
  act =
      menu->addAction(QString(action_id_name_translation(ACTION_HOME_CITY)));
  menu_list.insert(HOMECITY, act);
  shortcuts->link_action(SC_SETHOME, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_set_home);
  act = menu->addAction(_("Upgrade Unit"));
  shortcuts->link_action(SC_UPGRADE_UNIT, act);
  menu_list.insert(UPGRADE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_upgrade);
  act = menu->addAction(_("Convert Unit"));
  act->setShortcut(QKeySequence(tr("ctrl+o")));
  menu_list.insert(CONVERT, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_convert);
  act = menu->addAction(_("Disband Unit"));
  act->setShortcut(QKeySequence(tr("shift+d")));
  menu_list.insert(DISBAND, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_disband);

  menu->addSeparator();
  act = menu->addAction(_("Rename Unit"));
  menu_list.insert(RENAME, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_rename);

  // Combat Menu
  menu = this->addMenu(_("Combat"));
  act = menu->addAction(_("Fortify Unit"));
  menu_list.insert(FORTIFY, act);
  shortcuts->link_action(SC_FORTIFY, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_fortify);
  act = menu->addAction(QString(Q_(terrain_control.gui_type_base0)));
  menu_list.insert(FORTRESS, act);
  act->setShortcut(QKeySequence(tr("shift+f")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_fortress);
  act = menu->addAction(QString(Q_(terrain_control.gui_type_base1)));
  menu_list.insert(AIRBASE, act);
  act->setShortcut(QKeySequence(tr("shift+e")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_unit_airbase);
  bases_menu = menu->addMenu(_("Build Base"));
  menu->addSeparator();
  act = menu->addAction(_("Pillage"));
  menu_list.insert(PILLAGE, act);
  shortcuts->link_action(SC_PILLAGE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_pillage);
  // TRANS: Menu item to bring up the action selection dialog.
  act = menu->addAction(_("Do..."));
  menu_list.insert(ORDER_DIPLOMAT_DLG, act);
  shortcuts->link_action(SC_DO, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action);

  // Work Menu
  menu = this->addMenu(_("Work"));
  act = menu->addAction(
      QString(action_id_name_translation(ACTION_FOUND_CITY)));
  shortcuts->link_action(SC_BUILDCITY, act);
  menu_list.insert(BUILD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_build_city);
  act = menu->addAction(_("Auto Settler"));
  shortcuts->link_action(SC_AUTOMATE, act);
  menu_list.insert(AUTOSETTLER, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_auto_settler);
  menu->addSeparator();
  act = menu->addAction(_("Build Road"));
  menu_list.insert(ROAD, act);
  shortcuts->link_action(SC_BUILDROAD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_build_road);
  roads_menu = menu->addMenu(_("Build Path"));
  act = menu->addAction(_("Build Irrigation"));
  shortcuts->link_action(SC_BUILDIRRIGATION, act);
  menu_list.insert(IRRIGATION, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_build_irrigation);
  act = menu->addAction(_("Cultivate"));
  shortcuts->link_action(SC_CULTIVATE, act);
  menu_list.insert(CULTIVATE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_cultivate);
  act = menu->addAction(_("Build Mine"));
  shortcuts->link_action(SC_BUILDMINE, act);
  menu_list.insert(MINE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_build_mine);
  act = menu->addAction(_("Plant"));
  shortcuts->link_action(SC_PLANT, act);
  menu_list.insert(PLANT, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_plant);
  menu->addSeparator();
  act = menu->addAction(_("Connect with Road"));
  act->setShortcut(QKeySequence(tr("ctrl+r")));
  menu_list.insert(CONNECT_ROAD, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_conn_road);
  act = menu->addAction(_("Connect with Railroad"));
  menu_list.insert(CONNECT_RAIL, act);
  act->setShortcut(QKeySequence(tr("ctrl+l")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_conn_rail);
  act = menu->addAction(_("Connect with Irrigation"));
  menu_list.insert(CONNECT_IRRIGATION, act);
  act->setShortcut(QKeySequence(tr("ctrl+i")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_conn_irrigation);
  menu->addSeparator();
  act = menu->addAction(_("Transform Terrain"));
  menu_list.insert(TRANSFORM, act);
  shortcuts->link_action(SC_TRANSFORM, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_transform);
  act = menu->addAction(_("Clean Pollution"));
  menu_list.insert(POLLUTION, act);
  shortcuts->link_action(SC_PARADROP, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_clean_pollution);
  act = menu->addAction(_("Clean Nuclear Fallout"));
  menu_list.insert(FALLOUT, act);
  act->setShortcut(QKeySequence(tr("n")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_clean_fallout);
  act = menu->addAction(
      QString(action_id_name_translation(ACTION_HELP_WONDER)));
  act->setShortcut(QKeySequence(tr("b")));
  menu_list.insert(BUILD_WONDER, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_build_city);
  act = menu->addAction(
      QString(action_id_name_translation(ACTION_TRADE_ROUTE)));
  act->setShortcut(QKeySequence(tr("r")));
  menu_list.insert(ORDER_TRADEROUTE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_build_road);

  multiplayer_menu = this->addMenu(_("Multiplayer"));
  act = multiplayer_menu->addAction(_("Delayed Go To"));
  act->setShortcut(QKeySequence(tr("z")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_delayed_goto);
  act = multiplayer_menu->addAction(_("Delayed Orders Execute"));
  act->setShortcut(QKeySequence(tr("ctrl+z")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_execute_orders);
  act = multiplayer_menu->addAction(_("Clear Orders"));
  act->setShortcut(QKeySequence(tr("ctrl+shift+c")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_orders_clear);
  multiplayer_menu->addSeparator();
  act = multiplayer_menu->addAction(_("Add All Cities to Trade Planning"));
  connect(act, &QAction::triggered, this, &mr_menu::slot_trade_add_all);
  act = multiplayer_menu->addAction(_("Calculate Trade Planning"));
  connect(act, &QAction::triggered, this, &mr_menu::slot_calculate);
  act = multiplayer_menu->addAction(_("Add / Remove City"));
  act->setShortcut(QKeySequence(tr("ctrl+t")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_trade_city);
  act = multiplayer_menu->addAction(_("Clear Trade Planning"));
  connect(act, &QAction::triggered, this, &mr_menu::slot_clear_trade);
  act = multiplayer_menu->addAction(_("Automatic Caravan"));
  menu_list.insert(AUTOTRADEROUTE, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_autocaravan);
  act->setShortcut(QKeySequence(tr("ctrl+j")));
  multiplayer_menu->addSeparator();
  act = multiplayer_menu->addAction(_("Set / Unset Rally Point"));
  act->setShortcut(QKeySequence(tr("shift+s")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_rally);
  act = multiplayer_menu->addAction(_("Quick Airlift"));
  act->setShortcut(QKeySequence(tr("ctrl+y")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_quickairlift);
  airlift_type = new QActionGroup(this);
  airlift_menu =
      multiplayer_menu->addMenu(_("Unit Type for Quickairlifting"));

  // Default diplo
  action_vs_city = new QActionGroup(this);
  action_vs_unit = new QActionGroup(this);
  action_unit_menu = multiplayer_menu->addMenu(_("Default Action vs Unit"));

  act = action_unit_menu->addAction(_("Ask"));
  act->setCheckable(true);
  act->setChecked(true);
  act->setData(-1);
  action_vs_unit->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_unit);

  act = action_unit_menu->addAction(_("Bribe"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_BRIBE_UNIT);
  action_vs_unit->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_unit);

  act = action_unit_menu->addAction(_("Sabotage"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_SABOTAGE_UNIT);
  action_vs_unit->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_unit);

  act = action_unit_menu->addAction(_("Sabotage Unit Escape"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_SABOTAGE_UNIT_ESC);
  action_vs_unit->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_unit);

  action_city_menu = multiplayer_menu->addMenu(_("Default Action vs City"));
  act = action_city_menu->addAction(_("Ask"));
  act->setCheckable(true);
  act->setChecked(true);
  act->setData(-1);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Investigate City"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_INVESTIGATE_CITY);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Investigate City (Spends the Unit)"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_INV_CITY_SPEND);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Establish Embassy"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_ESTABLISH_EMBASSY);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Become Ambassador"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_ESTABLISH_EMBASSY_STAY);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Steal Technology"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_STEAL_TECH);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Steal Technology and Escape"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_STEAL_TECH_ESC);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Incite a Revolt"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_INCITE_CITY);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Incite a Revolt and Escape"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_INCITE_CITY_ESC);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Poison City"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_POISON);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  act = action_city_menu->addAction(_("Poison City and Escape"));
  act->setCheckable(true);
  act->setChecked(false);
  act->setData(ACTION_SPY_POISON_ESC);
  action_vs_city->addAction(act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_action_vs_city);

  // Civilization menu
  menu = this->addMenu(_("Civilization"));
  act = menu->addAction(_("National Budget..."));
  menu_list.insert(NOT_4_OBS, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_popup_tax_rates);
  menu->addSeparator();

  act = menu->addAction(_("Policies..."));
  menu_list.insert(MULTIPLIERS, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_popup_mult_rates);
  menu->addSeparator();

  menu->addMenu(new class gov_menu(this));
  menu->addSeparator();

  act = menu->addAction(Q_("?noun:Map View"));
  act->setShortcut(QKeySequence(tr("F1")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_show_map);

  act = menu->addAction(_("Units View"));
  act->setShortcut(QKeySequence(tr("F2")));
  connect(act, &QAction::triggered, this, &top_bar_units_view);

  // TRANS: Also menu item, but 'headers' should be good enough.
  act = menu->addAction(Q_("?header:Nations View"));
  act->setShortcut(QKeySequence(tr("F3")));
  connect(act, &QAction::triggered, this, &popup_players_dialog);

  act = menu->addAction(_("Cities View"));
  act->setShortcut(QKeySequence(tr("F4")));
  connect(act, &QAction::triggered, this, &city_report_dialog_popup);

  act = menu->addAction(_("Economy View"));
  act->setShortcut(QKeySequence(tr("F5")));
  connect(act, &QAction::triggered, this, &economy_report_dialog_popup);

  act = menu->addAction(_("Research View"));
  act->setShortcut(QKeySequence(tr("F6")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_show_research_tab);

  act = menu->addAction(_("Wonders of the World Report"));
  act->setShortcut(QKeySequence(tr("F7")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_traveler);

  act = menu->addAction(_("Top Five Cities Report"));
  act->setShortcut(QKeySequence(tr("F8")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_top_five);

  act = menu->addAction(_("Demographics Report"));
  act->setShortcut(QKeySequence(tr("F11")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_demographics);

  act = menu->addAction(_("Spaceship View"));
  act->setShortcut(QKeySequence(tr("F12")));
  connect(act, &QAction::triggered, this, &mr_menu::slot_spaceship);

  act = menu->addAction(_("Achievements Report"));
  connect(act, &QAction::triggered, this, &mr_menu::slot_achievements);

  act = menu->addAction(_("Endgame Report"));
  menu_list.insert(ENDGAME, act);
  connect(act, &QAction::triggered, this, &mr_menu::slot_endgame);

  // Help Menu
  menu = this->addMenu(_("Help"));

  act = menu->addAction(Q_(HELP_OVERVIEW_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_OVERVIEW_ITEM); });

  act = menu->addAction(Q_(HELP_PLAYING_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_PLAYING_ITEM); });

  act = menu->addAction(Q_(HELP_TERRAIN_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_TERRAIN_ITEM); });

  act = menu->addAction(Q_(HELP_ECONOMY_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_ECONOMY_ITEM); });

  act = menu->addAction(Q_(HELP_CITIES_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_CITIES_ITEM); });

  act = menu->addAction(Q_(HELP_IMPROVEMENTS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_IMPROVEMENTS_ITEM); });

  act = menu->addAction(Q_(HELP_WONDERS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_WONDERS_ITEM); });

  act = menu->addAction(Q_(HELP_UNITS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_UNITS_ITEM); });

  act = menu->addAction(Q_(HELP_COMBAT_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_COMBAT_ITEM); });

  act = menu->addAction(Q_(HELP_ZOC_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_ZOC_ITEM); });

  act = menu->addAction(Q_(HELP_GOVERNMENT_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_GOVERNMENT_ITEM); });

  act = menu->addAction(Q_(HELP_EFFECTS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_EFFECTS_ITEM); });

  act = menu->addAction(Q_(HELP_DIPLOMACY_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_DIPLOMACY_ITEM); });

  act = menu->addAction(Q_(HELP_TECHS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_TECHS_ITEM); });

  act = menu->addAction(Q_(HELP_SPACE_RACE_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_SPACE_RACE_ITEM); });

  act = menu->addAction(Q_(HELP_TILESET_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_TILESET_ITEM); });

  act = menu->addAction(Q_(HELP_RULESET_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_RULESET_ITEM); });

  act = menu->addAction(Q_(HELP_NATIONS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_NATIONS_ITEM); });

  menu->addSeparator();

  act = menu->addAction(Q_(HELP_CONNECTING_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_CONNECTING_ITEM); });

  act = menu->addAction(Q_(HELP_CONTROLS_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_CONTROLS_ITEM); });

  act = menu->addAction(Q_(HELP_CMA_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_CMA_ITEM); });

  act = menu->addAction(Q_(HELP_CHATLINE_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_CHATLINE_ITEM); });

  act = menu->addAction(Q_(HELP_WORKLIST_EDITOR_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_WORKLIST_EDITOR_ITEM); });

  menu->addSeparator();

  act = menu->addAction(Q_(HELP_LANGUAGES_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_LANGUAGES_ITEM); });

  act = menu->addAction(Q_(HELP_COPYING_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_COPYING_ITEM); });

  act = menu->addAction(Q_(HELP_ABOUT_ITEM));
  QObject::connect(act, &QAction::triggered,
                   [this]() { slot_help(HELP_ABOUT_ITEM); });

  menus = this->findChildren<QMenu *>();
  for (i = 0; i < menus.count(); i++) {
    menus[i]->setAttribute(Qt::WA_TranslucentBackground);
  }
  this->setVisible(false);
  initialized = true;

  shortcuts->create_no_action_shortcuts(queen()->mapview_wdg);
}

/**
 * Clears all menus, making sure them and their actions are deleted.
 */
void mr_menu::clear_menus()
{
  clear();
  menu_list.clear();
  for (const auto menu : king()->findChildren<QMenu *>()) {
    for (const auto action : menu->actions()) {
      // Don't delete submenu actions, they're managed by the submenus
      if (!action->menu()) {
        action->deleteLater();
      }
    }
    menu->deleteLater();
  }
}

/**
   Sets given tile for delayed order
 */
void mr_menu::set_tile_for_order(tile *ptile)
{
  for (auto &fui : units_list) {
    fui.ptile = ptile;
  }
}

/**
   Returns string assigned to shortcut or empty string if doesnt exist
 */
bool mr_menu::shortcut_exists(const fc_shortcut &fcs, QString &where)
{
  if (fcs.type == fc_shortcut::mouse) {
    where = QString();
    return false;
  }

  for (const QMenu *m : findChildren<QMenu *>()) {
    for (const auto action : m->actions()) {
      if (action->shortcut() == fcs.keys) {
        where = m->title() + " > " + action->text();
        return true;
      }
    }
  }

  where = QString();
  return false;
}

/**
   Updates airlift menu
 */
void mr_menu::update_airlift_menu()
{
  Unit_type_id utype_id;
  QAction *act;

  if (!initialized || !client.conn.playing) {
    return;
  }
  airlift_menu->clear();
  if (client_is_observer()) {
    return;
  }
  unit_type_iterate(utype)
  {
    utype_id = utype_index(utype);

    if (!can_player_build_unit_now(client.conn.playing, utype)
        || !utype_can_do_action(utype, ACTION_AIRLIFT)) {
      continue;
    }
    if (!can_player_build_unit_now(client.conn.playing, utype)
        && !has_player_unit_type(utype_id)) {
      continue;
    }
    // Defeat keyboard shortcut mnemonics
    act = airlift_menu->addAction(QString(utype_name_translation(utype)));
    act->setCheckable(true);
    act->setData(utype_id);
    if (airlift_type_id == utype_id) {
      act->setChecked(true);
    }
    connect(act, &QAction::triggered, this, &mr_menu::slot_quickairlift_set);
    airlift_type->addAction(act);
  }
  unit_type_iterate_end;
}

/**
  Updates "build path" menu
 */
void mr_menu::update_roads_menu()
{
  QAction *act;
  bool enabled = false;

  if (!initialized) {
    return;
  }
  QList<QAction *> actions = roads_menu->actions();
  for (auto *act : qAsConst(actions)) {
    removeAction(act);
    act->deleteLater();
  }
  roads_menu->clear();
  roads_menu->setDisabled(true);
  if (client_is_observer()) {
    return;
  }

  extra_type_by_cause_iterate(EC_ROAD, pextra)
  {
    if (pextra->buildable) {
      int road_id;

      // Defeat keyboard shortcut mnemonics
      act = roads_menu->addAction(QString(extra_name_translation(pextra)));
      road_id = pextra->id;
      act->setData(road_id);
      QObject::connect(act, &QAction::triggered,
                       [this, road_id]() { slot_build_path(road_id); });
      if (can_units_do_activity_targeted(get_units_in_focus(),
                                         ACTIVITY_GEN_ROAD, pextra)) {
        act->setEnabled(true);
        enabled = true;
      } else {
        act->setDisabled(true);
      }
    }
  }
  extra_type_by_cause_iterate_end;

  if (enabled) {
    roads_menu->setEnabled(true);
  }
}

/**
  Updates "build bases" menu
 */
void mr_menu::update_bases_menu()
{
  QAction *act;
  bool enabled = false;
  if (!initialized) {
    return;
  }

  QList<QAction *> actions = bases_menu->actions();
  for (auto *act : qAsConst(actions)) {
    removeAction(act);
    act->deleteLater();
  }
  bases_menu->clear();
  bases_menu->setDisabled(true);

  if (client_is_observer()) {
    return;
  }

  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    if (pextra->buildable) {
      int base_id;

      // Defeat keyboard shortcut mnemonics
      act = bases_menu->addAction(QString(extra_name_translation(pextra)));
      base_id = pextra->id;
      act->setData(base_id);
      QObject::connect(act, &QAction::triggered,
                       [this, base_id]() { slot_build_base(base_id); });
      if (can_units_do_activity_targeted(get_units_in_focus(), ACTIVITY_BASE,
                                         pextra)) {
        act->setEnabled(true);
        enabled = true;
      } else {
        act->setDisabled(true);
      }
    }
  }
  extra_type_by_cause_iterate_end;

  if (enabled) {
    bases_menu->setEnabled(true);
  }
}

// Make menus non dependent on unit sensitive
void mr_menu::nonunit_sensitivity()
{
  QMultiHash<munit, QAction *>::iterator i;
  QList<munit> keys = menu_list.keys();
  for (munit key : qAsConst(keys)) {
    i = menu_list.find(key);
    while (i != menu_list.end() && i.key() == key) {
      switch (key) {
      case SAVE:
        if (can_client_access_hack() && C_S_RUNNING <= client_state()) {
          i.value()->setEnabled(true);
        }
        break;
      case NOT_4_OBS:
        if (!client_is_observer()) {
          i.value()->setEnabled(true);
        }
        break;
      case MULTIPLIERS:
        if (!client_is_observer() && multiplier_count() > 0) {
          i.value()->setEnabled(true);
          i.value()->setVisible(true);
        } else {
          i.value()->setVisible(false);
        }
        break;
      case ENDGAME:
        if (queen()->isRepoDlgOpen(QStringLiteral("END"))) {
          i.value()->setEnabled(true);
          i.value()->setVisible(true);
        } else {
          i.value()->setVisible(false);
        }
        break;
      default:
        break;
      }
      i++;
    }
  }
}

static struct extra_type *next_extra(const std::vector<unit *> &punits,
                                     extra_cause cause)
{
  struct extra_type *pextra = nullptr;
  /* FIXME: this overloading doesn't work well with multiple focus
   * units. */
  for (const auto builder : punits) {
    pextra = next_extra_for_tile(unit_tile(builder), cause,
                                 unit_owner(builder), builder);
    if (pextra != nullptr) {
      break;
    }
  }
  return pextra;
}

/**
   Enables/disables menu items and renames them depending on key in menu_list
 */
void mr_menu::menus_sensitive()
{
  QList<munit> keys;
  QMultiHash<munit, QAction *>::iterator i;
  struct road_type *proad;
  struct extra_type *tgt;
  bool city_on_tile = false;
  bool units_all_same_tile;
  struct terrain *pterrain;

  if (!initialized) {
    return;
  }

  /** Disable first all sensitive menus */
  for (QAction *a : qAsConst(menu_list)) {
    a->setEnabled(false);
  }

  if (client_is_observer()) {
    multiplayer_menu->setDisabled(true);
  } else {
    multiplayer_menu->setDisabled(false);
  }

  nonunit_sensitivity();

  if (!can_client_issue_orders() || get_num_units_in_focus() == 0) {
    return;
  }

  const auto &punits = get_units_in_focus();
  city_on_tile = any_unit_in_city(punits);
  units_all_same_tile = units_on_the_same_tile(punits);

  keys = menu_list.keys();
  for (munit key : qAsConst(keys)) {
    i = menu_list.find(key);
    while (i != menu_list.end() && i.key() == key) {
      switch (key) {
      case STANDARD:
        i.value()->setEnabled(true);
        break;

      case EXPLORE:
        if (can_units_do_activity(punits, ACTIVITY_EXPLORE)) {
          i.value()->setEnabled(true);
        }
        break;

      case LOAD:
        if (units_can_load(punits)) {
          i.value()->setEnabled(true);
        }
        break;

      case UNLOAD:
        if (units_can_unload(punits)) {
          i.value()->setEnabled(true);
        }
        break;

      case TRANSPORTER:
        if (units_are_occupied(punits)) {
          i.value()->setEnabled(true);
        }
        break;

      case CONVERT:
        if (units_can_convert(punits)) {
          i.value()->setEnabled(true);
        }
        break;

      case MINE:
        if (can_units_do_activity(punits, ACTIVITY_MINE)) {
          i.value()->setEnabled(true);
        }

        if (units_all_same_tile) {
          struct unit *punit = punits.front();

          pterrain = tile_terrain(unit_tile(punit));

          if (units_have_type_flag(punits, UTYF_SETTLERS, true)) {
            struct extra_type *pextra = next_extra(punits, EC_MINE);

            if (pextra != nullptr) {
              i.value()->setText(
                  // TRANS: Build mine of specific type
                  QString(_("Build %1"))
                      .arg(extra_name_translation(pextra)));
            } else {
              i.value()->setText(QString(_("Build Mine")));
            }
          } else {
            i.value()->setText(QString(_("Build Mine")));
          }
        }
        break;

      case IRRIGATION:
        if (can_units_do_activity(punits, ACTIVITY_IRRIGATE)) {
          i.value()->setEnabled(true);
        }
        if (units_all_same_tile) {
          struct unit *punit = punits.front();

          pterrain = tile_terrain(unit_tile(punit));

          if (units_have_type_flag(punits, UTYF_SETTLERS, true)) {
            struct extra_type *pextra = next_extra(punits, EC_IRRIGATION);

            if (pextra != nullptr) {
              i.value()->setText(
                  // TRANS: Build irrigation of specific type
                  QString(_("Build %1"))
                      .arg(extra_name_translation(pextra)));
            } else {
              i.value()->setText(QString(_("Build Irrigation")));
            }
          } else {
            i.value()->setText(QString(_("Build Irrigation")));
          }
        }
        break;

      case CULTIVATE:
        if (can_units_do_activity(punits, ACTIVITY_CULTIVATE)) {
          i.value()->setEnabled(true);
        }
        if (units_all_same_tile) {
          struct unit *punit = punits.front();

          pterrain = tile_terrain(unit_tile(punit));
          if (pterrain->irrigation_result != T_NONE
              && pterrain->irrigation_result != pterrain) {
            i.value()->setText(
                // TRANS: Transform terrain to specific type
                QString(_("Cultivate to %1"))
                    .arg(QString(get_tile_change_menu_text(
                        unit_tile(punit), ACTIVITY_CULTIVATE))));
          } else {
            i.value()->setText(QString(_("Cultivate")));
          }
        } else {
          i.value()->setText(QString(_("Cultivate")));
        }
        break;

      case PLANT:
        if (can_units_do_activity(punits, ACTIVITY_PLANT)) {
          i.value()->setEnabled(true);
        }
        if (units_all_same_tile) {
          struct unit *punit = punits.front();

          pterrain = tile_terrain(unit_tile(punit));
          if (pterrain->mining_result != T_NONE
              && pterrain->mining_result != pterrain) {
            i.value()->setText(
                // TRANS: Transform terrain to specific type
                QString(_("Plant to %1"))
                    .arg(QString(get_tile_change_menu_text(
                        unit_tile(punit), ACTIVITY_PLANT))));
          } else {
            i.value()->setText(QString(_("Plant")));
          }
        } else {
          i.value()->setText(QString(_("Plant")));
        }
        break;

      case TRANSFORM:
        if (can_units_do_activity(punits, ACTIVITY_TRANSFORM)) {
          i.value()->setEnabled(true);
        } else {
          break;
        }
        if (units_all_same_tile) {
          struct unit *punit = punits.front();
          pterrain = tile_terrain(unit_tile(punit));
          if (pterrain->transform_result != T_NONE
              && pterrain->transform_result != pterrain) {
            i.value()->setText(
                // TRANS: Transform terrain to specific type
                QString(_("Transform to %1"))
                    .arg(QString(get_tile_change_menu_text(
                        unit_tile(punit), ACTIVITY_TRANSFORM))));
          } else {
            i.value()->setText(_("Transform Terrain"));
          }
        }
        break;

      case BUILD:
        if (can_units_do(punits, unit_can_add_or_build_city)) {
          i.value()->setEnabled(true);
        }
        if (city_on_tile
            && units_can_do_action(punits, ACTION_JOIN_CITY, true)) {
          i.value()->setText(
              QString(action_id_name_translation(ACTION_JOIN_CITY)));
        } else {
          i.value()->setText(
              QString(action_id_name_translation(ACTION_FOUND_CITY)));
        }
        break;

      case ROAD: {
        struct extra_type *pextra = next_extra(punits, EC_ROAD);

        if (can_units_do_any_road(punits)) {
          i.value()->setEnabled(true);
        }

        if (pextra != nullptr) {
          i.value()->setText(
              // TRANS: Build road of specific type
              QString(_("Build %1")).arg(extra_name_translation(pextra)));
        }
      } break;

      case FORTIFY:
        if (can_units_do_activity(punits, ACTIVITY_FORTIFYING)) {
          i.value()->setEnabled(true);
        }
        break;

      case FORTRESS:
        if (can_units_do_base_gui(punits, BASE_GUI_FORTRESS)) {
          i.value()->setEnabled(true);
        }
        break;

      case AIRBASE:
        if (can_units_do_base_gui(punits, BASE_GUI_AIRBASE)) {
          i.value()->setEnabled(true);
        }
        break;

      case POLLUTION:
        if (can_units_do_activity(punits, ACTIVITY_POLLUTION)
            || can_units_do(punits, can_unit_paradrop)) {
          i.value()->setEnabled(true);
        }
        if (units_can_do_action(punits, ACTION_PARADROP, true)) {
          i.value()->setText(
              QString(action_id_name_translation(ACTION_PARADROP)));
        } else {
          i.value()->setText(_("Clean Pollution"));
        }
        break;

      case FALLOUT:
        if (can_units_do_activity(punits, ACTIVITY_FALLOUT)) {
          i.value()->setEnabled(true);
        }
        break;

      case SENTRY:
        if (can_units_do_activity(punits, ACTIVITY_SENTRY)) {
          i.value()->setEnabled(true);
        }
        break;

      case PILLAGE:
        if (can_units_do_activity(punits, ACTIVITY_PILLAGE)) {
          i.value()->setEnabled(true);
        }
        break;

      case HOMECITY:
        if (can_units_do(punits, can_unit_change_homecity)) {
          i.value()->setEnabled(true);
        }
        break;

      case WAKEUP:
        if (units_have_activity_on_tile(punits, ACTIVITY_SENTRY)) {
          i.value()->setEnabled(true);
        }
        break;

      case AUTOSETTLER:
        if (can_units_do(punits, can_unit_do_autosettlers)) {
          i.value()->setEnabled(true);
        }
        if (units_contain_cityfounder(punits)) {
          i.value()->setText(_("Auto Settler"));
        } else {
          i.value()->setText(_("Auto Worker"));
        }
        break;
      case CONNECT_ROAD:
        proad = road_by_compat_special(ROCO_ROAD);
        if (proad != nullptr) {
          tgt = road_extra_get(proad);
        } else {
          break;
        }
        if (can_units_do_connect(punits, ACTIVITY_GEN_ROAD, tgt)) {
          i.value()->setEnabled(true);
        }
        break;

      case DISBAND:
        if (units_can_do_action(punits, ACTION_DISBAND_UNIT, true)) {
          i.value()->setEnabled(true);
        }
        break;

      case RENAME:
        i.value()->setEnabled(get_num_units_in_focus() == 1);
        break;

      case CONNECT_RAIL:
        proad = road_by_compat_special(ROCO_RAILROAD);
        if (proad != nullptr) {
          tgt = road_extra_get(proad);
        } else {
          break;
        }
        if (can_units_do_connect(punits, ACTIVITY_GEN_ROAD, tgt)) {
          i.value()->setEnabled(true);
        }
        break;

      case CONNECT_IRRIGATION: {
        struct extra_type_list *extras =
            extra_type_list_by_cause(EC_IRRIGATION);

        if (extra_type_list_size(extras) > 0) {
          struct extra_type *pextra;

          pextra = extra_type_list_get(
              extra_type_list_by_cause(EC_IRRIGATION), 0);
          if (can_units_do_connect(punits, ACTIVITY_IRRIGATE, pextra)) {
            i.value()->setEnabled(true);
          }
        }
      } break;

      case GOTO_CITY:
        i.value()->setEnabled(true);
        break;

      case AIRLIFT:
        i.value()->setEnabled(true);
        break;

      case BUILD_WONDER:
        i.value()->setText(
            QString(action_id_name_translation(ACTION_HELP_WONDER)));
        if (can_units_do(punits, unit_can_help_build_wonder_here)) {
          i.value()->setEnabled(true);
        }
        break;

      case AUTOTRADEROUTE:
        if (units_can_do_action(punits, ACTION_TRADE_ROUTE, true)) {
          i.value()->setEnabled(true);
        }
        break;

      case ORDER_TRADEROUTE:
        i.value()->setText(
            QString(action_id_name_translation(ACTION_TRADE_ROUTE)));
        if (can_units_do(punits, unit_can_est_trade_route_here)) {
          i.value()->setEnabled(true);
        }
        break;

      case ORDER_DIPLOMAT_DLG:
        if (units_can_do_action(punits, ACTION_ANY, true)) {
          i.value()->setEnabled(true);
        }
        break;

      case UPGRADE:
        if (units_can_upgrade(punits)) {
          i.value()->setEnabled(true);
        }
        break;
      default:
        break;
      }

      i++;
    }
  }
}

/**
   Slot for showing research tab
 */
void mr_menu::slot_show_research_tab() { science_report_dialog_popup(true); }

/**
   Slot for showing spaceship
 */
void mr_menu::slot_spaceship()
{
  if (nullptr != client.conn.playing) {
    popup_spaceship_dialog(client.conn.playing);
  }
}

/**
   Changes tab to mapview
 */
void mr_menu::slot_show_map() { top_bar_show_map(); }

/**
   Action "BUILD_CITY"
 */
void mr_menu::slot_build_city()
{
  for (const auto punit : get_units_in_focus()) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    /* Enable the button for adding to a city in all cases, so we
       get an eventual error message from the server if we try. */
    if (unit_can_add_or_build_city(punit)) {
      request_unit_build_city(punit);
    } else if (utype_can_do_action(unit_type_get(punit),
                                   ACTION_HELP_WONDER)) {
      request_unit_caravan_action(punit, ACTION_HELP_WONDER);
    }
  }
}

/**
   Action "CLEAN FALLOUT"
 */
void mr_menu::slot_clean_fallout() { key_unit_fallout(); }

/**
   Action "CLEAN POLLUTION and PARADROP"
 */
void mr_menu::slot_clean_pollution()
{
  for (const auto punit : get_units_in_focus()) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    struct extra_type *pextra;

    pextra = prev_extra_in_tile(unit_tile(punit), ERM_CLEANPOLLUTION,
                                unit_owner(punit), punit);
    if (pextra != nullptr) {
      request_new_unit_activity_targeted(punit, ACTIVITY_POLLUTION, pextra);
    } else if (can_unit_paradrop(punit)) {
      /* FIXME: This is getting worse, we use a key_unit_*() function
       * which assign the order for all units!  Very bad! */
      key_unit_paradrop();
    }
  }
}

/**
   Action "CONNECT WITH IRRIGATION"
 */
void mr_menu::slot_conn_irrigation()
{
  struct extra_type_list *extras = extra_type_list_by_cause(EC_IRRIGATION);

  if (extra_type_list_size(extras) > 0) {
    struct extra_type *pextra;

    pextra = extra_type_list_get(extra_type_list_by_cause(EC_IRRIGATION), 0);

    key_unit_connect(ACTIVITY_IRRIGATE, pextra);
  }
}

/**
   Action "CONNECT WITH RAILROAD"
 */
void mr_menu::slot_conn_rail()
{
  struct road_type *prail = road_by_compat_special(ROCO_RAILROAD);

  if (prail != nullptr) {
    struct extra_type *tgt;

    tgt = road_extra_get(prail);
    key_unit_connect(ACTIVITY_GEN_ROAD, tgt);
  }
}

/**
   Action "BUILD FORTRESS"
 */
void mr_menu::slot_unit_fortress() { key_unit_fortress(); }

/**
   Action "BUILD AIRBASE"
 */
void mr_menu::slot_unit_airbase() { key_unit_airbase(); }

/**
   Action "CONNECT WITH ROAD"
 */
void mr_menu::slot_conn_road()
{
  struct road_type *proad = road_by_compat_special(ROCO_ROAD);

  if (proad != nullptr) {
    struct extra_type *tgt;

    tgt = road_extra_get(proad);
    key_unit_connect(ACTIVITY_GEN_ROAD, tgt);
  }
}

/**
   Action "TRANSFROM TERRAIN"
 */
void mr_menu::slot_transform() { key_unit_transform(); }

/**
   Action "PILLAGE"
 */
void mr_menu::slot_pillage() { key_unit_pillage(); }

/**
   Do... the selected action
 */
void mr_menu::slot_action() { key_unit_action_select_tgt(); }

/**
   Action "AUTO_SETTLER"
 */
void mr_menu::slot_auto_settler() { key_unit_auto_settle(); }

/**
   Action "BUILD_ROAD"
 */
void mr_menu::slot_build_road()
{
  for (const auto punit : get_units_in_focus()) {
    /* FIXME: this can provide different actions for different units...
     * not good! */
    struct extra_type *tgt = next_extra_for_tile(unit_tile(punit), EC_ROAD,
                                                 unit_owner(punit), punit);
    bool building_road = false;

    if (tgt != nullptr
        && can_unit_do_activity_targeted(punit, ACTIVITY_GEN_ROAD, tgt)) {
      request_new_unit_activity_targeted(punit, ACTIVITY_GEN_ROAD, tgt);
      building_road = true;
    }

    if (!building_road && unit_can_est_trade_route_here(punit)) {
      request_unit_caravan_action(punit, ACTION_TRADE_ROUTE);
    }
  }
}

/**
   Action "BUILD_IRRIGATION"
 */
void mr_menu::slot_build_irrigation() { key_unit_irrigate(); }

/**
   Action "CULTIVATE"
 */
void mr_menu::slot_cultivate() { key_unit_cultivate(); }

/**
   Action "BUILD_MINE"
 */
void mr_menu::slot_build_mine() { key_unit_mine(); }

/**
   Action "PLANT"
 */
void mr_menu::slot_plant() { key_unit_plant(); }

/**
   Action "FORTIFY"
 */
void mr_menu::slot_unit_fortify() { key_unit_fortify(); }

/**
   Action "SENTRY"
 */
void mr_menu::slot_unit_sentry() { key_unit_sentry(); }

/**
   Action "CONVERT"
 */
void mr_menu::slot_convert() { key_unit_convert(); }

/**
   Action "DISBAND UNIT"
 */
void mr_menu::slot_disband() { popup_disband_dialog(get_units_in_focus()); }

/**
 * Action "RENAME UNIT"
 */
void mr_menu::slot_rename()
{
  if (get_num_units_in_focus() != 1) {
    return;
  }
  for (const auto punit : get_units_in_focus()) {
    auto ask = new hud_input_box(king()->central_wdg);

    ask->set_text_title_definput(_("New unit name:"), _("Rename Unit"),
                                 punit->name);
    ask->setAttribute(Qt::WA_DeleteOnClose);

    int id = punit->id;
    connect(ask, &QDialog::accepted, [ask, id]() {
      // Unit might have been removed, make sure it's still there
      auto unit = game_unit_by_number(id);
      if (unit) {
        dsend_packet_unit_rename(&client.conn, id,
                                 ask->input_edit.text().toUtf8());
      }
    });
    ask->show();
  }
}

/**
   Clears delayed orders
 */
void mr_menu::slot_orders_clear()
{
  delayed_order = false;
  units_list.clear();
}

/**
   Sets/unset rally point
 */
void mr_menu::slot_rally()
{
  king()->rallies.hover_tile = false;
  king()->rallies.hover_city = true;
}

/**
   Adds one city to trade planning
 */
void mr_menu::slot_trade_city() { king()->trade_gen.hover_city = true; }

/**
   Adds all cities to trade planning
 */
void mr_menu::slot_trade_add_all() { king()->trade_gen.add_all_cities(); }

/**
   Trade calculation slot
 */
void mr_menu::slot_calculate() { king()->trade_gen.calculate(); }

/**
   Slot for clearing trade routes
 */
void mr_menu::slot_clear_trade() { king()->trade_gen.clear_trade_planing(); }

/**
   Sends automatic caravan
 */
void mr_menu::slot_autocaravan()
{
  struct unit *punit;
  struct city *homecity;
  struct tile *home_tile;
  struct tile *dest_tile;
  bool sent = false;

  punit = head_of_units_in_focus();
  homecity = game_city_by_number(punit->homecity);
  home_tile = homecity->tile;
  for (auto gilles : qAsConst(king()->trade_gen.lines)) {
    if ((gilles.t1 == home_tile || gilles.t2 == home_tile)
        && gilles.autocaravan == nullptr) {
      // send caravan
      if (gilles.t1 == home_tile) {
        dest_tile = gilles.t2;
      } else {
        dest_tile = gilles.t1;
      }
      if (send_goto_tile(punit, dest_tile)) {
        int i;
        i = king()->trade_gen.lines.indexOf(gilles);
        gilles = king()->trade_gen.lines.takeAt(i);
        gilles.autocaravan = punit;
        king()->trade_gen.lines.append(gilles);
        sent = true;
        break;
      }
    }
  }

  if (!sent) {
    queen()->chat->append(_("Didn't find any trade route to establish"));
  }
}

/**
   Slot for setting quick airlift
 */
void mr_menu::slot_quickairlift_set()
{
  QVariant v;
  QAction *act;

  act = qobject_cast<QAction *>(sender());
  v = act->data();
  airlift_type_id = v.toInt();
}

/**
   Slot for choosing default action vs unit
 */
void mr_menu::slot_action_vs_unit()
{
  QAction *act;

  act = qobject_cast<QAction *>(sender());
  qdef_act::action()->vs_unit_set(act->data().toInt());
}

/**
   Slot for choosing default action vs city
 */
void mr_menu::slot_action_vs_city()
{
  QAction *act;

  act = qobject_cast<QAction *>(sender());
  qdef_act::action()->vs_city_set(act->data().toInt());
}

/**
   Slot for quick airlifting
 */
void mr_menu::slot_quickairlift() { quick_airlifting = true; }

/**
   Delayed goto
 */
void mr_menu::slot_delayed_goto()
{
  delay_order dg;

  delayed_order = true;
  dg = D_GOTO;

  const auto &focus = get_units_in_focus();
  if (focus.empty()) {
    return;
  }
  if (hover_state != HOVER_GOTO) {
    set_hover_state(focus, HOVER_GOTO, ACTIVITY_LAST, nullptr, NO_TARGET,
                    NO_TARGET, ACTION_NONE, ORDER_LAST);
    enter_goto_state(focus);
    create_line_at_mouse_pos();
    control_mouse_cursor(nullptr);
  }
  for (const auto punit : focus) {
    units_list.emplace_back(dg, punit->id);
  }
}

/**
   Executes stored orders
 */
void mr_menu::slot_execute_orders()
{
  struct unit *punit;
  struct tile *last_tile;
  struct tile *new_tile;
  int i = 0;

  for (const auto &fui : units_list) {
    i++;
    punit = unit_list_find(client_player()->units, fui.id);
    if (punit == nullptr) {
      continue;
    }
    last_tile = punit->tile;
    new_tile = find_last_unit_pos(punit, i);
    if (new_tile != nullptr) {
      punit->tile = new_tile;
    }
    if (is_tiles_adjacent(punit->tile, fui.ptile)) {
      request_move_unit_direction(
          punit, get_direction_for_step(&(wld.map), punit->tile, fui.ptile));
    } else {
      send_attack_tile(punit, fui.ptile);
    }
    punit->tile = last_tile;
  }
  units_list.clear();
}

/**
   Action "LOAD INTO TRANSPORTER"
 */
void mr_menu::slot_load()
{
  for (const auto punit : get_units_in_focus()) {
    request_transport(punit, unit_tile(punit));
  }
}

/**
   Action "UNIT PATROL"
 */
void mr_menu::slot_patrol() { key_unit_patrol(); }

/**
   Action "GOTO/AIRLIFT TO CITY"
 */
void mr_menu::slot_airlift() { popup_goto_dialog(); }

/**
   Action "SET HOMECITY"
 */
void mr_menu::slot_set_home() { key_unit_homecity(); }

/**
   Action "UNLOAD FROM TRANSPORTED"
 */
void mr_menu::slot_unload()
{
  for (const auto punit : get_units_in_focus()) {
    request_unit_unload(punit);
  }
}

/**
   Action "UNLOAD ALL UNITS FROM TRANSPORTER"
 */
void mr_menu::slot_unload_all() { key_unit_unload_all(); }

/**
   Action "UNSENTRY(WAKEUP) ALL UNITS"
 */
void mr_menu::slot_unsentry() { key_unit_wakeup_others(); }

/**
   Action "UPGRADE UNITS"
 */
void mr_menu::slot_upgrade() { popup_upgrade_dialog(get_units_in_focus()); }

/**
   Action "GOTO"
 */
void mr_menu::slot_unit_goto() { key_unit_goto(); }

/**
   Action "EXPLORE"
 */
void mr_menu::slot_unit_explore() { key_unit_auto_explore(); }

/**
   Action "CENTER VIEW"
 */
void mr_menu::slot_center_view() { request_center_focus_unit(); }

/**
   Action "Lock interface"
 */
void mr_menu::slot_lock()
{
  if (king()->interface_locked) {
    enable_interface(false);
  } else {
    enable_interface(true);
  }
  king()->interface_locked = !king()->interface_locked;
}

/**
   Helper function to hide/show widgets
 */
void enable_interface(bool enable)
{
  QList<close_widget *> lc;
  QList<move_widget *> lm;
  int i;

  lc = king()->findChildren<close_widget *>();
  lm = king()->findChildren<move_widget *>();

  for (i = 0; i < lc.size(); ++i) {
    lc.at(i)->setVisible(!enable);
  }
  for (i = 0; i < lm.size(); ++i) {
    lm.at(i)->setVisible(!enable);
  }
}

/**
   Action "SET FULLSCREEN"
 */
void mr_menu::slot_fullscreen()
{
  gui_options->gui_qt_fullscreen = !gui_options->gui_qt_fullscreen;
  if (gui_options->gui_qt_fullscreen) {
    king()->setWindowState(king()->windowState() | Qt::WindowFullScreen);
  } else {
    king()->setWindowState(king()->windowState() & ~Qt::WindowFullScreen);
  }
}

/**
   Action "Show/Dont show new turn info"
 */
void mr_menu::slot_show_new_turn_text()
{
  king()->qt_settings.show_new_turn_text = osd_status->isChecked();
}

/**
   Action "Show/Dont battle log"
 */
void mr_menu::slot_battlelog()
{
  king()->qt_settings.show_battle_log = btlog_status->isChecked();
}

/**
   Action "SHOW BORDERS"
 */
void mr_menu::slot_borders() { key_map_borders_toggle(); }

/**
   Action "SHOW NATIVE TILES"
 */
void mr_menu::slot_native_tiles() { key_map_native_toggle(); }

/**
   Action "SHOW BUY COST"
 */
void mr_menu::slot_city_buycost() { key_city_buycost_toggle(); }

/**
   Action "SHOW CITY GROWTH"
 */
void mr_menu::slot_city_growth() { key_city_growth_toggle(); }

/**
   Action "SCALE FONTS WHEN SCALING MAP"
 */
void mr_menu::zoom_scale_fonts()
{
  gui_options->zoom_scale_fonts = scale_fonts_status->isChecked();
  update_map_canvas_visible();
}

/**
   Action "SHOW CITY NAMES"
 */
void mr_menu::slot_city_names() { key_city_names_toggle(); }

/**
   Action "SHOW CITY OUTLINES"
 */
void mr_menu::slot_city_outlines() { key_city_outlines_toggle(); }

/**
   Action "Citybar changed"
 */
void mr_menu::slot_set_citybar()
{
  for (auto *a : action_citybar->actions()) {
    if (a->isChecked()) {
      fc_strlcpy(gui_options->default_city_bar_style_name,
                 qUtf8Printable(a->data().toString()),
                 sizeof(gui_options->default_city_bar_style_name));
      options_iterate(client_optset, poption)
      {
        if (QString(option_name(poption))
            == QLatin1String("default_city_bar_style_name")) {
          citybar_painter::option_changed(poption);
        }
      }
      options_iterate_end;
    }
  }
}

/**
   Action "SHOW CITY OUTPUT"
 */
void mr_menu::slot_city_output() { key_city_output_toggle(); }

/**
   Action "SHOW CITY PRODUCTION"
 */
void mr_menu::slot_city_production() { key_city_productions_toggle(); }

/**
   Action "SHOW CITY TRADEROUTES"
 */
void mr_menu::slot_city_traderoutes() { key_city_trade_routes_toggle(); }

/**
   Action "SHOW MAP GRID"
 */
void mr_menu::slot_map_grid() { key_map_grid_toggle(); }

/**
   Action "DONE MOVING"
 */
void mr_menu::slot_done_moving() { key_unit_done(); }

/**
   Action "SELECT ALL UNITS ON TILE"
 */
void mr_menu::slot_select_all_tile()
{
  request_unit_select(get_units_in_focus(), SELTYPE_ALL, SELLOC_TILE);
}

/**
   Action "SELECT ONE UNITS/DESELECT OTHERS"
 */
void mr_menu::slot_select_one()
{
  request_unit_select(get_units_in_focus(), SELTYPE_SINGLE, SELLOC_TILE);
}

/**
   Action "SELLECT SAME UNITS ON CONTINENT"
 */
void mr_menu::slot_select_same_continent()
{
  request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_CONT);
}

/**
   Action "SELECT SAME TYPE EVERYWHERE"
 */
void mr_menu::slot_select_same_everywhere()
{
  request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_WORLD);
}

/**
   Action "SELECT SAME TYPE ON TILE"
 */
void mr_menu::slot_select_same_tile()
{
  request_unit_select(get_units_in_focus(), SELTYPE_SAME, SELLOC_TILE);
}

/**
   Action "WAIT"
 */
void mr_menu::slot_wait() { key_unit_wait(); }

/**
   Shows units filter
 */
void mr_menu::slot_unit_filter()
{
  unit_hud_selector *uhs;
  uhs = new unit_hud_selector(king()->central_wdg);
  uhs->show_me();
}

/**
   Action "SHOW DEMOGRAPGHICS REPORT"
 */
void mr_menu::slot_demographics()
{
  send_report_request(REPORT_DEMOGRAPHIC);
}

/**
   Action "SHOW ACHIEVEMENTS REPORT"
 */
void mr_menu::slot_achievements()
{
  send_report_request(REPORT_ACHIEVEMENTS);
}

/**
   Action "SHOW ENDGAME REPORT"
 */
void mr_menu::slot_endgame() { popup_endgame_report(); }

/**
   Action "SHOW TOP FIVE CITIES"
 */
void mr_menu::slot_top_five() { send_report_request(REPORT_TOP_5_CITIES); }

/**
   Action "SHOW WONDERS REPORT"
 */
void mr_menu::slot_traveler()
{
  send_report_request(REPORT_WONDERS_OF_THE_WORLD);
}

/**
   Shows rulesets to load
 */
void mr_menu::tileset_custom_load()
{
  QDialog *dialog = new QDialog(this);
  QLabel *label;
  QPushButton *but;
  QVBoxLayout *layout;
  const struct option *poption;
  QStringList sl;

  sl << QStringLiteral("default_tileset_square_name")
     << QStringLiteral("default_tileset_hex_name")
     << QStringLiteral("default_tileset_isohex_name");
  layout = new QVBoxLayout;
  dialog->setWindowTitle(_("Available tilesets"));
  label = new QLabel;
  label->setText(_("Some tilesets might not be compatible with current"
                   " map topology!"));
  layout->addWidget(label);

  // Gather the list of tilesets
  QStringList tilesets;
  for (auto const &s : qAsConst(sl)) {
    poption = optset_option_by_name(client_optset, qUtf8Printable(s));
    for (const auto &name : qAsConst(*get_tileset_list(poption))) {
      tilesets.append(name);
    }
  }

  // Remove duplicates
  tilesets.sort();
  tilesets.erase(std::unique(tilesets.begin(), tilesets.end()),
                 tilesets.end());

  // Add the buttons
  for (const auto &name : qAsConst(tilesets)) {
    but = new QPushButton(name);
    connect(but, &QAbstractButton::clicked, this,
            &mr_menu::load_new_tileset);
    layout->addWidget(but);
  }
  dialog->setSizeGripEnabled(true);
  dialog->setLayout(layout);
  dialog->show();
}

/**
 * Slot for loading modpack installer
 */
void mr_menu::show_tileset_options()
{
  auto dialog = new freeciv::tileset_options_dialog(tileset, this);
  dialog->show();
}

/**
 * Slot for loading modpack installer
 */
void mr_menu::add_modpacks() { king()->load_modpack(); }

/**
   Slot for loading new tileset
 */
void mr_menu::load_new_tileset()
{
  QPushButton *but;
  QByteArray tn_bytes;

  but = qobject_cast<QPushButton *>(sender());
  tn_bytes = but->text().toLocal8Bit();
  tilespec_reread(tn_bytes.data(), true);
  queen()->mapview_wdg->set_scale(1.0);
  but->parentWidget()->close();
}

/**
   Action "NATIONAL BUDGET"
 */
void mr_menu::slot_popup_tax_rates() { queen()->popup_budget_dialog(); }

/**
   Action "MULTIPLERS RATES"
 */
void mr_menu::slot_popup_mult_rates() { popup_multiplier_dialog(); }

/**
   Actions "HELP_*"
 */
void mr_menu::slot_help(const QString &topic)
{
  popup_help_dialog_typed(Q_(topic.toStdString().c_str()), HELP_ANY);
}

/**
  Actions "BUILD_PATH_*"
 */
void mr_menu::slot_build_path(int id)
{
  for (const auto punit : get_units_in_focus()) {
    extra_type_by_cause_iterate(EC_ROAD, pextra)
    {
      if (pextra->buildable && pextra->id == id
          && can_unit_do_activity_targeted(punit, ACTIVITY_GEN_ROAD,
                                           pextra)) {
        request_new_unit_activity_targeted(punit, ACTIVITY_GEN_ROAD, pextra);
      }
    }
    extra_type_by_cause_iterate_end;
  }
}

/**
  Actions "BUILD_BASE_*"
 */
void mr_menu::slot_build_base(int id)
{
  for (const auto punit : get_units_in_focus()) {
    extra_type_by_cause_iterate(EC_BASE, pextra)
    {
      if (pextra->buildable && pextra->id == id
          && can_unit_do_activity_targeted(punit, ACTIVITY_BASE, pextra)) {
        request_new_unit_activity_targeted(punit, ACTIVITY_BASE, pextra);
      }
    }
    extra_type_by_cause_iterate_end;
  }
}

/**
 * Reimplemented virtual function.
 */
bool mr_menu::event(QEvent *event)
{
  if (event->type() == TilesetChanged) {
    tileset_options->setEnabled(tileset_has_options(tileset));
  }
  return QMenuBar::event(event);
}

/**
   Invoke dialog with interface (local) options
 */
void mr_menu::local_options() { popup_client_options(); }

/**
   Invoke dialog with shortcut options
 */
void mr_menu::shortcut_options() { popup_shortcuts_dialog(); }

/**
   Invoke dialog with server options
 */
void mr_menu::server_options()
{
  option_dialog_popup(_("Game Options"), server_optset);
}

/**
   Invoke dialog with messages options
 */
void mr_menu::messages_options() { popup_messageopt_dialog(); }

/**
   Menu Save Options Now
 */
void mr_menu::save_options_now() { options_save(nullptr); }

/**
   Invoke popup for quiting game
 */
void mr_menu::quit_game() { popup_quit_dialog(); }

/**
   Menu Save Map Image
 */
void mr_menu::save_image()
{
  // Save the size of the map view
  QSize current = queen()->mapview_wdg->size();

  // The size of the image we want to render
  QSize full_size((wld.map.xsize + 2) * tileset_tile_width(tileset),
                  (wld.map.ysize + 2) * tileset_tile_height(tileset));
  if (tileset_is_isometric(tileset)) {
    full_size.rheight() /= 2;
  }

  auto renderer = new freeciv::renderer;
  renderer->set_viewport_size(full_size);

  // Wait for a fullscreen repaint_needed. It will come asynchronously.
  connect(
      renderer, &freeciv::renderer::repaint_needed,
      [=](const QRegion &where) {
        // Maybe some other update sneaked in -- make sure to ignore it
        if (where.boundingRect().contains(QRect(QPoint(), full_size))) {
          // File path
          QString img_name =
              QStringLiteral("Freeciv21-Turn%1").arg(game.info.turn);
          if (client_has_player()) {
            img_name += QStringLiteral("-")
                        + QString(nation_plural_for_player(client_player()));
          }

          auto path = QStandardPaths::writableLocation(
              QStandardPaths::PicturesLocation);
          if (path.isEmpty() || !QDir(path).exists()) {
            path = QStandardPaths::writableLocation(
                QStandardPaths::HomeLocation);
          }
          if (path.isEmpty() || !QDir(path).exists()) {
            path = freeciv_storage_dir();
          }
          img_name =
              path + QStringLiteral("/") + img_name + QStringLiteral(".png");

          // Render
          auto pixmap = QPixmap(full_size);
          pixmap.fill(Qt::black);

          auto painter = QPainter(&pixmap);
          renderer->render(painter, QRect(pixmap.rect()));
          renderer->deleteLater();

          // Save
          bool map_saved = pixmap.save(img_name, "png");

          // Restore the old mapview size
          map_canvas_resized(current.width(), current.height());

          // Let the user know
          hud_message_box *saved = new hud_message_box(king()->central_wdg);
          saved->setStandardButtons(QMessageBox::Ok);
          saved->setDefaultButton(QMessageBox::Ok);
          saved->setAttribute(Qt::WA_DeleteOnClose);
          if (map_saved) {
            saved->set_text_title(_("Image saved as:\n") + img_name,
                                  _("Success"));
          } else {
            saved->set_text_title(_("Failed to save image of the map"),
                                  _("Error"));
          }
          saved->show();
        }
      });
}

/**
   Menu Save Game
 */
void mr_menu::save_game() { send_save_game(nullptr); }

/**
   Menu Save Game As...
 */
void mr_menu::save_game_as()
{
  QString str;
  QString current_file;
  QString location;

  for (const auto &dirname : get_save_dirs()) {
    location = dirname;
    // choose last location
  }

  str = QString(_("Save Games"))
        + QStringLiteral(" (*.sav *.sav.bz2 *.sav.gz *.sav.xz *.sav.zst)");
  current_file = QFileDialog::getSaveFileName(
      king()->central_wdg, _("Save Game As..."), location, str);
  if (!current_file.isEmpty()) {
    QByteArray cf_bytes;

    cf_bytes = current_file.toLocal8Bit();
    send_save_game(cf_bytes.data());
  }
}

/**
   Back to Main Menu
 */
void mr_menu::back_to_menu()
{
  hud_message_box *ask;

  if (is_server_running()) {
    ask = new hud_message_box(king()->central_wdg);
    ask->set_text_title(
        _("Do you want to leave the game?\n\nLeaving a single-player game "
          "will end it. Be sure to save first."),
        QStringLiteral("Leave Game"));
    ask->setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    ask->setDefaultButton(QMessageBox::No);
    ask->button(QMessageBox::No)->setText(_("Keep Playing"));
    ask->button(QMessageBox::Yes)->setText(_("Leave"));
    ask->setAttribute(Qt::WA_DeleteOnClose);

    connect(ask, &hud_message_box::accepted, [=]() {
      if (client.conn.used) {
        disconnect_from_server();
      }
    });
    ask->show();
  } else {
    disconnect_from_server();
  }
}

/**
   Airlift unit type to city acity from each city
 */
void multiairlift(struct city *acity, Unit_type_id ut)
{
  struct tile *ptile;
  city_list_iterate(client.conn.playing->cities, pcity)
  {
    if (get_city_bonus(pcity, EFT_AIRLIFT) > 0) {
      ptile = city_tile(pcity);
      unit_list_iterate(ptile->units, punit)
      {
        if (punit->utype == utype_by_number(ut)) {
          request_unit_airlift(punit, acity);
          break;
        }
      }
      unit_list_iterate_end;
    }
  }
  city_list_iterate_end;
}
