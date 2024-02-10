/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "diplodlg.h"

#include <limits>

// Qt
#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
// utility
#include "fcintl.h"
// common
#include "climisc.h"
#include "game.h"
#include "government.h"
#include "nation.h"
#include "player.h"
#include "research.h"
// client
#include "client_main.h"
#include "colors_common.h"
#include "diplodlg_g.h"
#include "fc_client.h"
#include "icons.h"
#include "page_game.h"
#include "tileset/sprite.h"
#include "top_bar.h"
#include "views/view_nations.h"

/**
   Constructor for diplomacy widget
 */
diplo_wdg::diplo_wdg(int counterpart, int initiated_from)
    : QWidget(), active_menu(0), curr_player(0), index(0)
{
  QString text;
  QString text2;
  QString text_tooltip;
  QLabel *plr1_label;
  QLabel *plr2_label;
  QLabel *label;
  QLabel *label2;
  QLabel *label3;
  QLabel *label4;
  QLabel *goldlab1;
  QLabel *goldlab2;
  QPushButton *add_clause1;
  QPushButton *add_clause2;
  const QPixmap *sprite, *sprite2;
  char plr_buf[4 * MAX_LEN_NAME];
  const struct player_diplstate *state;
  QHeaderView *header;
  QColor textcolors[2] = {get_color(tileset, COLOR_MAPVIEW_CITYTEXT),
                          get_color(tileset, COLOR_MAPVIEW_CITYTEXT_DARK)};
  if (counterpart == initiated_from) {
    initiated_from = client_player_number();
  }
  p1_accept = false;
  p2_accept = false;
  player1 = initiated_from;
  player2 = counterpart;
  layout = new QGridLayout;

  init_treaty(&treaty, player_by_number(counterpart),
              player_by_number(initiated_from));
  state = player_diplstate_get(player_by_number(player1),
                               player_by_number(player2));
  text_tooltip = QString(diplstate_type_translated_name(state->type));
  if (state->turns_left > 0) {
    text_tooltip = text_tooltip + " (";
    text_tooltip =
        text_tooltip
        + QString(PL_("%1 turn left", "%1 turns left", state->turns_left))
              .arg(state->turns_left);
    text_tooltip = text_tooltip + ")";
  }
  label3 = new QLabel;
  text =
      "<b><h3><center>"
      + QString(nation_plural_for_player(player_by_number(initiated_from)))
            .toHtmlEscaped()
      + "</center></h3></b>";
  auto color = get_player_color(tileset, player_by_number(initiated_from));
  text =
      "<style>h3{background-color: " + color.name() + ";" + "color: "
      + color_best_contrast(color, textcolors, ARRAY_SIZE(textcolors)).name()
      + "}</style>" + text;
  label3->setText(text);
  label3->setMinimumWidth(300);
  label4 = new QLabel;
  text = "<b><h3><center>"
         + QString(nation_plural_for_player(player_by_number(counterpart)))
               .toHtmlEscaped()
         + "</center></h3></b></body>";
  color = get_player_color(tileset, player_by_number(counterpart));
  text =
      "<style>h3{background-color: " + color.name() + ";" + "color: "
      + color_best_contrast(color, textcolors, ARRAY_SIZE(textcolors)).name()
      + "}</style>" + text;
  label4->setMinimumWidth(300);
  label4->setText(text);
  layout->addWidget(label3, 0, 5);
  layout->addWidget(label4, 5, 5);
  plr1_label = new QLabel;
  label = new QLabel;
  sprite = get_nation_flag_sprite(
      tileset, nation_of_player(player_by_number(initiated_from)));
  if (sprite != nullptr) {
    plr1_label->setPixmap(*sprite);
  } else {
    plr1_label->setText(QStringLiteral("FLAG MISSING"));
  }
  text = ruler_title_for_player(player_by_number(initiated_from), plr_buf,
                                sizeof(plr_buf));
  text = "<b><center>" + text.toHtmlEscaped() + "</center></b>";
  label->setText(text);
  plr1_accept = new QLabel;
  layout->addWidget(plr1_label, 1, 0);
  layout->addWidget(label, 1, 5);
  layout->addWidget(plr1_accept, 1, 10);
  label2 = new QLabel;
  sprite2 = get_nation_flag_sprite(
      tileset, nation_of_player(player_by_number(counterpart)));
  plr2_label = new QLabel;
  if (sprite2 != nullptr) {
    plr2_label->setPixmap(*sprite2);
  } else {
    plr2_label->setText(QStringLiteral("FLAG MISSING"));
  }
  text2 = ruler_title_for_player(player_by_number(counterpart), plr_buf,
                                 sizeof(plr_buf));
  text2 = "<b><center>" + text2.toHtmlEscaped() + "</center></b>";
  label2->setText(text2);
  plr2_accept = new QLabel;
  layout->addWidget(plr2_label, 6, 0);
  layout->addWidget(label2, 6, 5);
  layout->addWidget(plr2_accept, 6, 10);
  goldlab1 = new QLabel(_("Gold:"));
  goldlab1->setAlignment(Qt::AlignRight);
  goldlab2 = new QLabel(_("Gold:"));
  goldlab2->setAlignment(Qt::AlignRight);
  gold_edit1 = new QSpinBox;
  gold_edit2 = new QSpinBox;
  gold_edit1->setMinimum(0);
  gold_edit2->setMinimum(0);
  gold_edit1->setMaximum(std::numeric_limits<std::int32_t>::max());
  gold_edit2->setMaximum(std::numeric_limits<std::int32_t>::max());
  gold_edit1->setFocusPolicy(Qt::ClickFocus);
  gold_edit2->setFocusPolicy(Qt::ClickFocus);
  gold_edit1->setEnabled(game.info.trading_gold);
  gold_edit2->setEnabled(game.info.trading_gold);
  connect(gold_edit1, qOverload<int>(&QSpinBox::valueChanged), this,
          &diplo_wdg::gold_changed1);
  connect(gold_edit2, qOverload<int>(&QSpinBox::valueChanged), this,
          &diplo_wdg::gold_changed2);
  add_clause1 = new QPushButton(style()->standardIcon(QStyle::SP_ArrowRight),
                                _("Add Clause..."));
  add_clause2 = new QPushButton(style()->standardIcon(QStyle::SP_ArrowRight),
                                _("Add Clause..."));
  add_clause1->setFocusPolicy(Qt::ClickFocus);
  add_clause2->setFocusPolicy(Qt::ClickFocus);
  connect(add_clause1, &QAbstractButton::clicked, this,
          &diplo_wdg::show_menu_p2);
  connect(add_clause2, &QAbstractButton::clicked, this,
          &diplo_wdg::show_menu_p1);
  layout->addWidget(goldlab1, 7, 4);
  layout->addWidget(goldlab2, 3, 4);
  layout->addWidget(gold_edit1, 7, 5);
  layout->addWidget(gold_edit2, 3, 5);
  layout->addWidget(add_clause1, 7, 6);
  layout->addWidget(add_clause2, 3, 6);

  text_edit = new QTableWidget();
  text_edit->setColumnCount(1);
  text_edit->setProperty("showGrid", "false");
  text_edit->setProperty("selectionBehavior", "SelectRows");
  text_edit->setEditTriggers(QAbstractItemView::NoEditTriggers);
  text_edit->verticalHeader()->setVisible(false);
  text_edit->horizontalHeader()->setVisible(false);
  text_edit->setSelectionMode(QAbstractItemView::SingleSelection);
  text_edit->setFocusPolicy(Qt::NoFocus);
  header = text_edit->horizontalHeader();
  header->setStretchLastSection(true);
  connect(text_edit, &QTableWidget::itemDoubleClicked, this,
          &diplo_wdg::dbl_click);
  text_edit->clearContents();
  text_edit->setRowCount(0);
  layout->addWidget(text_edit, 9, 0, 8, 11);
  accept_treaty = new QPushButton(
      style()->standardIcon(QStyle::SP_DialogYesButton), _("Accept treaty"));
  cancel_treaty = new QPushButton(
      style()->standardIcon(QStyle::SP_DialogNoButton), _("Cancel meeting"));
  connect(accept_treaty, &QAbstractButton::clicked, this,
          &diplo_wdg::response_accept);
  connect(cancel_treaty, &QAbstractButton::clicked, this,
          &diplo_wdg::response_cancel);
  layout->addWidget(accept_treaty, 17, 5);
  layout->addWidget(cancel_treaty, 17, 6);

  if (client_player_number() != counterpart) {
    label4->setToolTip(text_tooltip);
    plr2_label->setToolTip(text_tooltip);
  }
  if (client_player_number() != initiated_from) {
    label3->setToolTip(text_tooltip);
    plr1_label->setToolTip(text_tooltip);
  }

  accept_treaty->setAutoDefault(true);
  accept_treaty->setDefault(true);
  cancel_treaty->setAutoDefault(true);
  setLayout(layout);
  update_wdg();
}

/**
   Destructor for diplomacy widget
 */
diplo_wdg::~diplo_wdg() = default;

/**
   Double click on treaty list - it removes clicked clause from list
 */
void diplo_wdg::dbl_click(QTableWidgetItem *item)
{
  int i, r;

  r = item->row();
  i = 0;
  clause_list_iterate(treaty.clauses, pclause)
  {
    if (i == r) {
      dsend_packet_diplomacy_remove_clause_req(
          &client.conn, player_number(treaty.plr0),
          player_number(pclause->from), pclause->type, pclause->value);
      return;
    }
    i++;
  }
  clause_list_iterate_end;
}

/**
   Received event about diplomacy widget being closed
 */
void diplo_wdg::closeEvent(QCloseEvent *event)
{
  if (C_S_RUNNING == client_state()) {
    response_cancel();
  }
  event->accept();
}

/**
   Gold changed on first spinner
 */
void diplo_wdg::gold_changed1(int val)
{
  dsend_packet_diplomacy_create_clause_req(&client.conn, player2, player2,
                                           CLAUSE_GOLD, val);
}

/**
   Gold changed on second spinner
 */
void diplo_wdg::gold_changed2(int val)
{
  dsend_packet_diplomacy_create_clause_req(&client.conn, player2, player1,
                                           CLAUSE_GOLD, val);
}

/**
   Shows popup menu with available clauses to create
 */
void diplo_wdg::show_menu(int player)
{
  int other_player;
  struct player *pgiver, *pother;
  enum diplstate_type ds;
  QAction *all_advancs;
  QAction *some_action;
  QAction *world_map, *sea_map;
  QMap<QString, int> city_list;
  QMap<QString, int>::const_iterator city_iter;
  QMap<QString, Tech_type_id> adv_list;
  QMap<QString, Tech_type_id>::const_iterator adv_iter;
  QMenu *map_menu, *adv_menu, *city_menu, *pacts_menu;
  QMenu *menu = new QMenu(this);
  int id;

  curr_player = player;
  if (curr_player == player1) {
    other_player = player2;
  } else {
    other_player = player1;
  }
  pgiver = player_by_number(player);
  pother = player_by_number(other_player);

  // Maps
  map_menu = menu->addMenu(_("Maps"));
  world_map = new QAction(_("World-map"), this);
  connect(world_map, &QAction::triggered, this,
          &diplo_wdg::world_map_clause);
  map_menu->addAction(world_map);
  sea_map = new QAction(_("Sea-map"), this);
  connect(sea_map, &QAction::triggered, this, &diplo_wdg::sea_map_clause);
  map_menu->addAction(sea_map);

  // Trading: advances
  if (game.info.trading_tech) {
    const struct research *gresearch = research_get(pgiver);
    const struct research *oresearch = research_get(pother);
    adv_menu = menu->addMenu(_("Advances"));
    advance_iterate(A_FIRST, padvance)
    {
      Tech_type_id i = advance_number(padvance);

      if (research_invention_state(gresearch, i) == TECH_KNOWN
          && research_invention_gettable(oresearch, i,
                                         game.info.tech_trade_allow_holes)
          && (research_invention_state(oresearch, i) == TECH_UNKNOWN
              || research_invention_state(oresearch, i)
                     == TECH_PREREQS_KNOWN)) {
        adv_list.insert(advance_name_translation(padvance), i);
      }
    }
    advance_iterate_end;

    // All advances
    all_advancs = new QAction(_("All advances"), this);
    connect(all_advancs, &QAction::triggered, this,
            &diplo_wdg::all_advances);
    adv_menu->addAction(all_advancs);
    adv_menu->addSeparator();

    // QMap is sorted by default when iterating
    adv_iter = adv_list.constBegin();
    if (adv_list.count() > 0) {
      while (adv_iter != adv_list.constEnd()) {
        id = adv_iter.value();
        some_action = adv_menu->addAction(adv_iter.key());
        connect(some_action, &QAction::triggered, this,
                [=]() { give_advance(id); });
        ++adv_iter;
      }
    } else {
      adv_menu->setDisabled(true);
    }
  }

  // Trading: cities.
  if (game.info.trading_city) {
    city_menu = menu->addMenu(_("Cities"));

    city_list_iterate(pgiver->cities, pcity)
    {
      if (!is_capital(pcity)) {
        city_list.insert(pcity->name, pcity->id);
      }
    }
    city_list_iterate_end;
    city_iter = city_list.constBegin();
    if (city_list.count() > 0) {
      while (city_iter != city_list.constEnd()) {
        id = city_iter.value();
        some_action = city_menu->addAction(city_iter.key());
        connect(some_action, &QAction::triggered, this,
                [=]() { give_city(id); });
        ++city_iter;
      }
    } else {
      city_menu->setDisabled(true);
    }
  }
  some_action = new QAction(_("Give shared vision"), this);
  connect(some_action, &QAction::triggered, this,
          &diplo_wdg::give_shared_vision);
  menu->addAction(some_action);
  if (gives_shared_vision(pgiver, pother)) {
    some_action->setDisabled(true);
  }
  some_action = new QAction(_("Give embassy"), this);
  connect(some_action, &QAction::triggered, this, &diplo_wdg::give_embassy);
  menu->addAction(some_action);
  if (player_has_real_embassy(pother, pgiver)) {
    some_action->setDisabled(true);
  }

  // Pacts
  if (player_by_number(curr_player) == client_player()) {
    pacts_menu = menu->addMenu(_("Pacts"));
    ds = player_diplstate_get(pgiver, pother)->type;
    some_action = new QAction(Q_("?diplomatic_state:Cease-fire"), this);
    connect(some_action, &QAction::triggered, this,
            &diplo_wdg::pact_ceasfire);
    pacts_menu->addAction(some_action);
    if (ds == DS_CEASEFIRE || ds == DS_TEAM) {
      some_action->setDisabled(true);
    }
    some_action = new QAction(Q_("?diplomatic_state:Peace"), this);
    connect(some_action, &QAction::triggered, this, &diplo_wdg::pact_peace);
    pacts_menu->addAction(some_action);
    if (ds == DS_PEACE || ds == DS_TEAM) {
      some_action->setDisabled(true);
    }
    some_action = new QAction(Q_("?diplomatic_state:Alliance"), this);
    connect(some_action, &QAction::triggered, this,
            &diplo_wdg::pact_allianze);
    pacts_menu->addAction(some_action);
    if (ds == DS_ALLIANCE || ds == DS_TEAM) {
      some_action->setDisabled(true);
    }
  }

  // Check user response for not defined responses in slots
  menu->setAttribute(Qt::WA_DeleteOnClose);
  menu->popup(QCursor::pos());
}

/**
   Give embassy menu activated
 */
void diplo_wdg::give_embassy()
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(treaty.plr0),
                                           curr_player, CLAUSE_EMBASSY, 0);
}

/**
   Give shared vision menu activated
 */
void diplo_wdg::give_shared_vision()
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(treaty.plr0),
                                           curr_player, CLAUSE_VISION, 0);
}

/**
   Create alliance menu activated
 */
void diplo_wdg::pact_allianze()
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(treaty.plr0),
                                           curr_player, CLAUSE_ALLIANCE, 0);
}

/**
   Ceasefire pact menu activated
 */
void diplo_wdg::pact_ceasfire()
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(treaty.plr0),
                                           curr_player, CLAUSE_CEASEFIRE, 0);
}

/**
   Peace pact menu activated
 */
void diplo_wdg::pact_peace()
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(treaty.plr0),
                                           curr_player, CLAUSE_PEACE, 0);
}

/**
   Sea map menu activated
 */
void diplo_wdg::sea_map_clause()
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
                                           player_number(treaty.plr0),
                                           curr_player, CLAUSE_SEAMAP, 0);
}

/**
   World map menu activated
 */
void diplo_wdg::world_map_clause()
{
  dsend_packet_diplomacy_create_clause_req(
      &client.conn, player_number(treaty.plr0), curr_player, CLAUSE_MAP, 0);
}

/**
   Give city menu activated
 */
void diplo_wdg::give_city(int city_num)
{
  int giver, dest, other;

  giver = curr_player;
  if (curr_player == player1) {
    dest = player2;
  } else {
    dest = player1;
  }

  if (player_by_number(giver) == client_player()) {
    other = dest;
  } else {
    other = giver;
  }

  dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
                                           CLAUSE_CITY, city_num);
}

/**
   Give advance menu activated
 */
void diplo_wdg::give_advance(int tech)
{
  int giver, dest, other;

  giver = curr_player;
  if (curr_player == player1) {
    dest = player2;
  } else {
    dest = player1;
  }

  if (player_by_number(giver) == client_player()) {
    other = dest;
  } else {
    other = giver;
  }

  dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
                                           CLAUSE_ADVANCE, tech);
}

/**
   Give all advances menu activated
 */
void diplo_wdg::all_advances()
{
  int giver, dest, other;
  const struct research *dresearch, *gresearch;

  giver = curr_player;
  if (curr_player == player1) {
    dest = player2;
  } else {
    dest = player1;
  }

  if (player_by_number(giver) == client_player()) {
    other = dest;
  } else {
    other = giver;
  }

  // All techs.
  struct player *pgiver = player_by_number(giver);
  struct player *pdest = player_by_number(dest);

  fc_assert_ret(nullptr != pgiver);
  fc_assert_ret(nullptr != pdest);

  dresearch = research_get(pdest);
  gresearch = research_get(pgiver);

  advance_iterate(A_FIRST, padvance)
  {
    Tech_type_id i = advance_number(padvance);

    if (research_invention_state(gresearch, i) == TECH_KNOWN
        && research_invention_gettable(dresearch, i,
                                       game.info.tech_trade_allow_holes)
        && (research_invention_state(dresearch, i) == TECH_UNKNOWN
            || research_invention_state(dresearch, i)
                   == TECH_PREREQS_KNOWN)) {
      dsend_packet_diplomacy_create_clause_req(&client.conn, other, giver,
                                               CLAUSE_ADVANCE, i);
    }
  }
  advance_iterate_end;
}

/**
   Show menu for second player
 */
void diplo_wdg::show_menu_p2() { show_menu(player2); }

/**
   Show menu for first player
 */
void diplo_wdg::show_menu_p1() { show_menu(player1); }

/**
   Sets index in QTabWidget
 */
void diplo_wdg::set_index(int ind) { index = ind; }

/**
   Sets index in QTabWidget
 */
int diplo_wdg::get_index() { return index; }

/**
   Updates diplomacy widget - updates clauses and redraws pixmaps
 */
void diplo_wdg::update_wdg()
{
  bool blank;
  int i;
  QTableWidgetItem *qitem;

  blank = true;
  text_edit->clearContents();
  text_edit->setRowCount(0);
  i = 0;
  clause_list_iterate(treaty.clauses, pclause)
  {
    char buf[128];
    client_diplomacy_clause_string(buf, sizeof(buf), pclause);
    text_edit->insertRow(i);
    qitem = new QTableWidgetItem();
    qitem->setText(buf);
    qitem->setTextAlignment(Qt::AlignLeft);
    text_edit->setItem(i, 0, qitem);
    blank = false;
    i++;
  }
  clause_list_iterate_end;

  if (blank) {
    text_edit->insertRow(0);
    qitem = new QTableWidgetItem();
    qitem->setText(_("--- This treaty is blank. "
                     "Please add some clauses. ---"));
    qitem->setTextAlignment(Qt::AlignLeft);
    text_edit->setItem(0, 0, qitem);
  }

  auto sprite = get_treaty_thumb_sprite(tileset, treaty.accept0);
  if (sprite != nullptr) {
    plr1_accept->setPixmap(*sprite);
  } else {
    plr1_accept->setText(QStringLiteral("PIXMAP MISSING"));
  }

  sprite = get_treaty_thumb_sprite(tileset, treaty.accept1);
  if (sprite != nullptr) {
    plr2_accept->setPixmap(*sprite);
  } else {
    plr2_accept->setText(QStringLiteral("PIXMAP MISSING"));
  }
}

/**
   Restores original nations pixmap
 */
void diplo_wdg::restore_pixmap()
{
  queen()->sw_diplo->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("nations")));
  queen()->sw_diplo->update();
}

/**
   Button 'Accept treaty' has been clicked
 */
void diplo_wdg::response_accept()
{
  restore_pixmap();
  dsend_packet_diplomacy_accept_treaty_req(&client.conn,
                                           player_number(treaty.plr0));
}

/**
   Button 'Cancel treaty' has been clicked
 */
void diplo_wdg::response_cancel()
{
  restore_pixmap();
  dsend_packet_diplomacy_cancel_meeting_req(&client.conn,
                                            player_number(treaty.plr0));
}

/**
   Constructor for diplomacy dialog
 */
diplo_dlg::diplo_dlg(int counterpart, int initiated_from) : QTabWidget()
{
  add_widget(counterpart, initiated_from);
  setFocusPolicy(Qt::ClickFocus);
}

/**
   Creates new diplomacy widget and adds to diplomacy dialog
 */
void diplo_dlg::add_widget(int counterpart, int initiated_from)
{
  diplo_wdg *dw;
  int i;

  dw = new diplo_wdg(counterpart, initiated_from);
  treaty_list.insert(counterpart, dw);
  i = addTab(dw, nation_plural_for_player(player_by_number(counterpart)));
  dw->set_index(i);
  auto sprite = get_nation_flag_sprite(
      tileset, nation_of_player(player_by_number(counterpart)));
  if (sprite != nullptr) {
    setTabIcon(i, QIcon(*sprite));
  }
}

/**
   Sets given widget as active in current dialog
 */
void diplo_dlg::make_active(int party)
{
  QWidget *w;

  w = find_widget(party);
  if (w == nullptr) {
    return;
  }
  setCurrentWidget(w);
}

/**
   Initializes some data for diplomacy dialog
 */
bool diplo_dlg::init(bool raise)
{
  Q_UNUSED(raise)
  if (!can_client_issue_orders()) {
    return false;
  }
  if (!is_human(client.conn.playing)) {
    return false;
  }
  setAttribute(Qt::WA_DeleteOnClose);
  queen()->gimmePlace(this, QStringLiteral("DDI"));
  index = queen()->addGameTab(this);
  queen()->game_tab_widget->setCurrentIndex(index);

  return true;
}

/**
   Destructor for diplomacy dialog
 */
diplo_dlg::~diplo_dlg()
{
  QMapIterator<int, diplo_wdg *> i(treaty_list);
  diplo_wdg *dw;

  while (i.hasNext()) {
    i.next();
    dw = i.value();
    dw->close();
    removeTab(dw->get_index());
    dw->deleteLater();
  }
  queen()->removeRepoDlg(QStringLiteral("DDI"));
  top_bar_show_map();
}

/**
   Finds diplomacy widget in current dialog
 */
diplo_wdg *diplo_dlg::find_widget(int counterpart)
{
  return treaty_list.value(counterpart);
}

/**
   Closes given diplomacy widget
 */
void diplo_dlg::close_widget(int counterpart)
{
  diplo_wdg *dw;

  dw = treaty_list.take(counterpart);
  removeTab(dw->get_index());
  dw->deleteLater();
  if (treaty_list.isEmpty()) {
    close();
  }
}

/**
   Updates all diplomacy widgets in current dialog
 */
void diplo_dlg::update_dlg()
{
  QMapIterator<int, diplo_wdg *> i(treaty_list);
  diplo_wdg *dw;

  while (i.hasNext()) {
    i.next();
    dw = i.value();
    dw->update_wdg();
  }
}

/**
   Update a player's acceptance status of a treaty (traditionally shown
   with the thumbs-up/thumbs-down sprite).
 */
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
                                    bool other_accepted)
{
  int i;
  diplo_dlg *dd;
  diplo_wdg *dw;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("DDI"))) {
    update_top_bar_diplomacy_status(false);
    return;
  }
  i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
  fc_assert(i != -1);
  w = queen()->game_tab_widget->widget(i);
  dd = qobject_cast<diplo_dlg *>(w);
  dw = dd->find_widget(counterpart);
  dw->treaty.accept0 = I_accepted;
  dw->treaty.accept1 = other_accepted;
  dw->update_wdg();
  update_top_bar_diplomacy_status(dd->count() > 0);
}

/**
   Handle the start of a diplomacy meeting - usually by poping up a
   diplomacy dialog.
 */
void handle_diplomacy_init_meeting(int counterpart, int initiated_from)
{
  int i;
  diplo_dlg *dd;
  QWidget *w;
  QWidget *fw;

  if (client_is_observer()) {
    return;
  }

  if (king()->current_page() != PAGE_GAME) {
    king()->switch_page(PAGE_GAME);
  }

  if (!queen()->isRepoDlgOpen(QStringLiteral("DDI"))) {
    dd = new diplo_dlg(counterpart, initiated_from);

    if (!dd->init(false)) {
      delete dd;
      return;
    }
    dd->update_dlg();
    dd->make_active(counterpart);
  }
  i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
  fc_assert(i != -1);
  w = queen()->game_tab_widget->widget(i);
  dd = qobject_cast<diplo_dlg *>(w);
  fw = dd->find_widget(counterpart);
  if (fw == nullptr) {
    dd->add_widget(counterpart, initiated_from);
    queen()->game_tab_widget->setCurrentIndex(i);
  }
  dd->make_active(counterpart);

  // Bring it to front if user requested meeting
  if (player_by_number(initiated_from) == client.conn.playing) {
    queen()->game_tab_widget->setCurrentIndex(i);
  }
  update_top_bar_diplomacy_status(dd->count() > 0);
}

/**
   Update the diplomacy dialog by adding a clause.
 */
void handle_diplomacy_create_clause(int counterpart, int giver,
                                    enum clause_type type, int value)
{
  int i;
  diplo_dlg *dd;
  diplo_wdg *dw;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("DDI"))) {
    update_top_bar_diplomacy_status(false);
    return;
  }
  i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
  fc_assert(i != -1);
  w = queen()->game_tab_widget->widget(i);
  dd = qobject_cast<diplo_dlg *>(w);
  dw = dd->find_widget(counterpart);
  add_clause(&dw->treaty, player_by_number(giver), type, value);
  dw->update_wdg();
}

/**
   Update the diplomacy dialog when the meeting is canceled (the dialog
   should be closed).
 */
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from)
{
  int i;
  diplo_dlg *dd;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("DDI"))) {
    update_top_bar_diplomacy_status(false);
    return;
  }
  i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
  fc_assert(i != -1);
  w = queen()->game_tab_widget->widget(i);
  dd = qobject_cast<diplo_dlg *>(w);
  dd->close_widget(counterpart);
  update_top_bar_diplomacy_status(dd->count() > 0);
}

/**
   Update the diplomacy dialog by removing a clause.
 */
void handle_diplomacy_remove_clause(int counterpart, int giver,
                                    enum clause_type type, int value)
{
  int i;
  diplo_dlg *dd;
  diplo_wdg *dw;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("DDI"))) {
    update_top_bar_diplomacy_status(false);
    return;
  }
  i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
  fc_assert(i != -1);
  w = queen()->game_tab_widget->widget(i);
  dd = qobject_cast<diplo_dlg *>(w);
  dw = dd->find_widget(counterpart);
  remove_clause(&dw->treaty, player_by_number(giver), type, value);
  dw->update_wdg();
}

/**
   Close all open diplomacy dialogs.

   Called when the client disconnects from game.
 */
void close_all_diplomacy_dialogs()
{
  int i;
  diplo_dlg *dd;
  QWidget *w;

  qApp->alert(king()->central_wdg);
  if (!queen()->isRepoDlgOpen(QStringLiteral("DDI"))) {
    update_top_bar_diplomacy_status(false);
    return;
  }
  i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
  fc_assert(i != -1);
  w = queen()->game_tab_widget->widget(i);
  dd = qobject_cast<diplo_dlg *>(w);
  update_top_bar_diplomacy_status(false);
  dd->close();
  delete dd;
}
