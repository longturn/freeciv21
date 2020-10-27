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

#include "page_game.h"
// Qt
#include <QGridLayout>
// utility
#include "fcintl.h"
// common
#include "cityrep_g.h"
#include "government.h"
#include "repodlgs_g.h"
// client
#include "client_main.h"
#include "mapview_common.h"
#include "text.h"
// gui-qt - Eye of Storm
#include "fc_client.h"
#include "gotodlg.h"
#include "hudwidget.h"
#include "icons.h"
#include "mapview.h"
#include "messagewin.h"
#include "minimap.h"
#include "plrdlg.h"
#include "sidebar.h"
#include "voteinfo_bar.h"

int last_center_capital = 0;
int last_center_player_city = 0;
int last_center_enemy_city = 0;
int last_center_enemy = 0;

static void center_next_enemy_city();
static void center_next_player_city();
static void center_next_player_capital();
static void cycle_enemy_units();

extern void toggle_units_report(bool);

page_game::page_game(QWidget *parent, fc_client *c) : QWidget(parent)
{
  QGridLayout *page_game_layout;
  king = c;
  QGridLayout *game_layout;

  page_game_layout = new QGridLayout;
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

  game_tab_widget->addWidget(game_main_widget);
  if (gui_options.gui_qt_sidebar_left) {
    page_game_layout->addWidget(sidebar_wdg, 1, 0);
  } else {
    page_game_layout->addWidget(sidebar_wdg, 1, 2);
  }
  page_game_layout->addWidget(game_tab_widget, 1, 1);
  page_game_layout->setContentsMargins(0, 0, 0, 0);
  page_game_layout->setSpacing(0);
  setLayout(page_game_layout);
  game_tab_widget->init();
}

page_game::~page_game() {}

/**********************************************************************/ /**
   Reloads sidebar icons (useful on theme change)
 **************************************************************************/
void page_game::reload_sidebar_icons()
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
void page_game::update_sidebar_tooltips()
{
  QString str;
  int max;
  int entries_used, building_total, unit_total, tax;
  char buf[256];

  struct improvement_entry building_entries[B_LAST];
  struct unit_entry unit_entries[U_LAST];

  if (gui()->current_page() != PAGE_GAME) {
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

page_game *queen() {
  return qobject_cast<page_game *>(gui()->pages[PAGE_GAME]);
}