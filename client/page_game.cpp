/******  ,   ,  **********************************************************
         \\  \\             Copyright (c) 1996-2023 Freeciv21 and Freeciv
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
#include <QIcon>
#include <QResizeEvent>
#include <QScreen>

// utility
#include "fcintl.h"
// common
#include "calendar.h"
#include "cityrep_g.h"
#include "government.h"
#include "repodlgs_g.h"
// client
#include "chatline.h"
#include "citydlg.h"
#include "client_main.h"
#include "fc_client.h"
#include "gotodlg.h"
#include "hudwidget.h"
#include "icons.h"
#include "mapctrl_common.h"
#include "messagewin.h"
#include "minimap.h"
#include "minimap_panel.h"
#include "ratesdlg.h"
#include "text.h"
#include "tileset/tilespec.h"
#include "top_bar.h"
#include "views/view_map.h"
#include "views/view_map_common.h"
#include "views/view_nations.h"
#include "views/view_units.h"

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
  sw_map = new top_bar_widget(Q_("?noun:View"), QStringLiteral("MAP"),
                              top_bar_show_map);

  // Show the national flag, unless global observing
  if (client.conn.playing != nullptr) {
    auto sprite = *get_nation_flag_sprite(
        tileset, nation_of_player(client.conn.playing));
    sw_map->setIcon(QIcon(sprite));
    sw_map->setIconSize(sprite.size());
  } else {
    sw_map->setIcon(fcIcons::instance()->getIcon(QStringLiteral("globe")));
    sw_map->setIconSize(QSize(24, 24));
  }

  // Units view (F2)
  sw_cunit = new top_bar_widget(_("Units"), QStringLiteral("UNI"),
                                top_bar_units_view);
  sw_cunit->setIcon(fcIcons::instance()->getIcon(QStringLiteral("units")));
  sw_cunit->setWheelUp(cycle_enemy_units);
  sw_cunit->setWheelDown(key_unit_wait);
  sw_cunit->setCheckable(true);

  // Nations view (F3)
  sw_diplo = new top_bar_widget(_("Nations"), QStringLiteral("PLR"),
                                popup_players_dialog);
  sw_diplo->setIcon(fcIcons::instance()->getIcon(QStringLiteral("flag")));
  sw_diplo->setWheelUp(center_next_player_capital);
  sw_diplo->setWheelDown(key_center_capital);
  sw_diplo->setCheckable(true);
  sw_diplo->setRightClick(top_bar_right_click_diplomacy);

  // Cities view (F4)
  sw_cities = new top_bar_widget(_("Cities"), QStringLiteral("CTS"),
                                 city_report_dialog_popup);
  sw_cities->setIcon(fcIcons::instance()->getIcon(QStringLiteral("cities")));
  sw_cities->setWheelUp(center_next_enemy_city);
  sw_cities->setWheelDown(center_next_player_city);
  sw_cities->setCheckable(true);

  // Economics view (F5)
  sw_economy = new gold_widget;
  sw_economy->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("economy")));
  sw_economy->setCheckable(true);

  // Research view (F6)
  sw_science = new top_bar_widget(_("Research"), QStringLiteral("SCI"),
                                  top_bar_left_click_science);
  sw_science->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("research")));
  sw_science->setCheckable(true);
  sw_science->setRightClick(top_bar_right_click_science);

  // National budget widget
  sw_tax = new national_budget_widget();
  connect(sw_tax, &QAbstractButton::clicked, this,
          &pageGame::popup_budget_dialog);

  // National status
  sw_indicators = new indicators_widget();
  connect(sw_indicators, &QAbstractButton::clicked, top_bar_indicators_menu);

  // Messages widget
  message = new message_widget(mapview_wdg);
  message->setAttribute(Qt::WA_NoMousePropagation);
  message->hide();
  sw_message = new top_bar_widget(_("Messages"), QLatin1String(""), nullptr);
  sw_message->setCheckable(true);
  connect(
      sw_message, &QToolButton::toggled, +[](bool checked) {
        if (const auto message = queen()->message; message) {
          message->setVisible(checked);

          // change icon to default if icon is notify
          const auto sw_message = queen()->sw_message;
          if (sw_message) {
            queen()->sw_message->setIcon(
                fcIcons::instance()->getIcon(QStringLiteral("messages")));
          }
        }
      });
  sw_message->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("messages")));
  connect(
      message, &message_widget::add_msg, +[] {
        const auto message = queen()->message;
        if (message && !message->isVisible()) {
          queen()->sw_message->setIcon(
              fcIcons::instance()->getIcon(QStringLiteral("notify")));
        }
      });

  // The mini-map widget
  minimap_panel = new ::minimap_panel(mapview_wdg, mapview_wdg);
  city_overlay = new city_dialog(mapview_wdg);
  connect(mapview_wdg, &map_view::scale_changed, city_overlay,
          &city_dialog::refresh);
  city_overlay->hide();

  // Battle log widget
  unitinfo_wdg = new hud_units(mapview_wdg);
  unitinfo_wdg->setAttribute(Qt::WA_NoMousePropagation);
  battlelog_wdg = new hud_battle_log(mapview_wdg);
  battlelog_wdg->setAttribute(Qt::WA_NoMousePropagation);
  battlelog_wdg->hide();

  // Chatline widget
  chat = new chat_widget(mapview_wdg);
  chat->setAttribute(Qt::WA_NoMousePropagation);
  chat->show();

  // Voting bar widget
  x_vote = new xvote(mapview_wdg);
  x_vote->setAttribute(Qt::WA_NoMousePropagation);
  x_vote->hide();

  // Goto visuals
  gtd = new goto_dialog(mapview_wdg);
  gtd->setAttribute(Qt::WA_NoMousePropagation);
  gtd->hide();

  top_bar_wdg->addWidget(sw_map);     // F1
  top_bar_wdg->addWidget(sw_cunit);   // F2
  top_bar_wdg->addWidget(sw_diplo);   // F3
  top_bar_wdg->addWidget(sw_cities);  // F4
  top_bar_wdg->addWidget(sw_economy); // F5
  top_bar_wdg->addWidget(sw_science); // F6
  top_bar_wdg->addWidget(sw_tax);
  top_bar_wdg->addWidget(sw_indicators);
  top_bar_wdg->addWidget(sw_message);

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

  budget_dialog = new national_budget_dialog(this);
}

pageGame::~pageGame() = default;

/**
   Reloads top bar icons (useful on theme change)
 */
void pageGame::reloadSidebarIcons()
{
  if (client.conn.playing != nullptr) {
    auto sprite = *get_nation_flag_sprite(
        tileset, nation_of_player(client.conn.playing));
    sw_map->setIcon(QIcon(sprite));
    sw_map->setIconSize(sprite.size());
  } else {
    sw_map->setIcon(fcIcons::instance()->getIcon(QStringLiteral("globe")));
    sw_map->setIconSize(QSize(24, 24));
  }

  sw_cunit->setIcon(fcIcons::instance()->getIcon(QStringLiteral("units")));
  sw_cities->setIcon(fcIcons::instance()->getIcon(QStringLiteral("cities")));
  sw_diplo->setIcon(fcIcons::instance()->getIcon(
      diplomacy_notify ? QStringLiteral("flag-active")
                       : QStringLiteral("flag")));
  sw_science->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("research")));
  sw_economy->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("economy")));
  sw_message->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("messages")));
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
    reloadSidebarIcons();
    return;
  }
}

/**
 * Popup (or raise) the (tax/science/luxury) rates selection dialog.
 */
void pageGame::popup_budget_dialog()
{
  if (client_is_observer()) {
    // Can't change tax rates
    return;
  }

  budget_dialog->refresh();

  const auto rect = screen()->geometry();
  auto p = sw_tax->mapToGlobal(QPoint(0, sw_tax->height()));
  if (p.y() + budget_dialog->height() > rect.bottom()) {
    p.setY(rect.bottom() - budget_dialog->height());
  }
  if (p.x() + budget_dialog->width() > rect.right()) {
    p.setX(rect.right() - budget_dialog->width());
  }

  budget_dialog->move(p);
  budget_dialog->show();
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
  reloadSidebarIcons();

  if (client.conn.playing != nullptr) {
    sw_economy->set_gold(client.conn.playing->economic.gold);
    sw_economy->set_income(player_get_expected_income(client.conn.playing));
    sw_economy->setEnabled(true);
  } else {
    sw_economy->set_gold(0);
    sw_economy->set_income(0);
    sw_economy->setEnabled(false);
  }

  sw_indicators->update();
  sw_tax->update();
  sw_economy->update();
  delete update_info_timer;
  update_info_timer = nullptr;
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

  if (nullptr != client.conn.playing) {
    max = get_player_bonus(client.conn.playing, EFT_MAX_RATES);
  } else {
    max = 100;
  }

  if (client.conn.playing && !client_is_global_observer()
      && C_S_RUNNING == client_state()) {
    sw_science->setToolTip(science_dialog_text());
    str = QString(nation_plural_for_player(client_player()));
    str = str + '\n' + get_info_label_text(false);
    sw_map->setToolTip(str);
    reloadSidebarIcons();
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
    sw_economy->setToolTip(buf);
    sw_cities->setToolTip(QString(_("Cities: %1 total"))
                              .arg(city_list_size(client_player()->cities)));
    sw_science->show();
    sw_economy->show();
  } else {
    sw_science->hide();
    sw_map->setToolTip(QLatin1String(""));
    sw_economy->hide();
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
          queen()->mapview_wdg->center_on_tile(pcity->tile);
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
    queen()->mapview_wdg->center_on_tile(ptile);
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
          queen()->mapview_wdg->center_on_tile(pcity->tile);
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
    queen()->mapview_wdg->center_on_tile(ptile);
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
        queen()->mapview_wdg->center_on_tile(capital->tile);
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
    queen()->mapview_wdg->center_on_tile(ptile);
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
          queen()->mapview_wdg->center_on_tile(punit->tile);
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
    queen()->mapview_wdg->center_on_tile(ptile);
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
bool fc_game_tab_widget::event(QEvent *event)
{
  if (C_S_RUNNING <= client_state()
      && (event->type() == QEvent::Resize
          || event->type() == QEvent::LayoutRequest)) {
    const auto size = event->type() == QEvent::Resize
                          ? static_cast<QResizeEvent *>(event)->size()
                          : this->size();
    if (event->type() == QEvent::Resize) {
      queen()->message->resize(
          qRound(size.width() * king()->qt_settings.chat_fwidth),
          qRound(size.height() * king()->qt_settings.chat_fheight));
      queen()->message->move(size.width() - queen()->message->width(), 0);

      auto chat = queen()->chat;
      const bool visible = chat->is_chat_visible(); // Save old state
      chat->set_chat_visible(true);
      chat->resize(qRound(size.width() * king()->qt_settings.chat_fwidth),
                   qRound(size.height() * king()->qt_settings.chat_fheight));
      chat->move(qRound(size.width() * king()->qt_settings.chat_fx_pos),
                 qRound(size.height() * king()->qt_settings.chat_fy_pos));
      chat->set_chat_visible(visible); // Restore state

      queen()->battlelog_wdg->set_scale(king()->qt_settings.battlelog_scale);
      queen()->battlelog_wdg->move(
          qRound(king()->qt_settings.battlelog_x * mapview.width),
          qRound(king()->qt_settings.battlelog_y * mapview.height));
      queen()->x_vote->move(width() / 2 - queen()->x_vote->width() / 2, 0);

      queen()->updateSidebarTooltips();
      queen()->minimap_panel->turn_done()->setEnabled(
          get_turn_done_button_state());
      queen()->mapview_wdg->resize(size.width(), size.height());
      queen()->city_overlay->resize(queen()->mapview_wdg->size());
      queen()->unitinfo_wdg->update_actions();
    }

    /*
     * Resize the panel at the bottom right.
     * Keep current size if the widget has been resized.
     */
    const auto max_size = QSize(std::max(300, size.width() / 4),
                                std::max(200, size.height() / 3));
    const auto curr_size = QSize(queen()->minimap_panel->width(),
                                 queen()->minimap_panel->height());
    if (curr_size.width() != max_size.width()
        || curr_size.height() != max_size.height()) {
      const auto panel_size =
          QLayout::closestAcceptableSize(queen()->minimap_panel, curr_size);
      const auto location = size - panel_size;
      queen()->minimap_panel->move(location.width(), location.height());
      queen()->minimap_panel->resize(panel_size);
    } else {
      const auto panel_size =
          QLayout::closestAcceptableSize(queen()->minimap_panel, max_size);
      const auto location = size - panel_size;
      queen()->minimap_panel->move(location.width(), location.height());
      queen()->minimap_panel->resize(panel_size);
    }

    return true;
  }

  return QWidget::event(event);
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

  return w != nullptr;
}

/**
   Typically an info box is provided to tell the player about the state
   of their civilization.  This function is called when the label is
   changed.
 */
void update_info_label(void) { queen()->updateInfoLabel(); }
