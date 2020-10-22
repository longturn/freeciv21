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
static void cycle_enemy_units();
int last_center_enemy = 0;


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
   spawn a server, if there isn't one, using the default settings.
 **************************************************************************/
void fc_client::start_new_game()
{
  if (is_server_running() || client_start_server()) {
    /* saved settings are sent in client/options.c load_settable_options() */
  }
}


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
