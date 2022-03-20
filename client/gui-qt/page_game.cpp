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
#include <QResizeEvent>
// utility
#include "fcintl.h"
// common
#include "calendar.h"
#include "cityrep_g.h"
#include "government.h"
#include "repodlgs_g.h"
// client
#include "civstatus.h"
#include "client_main.h"
#include "climisc.h"
#include "mapview_common.h"
#include "text.h"
// gui-qt - Eye of Storm
#include "chatline.h"
#include "citydlg.h"
#include "fc_client.h"
#include "gotodlg.h"
#include "hudwidget.h"
#include "icons.h"
#include "mapctrl_common.h"
#include "mapview.h"
#include "messagewin.h"
#include "minimap.h"
#include "plrdlg.h"
#include "top_bar.h"
#include "voteinfo_bar.h"

int last_center_capital = 0;
int last_center_player_city = 0;
int last_center_enemy_city = 0;
int last_center_enemy = 0;

static void center_next_enemy_city();
static void center_next_player_city();
static void center_next_player_capital();
static void cycle_enemy_units();

extern void toggle_units_report();

pageGame::pageGame(QWidget *parent)
    : QWidget(parent), unit_selector(nullptr), update_info_timer(nullptr)
{
  QGridLayout *game_layout;

  game_main_widget = new QWidget;
  game_layout = new QGridLayout;
  game_layout->setContentsMargins(0, 0, 0, 0);
  game_layout->setSpacing(0);
  mapview_wdg = new map_view();
  mapview_wdg->setFocusPolicy(Qt::WheelFocus);
  top_bar_wdg = new top_bar();
  sw_map = new top_bar_widget(
      Q_("?noun:View"), QStringLiteral("MAP"), top_bar_show_map);
  sw_map->setIcon(fcIcons::instance()->getIcon(QStringLiteral("view")));

  sw_tax = new tax_rates_widget();
  connect(sw_tax, &QAbstractButton::clicked, top_bar_rates_wdg);

  sw_indicators = new indicators_widget();
  connect(sw_indicators, &QAbstractButton::clicked, top_bar_indicators_menu);

  sw_cunit =
      new top_bar_widget(_("Units"), QLatin1String(""), toggle_units_report);
  sw_cunit->setIcon(fcIcons::instance()->getIcon(QStringLiteral("units")));
  sw_cities = new top_bar_widget(_("Cities"), QStringLiteral("CTS"),
                                 city_report_dialog_popup);
  sw_cities->setIcon(fcIcons::instance()->getIcon(QStringLiteral("cities")));
  sw_cities->setWheelUp(center_next_enemy_city);
  sw_cities->setWheelDown(center_next_player_city);
  sw_cities->setCheckable(true);
  sw_diplo = new top_bar_widget(
      _("Nations"), QStringLiteral("PLR"), popup_players_dialog);
  sw_diplo->setIcon(fcIcons::instance()->getIcon(QStringLiteral("nations")));
  sw_diplo->setWheelUp(center_next_player_capital);
  sw_diplo->setWheelDown(key_center_capital);
  sw_diplo->setCheckable(true);
  sw_science = new top_bar_widget(
      _("Research"), QStringLiteral("SCI"), top_bar_left_click_science);
  sw_science->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("research")));
  sw_science->setCheckable(true);
  sw_economy = new top_bar_widget(
      _("Economy"), QStringLiteral("ECO"), economy_report_dialog_popup);
  sw_economy->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("economy")));
  sw_economy->setCheckable(true);
  sw_endturn = new top_bar_widget(
      _("Turn Done"), QLatin1String(""), top_bar_finish_turn);
  sw_endturn->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("endturn")));
  sw_cunit->setRightClick(top_bar_center_unit);
  sw_cunit->setWheelUp(cycle_enemy_units);
  sw_cunit->setWheelDown(key_unit_wait);
  sw_diplo->setRightClick(top_bar_right_click_diplomacy);
  sw_science->setRightClick(top_bar_right_click_science);

  top_bar_wdg->addWidget(sw_map);
  top_bar_wdg->addWidget(sw_cunit);
  top_bar_wdg->addWidget(sw_cities);
  top_bar_wdg->addWidget(sw_diplo);
  top_bar_wdg->addWidget(sw_science);
  top_bar_wdg->addWidget(sw_economy);
  top_bar_wdg->addWidget(sw_tax);
  top_bar_wdg->addWidget(sw_indicators);
  top_bar_wdg->addWidget(sw_endturn);

  civ_status = new civstatus(mapview_wdg);
  civ_status->setAttribute(Qt::WA_NoMousePropagation);
  civ_status->show();

  city_overlay = new city_dialog(mapview_wdg);
  connect(mapview_wdg, &map_view::scale_changed, city_overlay,
          &city_dialog::refresh);
  city_overlay->hide();
  minimapview_wdg = new minimap_view(mapview_wdg);
  minimapview_wdg->setAttribute(Qt::WA_NoMousePropagation);
  minimapview_wdg->show();
  unitinfo_wdg = new hud_units(mapview_wdg);
  unitinfo_wdg->setAttribute(Qt::WA_NoMousePropagation);
  battlelog_wdg = new hud_battle_log(mapview_wdg);
  battlelog_wdg->setAttribute(Qt::WA_NoMousePropagation);
  battlelog_wdg->hide();
  message = new message_widget(mapview_wdg);
  message->setAttribute(Qt::WA_NoMousePropagation);
  message->show();
  chat = new chat_widget(mapview_wdg);
  chat->setAttribute(Qt::WA_NoMousePropagation);
  chat->show();
  x_vote = new xvote(mapview_wdg);
  x_vote->setAttribute(Qt::WA_NoMousePropagation);
  x_vote->hide();
  gtd = new goto_dialog(mapview_wdg);
  gtd->setAttribute(Qt::WA_NoMousePropagation);
  gtd->hide();

  game_layout->addWidget(mapview_wdg, 1, 0);
  game_main_widget->setLayout(game_layout);
  game_tab_widget = new fc_game_tab_widget;
  game_tab_widget->setMinimumSize(600, 400);
  game_tab_widget->setContentsMargins(0, 0, 0, 0);

  game_tab_widget->addWidget(game_main_widget);

  auto page_game_layout = new QVBoxLayout;
  page_game_layout->addWidget(top_bar_wdg);
  page_game_layout->setStretchFactor(top_bar_wdg, 0);
  page_game_layout->addWidget(game_tab_widget);
  page_game_layout->setStretchFactor(game_tab_widget, 1);
  page_game_layout->setContentsMargins(0, 0, 0, 0);
  page_game_layout->setSpacing(0);
  setLayout(page_game_layout);

  game_tab_widget->init();
}

pageGame::~pageGame() = default;

/**
   Reloads top bar icons (useful on theme change)
 */
void pageGame::reloadSidebarIcons()
{
  sw_map->setIcon(fcIcons::instance()->getIcon(QStringLiteral("view")));
  sw_cunit->setIcon(fcIcons::instance()->getIcon(QStringLiteral("units")));
  sw_cities->setIcon(fcIcons::instance()->getIcon(QStringLiteral("cities")));
  sw_diplo->setIcon(fcIcons::instance()->getIcon(QStringLiteral("nations")));
  sw_science->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("research")));
  sw_economy->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("economy")));
  sw_endturn->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("endturn")));
}

/**
   Real update, updates only once per 300 ms.
 */
void pageGame::updateInfoLabel()
{
  if (king()->current_page() != PAGE_GAME) {
    return;
  }
  if (update_info_timer == nullptr) {
    update_info_timer = new QTimer();
    update_info_timer->setSingleShot(true);
    connect(update_info_timer, &QTimer::timeout, this,
            &pageGame::updateInfoLabelTimeout);
    update_info_timer->start(300);
    return;
  }
}

void pageGame::updateInfoLabelTimeout()
{
  QString s, eco_info;
  if (update_info_timer->remainingTime() != -1) {
    return;
  }
  updateSidebarTooltips();
  if (head_of_units_in_focus() != nullptr) {
    real_menus_update();
  }
  // TRANS: T is shortcut from Turn
  s = QString(_("%1 \nT:%2"))
          .arg(calendar_text(), QString::number(game.info.turn));

  sw_map->setCustomLabels(s);
  sw_map->update();

  if (client.conn.playing != NULL) {
    if (player_get_expected_income(client.conn.playing) > 0) {
      eco_info =
          QString(_("%1 (+%2)"))
              .arg(QString::number(client.conn.playing->economic.gold),
                   QString::number(
                       player_get_expected_income(client.conn.playing)));
    } else {
      eco_info =
          QString(_("%1 (%2)"))
              .arg(QString::number(client.conn.playing->economic.gold),
                   QString::number(
                       player_get_expected_income(client.conn.playing)));
    }
    sw_economy->setCustomLabels(eco_info);
  } else {
    sw_economy->setCustomLabels(QLatin1String(""));
  }
  sw_indicators->update();
  sw_tax->update();
  sw_economy->update();
  FC_FREE(update_info_timer);
}

/**
   Updates top bar tooltips
 */
void pageGame::updateSidebarTooltips()
{
  QString str;
  int max;
  int entries_used, building_total, unit_total, tax;
  char buf[256];

  struct improvement_entry building_entries[B_LAST];
  struct unit_entry unit_entries[U_LAST];

  if (king()->current_page() != PAGE_GAME) {
    return;
  }

  if (NULL != client.conn.playing) {
    max = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  } else {
    max = 100;
  }

  if (client.conn.playing && !client_is_global_observer()
      && C_S_RUNNING == client_state()) {
    sw_science->setTooltip(science_dialog_text());
    str = QString(nation_plural_for_player(client_player()));
    str = str + '\n' + get_info_label_text(false);
    sw_map->setTooltip(str);
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
    sw_economy->setTooltip(buf);
    if (player_primary_capital(client_player())) {
      sw_cities->setTooltip(
          text_happiness_cities(player_primary_capital(client_player())));
    }
  } else {
    sw_science->setTooltip(QLatin1String(""));
    sw_map->setTooltip(QLatin1String(""));
    sw_economy->setTooltip(QLatin1String(""));
  }
  sw_indicators->setToolTip(get_info_label_text_popup());
}

/**
   Centers next enemy city on view
 */
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

/**
   Centers next player city on view
 */
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

/**
   Centers next enemy capital
 */
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

/**
   Center on next enemy unit
 */
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

pageGame *queen()
{
  return qobject_cast<pageGame *>(king()->pages[PAGE_GAME]);
}

/**
   Game tab widget constructor
 */
fc_game_tab_widget::fc_game_tab_widget() : QStackedWidget() {}

/**
   Init default settings for game_tab_widget
 */
void fc_game_tab_widget::init()
{
  connect(this, &QStackedWidget::currentChanged, this,
          &fc_game_tab_widget::current_changed);
}

/**
   Resize event for all game tab widgets
 */
void fc_game_tab_widget::resizeEvent(QResizeEvent *event)
{
  QSize size;
  size = event->size();
  if (C_S_RUNNING <= client_state()) {
    map_canvas_resized(size.width(), size.height());
    queen()->message->resize(
        qRound((size.width() * king()->qt_settings.chat_fwidth)),
        qRound((size.height() * king()->qt_settings.chat_fheight)));
    queen()->message->move(0, 0);
    queen()->chat->resize(
        qRound((size.width() * king()->qt_settings.chat_fwidth)),
        qRound((size.height() * king()->qt_settings.chat_fheight)));
    queen()->chat->move(
        qRound((size.width() * king()->qt_settings.chat_fx_pos)),
        qRound((size.height() * king()->qt_settings.chat_fy_pos)));
    queen()->minimapview_wdg->move(
        qRound(king()->qt_settings.minimap_x * mapview.width),
        qRound(king()->qt_settings.minimap_y * mapview.height));
    queen()->minimapview_wdg->resize(
        qRound(king()->qt_settings.minimap_width * mapview.width),
        qRound(king()->qt_settings.minimap_height * mapview.height));
    queen()->battlelog_wdg->set_scale(king()->qt_settings.battlelog_scale);
    queen()->battlelog_wdg->move(
        qRound(king()->qt_settings.battlelog_x * mapview.width),
        qRound(king()->qt_settings.battlelog_y * mapview.height));
    queen()->x_vote->move(width() / 2 - queen()->x_vote->width() / 2, 0);
    queen()->updateSidebarTooltips();
    top_bar_disable_end_turn(get_turn_done_button_state());
    queen()->mapview_wdg->resize(event->size().width(), size.height());
    queen()->city_overlay->resize(queen()->mapview_wdg->size());
    queen()->unitinfo_wdg->update_actions(nullptr);
    queen()->civ_status->move(
        qRound(king()->qt_settings.civstatus_x * mapview.width),
        qRound(king()->qt_settings.civstatus_y * mapview.height));
    // It could be resized before mapview, so delayed it a bit
  }
  event->setAccepted(true);
}

/**
   Tab has been changed
 */
void fc_game_tab_widget::current_changed(int index)
{
  if (king()->is_closing()) {
    return;
  }

  for (auto *sw : qAsConst(queen()->top_bar_wdg->objects)) {
    sw->update();
  }
  currentWidget()->hide();
  widget(index)->show();

  // Set focus to map instead sidebar
  if (queen()->mapview_wdg && king()->current_page() == PAGE_GAME
      && index == 0) {
    queen()->mapview_wdg->setFocus();
  }
}

/**
   Inserts tab widget to game view page
 */
int pageGame::addGameTab(QWidget *widget)
{
  int i;

  i = game_tab_widget->addWidget(widget);
  game_tab_widget->setCurrentWidget(widget);
  return i;
}

/**
   Removes given tab widget from game page
 */
void pageGame::rmGameTab(int index)
{
  game_tab_widget->removeWidget(queen()->game_tab_widget->widget(index));
}

/**
   Finds not used index on game_view_tab and returns it
 */
void pageGame::gimmePlace(QWidget *widget, const QString &str)
{
  QString x;

  x = opened_repo_dlgs.key(widget);

  if (x.isEmpty()) {
    opened_repo_dlgs.insert(str, widget);
    return;
  }
  qCritical("Failed to find place for new tab widget");
}

/**
   Returns index on game tab page of given report dialog
 */
int pageGame::gimmeIndexOf(const QString &str)
{
  int i;
  QWidget *w;

  if (str == QLatin1String("MAP")) {
    return 0;
  }

  w = opened_repo_dlgs.value(str);
  i = queen()->game_tab_widget->indexOf(w);
  return i;
}

/**
   Removes report dialog string from the list marking it as closed
 */
void pageGame::removeRepoDlg(const QString &str)
{
  // if app is closing opened_repo_dlg is already deleted
  if (!king()->is_closing()) {
    opened_repo_dlgs.remove(str);
  }
}

/**
   Checks if given report is opened, if you create new report as tab on game
   page, figure out some original string and put in in repodlg.h as comment
 to that QWidget class.
 */
bool pageGame::isRepoDlgOpen(const QString &str)
{
  QWidget *w;

  w = opened_repo_dlgs.value(str);

  return w != NULL;
}

/**
   Typically an info box is provided to tell the player about the state
   of their civilization.  This function is called when the label is
   changed.
 */
void update_info_label(void) { queen()->updateInfoLabel(); }
