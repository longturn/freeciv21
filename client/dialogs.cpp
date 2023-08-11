/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "dialogs.h"
// Qt
#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QtMath>
// utility
#include "astring.h"
#include "fcintl.h"
// common
#include "actions.h"
#include "city.h"
#include "climisc.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "movement.h"
#include "nation.h"
#include "research.h"
#include "style.h"
// client
#include "audio/audio.h"
#include "chatline_common.h"
#include "client_main.h"
#include "control.h"
#include "packhand.h"
#include "text.h"
#include "tileset/tilespec.h"
#include "views/view_map_common.h"
// gui-qt - awesome client
#include "fc_client.h"
#include "fonts.h"
#include "helpdlg.h"
#include "hudwidget.h"
#include "icons.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "tileset/sprite.h"
#include "unithudselector.h"
#include "unitselect.h"
#include "views/view_map.h"
#include "widgets/report_widget.h"

// Locations for non action enabler controlled buttons.
#define BUTTON_MOVE ACTION_COUNT
#define BUTTON_WAIT BUTTON_MOVE + 1
#define BUTTON_CANCEL BUTTON_MOVE + 2
#define BUTTON_COUNT BUTTON_MOVE + 3

extern void popdown_all_spaceships_dialogs();
extern void popdown_players_report();
extern void popdown_economy_report();
extern void popdown_science_report();
extern void popdown_city_report();
extern void popdown_endgame_report();

static void act_sel_keep_moving(QVariant data1, QVariant data2);
static void spy_request_strike_bld_list(QVariant data1, QVariant data2);
static void diplomat_incite(QVariant data1, QVariant data2);
static void diplomat_incite_escape(QVariant data1, QVariant data2);
static void spy_request_sabotage_list(QVariant data1, QVariant data2);
static void spy_request_sabotage_esc_list(QVariant data1, QVariant data2);
static void spy_sabotage(QVariant data1, QVariant data2);
static void spy_steal(QVariant data1, QVariant data2);
static void spy_steal_esc(QVariant data1, QVariant data2);
static void spy_steal_something(QVariant data1, QVariant data2);
static void diplomat_steal(QVariant data1, QVariant data2);
static void diplomat_steal_esc(QVariant data1, QVariant data2);
static void spy_poison(QVariant data1, QVariant data2);
static void spy_poison_esc(QVariant data1, QVariant data2);
static void spy_steal_gold(QVariant data1, QVariant data2);
static void spy_steal_gold_esc(QVariant data1, QVariant data2);
static void spy_steal_maps(QVariant data1, QVariant data2);
static void spy_steal_maps_esc(QVariant data1, QVariant data2);
static void spy_nuke_city(QVariant data1, QVariant data2);
static void spy_nuke_city_esc(QVariant data1, QVariant data2);
static void nuke_city(QVariant data1, QVariant data2);
static void destroy_city(QVariant data1, QVariant data2);
static void diplomat_embassy(QVariant data1, QVariant data2);
static void spy_embassy(QVariant data1, QVariant data2);
static void spy_sabotage_unit(QVariant data1, QVariant data2);
static void spy_sabotage_unit_esc(QVariant data1, QVariant data2);
static void spy_investigate(QVariant data1, QVariant data2);
static void diplomat_investigate(QVariant data1, QVariant data2);
static void diplomat_sabotage(QVariant data1, QVariant data2);
static void diplomat_sabotage_esc(QVariant data1, QVariant data2);
static void diplomat_bribe(QVariant data1, QVariant data2);
static void caravan_marketplace(QVariant data1, QVariant data2);
static void caravan_establish_trade(QVariant data1, QVariant data2);
static void caravan_help_build(QVariant data1, QVariant data2);
static void unit_recycle(QVariant data1, QVariant data2);
static void capture_units(QVariant data1, QVariant data2);
static void nuke_units(QVariant data1, QVariant data2);
static void expel_unit(QVariant data1, QVariant data2);
static void bombard(QVariant data1, QVariant data2);
static void bombard2(QVariant data1, QVariant data2);
static void bombard3(QVariant data1, QVariant data2);
static void found_city(QVariant data1, QVariant data2);
static void transform_terrain(QVariant data1, QVariant data2);
static void cultivate(QVariant data1, QVariant data2);
static void plant(QVariant data1, QVariant data2);
static void pillage(QVariant data1, QVariant data2);
static void clean_pollution(QVariant data1, QVariant data2);
static void clean_fallout(QVariant data1, QVariant data2);
static void road(QVariant data1, QVariant data2);
static void base(QVariant data1, QVariant data2);
static void mine(QVariant data1, QVariant data2);
static void irrigate(QVariant data1, QVariant data2);
static void nuke(QVariant data1, QVariant data2);
static void attack(QVariant data1, QVariant data2);
static void suicide_attack(QVariant data1, QVariant data2);
static void paradrop(QVariant data1, QVariant data2);
static void disembark1(QVariant data1, QVariant data2);
static void disembark2(QVariant data1, QVariant data2);
static void convert_unit(QVariant data1, QVariant data2);
static void fortify(QVariant data1, QVariant data2);
static void disband_unit(QVariant data1, QVariant data2);
static void join_city(QVariant data1, QVariant data2);
static void unit_home_city(QVariant data1, QVariant data2);
static void unit_upgrade(QVariant data1, QVariant data2);
static void airlift(QVariant data1, QVariant data2);
static void conquer_city(QVariant data1, QVariant data2);
static void conquer_city2(QVariant data1, QVariant data2);
static void heal_unit(QVariant data1, QVariant data2);
static void transport_board(QVariant data1, QVariant data2);
static void transport_embark(QVariant data1, QVariant data2);
static void transport_alight(QVariant data1, QVariant data2);
static void transport_unload(QVariant data1, QVariant data2);
static void keep_moving(QVariant data1, QVariant data2);
static void pillage_something(QVariant data1, QVariant data2);
static void user_action_1(QVariant data1, QVariant data2);
static void user_action_2(QVariant data1, QVariant data2);
static void user_action_3(QVariant data1, QVariant data2);
static void action_entry(choice_dialog *cd, action_id act,
                         const struct act_prob *act_probs,
                         const QString custom, QVariant data1,
                         QVariant data2);

static bool is_showing_pillage_dialog = false;
static races_dialog *race_dialog;
static bool is_race_dialog_open = false;

/* Information used in action selection follow up questions. Can't be
 * stored in the action selection dialog since it is closed before the
 * follow up question is asked. */
static bool is_more_user_input_needed = false;

/* Don't remove a unit's action decision want or move on to the next actor
 unit that wants a decision in the current unit selection. */
static bool did_not_decide = false;

extern QString forced_tileset_name;
qdef_act *qdef_act::m_instance = nullptr;

/**
   Initialize a mapping between an action and the function to call if
   the action's button is pushed.
 */
static const QHash<action_id, pfcn_void> af_map_init()
{
  QHash<action_id, pfcn_void> action_function;

  // Unit acting against a city target.
  action_function[ACTION_ESTABLISH_EMBASSY] = spy_embassy;
  action_function[ACTION_ESTABLISH_EMBASSY_STAY] = diplomat_embassy;
  action_function[ACTION_SPY_INVESTIGATE_CITY] = spy_investigate;
  action_function[ACTION_INV_CITY_SPEND] = diplomat_investigate;
  action_function[ACTION_SPY_POISON] = spy_poison;
  action_function[ACTION_SPY_POISON_ESC] = spy_poison_esc;
  action_function[ACTION_SPY_STEAL_GOLD] = spy_steal_gold;
  action_function[ACTION_SPY_STEAL_GOLD_ESC] = spy_steal_gold_esc;
  action_function[ACTION_SPY_SABOTAGE_CITY] = diplomat_sabotage;
  action_function[ACTION_SPY_SABOTAGE_CITY_ESC] = diplomat_sabotage_esc;
  action_function[ACTION_SPY_TARGETED_SABOTAGE_CITY] =
      spy_request_sabotage_list;
  action_function[ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC] =
      spy_request_sabotage_esc_list;
  action_function[ACTION_SPY_STEAL_TECH] = diplomat_steal;
  action_function[ACTION_SPY_STEAL_TECH_ESC] = diplomat_steal_esc;
  action_function[ACTION_SPY_TARGETED_STEAL_TECH] = spy_steal;
  action_function[ACTION_SPY_TARGETED_STEAL_TECH_ESC] = spy_steal_esc;
  action_function[ACTION_SPY_INCITE_CITY] = diplomat_incite;
  action_function[ACTION_SPY_INCITE_CITY_ESC] = diplomat_incite_escape;
  action_function[ACTION_TRADE_ROUTE] = caravan_establish_trade;
  action_function[ACTION_MARKETPLACE] = caravan_marketplace;
  action_function[ACTION_HELP_WONDER] = caravan_help_build;
  action_function[ACTION_JOIN_CITY] = join_city;
  action_function[ACTION_STEAL_MAPS] = spy_steal_maps;
  action_function[ACTION_STEAL_MAPS_ESC] = spy_steal_maps_esc;
  action_function[ACTION_SPY_NUKE] = spy_nuke_city;
  action_function[ACTION_SPY_NUKE_ESC] = spy_nuke_city_esc;
  action_function[ACTION_DESTROY_CITY] = destroy_city;
  action_function[ACTION_RECYCLE_UNIT] = unit_recycle;
  action_function[ACTION_HOME_CITY] = unit_home_city;
  action_function[ACTION_UPGRADE_UNIT] = unit_upgrade;
  action_function[ACTION_AIRLIFT] = airlift;
  action_function[ACTION_CONQUER_CITY] = conquer_city;
  action_function[ACTION_CONQUER_CITY2] = conquer_city2;
  action_function[ACTION_STRIKE_BUILDING] = spy_request_strike_bld_list;
  action_function[ACTION_NUKE_CITY] = nuke_city;

  // Unit acting against a unit target.
  action_function[ACTION_SPY_BRIBE_UNIT] = diplomat_bribe;
  action_function[ACTION_SPY_SABOTAGE_UNIT] = spy_sabotage_unit;
  action_function[ACTION_SPY_SABOTAGE_UNIT_ESC] = spy_sabotage_unit_esc;
  action_function[ACTION_EXPEL_UNIT] = expel_unit;
  action_function[ACTION_HEAL_UNIT] = heal_unit;
  action_function[ACTION_TRANSPORT_ALIGHT] = transport_alight;
  action_function[ACTION_TRANSPORT_UNLOAD] = transport_unload;
  action_function[ACTION_TRANSPORT_BOARD] = transport_board;
  action_function[ACTION_TRANSPORT_EMBARK] = transport_embark;

  // Unit acting against all units at a tile.
  action_function[ACTION_CAPTURE_UNITS] = capture_units;
  action_function[ACTION_BOMBARD] = bombard;
  action_function[ACTION_BOMBARD2] = bombard2;
  action_function[ACTION_BOMBARD3] = bombard3;
  action_function[ACTION_NUKE_UNITS] = nuke_units;

  // Unit acting against a tile.
  action_function[ACTION_FOUND_CITY] = found_city;
  action_function[ACTION_NUKE] = nuke;
  action_function[ACTION_PARADROP] = paradrop;
  action_function[ACTION_ATTACK] = attack;
  action_function[ACTION_SUICIDE_ATTACK] = suicide_attack;
  action_function[ACTION_TRANSFORM_TERRAIN] = transform_terrain;
  action_function[ACTION_CULTIVATE] = cultivate;
  action_function[ACTION_PLANT] = plant;
  action_function[ACTION_PILLAGE] = pillage;
  action_function[ACTION_CLEAN_POLLUTION] = clean_pollution;
  action_function[ACTION_CLEAN_FALLOUT] = clean_fallout;
  action_function[ACTION_ROAD] = road;
  action_function[ACTION_BASE] = base;
  action_function[ACTION_MINE] = mine;
  action_function[ACTION_IRRIGATE] = irrigate;
  action_function[ACTION_TRANSPORT_DISEMBARK1] = disembark1;
  action_function[ACTION_TRANSPORT_DISEMBARK2] = disembark2;

  // Unit acting with no target except itself.
  action_function[ACTION_DISBAND_UNIT] = disband_unit;
  action_function[ACTION_FORTIFY] = fortify;
  action_function[ACTION_CONVERT] = convert_unit;

  // Unit target depends on ruleset
  action_function[ACTION_USER_ACTION1] = user_action_1;
  action_function[ACTION_USER_ACTION2] = user_action_2;
  action_function[ACTION_USER_ACTION3] = user_action_3;

  return action_function;
}

/* Mapping from an action to the function to call when its button is
 * pushed. */
static const QHash<action_id, pfcn_void> af_map = af_map_init();

/**
   Constructor for custom dialog with themed titlebar
 */
qfc_dialog::qfc_dialog(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint)
{
  titlebar_height = 0;
  moving_now = false;
  setSizeGripEnabled(true);
  close_pix = fcIcons::instance()->getPixmap(QStringLiteral("cclose"));
}

qfc_dialog::~qfc_dialog() { delete close_pix; }

/**
   Paint event for themed dialog
 */
void qfc_dialog::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)

  QPainter p(this);
  QStyleOptionTitleBar tbar_opt;
  QStyleOption win_opt;
  QStyle *style = this->style();
  QRect active_area = this->rect();
  QPalette qpal;
  QRect close_rect, text_rect;

  qpal.setColor(QPalette::Active, QPalette::ToolTipText, Qt::white);
  tbar_opt.initFrom(this);
  titlebar_height =
      style->pixelMetric(QStyle::PM_TitleBarHeight, &tbar_opt, this) + 2;
  QPixmap *old = close_pix;
  close_pix = new QPixmap(close_pix->scaledToHeight(titlebar_height));
  delete old;
  tbar_opt.rect = QRect(0, 0, this->width(), titlebar_height);
  text_rect =
      QRect(0, 0, this->width() - close_pix->width(), titlebar_height);
  close_rect = QRect(this->width() - close_pix->width(), 0, this->width(),
                     titlebar_height);
  tbar_opt.titleBarState = this->windowState();
  tbar_opt.text = tbar_opt.fontMetrics.elidedText(
      this->windowTitle(), Qt::ElideRight, text_rect.width());
  style->drawComplexControl(QStyle::CC_TitleBar, &tbar_opt, &p, this);
  style->drawItemText(&p, text_rect, Qt::AlignCenter, qpal, true,
                      tbar_opt.text, QPalette::ToolTipText);
  style->drawItemPixmap(&p, close_rect, Qt::AlignLeft,
                        close_pix->scaledToHeight(titlebar_height));

  active_area.setTopLeft(QPoint(0, titlebar_height));
  this->setContentsMargins(0, titlebar_height, 0, 0);
  win_opt.initFrom(this);
  win_opt.rect = active_area;
  style->drawPrimitive(QStyle::PE_Widget, &win_opt, &p, this);
}

/**
   Mouse move event for themed titlebar (moves dialog with left mouse)
 */
void qfc_dialog::mouseMoveEvent(QMouseEvent *event)
{
  if (moving_now) {
    move(event->globalPos() - point);
  }
}

/**
   Mouse press event - catches left click
 */
void qfc_dialog::mousePressEvent(QMouseEvent *event)
{
  if (event->pos().y() <= titlebar_height
      && event->pos().x() <= width() - close_pix->width()) {
    point = event->globalPos() - geometry().topLeft();
    moving_now = true;
    setCursor(Qt::SizeAllCursor);
  } else if (event->pos().y() <= titlebar_height
             && event->pos().x() > width() - close_pix->width()) {
    close();
  }
}

/**
   Mouse release event for themed dialog
 */
void qfc_dialog::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  moving_now = false;
  setCursor(Qt::ArrowCursor);
}

/**
  Constructor for selecting nations
 */
races_dialog::races_dialog(struct player *pplayer, QWidget *parent)
    : qfc_dialog(parent), nations_tabs_list(nullptr)
{
  int i;
  QGridLayout *qgroupbox_layout;
  QGroupBox *no_name;
  QTableWidgetItem *item;
  QHeaderView *header;
  QSize size;
  QString title;
  QLabel *ns_label;

  setAttribute(Qt::WA_DeleteOnClose);
  is_race_dialog_open = true;
  main_layout = new QGridLayout;
  selected_nation_tabs = new QTableWidget;
  nation_tabs = new QTableWidget();
  styles = new QTableWidget;
  ok_button = new QPushButton;
  qnation_set = new QComboBox;
  ns_label = new QLabel;
  tplayer = pplayer;

  selected_nation = -1;
  selected_style = -1;
  setWindowTitle(_("Select Nation"));
  selected_nation_tabs->setRowCount(0);
  selected_nation_tabs->setColumnCount(1);
  selected_nation_tabs->setSelectionMode(QAbstractItemView::SingleSelection);
  selected_nation_tabs->verticalHeader()->setVisible(false);
  selected_nation_tabs->horizontalHeader()->setVisible(false);
  selected_nation_tabs->setProperty("showGrid", "true");
  selected_nation_tabs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  selected_nation_tabs->setShowGrid(false);
  selected_nation_tabs->setAlternatingRowColors(true);

  nation_tabs->setRowCount(0);
  nation_tabs->setColumnCount(1);
  nation_tabs->setSelectionMode(QAbstractItemView::SingleSelection);
  nation_tabs->verticalHeader()->setVisible(false);
  nation_tabs->horizontalHeader()->setVisible(false);
  nation_tabs->setProperty("showGrid", "true");
  nation_tabs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  nation_tabs->setShowGrid(false);
  ns_label->setText(_("Nation Set:"));
  styles->setRowCount(0);
  styles->setColumnCount(2);
  styles->setSelectionMode(QAbstractItemView::SingleSelection);
  styles->verticalHeader()->setVisible(false);
  styles->horizontalHeader()->setVisible(false);
  styles->setProperty("showGrid", "false");
  styles->setProperty("selectionBehavior", "SelectRows");
  styles->setEditTriggers(QAbstractItemView::NoEditTriggers);
  styles->setShowGrid(false);

  qgroupbox_layout = new QGridLayout;
  no_name = new QGroupBox(parent);
  leader_name = new QComboBox(no_name);
  is_male = new QRadioButton(no_name);
  is_female = new QRadioButton(no_name);

  leader_name->setEditable(true);
  qgroupbox_layout->addWidget(leader_name, 1, 0, 1, 2);
  qgroupbox_layout->addWidget(is_male, 2, 1);
  qgroupbox_layout->addWidget(is_female, 2, 0);
  is_female->setText(_("Female"));
  is_male->setText(_("Male"));
  // Select a default fairly
  is_male->setChecked(QRandomGenerator::global()->generate() % 2 == 0);
  is_female->setChecked(!is_male->isChecked());
  no_name->setLayout(qgroupbox_layout);

  description = new QTextEdit;
  description->setReadOnly(true);
  description->setPlainText(_("Choose nation"));
  no_name->setTitle(_("Your leader name"));

  /**
   * Fill styles, no need to update them later
   */

  styles_iterate(pstyle)
  {
    i = basic_city_style_for_style(pstyle);

    if (i >= 0) {
      item = new QTableWidgetItem;
      styles->insertRow(i);
      auto pix = get_sample_city_sprite(tileset, i);
      item->setData(Qt::DecorationRole, *pix);
      item->setData(Qt::UserRole, style_number(pstyle));
      size.setWidth(pix->width());
      size.setHeight(pix->height());
      item->setSizeHint(size);
      styles->setItem(i, 0, item);
      item = new QTableWidgetItem;
      item->setText(style_name_translation(pstyle));
      styles->setItem(i, 1, item);
    }
  }
  styles_iterate_end;

  header = styles->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Stretch);
  header->resizeSections(QHeaderView::ResizeToContents);
  header = styles->verticalHeader();
  header->resizeSections(QHeaderView::ResizeToContents);
  nation_sets_iterate(pset)
  {
    qnation_set->addItem(nation_set_name_translation(pset),
                         nation_set_rule_name(pset));
  }
  nation_sets_iterate_end;
  // create nation sets
  refresh();

  connect(styles->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &races_dialog::style_selected);
  connect(selected_nation_tabs->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &races_dialog::nation_selected);
  connect(leader_name, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &races_dialog::leader_selected);
  connect(leader_name->lineEdit(), &QLineEdit::returnPressed, this,
          &races_dialog::ok_pressed);
  connect(qnation_set, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &races_dialog::nationset_changed);
  connect(nation_tabs->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &races_dialog::group_selected);

  ok_button = new QPushButton;
  ok_button->setText(_("Cancel"));
  connect(ok_button, &QAbstractButton::pressed, this,
          &races_dialog::cancel_pressed);
  main_layout->addWidget(ok_button, 8, 2, 1, 1);
  random_button = new QPushButton;
  random_button->setText(_("Random"));
  connect(random_button, &QAbstractButton::pressed, this,
          &races_dialog::random_pressed);
  main_layout->addWidget(random_button, 8, 0, 1, 1);
  ok_button = new QPushButton;
  ok_button->setText(_("Ok"));
  connect(ok_button, &QAbstractButton::pressed, this,
          &races_dialog::ok_pressed);
  main_layout->addWidget(ok_button, 8, 3, 1, 1);
  main_layout->addWidget(no_name, 0, 3, 2, 1);
  if (nation_set_count() > 1) {
    main_layout->addWidget(ns_label, 0, 0, 1, 1);
    main_layout->addWidget(qnation_set, 0, 1, 1, 1);
    main_layout->addWidget(nation_tabs, 1, 0, 5, 2);
  } else {
    main_layout->addWidget(nation_tabs, 0, 0, 6, 2);
  }
  main_layout->addWidget(styles, 2, 3, 4, 1);
  main_layout->addWidget(description, 6, 0, 2, 4);
  main_layout->addWidget(selected_nation_tabs, 0, 2, 6, 1);

  setLayout(main_layout);
  set_index(-99);

  if (C_S_RUNNING == client_state()) {
    title = _("Edit Nation");
  } else if (nullptr != pplayer && pplayer == client.conn.playing) {
    title = _("What Nation Will You Be?");
  } else {
    title = _("Pick Nation");
  }

  update_nationset_combo();
  setWindowTitle(title);
}

/**
   Destructor for races dialog
 */
races_dialog::~races_dialog() { ::is_race_dialog_open = false; }

/**
   Sets first index to call update of nation list
 */
void races_dialog::refresh()
{
  struct nation_group *group;
  QTableWidgetItem *item;
  QHeaderView *header;
  int i;

  nation_tabs->clearContents();
  nation_tabs->setRowCount(0);
  nation_tabs->insertRow(0);
  item = new QTableWidgetItem;
  item->setText(_("All nations"));
  item->setData(Qt::UserRole, -99);
  nation_tabs->setItem(0, 0, item);

  for (i = 1; i < nation_group_count() + 1; i++) {
    group = nation_group_by_number(i - 1);
    if (is_nation_group_hidden(group)) {
      continue;
    }
    auto count = std::count_if(
        nations.begin(), nations.end(), [group](nation_type &n) {
          return is_nation_playable(&n) && is_nation_pickable(&n)
                 && nation_is_in_group(&n, group);
        });
    if (count == 0) {
      continue;
    }
    nation_tabs->insertRow(i);
    item = new QTableWidgetItem;
    item->setData(Qt::UserRole, i - 1);
    item->setText(nation_group_name_translation(group));
    nation_tabs->setItem(i, 0, item);
  }
  header = nation_tabs->horizontalHeader();
  header->resizeSections(QHeaderView::Stretch);
  header = nation_tabs->verticalHeader();
  header->resizeSections(QHeaderView::ResizeToContents);
  set_index(-99);
}

/**
   Updates nation_set combo ( usually called from option change )
 */
void races_dialog::update_nationset_combo()
{
  struct option *popt;
  struct nation_set *s;

  popt = optset_option_by_name(server_optset, "nationset");
  if (popt) {
    s = nation_set_by_setting_value(option_str_get(popt));
    qnation_set->setCurrentIndex(nation_set_index(s));
    qnation_set->setToolTip(_(nation_set_description(s)));
  }
}

/**
   Selected group of nation
 */
void races_dialog::group_selected(const QItemSelection &sl,
                                  const QItemSelection &ds)
{
  Q_UNUSED(ds)
  QModelIndexList indexes = sl.indexes();
  QModelIndex index;

  if (indexes.isEmpty()) {
    return;
  }
  index = indexes.at(0);
  set_index(index.row());
}

/**
   Sets new nations' group by current current selection,
   index is used only when there is no current selection.
 */
void races_dialog::set_index(int index)
{
  QTableWidgetItem *item;
  QFont f;
  struct nation_group *group;
  int i;
  QHeaderView *header;
  selected_nation_tabs->clearContents();
  selected_nation_tabs->setRowCount(0);

  last_index = 0;
  i = nation_tabs->currentRow();
  if (i != -1) {
    item = nation_tabs->item(i, 0);
    index = item->data(Qt::UserRole).toInt();
  }

  group = nation_group_by_number(index);
  i = 0;
  for (const auto &pnation : nations) {
    if (!is_nation_playable(&pnation) || !is_nation_pickable(&pnation)) {
      continue;
    }
    if (!nation_is_in_group(&pnation, group) && index != -99) {
      continue;
    }
    item = new QTableWidgetItem;
    selected_nation_tabs->insertRow(i);
    auto s = get_nation_flag_sprite(tileset, &pnation);
    if (pnation.player) {
      f = item->font();
      f.setStrikeOut(true);
      item->setFont(f);
    }
    item->setData(Qt::DecorationRole, *s);
    item->setData(Qt::UserRole, nation_index(&pnation));
    item->setText(nation_adjective_translation(&pnation));
    selected_nation_tabs->setItem(i, 0, item);
  } // iterate over nations - pnation

  selected_nation_tabs->sortByColumn(0, Qt::AscendingOrder);
  header = selected_nation_tabs->horizontalHeader();
  header->resizeSections(QHeaderView::Stretch);
  header = selected_nation_tabs->verticalHeader();
  header->resizeSections(QHeaderView::ResizeToContents);
}

/**
   Sets selected nation and updates style and leaders selector
 */
void races_dialog::nation_selected(const QItemSelection &selected,
                                   const QItemSelection &deselcted)
{
  Q_UNUSED(deselcted)
  char buf[4096];
  QModelIndex index;
  QVariant qvar;
  QModelIndexList indexes = selected.indexes();
  QString str;
  QTableWidgetItem *item;
  int style, ind;

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  if (indexes.isEmpty()) {
    return;
  }
  qvar = index.data(Qt::UserRole);
  selected_nation = qvar.toInt();

  helptext_nation(buf, sizeof(buf), nation_by_number(selected_nation),
                  nullptr);
  description->setPlainText(buf);
  leader_name->clear();
  if (client.conn.playing == tplayer) {
    // Add username preserving gender information
    leader_name->addItem(client.conn.playing->name, is_male->isChecked());
  }
  nation_leader_list_iterate(
      nation_leaders(nation_by_number(selected_nation)), pleader)
  {
    str = QString::fromUtf8(nation_leader_name(pleader));
    leader_name->addItem(str, nation_leader_is_male(pleader));
  }
  nation_leader_list_iterate_end;

  /**
   * select style for nation
   */

  style = style_number(style_of_nation(nation_by_number(selected_nation)));
  qvar = qvar.fromValue<int>(style);

  for (ind = 0; ind < styles->rowCount(); ind++) {
    item = styles->item(ind, 0);

    if (item->data(Qt::UserRole) == qvar) {
      styles->selectRow(ind);
    }
  }
}

/**
   Sets selected style
 */
void races_dialog::style_selected(const QItemSelection &selected,
                                  const QItemSelection &deselcted)
{
  Q_UNUSED(deselcted)
  QModelIndex index;
  QVariant qvar;
  QModelIndexList indexes = selected.indexes();

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  selected_style = qvar.toInt();
}

/**
   Sets selected leader
 */
void races_dialog::leader_selected(int index)
{
  if (leader_name->itemData(index).toBool()) {
    is_male->setChecked(true);
    is_female->setChecked(false);
  } else {
    is_male->setChecked(false);
    is_female->setChecked(true);
  }
}

/**
   Button accepting all selection has been pressed, closes dialog if
   everything is ok
 */
void races_dialog::ok_pressed()
{
  QByteArray ln_bytes;

  if (selected_nation == -1) {
    return;
  }

  fc_assert_ret(is_male->isChecked() || is_female->isChecked());

  if (selected_style == -1) {
    output_window_append(ftc_client, _("You must select your style."));
    return;
  }

  if (leader_name->currentText().length() == 0) {
    output_window_append(ftc_client, _("You must type a legal name."));
    return;
  }

  if (nation_by_number(selected_nation)->player != nullptr) {
    output_window_append(ftc_client,
                         _("Nation has been chosen by other player"));
    return;
  }
  ln_bytes = leader_name->currentText().toUtf8();
  dsend_packet_nation_select_req(&client.conn, player_number(tplayer),
                                 selected_nation, is_male->isChecked(),
                                 ln_bytes.data(), selected_style);
  close();
  deleteLater();
}

/**
   Default actions provider constructor
 */
qdef_act::qdef_act() {}

/**
   Returns instance of qdef_act
 */
qdef_act *qdef_act::action()
{
  if (!m_instance) {
    m_instance = new qdef_act;
  }
  return m_instance;
}

/**
   Deletes qdef_act instance
 */
void qdef_act::drop()
{
  delete m_instance;
  m_instance = nullptr;
}

/**
   Sets default action vs city
 */
void qdef_act::vs_city_set(int i) { vs_city = i; }

/**
   Sets default action vs unit
 */
void qdef_act::vs_unit_set(int i) { vs_unit = i; }

/**
   Returns default action vs city
 */
action_id qdef_act::vs_city_get() { return vs_city; }

/**
   Returns default action vs unit
 */
action_id qdef_act::vs_unit_get() { return vs_unit; }

/**
   Button canceling all selections has been pressed.
 */
void races_dialog::cancel_pressed() { delete this; }

/**
   Sets random nation
 */
void races_dialog::random_pressed()
{
  dsend_packet_nation_select_req(&client.conn, player_number(tplayer), -1,
                                 false, "", 0);
  delete this;
}

/**
   Notify goto dialog constructor
 */
notify_goto::notify_goto(const char *headline, const char *lines,
                         const struct text_tag_list *tags, tile *ptile,
                         QWidget *parent)
    : QMessageBox(parent)
{
  Q_UNUSED(tags)
  QString qlines;
  setAttribute(Qt::WA_DeleteOnClose);
  goto_but = this->addButton(_("Goto Location"), QMessageBox::ActionRole);
  goto_but->setIcon(fcIcons::instance()->getIcon(QStringLiteral("go-up")));
  inspect_but = this->addButton(_("Inspect City"), QMessageBox::ActionRole);
  inspect_but->setIcon(fcIcons::instance()->getIcon(QStringLiteral("plus")));

  close_but = this->addButton(QMessageBox::Close);
  gtile = ptile;
  if (!gtile) {
    goto_but->setVisible(false);
    inspect_but->setVisible(false);
  } else {
    struct city *pcity = tile_city(gtile);
    inspect_but->setVisible(nullptr != pcity
                            && city_owner(pcity) == client.conn.playing);
  }
  setWindowTitle(headline);
  qlines = lines;
  qlines.replace(QLatin1String("\n"), QLatin1String(" "));
  setText(qlines);
  connect(goto_but, &QAbstractButton::pressed, this,
          &notify_goto::goto_tile);
  connect(inspect_but, &QAbstractButton::pressed, this,
          &notify_goto::inspect_city);
  connect(close_but, &QAbstractButton::pressed, this, &QWidget::close);
  show();
}

/**
   Clicked goto tile in notify goto dialog
 */
void notify_goto::goto_tile()
{
  queen()->mapview_wdg->center_on_tile(gtile);
  close();
}

/**
   Clicked inspect city in notify goto dialog
 */
void notify_goto::inspect_city()
{
  struct city *pcity = tile_city(gtile);
  if (pcity) {
    real_city_dialog_popup(pcity);
  }
  close();
}

/**
   User changed nation_set
 */
void races_dialog::nationset_changed(int index)
{
  QString rule_name;
  QByteArray rn_bytes;
  const char *rn;
  struct option *poption = optset_option_by_name(server_optset, "nationset");

  rule_name = qnation_set->currentData().toString();
  rn_bytes = rule_name.toLocal8Bit(); /* Hold QByteArray in a variable to
                                       * extend its, and data() buffer's,
                                       * lifetime */
  rn = rn_bytes.data();
  if (nation_set_by_setting_value(option_str_get(poption))
      != nation_set_by_rule_name(rn)) {
    option_str_set(poption, rn);
  }
}

/**
   Popup a dialog to display information about an event that has a
   specific location.  The user should be given the option to goto that
   location.
 */
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  notify_goto *ask =
      new notify_goto(headline, lines, tags, ptile, king()->central_wdg);
  ask->show();
}

/**
   Popup a dialog to display connection message from server.
 */
void popup_connect_msg(const char *headline, const char *message)
{
  QMessageBox *msg = new QMessageBox(king()->central_wdg);

  msg->setText(message);
  msg->setStandardButtons(QMessageBox::Ok);
  msg->setWindowTitle(headline);
  msg->setAttribute(Qt::WA_DeleteOnClose);
  msg->show();
}

/**
   Popup a generic dialog to display some generic information.
 */
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines)
{
  /*
   Item 1084 Information widgets open multiple times
   See if there is already a dialog on the screen for the inputted type and
   if so remove it. There are 2 "Traveler's Report:" captions so must
   distinguish between the 2 by looking at the headline
  */

  const auto list =
      queen()->game_tab_widget->findChildren<freeciv::report_widget *>();
  for (auto report : list) {
    if (report->caption() == caption && report->headline() == headline) {
      report->close();
    }
  }

  freeciv::report_widget *nd = new freeciv::report_widget(
      caption, headline, lines, queen()->mapview_wdg);
  nd->show();
}

/**
   Popup the nation selection dialog.
 */
void popup_races_dialog(struct player *pplayer)
{
  if (!is_race_dialog_open) {
    race_dialog = new races_dialog(pplayer, king()->central_wdg);
    is_race_dialog_open = true;
    race_dialog->show();
  }
  race_dialog->showNormal();
}

/**
   Close the nation selection dialog.  This should allow the user to
   (at least) select a unit to activate.
 */
void popdown_races_dialog(void)
{
  if (is_race_dialog_open) {
    race_dialog->close();
    is_race_dialog_open = false;
  }
}

/**
   Popup a dialog window to select units on a particular tile.
 */
void unit_select_dialog_popup(struct tile *ptile)
{
  if (ptile != nullptr
      && (unit_list_size(ptile->units) > 1
          || (unit_list_size(ptile->units) == 1 && tile_city(ptile)))) {
    toggle_unit_sel_widget(ptile);
  }
}

/**
   Update the dialog window to select units on a particular tile.
 */
void unit_select_dialog_update_real(void *unused) { update_unit_sel(); }

/**
   Updates nationset combobox
 */
void update_nationset_combo()
{
  if (is_race_dialog_open) {
    race_dialog->update_nationset_combo();
  }
}

/**
   The server has changed the set of selectable nations.
 */
void races_update_pickable(bool nationset_change)
{
  Q_UNUSED(nationset_change)
  if (is_race_dialog_open) {
    race_dialog->refresh();
  }
}

/**
   In the nation selection dialog, make already-taken nations unavailable.
   This information is contained in the packet_nations_used packet.
 */
void races_toggles_set_sensitive(void)
{
  if (is_race_dialog_open) {
    race_dialog->refresh();
  }
}

/**
   Popup a dialog asking if the player wants to start a revolution.
 */
void popup_revolution_dialog(struct government *government)
{
  hud_message_box *ask;
  const Government_type_id government_id = government_number(government);

  if (0 > client.conn.playing->revolution_finishes) {
    ask = new hud_message_box(king()->central_wdg);
    ask->set_text_title(_("Do you want to overthrow the government?"),
                        _("Revolution!"));
    ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    ask->setDefaultButton(QMessageBox::Cancel);
    ask->button(QMessageBox::Yes)->setText(_("Yes Start a Revolution!"));

    ask->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(ask, &hud_message_box::accepted, [=]() {
      struct government *government = government_by_number(government_id);
      if (government) {
        revolution_response(government);
      }
    });
    ask->show();
  } else {
    revolution_response(government);
  }
}

/**
   Constructor for choice_dialog_button_data
 */
Choice_dialog_button::Choice_dialog_button(const QString title,
                                           pfcn_void func_in,
                                           QVariant data1_in,
                                           QVariant data2_in)
    : QPushButton(title), data1(data1_in), data2(data2_in)
{
  func = func_in;
}

/**
   Get the function to call when the button is pressed.
 */
pfcn_void Choice_dialog_button::getFunc() { return func; }

/**
   Get the first piece of data to feed the function when the button is
   pressed.
 */
QVariant Choice_dialog_button::getData1() { return data1; }

/**
   Get the second piece of data to feed the function when the button is
   pressed.
 */
QVariant Choice_dialog_button::getData2() { return data2; }

/**
   Sets the first piece of data
 */
void Choice_dialog_button::setData1(QVariant wariat) { data1 = wariat; }

/**
   Sets the second piece of data
 */
void Choice_dialog_button::setData2(QVariant wariat) { data2 = wariat; }

/**
   Constructor for choice_dialog
 */
choice_dialog::choice_dialog(const QString title, const QString text,
                             QWidget *parent, void (*run_on_close_in)(int))
    : QWidget(parent), target_unit_button(nullptr), unit_skip(nullptr)
{
  QLabel *l = new QLabel(text);

  setProperty("themed_choice", true);
  layout = new QVBoxLayout(this);
  run_on_close = run_on_close_in;

  layout->addWidget(l);
  setWindowFlags(Qt::Dialog);
  setWindowTitle(title);
  setAttribute(Qt::WA_DeleteOnClose);
  king()->set_diplo_dialog(this);

  unit_id = IDENTITY_NUMBER_ZERO;
  target_id[ATK_SELF] = unit_id;
  target_id[ATK_CITY] = IDENTITY_NUMBER_ZERO;
  target_id[ATK_UNIT] = IDENTITY_NUMBER_ZERO;
  target_id[ATK_UNITS] = TILE_INDEX_NONE;
  target_id[ATK_TILE] = TILE_INDEX_NONE;
  sub_target_id[ASTK_BUILDING] = B_LAST;
  sub_target_id[ASTK_TECH] = A_UNSET;
  sub_target_id[ASTK_EXTRA] = EXTRA_NONE;
  sub_target_id[ASTK_EXTRA_NOT_THERE] = EXTRA_NONE;

  targeted_unit = nullptr;
  // No buttons are added yet.
  for (int i = 0; i < BUTTON_COUNT; i++) {
    action_button_map << nullptr;
  }
}

/**
   Destructor for choice dialog
 */
choice_dialog::~choice_dialog()
{
  buttons_list.clear();
  action_button_map.clear();
  king()->set_diplo_dialog(nullptr);

  if (run_on_close) {
    run_on_close(unit_id);
    run_on_close = nullptr;
  }
}

/**
   Sets layout for choice dialog
 */
void choice_dialog::set_layout()
{
  targeted_unit = game_unit_by_number(target_id[ATK_UNIT]);

  if ((game_unit_by_number(unit_id)) && targeted_unit
      && unit_list_size(targeted_unit->tile->units) > 1) {
    QPixmap *pix;
    QPushButton *next, *prev;
    unit_skip = new QHBoxLayout;
    next = new QPushButton();
    next->setIcon(
        fcIcons::instance()->getIcon(QStringLiteral("city-right")));
    next->setIconSize(QSize(32, 32));
    next->setFixedSize(QSize(36, 36));
    prev = new QPushButton();
    prev->setIcon(fcIcons::instance()->getIcon(QStringLiteral("city-left")));
    prev->setIconSize(QSize(32, 32));
    prev->setFixedSize(QSize(36, 36));
    target_unit_button = new QPushButton;
    pix = new QPixmap(tileset_unit_width(tileset),
                      tileset_unit_height(tileset));
    pix->fill(Qt::transparent);
    put_unit(targeted_unit, pix, 0, 0);
    target_unit_button->setIcon(QIcon(*pix));
    delete pix;
    target_unit_button->setIconSize(QSize(96, 96));
    target_unit_button->setFixedSize(QSize(100, 100));
    unit_skip->addStretch(100);
    unit_skip->addWidget(prev, Qt::AlignCenter);
    unit_skip->addWidget(target_unit_button, Qt::AlignCenter);
    unit_skip->addWidget(next, Qt::AlignCenter);
    layout->addLayout(unit_skip);
    unit_skip->addStretch(100);
    connect(prev, &QAbstractButton::clicked, this,
            &choice_dialog::prev_unit);
    connect(next, &QAbstractButton::clicked, this,
            &choice_dialog::next_unit);
  }

  setLayout(layout);
}

/**
   Adds new action for choice dialog
 */
void choice_dialog::add_item(QString title, pfcn_void func, QVariant data1,
                             QVariant data2,
                             QString tool_tip = QLatin1String(""),
                             const int button_id = -1)
{
  Choice_dialog_button *button =
      new Choice_dialog_button(title, func, data1, data2);
  int action = buttons_list.count();

  QObject::connect(button, &QPushButton::clicked,
                   [=]() { execute_action(action); });

  buttons_list.append(button);

  if (!tool_tip.isEmpty()) {
    button->setToolTip(break_lines(tool_tip, 40));
  }

  if (0 <= button_id) {
    // The id is valid.
    action_button_map[button_id] = button;
  }

  layout->addWidget(button);
}

/**
   Shows choice dialog
 */
void choice_dialog::show_me()
{
  QPoint p;

  p = mapFromGlobal(QCursor::pos());
  p.setY(p.y() - this->height());
  p.setX(p.x() - this->width());
  move(p);
  show();
}

/**
   Returns layout in choice dialog
 */
QVBoxLayout *choice_dialog::get_layout() { return layout; }

/**
   Get the button with the given identity.
 */
Choice_dialog_button *choice_dialog::get_identified_button(const int id)
{
  if (id < 0) {
    fc_assert_msg(0 <= id, "Invalid button ID.");
    return nullptr;
  }

  return action_button_map[id];
}

/**
   Try to pick up default unit action
 */
bool try_default_unit_action(QVariant q1, QVariant q2)
{
  action_id action;
  pfcn_void func;

  action = qdef_act::action()->vs_unit_get();
  if (action == -1) {
    return false;
  }
  func = af_map[action];

  func(q1, q2);
  return true;
}

/**
   Try to pick up default city action
 */
bool try_default_city_action(QVariant q1, QVariant q2)
{
  action_id action;
  pfcn_void func;

  action = qdef_act::action()->vs_city_get();
  if (action == -1) {
    return false;
  }
  func = af_map[action];
  func(q1, q2);
  return true;
}

/**
   Focus next target
 */
void choice_dialog::next_unit()
{
  struct tile *ptile;
  struct unit *new_target = nullptr;
  bool break_next = false;
  bool first = true;
  QPixmap *pix;

  if (targeted_unit == nullptr) {
    return;
  }

  ptile = targeted_unit->tile;

  unit_list_iterate(ptile->units, ptgt)
  {
    if (first) {
      new_target = ptgt;
      first = false;
    }
    if (break_next) {
      new_target = ptgt;
      break;
    }
    if (ptgt == targeted_unit) {
      break_next = true;
    }
  }
  unit_list_iterate_end;
  targeted_unit = new_target;
  pix =
      new QPixmap(tileset_unit_width(tileset), tileset_unit_height(tileset));
  pix->fill(Qt::transparent);
  put_unit(targeted_unit, pix, 0, 0);
  target_unit_button->setIcon(QIcon(*pix));
  delete pix;
  switch_target();
}

/**
   Focus previous target
 */
void choice_dialog::prev_unit()
{
  struct tile *ptile;
  struct unit *new_target = nullptr;
  QPixmap *pix;
  if (targeted_unit == nullptr) {
    return;
  }

  ptile = targeted_unit->tile;
  unit_list_iterate(ptile->units, ptgt)
  {
    if ((ptgt == targeted_unit) && new_target != nullptr) {
      break;
    }
    new_target = ptgt;
  }
  unit_list_iterate_end;
  targeted_unit = new_target;
  pix =
      new QPixmap(tileset_unit_width(tileset), tileset_unit_height(tileset));
  pix->fill(Qt::transparent);
  put_unit(targeted_unit, pix, 0, 0);
  target_unit_button->setIcon(QIcon(*pix));
  delete pix;
  switch_target();
}

/**
   Update dialog for new target (targeted_unit)
 */
void choice_dialog::update_dialog(const struct act_prob *act_probs)
{
  struct unit *actor_unit = game_unit_by_number(unit_id);
  if (targeted_unit == nullptr) {
    return;
  }
  unit_skip->setParent(nullptr);
  fc_assert_ret(actor_unit);
  action_selection_refresh(actor_unit, nullptr, targeted_unit,
                           targeted_unit->tile,
                           (sub_target_id[ASTK_EXTRA] != EXTRA_NONE
                                ? extra_by_number(sub_target_id[ASTK_EXTRA])
                                : nullptr),
                           act_probs);
  layout->addLayout(unit_skip);
}

/**
   Switches target unit
 */
void choice_dialog::switch_target()
{
  if (targeted_unit == nullptr) {
    return;
  }

  unit_skip->setParent(nullptr);
  dsend_packet_unit_get_actions(&client.conn, unit_id, targeted_unit->id,
                                targeted_unit->tile->index,
                                action_selection_target_extra(), true);
  layout->addLayout(unit_skip);
}

/**
   Run chosen action and close dialog
 */
void choice_dialog::execute_action(const int action)
{
  Choice_dialog_button *button = buttons_list.at(action);
  pfcn_void func = button->getFunc();

  func(button->getData1(), button->getData2());
  close();
}

/**
   Put the button in the stack and temporarily remove it. When
   unstack_all_buttons() is called all buttons in the stack will be added
   to the end of the dialog.

   Can be used to place a button below existing buttons or below buttons
   added while it was in the stack.
 */
void choice_dialog::stack_button(Choice_dialog_button *button)
{
  // Store the data in the stack.
  last_buttons_stack.append(button);

  /* Temporary remove the button so it will end up below buttons added
   * before unstack_all_buttons() is called. */
  layout->removeWidget(button);

  // Synchronize the list with the layout.
  buttons_list.removeAll(button);
}

/**
   Put all the buttons in the stack back to the dialog. They will appear
   after any other buttons. See stack_button()
 */
void choice_dialog::unstack_all_buttons()
{
  while (!last_buttons_stack.isEmpty()) {
    Choice_dialog_button *button = last_buttons_stack.takeLast();

    // Reinsert the button below the other buttons.
    buttons_list.append(button);
    layout->addWidget(button);
  }
}

/**
   Action enter market place for choice dialog
 */
static void caravan_marketplace(QVariant data1, QVariant data2)
{
  int actor_unit_id = data1.toInt();
  int target_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_unit_id)
      && nullptr != game_city_by_number(target_city_id)) {
    request_do_action(ACTION_MARKETPLACE, actor_unit_id, target_city_id, 0,
                      "");
  }
}

/**
   Action establish trade for choice dialog
 */
static void caravan_establish_trade(QVariant data1, QVariant data2)
{
  int actor_unit_id = data1.toInt();
  int target_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_unit_id)
      && nullptr != game_city_by_number(target_city_id)) {
    request_do_action(ACTION_TRADE_ROUTE, actor_unit_id, target_city_id, 0,
                      "");
  }
}

/**
   Action help build wonder for choice dialog
 */
static void caravan_help_build(QVariant data1, QVariant data2)
{
  int caravan_id = data1.toInt();
  int caravan_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(caravan_id)
      && nullptr != game_city_by_number(caravan_target_id)) {
    request_do_action(ACTION_HELP_WONDER, caravan_id, caravan_target_id, 0,
                      "");
  }
}

/**
   Action Recycle Unit for choice dialog
 */
static void unit_recycle(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_RECYCLE_UNIT, actor_id, tgt_city_id, 0, "");
  }
}

/**
   Action Home City for choice dialog
 */
static void unit_home_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_HOME_CITY, actor_id, tgt_city_id, 0, "");
  }
}

/**
   Action "Upgrade Unit" for choice dialog
 */
static void unit_upgrade(QVariant data1, QVariant data2)
{
  struct unit *punit;

  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if ((punit = game_unit_by_number(actor_id))
      && nullptr != game_city_by_number(tgt_city_id)) {
    popup_upgrade_dialog({punit});
  }
}

/**
   Action "Airlift Unit" for choice dialog
 */
static void airlift(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_AIRLIFT, actor_id, tgt_city_id, 0, "");
  }
}

/**
   Action "Conquer City" for choice dialog
 */
static void conquer_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_CONQUER_CITY, actor_id, tgt_city_id, 0, "");
  }
}

/**
   Action "Conquer City 2" for choice dialog
 */
static void conquer_city2(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int tgt_city_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(tgt_city_id)) {
    request_do_action(ACTION_CONQUER_CITY2, actor_id, tgt_city_id, 0, "");
  }
}

/**
   Delay selection of what action to take.
 */
static void act_sel_wait(QVariant data1, QVariant data2) { key_unit_wait(); }

/**
   Empty action for choice dialog (just do nothing)
 */
static void keep_moving(QVariant data1, QVariant data2) {}

/**
   Starts revolution with targeted government as target or anarchy otherwise
 */
void revolution_response(struct government *government)
{
  if (!government) {
    start_revolution();
  } else {
    set_government_choice(government);
  }
}

/**
   Move the queue of diplomats that need user input forward unless the
   current diplomat will need more input.
 */
static void diplomat_queue_handle_primary(int actor_unit_id)
{
  if (!is_more_user_input_needed) {
    /* The client isn't waiting for information for any unanswered follow
     * up questions. */

    struct unit *actor_unit;

    if ((actor_unit = game_unit_by_number(actor_unit_id))) {
      /* The action selection dialog wasn't closed because the actor unit
       * was lost. */

      // The probabilities didn't just disappear, right?
      fc_assert_action(actor_unit->client.act_prob_cache,
                       client_unit_init_act_prob_cache(actor_unit));

      delete[] actor_unit->client.act_prob_cache;
      actor_unit->client.act_prob_cache = nullptr;
    }

    // The action selection process is over, at least for now.
    action_selection_no_longer_in_progress(actor_unit_id);

    if (did_not_decide) {
      /* The action selection dialog was closed but the player didn't
       * decide what the unit should do. */

      // Reset so the next action selection dialog does the right thing.
      did_not_decide = false;
    } else {
      // An action, or no action at all, was selected.
      action_decision_clear_want(actor_unit_id);
      action_selection_next_in_focus(actor_unit_id);
    }
  }
}

/**
   Move the queue of diplomats that need user input forward since the
   current diplomat got the extra input that was required.
 */
static void diplomat_queue_handle_secondary(int actor_id)
{
  // Stop waiting. Move on to the next queued diplomat.
  is_more_user_input_needed = false;
  diplomat_queue_handle_primary(actor_id);
}

/**
   Let the non shared client code know that the action selection process
   no longer is in progress for the specified unit.

   This allows the client to clean up any client specific assumptions.
 */
void action_selection_no_longer_in_progress_gui_specific(int actor_id)
{
  Q_UNUSED(actor_id)
  // Stop assuming the answer to a follow up question will arrive.
  is_more_user_input_needed = false;
}

/**
   Popup a dialog that allows the player to select what action a unit
   should take.
 */
void popup_action_selection(struct unit *actor_unit,
                            struct city *target_city,
                            struct unit *target_unit,
                            struct tile *target_tile,
                            struct extra_type *target_extra,
                            const struct act_prob *act_probs)
{
  QString title, text;
  choice_dialog *cd;
  QVariant qv1, qv2;
  pfcn_void func;
  struct city *actor_homecity;
  action_id unit_act;
  action_id city_act;

  unit_act = qdef_act::action()->vs_unit_get();
  city_act = qdef_act::action()->vs_city_get();

  for (auto caras : qAsConst(king()->trade_gen.lines)) {
    if (caras.autocaravan == actor_unit) {
      int i;
      if (nullptr != game_unit_by_number(actor_unit->id)
          && nullptr != game_city_by_number(target_city->id)) {
        request_do_action(ACTION_TRADE_ROUTE, actor_unit->id,
                          target_city->id, 0, "");
        client_unit_init_act_prob_cache(actor_unit);
        diplomat_queue_handle_primary(actor_unit->id);
        i = king()->trade_gen.lines.indexOf(caras);
        king()->trade_gen.lines.takeAt(i);
        return;
      }
    }
  }
  if (target_city && try_default_city_action(actor_unit->id, target_city->id)
      && action_prob_possible(act_probs[unit_act])) {
    diplomat_queue_handle_primary(actor_unit->id);
    return;
  }

  if (target_unit && try_default_unit_action(actor_unit->id, target_unit->id)
      && action_prob_possible(act_probs[city_act])) {
    diplomat_queue_handle_primary(actor_unit->id);
    return;
  }
  /* Could be caused by the server failing to reply to a request for more
   * information or a bug in the client code. */
  fc_assert_msg(!is_more_user_input_needed,
                "Diplomat queue problem. Is another diplomat window open?");

  // No extra input is required as no action has been chosen yet.
  is_more_user_input_needed = false;

  actor_homecity = game_city_by_number(actor_unit->homecity);

  title = // TRANS: %s is a unit name, e.g., Spy
      QString(_("Choose Your %1's Strategy"))
          .arg(unit_name_translation(actor_unit));

  if (target_city && actor_homecity) {
    text =
        QString(_("Your %1 from %2 reaches the city of %3.\nWhat now?"))
            .arg(unit_name_translation(actor_unit),
                 city_name_get(actor_homecity), city_name_get(target_city));
  } else if (target_city) {
    text = QString(_("Your %1 has arrived at %2.\nWhat is your command?"))
               .arg(unit_name_translation(actor_unit),
                    city_name_get(target_city));
  } else if (target_unit) {
    // TRANS: Your Spy is ready to act against Roman Freight.
    text = QString(_("Your %1 is ready to act against %2 %3."))
               .arg(unit_name_translation(actor_unit),
                    nation_adjective_for_player(unit_owner(target_unit)),
                    unit_name_translation(target_unit));
  } else {
    fc_assert_msg(target_unit || target_city || target_tile,
                  "No target specified.");
    // TRANS: %s is a unit name, e.g., Diplomat, Spy
    text = QString(_("Your %1 is waiting for your command."))
               .arg(unit_name_translation(actor_unit));
  }

  cd = king()->get_diplo_dialog();
  if ((cd != nullptr) && cd->targeted_unit != nullptr) {
    cd->update_dialog(act_probs);
    return;
  }
  cd = new choice_dialog(title, text, queen()->game_tab_widget,
                         diplomat_queue_handle_primary);
  qv1 = actor_unit->id;

  cd->unit_id = actor_unit->id;

  cd->target_id[ATK_SELF] = cd->unit_id;

  if (target_city) {
    cd->target_id[ATK_CITY] = target_city->id;
  } else {
    cd->target_id[ATK_CITY] = IDENTITY_NUMBER_ZERO;
  }

  if (target_unit) {
    cd->target_id[ATK_UNIT] = target_unit->id;
  } else {
    cd->target_id[ATK_UNIT] = IDENTITY_NUMBER_ZERO;
  }

  if (target_tile) {
    cd->target_id[ATK_UNITS] = tile_index(target_tile);
  } else {
    cd->target_id[ATK_UNITS] = TILE_INDEX_NONE;
  }

  if (target_tile) {
    cd->target_id[ATK_TILE] = tile_index(target_tile);
  } else {
    cd->target_id[ATK_TILE] = TILE_INDEX_NONE;
  }

  // No target building or target tech supplied. (Feb 2020)
  cd->sub_target_id[ASTK_BUILDING] = B_LAST;
  cd->sub_target_id[ASTK_TECH] = A_UNSET;

  if (target_extra) {
    cd->sub_target_id[ASTK_EXTRA] = extra_number(target_extra);
    cd->sub_target_id[ASTK_EXTRA_NOT_THERE] = extra_number(target_extra);
  } else {
    cd->sub_target_id[ASTK_EXTRA] = EXTRA_NONE;
    cd->sub_target_id[ASTK_EXTRA_NOT_THERE] = EXTRA_NONE;
  }

  // Unit acting against a city

  // Set the correct target for the following actions.
  qv2 = cd->target_id[ATK_CITY];

  action_iterate(act)
  {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_CITY) {
      action_entry(cd, act, act_probs,
                   get_act_sel_action_custom_text(action_by_number(act),
                                                  act_probs[act], actor_unit,
                                                  target_city),
                   qv1, qv2);
    }
  }
  action_iterate_end;

  // Unit acting against another unit

  // Set the correct target for the following actions.
  qv2 = cd->target_id[ATK_UNIT];

  action_iterate(act)
  {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_UNIT) {
      action_entry(cd, act, act_probs,
                   get_act_sel_action_custom_text(action_by_number(act),
                                                  act_probs[act], actor_unit,
                                                  target_city),
                   qv1, qv2);
    }
  }
  action_iterate_end;

  // Unit acting against all units at a tile

  // Set the correct target for the following actions.
  qv2 = cd->target_id[ATK_UNITS];

  action_iterate(act)
  {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_UNITS) {
      action_entry(cd, act, act_probs,
                   get_act_sel_action_custom_text(action_by_number(act),
                                                  act_probs[act], actor_unit,
                                                  target_city),
                   qv1, qv2);
    }
  }
  action_iterate_end;

  // Unit acting against a tile.

  // Set the correct target for the following actions.
  qv2 = cd->target_id[ATK_TILE];

  action_iterate(act)
  {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_TILE) {
      action_entry(cd, act, act_probs,
                   get_act_sel_action_custom_text(action_by_number(act),
                                                  act_probs[act], actor_unit,
                                                  target_city),
                   qv1, qv2);
    }
  }
  action_iterate_end;

  // Unit acting against itself

  // Set the correct target for the following actions.
  qv2 = cd->target_id[ATK_SELF];

  action_iterate(act)
  {
    if (action_id_get_actor_kind(act) == AAK_UNIT
        && action_id_get_target_kind(act) == ATK_SELF) {
      action_entry(cd, act, act_probs,
                   get_act_sel_action_custom_text(action_by_number(act),
                                                  act_probs[act], actor_unit,
                                                  target_city),
                   qv1, qv2);
    }
  }
  action_iterate_end;

  if (unit_can_move_to_tile(&(wld.map), actor_unit, target_tile, false,
                            false)) {
    qv2 = target_tile->index;

    func = act_sel_keep_moving;
    cd->add_item(QString(_("Keep moving")), func, qv1, qv2,
                 QLatin1String(""), BUTTON_MOVE);
  }

  func = act_sel_wait;
  cd->add_item(QString(_("&Wait")), func, qv1, qv2, QLatin1String(""),
               BUTTON_WAIT);

  func = keep_moving;
  cd->add_item(QString(_("Do nothing")), func, qv1, qv2, QLatin1String(""),
               BUTTON_CANCEL);

  cd->set_layout();
  cd->show_me();

  // Give follow up questions access to action probabilities.
  client_unit_init_act_prob_cache(actor_unit);
  action_iterate(act)
  {
    actor_unit->client.act_prob_cache[act] = act_probs[act];
  }
  action_iterate_end;
}

/**
   Get the non targeted version of an action so it, if enabled, can appear
   in the target selection dialog.
 */
static action_id get_non_targeted_action_id(action_id tgt_action_id)
{
  /* Don't add an action mapping here unless the non targeted version is
   * selectable in the targeted version's target selection dialog. */
  switch (static_cast<enum gen_action>(tgt_action_id)) {
  case ACTION_SPY_TARGETED_SABOTAGE_CITY:
    return ACTION_SPY_SABOTAGE_CITY;
  case ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC:
    return ACTION_SPY_SABOTAGE_CITY_ESC;
  case ACTION_SPY_TARGETED_STEAL_TECH:
    return ACTION_SPY_STEAL_TECH;
  case ACTION_SPY_TARGETED_STEAL_TECH_ESC:
    return ACTION_SPY_STEAL_TECH_ESC;
  default:
    // No non targeted version found.
    return ACTION_NONE;
  }
}

/**
   Get the production targeted version of an action so it, if enabled, can
   appear in the target selection dialog.
 */
static action_id get_production_targeted_action_id(action_id tgt_action_id)
{
  /* Don't add an action mapping here unless the non targeted version is
   * selectable in the targeted version's target selection dialog. */
  switch (static_cast<enum gen_action>(tgt_action_id)) {
  case ACTION_SPY_TARGETED_SABOTAGE_CITY:
    return ACTION_SPY_SABOTAGE_CITY_PRODUCTION;
  case ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC:
    return ACTION_SPY_SABOTAGE_CITY_PRODUCTION_ESC;
  case ACTION_STRIKE_BUILDING:
    return ACTION_STRIKE_PRODUCTION;
  default:
    // No non targeted version found.
    return ACTION_NONE;
  }
}

/**
   Show the user the action if it is enabled.
 */
static void action_entry(choice_dialog *cd, action_id act,
                         const struct act_prob *act_probs,
                         const QString custom, QVariant data1,
                         QVariant data2)
{
  QString title;
  QString tool_tip;

  if (!af_map.contains(act)) {
    /* The Qt client doesn't support ordering this action from the
     * action selection dialog. */
    return;
  }

  // Don't show disabled actions.
  if (!action_prob_possible(act_probs[act])) {
    return;
  }

  title = QString(action_prepare_ui_name(act, "&", act_probs[act], custom));

  tool_tip = QString(
      act_sel_action_tool_tip(action_by_number(act), act_probs[act]));

  cd->add_item(title, af_map[act], data1, data2, tool_tip, act);
}

/**
   Update an existing button.
 */
static void action_entry_update(Choice_dialog_button *button, action_id act,
                                const struct act_prob *act_probs,
                                const QString custom, QVariant data1,
                                QVariant data2)
{
  QString title;
  QString tool_tip;

  /* An action that just became impossible has its button disabled.
   * An action that became possible again must be reenabled. */
  button->setEnabled(action_prob_possible(act_probs[act]));
  button->setData1(data1);
  button->setData2(data2);
  // The probability may have changed.
  title = QString(action_prepare_ui_name(act, "&", act_probs[act], custom));

  tool_tip = QString(
      act_sel_action_tool_tip(action_by_number(act), act_probs[act]));

  button->setText(title);
  button->setToolTip(tool_tip);
}

/**
   Action Disband Unit for choice dialog
 */
static void disband_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_DISBAND_UNIT, actor_id, target_id, 0, "");
}

/**
   Action "Fortify" for choice dialog
 */
static void fortify(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)) {
    request_do_action(ACTION_FORTIFY, actor_id, target_id, 0, "");
  }
}

/**
   Action Convert Unit for choice dialog
 */
static void convert_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_CONVERT, actor_id, target_id, 0, "");
}

/**
   Action bribe unit for choice dialog
 */
static void diplomat_bribe(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_unit_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = true;

    request_action_details(ACTION_SPY_BRIBE_UNIT, diplomat_id,
                           diplomat_target_id);
  }
}

/**
   Action sabotage unit for choice dialog
 */
static void spy_sabotage_unit(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  request_do_action(ACTION_SPY_SABOTAGE_UNIT, diplomat_id,
                    diplomat_target_id, 0, "");
}

/**
   Action Sabotage Unit Escape for choice dialog
 */
static void spy_sabotage_unit_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  request_do_action(ACTION_SPY_SABOTAGE_UNIT_ESC, diplomat_id,
                    diplomat_target_id, 0, "");
}

/**
   Action "Heal Unit" for choice dialog
 */
static void heal_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_HEAL_UNIT, actor_id, target_id, 0, "");
}

/**
   Action "Transport Board" for choice dialog
 */
static void transport_board(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_TRANSPORT_BOARD, actor_id, target_id, 0, "");
}

/**
   Action "Transport Embark" for choice dialog
 */
static void transport_embark(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_TRANSPORT_EMBARK, actor_id, target_id, 0, "");
}

/**
   Action "Transport Unload" for choice dialog
 */
static void transport_unload(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_TRANSPORT_UNLOAD, actor_id, target_id, 0, "");
}

/**
   Action "Transport Alight" for choice dialog
 */
static void transport_alight(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_TRANSPORT_ALIGHT, actor_id, target_id, 0, "");
}

static void do_that_action(QVariant data1, QVariant data2, enum gen_action a)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)) {
    request_do_action(a, actor_id, target_id, 0, "");
  }
}

/**
   Action "Transport Disembark" for choice dialog
 */
static void disembark1(QVariant data1, QVariant data2)
{
  do_that_action(data1, data2, ACTION_TRANSPORT_DISEMBARK1);
}

/**
   Action "Transport Disembark 2" for choice dialog
 */
static void disembark2(QVariant data1, QVariant data2)
{
  do_that_action(data1, data2, ACTION_TRANSPORT_DISEMBARK2);
}

/**
   Action "Nuke Units" for choice dialog
 */
static void nuke_units(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_NUKE_UNITS, actor_id, target_id, 0, "");
}

/**
   Action capture units for choice dialog
 */
static void capture_units(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_CAPTURE_UNITS, actor_id, target_id, 0, "");
}

/**
   Action expel unit for choice dialog
 */
static void expel_unit(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_EXPEL_UNIT, actor_id, target_id, 0, "");
}

/**
   Action "Bombard" for choice dialog
 */
static void bombard(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_BOMBARD, actor_id, target_id, 0, "");
}

/**
   Action "Bombard 2" for choice dialog
 */
static void bombard2(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_BOMBARD2, actor_id, target_id, 0, "");
}

/**
   Action "Bombard 3" for choice dialog
 */
static void bombard3(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  request_do_action(ACTION_BOMBARD3, actor_id, target_id, 0, "");
}

/**
   Action build city for choice dialog
 */
static void found_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();

  dsend_packet_city_name_suggestion_req(&client.conn, actor_id);
}

/**
   Action "Transform Terrain" for choice dialog
 */
static void transform_terrain(QVariant data1, QVariant data2)
{
  do_that_action(data1, data2, ACTION_TRANSFORM_TERRAIN);
}

/**
   Action "Cultivate" for choice dialog
 */
static void cultivate(QVariant data1, QVariant data2)
{
  do_that_action(data1, data2, ACTION_CULTIVATE);
}

/**
   Action "Plant" for choice dialog
 */
static void plant(QVariant data1, QVariant data2)
{
  do_that_action(data1, data2, ACTION_PLANT);
}

/**
   Action "Pillage" for choice dialog
 */
static void pillage(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_PILLAGE, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Clean Pollution" for choice dialog
 */
static void clean_pollution(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_CLEAN_POLLUTION, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Clean Fallout" for choice dialog
 */
static void clean_fallout(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)) {
    request_do_action(ACTION_CLEAN_FALLOUT, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Build Road" for choice dialog
 */
static void road(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)
      && nullptr != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_ROAD, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Build Base" for choice dialog
 */
static void base(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)
      && nullptr != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_BASE, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Build Mine" for choice dialog
 */
static void mine(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)
      && nullptr != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_MINE, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Build Irrigation" for choice dialog
 */
static void irrigate(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)
      && nullptr != extra_by_number(action_selection_target_extra())) {
    request_do_action(ACTION_IRRIGATE, actor_id, target_id,
                      /* FIXME: will cause problems if more than
                       * one action selection dialog at a time
                       * becomes supported. */
                      action_selection_target_extra(), "");
  }
}

/**
   Action "Explode Nuclear" for choice dialog
 */
static void nuke(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != index_to_tile(&(wld.map), diplomat_target_id)) {
    request_do_action(ACTION_NUKE, diplomat_id, diplomat_target_id, 0, "");
  }
}

/**
   Action "Attack" for choice dialog
 */
static void attack(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != index_to_tile(&(wld.map), diplomat_target_id)) {
    request_do_action(ACTION_ATTACK, diplomat_id, diplomat_target_id, 0, "");
  }
}

/**
   Action "Suicide Attack" for choice dialog
 */
static void suicide_attack(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != index_to_tile(&(wld.map), diplomat_target_id)) {
    request_do_action(ACTION_SUICIDE_ATTACK, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action "Paradrop Unit" for choice dialog
 */
static void paradrop(QVariant data1, QVariant data2)
{
  do_that_action(data1, data2, ACTION_PARADROP);
}

/**
   Action join city for choice dialog
 */
static void join_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(target_id)) {
    request_do_action(ACTION_JOIN_CITY, actor_id, target_id, 0, "");
  }
}

/**
   Action steal tech with spy for choice dialog
 */
static void spy_steal_shared(QVariant data1, QVariant data2,
                             action_id act_id)
{
  QString str;
  QVariant qv1;
  pfcn_void func;
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();
  struct unit *actor_unit = game_unit_by_number(diplomat_id);
  struct city *pvcity = game_city_by_number(diplomat_target_id);
  struct player *pvictim = nullptr;
  choice_dialog *cd;
  QList<QVariant> actor_and_target;

  /* Wait for the player's reply before moving on to the next queued
   * diplomat. */
  is_more_user_input_needed = true;

  if (pvcity) {
    pvictim = city_owner(pvcity);
  }
  cd = king()->get_diplo_dialog();
  if (cd != nullptr) {
    cd->close();
  }
  QString stra;
  cd = new choice_dialog(_("Steal"), _("Steal Technology"),
                         queen()->game_tab_widget,
                         diplomat_queue_handle_secondary);

  // Put both actor and target city in qv1 since qv2 is taken
  actor_and_target.append(diplomat_id);
  actor_and_target.append(diplomat_target_id);
  actor_and_target.append(act_id);
  qv1 = QVariant::fromValue(actor_and_target);

  struct player *pplayer = client.conn.playing;
  if (pvictim) {
    const struct research *presearch = research_get(pplayer);
    const struct research *vresearch = research_get(pvictim);

    advance_index_iterate(A_FIRST, i)
    {
      if (research_invention_gettable(presearch, i,
                                      game.info.tech_steal_allow_holes)
          && research_invention_state(vresearch, i) == TECH_KNOWN
          && research_invention_state(presearch, i) != TECH_KNOWN) {
        func = spy_steal_something;
        str = QString(research_advance_name_translation(presearch, i));
        cd->add_item(str, func, qv1, i);
      }
    }
    advance_index_iterate_end;

    if (actor_unit
        && action_prob_possible(
            actor_unit->client
                .act_prob_cache[get_non_targeted_action_id(act_id)])) {
      stra = QString(_("At %1's Discretion"))
                 .arg(unit_name_translation(actor_unit));
      func = spy_steal_something;
      cd->add_item(stra, func, qv1, A_UNSET);
    }

    cd->set_layout();
    cd->show_me();
  }
}

/**
   Action "Targeted Steal Tech" for choice dialog
 */
static void spy_steal(QVariant data1, QVariant data2)
{
  spy_steal_shared(data1, data2, ACTION_SPY_TARGETED_STEAL_TECH);
}

/**
   Action "Targeted Steal Tech Escape Expected" for choice dialog
 */
static void spy_steal_esc(QVariant data1, QVariant data2)
{
  spy_steal_shared(data1, data2, ACTION_SPY_TARGETED_STEAL_TECH_ESC);
}

/**
   Action steal given tech for choice dialog
 */
static void spy_steal_something(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toList().at(0).toInt();
  int diplomat_target_id = data1.toList().at(1).toInt();
  action_id act_id = data1.toList().at(2).toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    if (data2.toInt() == A_UNSET) {
      // This is the untargeted version.
      request_do_action(get_non_targeted_action_id(act_id), diplomat_id,
                        diplomat_target_id, data2.toInt(), "");
    } else {
      // This is the targeted version.
      request_do_action(act_id, diplomat_id, diplomat_target_id,
                        data2.toInt(), "");
    }
  }
}

/**
   Action request "Surgical Strike Building" list for choice dialog
 */
static void spy_request_strike_bld_list(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = true;

    request_action_details(ACTION_STRIKE_BUILDING, actor_id, target_id);
  }
}

/**
   Action  request sabotage list for choice dialog
 */
static void spy_request_sabotage_list(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = true;

    request_action_details(ACTION_SPY_TARGETED_SABOTAGE_CITY, diplomat_id,
                           diplomat_target_id);
  }
}

/**
   Action request sabotage (and escape) list for choice dialog
 */
static void spy_request_sabotage_esc_list(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = true;

    request_action_details(ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC,
                           diplomat_id, diplomat_target_id);
  }
}

/**
   Action Poison City for choice dialog
 */
static void spy_poison(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_POISON, diplomat_id, diplomat_target_id, 0,
                      "");
  }
}

/**
   Action Poison City Escape for choice dialog
 */
static void spy_poison_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_POISON_ESC, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action suitcase nuke for choice dialog
 */
static void spy_nuke_city(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_NUKE, diplomat_id, diplomat_target_id, 0,
                      "");
  }
}

/**
   Action suitcase nuke escape for choice dialog
 */
static void spy_nuke_city_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_NUKE_ESC, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action "Nuke City" for choice dialog
 */
static void nuke_city(QVariant data1, QVariant data2)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(target_id)) {
    request_do_action(ACTION_NUKE_CITY, actor_id, target_id, 0, "");
  }
}

/**
   Action destroy city for choice dialog
 */
static void destroy_city(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_DESTROY_CITY, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action steal gold for choice dialog
 */
static void spy_steal_gold(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_GOLD, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action steal gold escape for choice dialog
 */
static void spy_steal_gold_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_GOLD_ESC, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/**
   Action steal maps for choice dialog
 */
static void spy_steal_maps(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_STEAL_MAPS, diplomat_id, diplomat_target_id, 0,
                      "");
  }
}

/**
   Action steal maps escape for choice dialog
 */
static void spy_steal_maps_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_STEAL_MAPS_ESC, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action establish embassy for choice dialog
 */
static void spy_embassy(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_ESTABLISH_EMBASSY, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/**
   Action establish embassy for choice dialog
 */
static void diplomat_embassy(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_ESTABLISH_EMBASSY_STAY, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/**
   Action investigate city for choice dialog
 */
static void spy_investigate(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_city_by_number(diplomat_target_id)
      && nullptr != game_unit_by_number(diplomat_id)) {
    request_do_action(ACTION_SPY_INVESTIGATE_CITY, diplomat_id,
                      diplomat_target_id, 0, "");
  }
}

/**
   Action Investigate City Spend Unit for choice dialog
 */
static void diplomat_investigate(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_city_by_number(diplomat_target_id)
      && nullptr != game_unit_by_number(diplomat_id)) {
    request_do_action(ACTION_INV_CITY_SPEND, diplomat_id, diplomat_target_id,
                      0, "");
  }
}

/**
   Action sabotage for choice dialog
 */
static void diplomat_sabotage(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_SABOTAGE_CITY, diplomat_id,
                      diplomat_target_id, B_LAST + 1, "");
  }
}

/**
   Action sabotage and escape for choice dialog
 */
static void diplomat_sabotage_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_SABOTAGE_CITY_ESC, diplomat_id,
                      diplomat_target_id, B_LAST + 1, "");
  }
}

/**
   Action steal with diplomat for choice dialog
 */
static void diplomat_steal(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_TECH, diplomat_id, diplomat_target_id,
                      A_UNSET, "");
  }
}

/**
   Action "Steal Tech Escape Expected" for choice dialog
 */
static void diplomat_steal_esc(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    request_do_action(ACTION_SPY_STEAL_TECH_ESC, diplomat_id,
                      diplomat_target_id, A_UNSET, "");
  }
}

/**
   Action incite revolt for choice dialog
 */
static void diplomat_incite(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = true;

    request_action_details(ACTION_SPY_INCITE_CITY, diplomat_id,
                           diplomat_target_id);
  }
}

/**
   Action incite revolt and escape for choice dialog
 */
static void diplomat_incite_escape(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    /* Wait for the server's reply before moving on to the next queued
     * diplomat. */
    is_more_user_input_needed = true;

    request_action_details(ACTION_SPY_INCITE_CITY_ESC, diplomat_id,
                           diplomat_target_id);
  }
}

/**
   Action keep moving with actor unit for choice dialog
 */
static void act_sel_keep_moving(QVariant data1, QVariant data2)
{
  struct unit *punit;
  struct tile *ptile;
  int diplomat_id = data1.toInt();
  int diplomat_target_id = data2.toInt();

  if ((punit = game_unit_by_number(diplomat_id))
      && (ptile = index_to_tile(&(wld.map), diplomat_target_id))
      && !same_pos(unit_tile(punit), ptile)) {
    request_unit_non_action_move(punit, ptile);
  }
}

/**
   Popup a window asking a diplomatic unit if it wishes to incite the
   given enemy city.
 */
void popup_incite_dialog(struct unit *actor, struct city *tcity, int cost,
                         const struct action *paction)
{
  QString buf, buf2;
  int diplomat_id = actor->id;
  int diplomat_target_id = tcity->id;
  const int action_id = paction->id;

  // Should be set before sending request to the server.
  fc_assert(is_more_user_input_needed);

  buf = QString::asprintf(PL_("Treasury contains %d gold.",
                              "Treasury contains %d gold.",
                              client_player()->economic.gold),
                          client_player()->economic.gold);

  if (INCITE_IMPOSSIBLE_COST == cost) {
    hud_message_box *impossible = new hud_message_box(king()->central_wdg);

    buf2 = QString::asprintf(_("You can't incite a revolt in %s."),
                             city_name_get(tcity));
    impossible->set_text_title(buf2, QStringLiteral("!"));
    impossible->setStandardButtons(QMessageBox::Ok);
    impossible->setAttribute(Qt::WA_DeleteOnClose);
    impossible->show();
  } else if (cost <= client_player()->economic.gold) {
    hud_message_box *ask = new hud_message_box(king()->central_wdg);

    buf2 = QString(PL_("Incite a revolt for %1 gold?\n%2",
                       "Incite a revolt for %1 gold?\n%2", cost))
               .arg(QString::number(cost), buf);
    ask->set_text_title(buf2, _("Incite a Revolt!"));
    ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    ask->setDefaultButton(QMessageBox::Cancel);
    ask->button(QMessageBox::Yes)->setText(_("Yes Incite a Revolt!"));
    ask->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(ask, &hud_message_box::accepted, [=]() {
      request_do_action(action_id, diplomat_id, diplomat_target_id, 0, "");
      diplomat_queue_handle_secondary(diplomat_id);
    });
    ask->show();
    return;
  } else {
    hud_message_box *too_much = new hud_message_box(king()->central_wdg);

    buf2 = QString(PL_("Inciting a revolt costs %1 gold.\n%2",
                       "Inciting a revolt costs %1 gold.\n%2", cost))
               .arg(QString::number(cost), buf);
    too_much->set_text_title(buf2, QStringLiteral("!"));
    too_much->setStandardButtons(QMessageBox::Ok);
    too_much->setAttribute(Qt::WA_DeleteOnClose);
    too_much->show();
  }

  diplomat_queue_handle_secondary(diplomat_id);
}

/**
   Popup a dialog asking a diplomatic unit if it wishes to bribe the
   given enemy unit.
 */
void popup_bribe_dialog(struct unit *actor, struct unit *tunit, int cost,
                        const struct action *paction)
{
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  QString buf, buf2;
  int diplomat_id = actor->id;
  int diplomat_target_id = tunit->id;
  const int action_id = paction->id;

  // Should be set before sending request to the server.
  fc_assert(is_more_user_input_needed);

  buf = QString::asprintf(PL_("Treasury contains %d gold.",
                              "Treasury contains %d gold.",
                              client_player()->economic.gold),
                          client_player()->economic.gold);

  if (cost <= client_player()->economic.gold) {
    buf2 = QString(PL_("Bribe unit for %1 gold?\n%2",
                       "Bribe unit for %1 gold?\n%2", cost))
               .arg(QString::number(cost), buf);
    ask->set_text_title(buf2, _("Bribe Enemy Unit"));
    ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    ask->setDefaultButton(QMessageBox::Cancel);
    ask->button(QMessageBox::Yes)->setText(_("Yes Bribe!"));
    ask->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(ask, &hud_message_box::accepted, [=]() {
      request_do_action(action_id, diplomat_id, diplomat_target_id, 0, "");
      diplomat_queue_handle_secondary(diplomat_id);
    });
    ask->show();
    return;
  } else {
    buf2 = QString(PL_("Bribing the unit costs %1 gold.\n%2",
                       "Bribing the unit costs %1 gold.\n%2", cost))
               .arg(QString::number(cost), buf);
    ask->set_text_title(buf2, _("Traitors Demand Too Much!"));
    ask->setAttribute(Qt::WA_DeleteOnClose);
    ask->show();
  }

  diplomat_queue_handle_secondary(diplomat_id);
}

/**
   Action pillage for choice dialog
 */
static void pillage_something(QVariant data1, QVariant data2)
{
  int punit_id;
  int what;
  struct unit *punit;
  struct extra_type *target;

  what = data1.toInt();
  punit_id = data2.toInt();
  punit = game_unit_by_number(punit_id);
  if (punit) {
    target = extra_by_number(what);
    request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, target);
  }
  ::is_showing_pillage_dialog = false;
}

/**
  User action with target kind ATK_UNIT
 */
static void user_action_unit_vs_unit(int actor_id, int target_id,
                                     action_id act_id)
{
  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_unit_by_number(target_id)) {
    request_do_action(act_id, actor_id, target_id, 0, "");
  }
}

/**
  User action with target kind ATK_CITY
 */
static void user_action_unit_vs_city(int actor_id, int target_id,
                                     action_id act_id)
{
  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != game_city_by_number(target_id)) {
    request_do_action(act_id, actor_id, target_id, 0, "");
  }
}

/**
  User action with target kind ATK_TILE or ATK_UNITS
 */
static void user_action_unit_vs_tile(int actor_id, int target_id,
                                     action_id act_id)
{
  if (nullptr != game_unit_by_number(actor_id)
      && nullptr != index_to_tile(&(wld.map), target_id)) {
    request_do_action(act_id, actor_id, target_id, 0, "");
  }
}

/**
  User action with target kind ATK_SELF
 */
static void user_action_unit_vs_self(int actor_id, int target_id,
                                     action_id act_id)
{
  if (nullptr != game_unit_by_number(actor_id)) {
    request_do_action(act_id, actor_id, target_id, 0, "");
  }
}

/**
  User action handler, dispatches on target kind
 */
static void user_action(QVariant data1, QVariant data2, action_id act_id)
{
  int actor_id = data1.toInt();
  int target_id = data2.toInt();

  switch (action_id_get_target_kind(act_id)) {
  case ATK_UNIT:
    user_action_unit_vs_unit(actor_id, target_id, act_id);
    break;
  case ATK_CITY:
    user_action_unit_vs_city(actor_id, target_id, act_id);
    break;
  case ATK_TILE:
  case ATK_UNITS:
    user_action_unit_vs_tile(actor_id, target_id, act_id);
    break;
  case ATK_SELF:
    user_action_unit_vs_self(actor_id, target_id, act_id);
    break;
  case ATK_COUNT:
    fc_assert_msg(ATK_COUNT != action_id_get_target_kind(act_id),
                  "Bad target kind");
    break;
  }
}

/**
   User Action 1 for choice dialog
 */
static void user_action_1(QVariant data1, QVariant data2)
{
  user_action(data1, data2, ACTION_USER_ACTION1);
}

/**
   User Action 2 for choice dialog
 */
static void user_action_2(QVariant data1, QVariant data2)
{
  user_action(data1, data2, ACTION_USER_ACTION2);
}

/**
   User Action 3 for choice dialog
 */
static void user_action_3(QVariant data1, QVariant data2)
{
  user_action(data1, data2, ACTION_USER_ACTION3);
}

/**
   Action sabotage with spy for choice dialog
 */
static void spy_sabotage(QVariant data1, QVariant data2)
{
  int diplomat_id = data1.toList().at(0).toInt();
  int diplomat_target_id = data1.toList().at(1).toInt();
  action_id act_id = data1.toList().at(2).toInt();

  if (nullptr != game_unit_by_number(diplomat_id)
      && nullptr != game_city_by_number(diplomat_target_id)) {
    if (data2.toInt() == B_LAST) {
      // This is the untargeted version.
      request_do_action(get_non_targeted_action_id(act_id), diplomat_id,
                        diplomat_target_id, data2.toInt(), "");
    } else if (data2.toInt() == -1) {
      // This is the city production version.
      request_do_action(get_production_targeted_action_id(act_id),
                        diplomat_id, diplomat_target_id, data2.toInt(), "");
    } else {
      // This is the targeted version.
      request_do_action(act_id, diplomat_id, diplomat_target_id,
                        data2.toInt(), "");
    }
  }
}

/**
   Popup a dialog asking a diplomatic unit if it wishes to sabotage the
   given enemy city.
 */
void popup_sabotage_dialog(struct unit *actor, struct city *tcity,
                           const struct action *paction)
{
  QVariant qv1, qv2;
  int diplomat_id = actor->id;
  int diplomat_target_id = tcity->id;
  pfcn_void func;
  choice_dialog *cd = new choice_dialog(
      _("Sabotage"), _("Select Improvement to Sabotage"),
      queen()->game_tab_widget, diplomat_queue_handle_secondary);
  int nr = 0;
  QString stra;
  QList<QVariant> actor_and_target;

  // Should be set before sending request to the server.
  fc_assert(is_more_user_input_needed);

  // Put both actor, target city and action in qv1 since qv2 is taken
  actor_and_target.append(diplomat_id);
  actor_and_target.append(diplomat_target_id);
  actor_and_target.append(paction->id);
  qv1 = QVariant::fromValue(actor_and_target);

  if (action_prob_possible(
          actor->client.act_prob_cache[get_production_targeted_action_id(
              paction->id)])) {
    func = spy_sabotage;
    cd->add_item(QString(_("City Production")), func, qv1, -1);
  }

  city_built_iterate(tcity, pimprove)
  {
    if (pimprove->sabotage > 0) {
      func = spy_sabotage;
      stra = QString(city_improvement_name_translation(tcity, pimprove));
      qv2 = nr;
      cd->add_item(stra, func, qv1, improvement_number(pimprove));
      nr++;
    }
  }
  city_built_iterate_end;

  if (action_prob_possible(
          actor->client
              .act_prob_cache[get_non_targeted_action_id(paction->id)])) {
    stra =
        QString(_("At %1's Discretion")).arg(unit_name_translation(actor));
    func = spy_sabotage;
    cd->add_item(stra, func, qv1, B_LAST);
  }
  cd->set_layout();
  cd->show_me();
}

/**
   Popup a dialog asking the unit which improvement they would like to
   pillage.
 */
void popup_pillage_dialog(struct unit *punit, bv_extras extras)
{
  QString str;
  QVariant qv1, qv2;
  pfcn_void func;
  choice_dialog *cd;
  struct extra_type *tgt;

  if (is_showing_pillage_dialog) {
    return;
  }
  cd = new choice_dialog(_("What To Pillage"), _("Select what to pillage:"),
                         queen()->game_tab_widget);
  qv2 = punit->id;
  while ((tgt = get_preferred_pillage(extras))) {
    int what;

    what = extra_index(tgt);
    BV_CLR(extras, what);

    func = pillage_something;
    str = QString(extra_name_translation(tgt));
    qv1 = what;
    cd->add_item(str, func, qv1, qv2);
  }
  cd->set_layout();
  cd->show_me();
}

/**
   Disband Message box contructor
 */
disband_box::disband_box(const std::vector<unit *> &punits, QWidget *parent)
    : hud_message_box(parent), cpunits(punits)
{
  QString str;
  QPushButton *pb;

  setAttribute(Qt::WA_DeleteOnClose);
  setModal(false);

  str = QString(PL_("Are you sure you want to disband that %1 unit?",
                    "Are you sure you want to disband those %1 units?",
                    punits.size()))
            .arg(punits.size());
  pb = addButton(_("Yes"), QMessageBox::AcceptRole);
  addButton(_("No"), QMessageBox::RejectRole);
  set_text_title(str, _("Disband units"));
  setDefaultButton(pb);
  connect(pb, &QAbstractButton::clicked, this,
          &disband_box::disband_clicked);
  pb->show();
}

/**
   Clicked Yes in disband box
 */
void disband_box::disband_clicked()
{
  for (const auto punit : cpunits) {
    if (unit_can_do_action(punit, ACTION_DISBAND_UNIT)) {
      request_unit_disband(punit);
    }
  }
}

/**
   Destructor for disband box
 */
disband_box::~disband_box() = default;

/**
   Pops up a dialog to confirm disband of the unit(s).
 */
void popup_disband_dialog(const std::vector<unit *> &punits)
{
  disband_box *ask = new disband_box(punits, king()->central_wdg);
  ask->show();
}

/**
   Ruleset (modpack) has suggested loading certain tileset. Confirm from
   user and load.
 */
void popup_tileset_suggestion_dialog(void)
{
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  QString text;
  QString title;

  title = QString(_("Modpack suggests using %1 tileset."))
              .arg(game.control.preferred_tileset);
  text = QStringLiteral("It might not work with other tilesets.\n"
                        "You are currently using tileset %1.")
             .arg(tileset_basename(tileset));
  ask->addButton(_("Keep current tileset"), QMessageBox::RejectRole);
  ask->addButton(_("Load tileset"), QMessageBox::AcceptRole);
  ask->set_text_title(text, title);
  ask->setAttribute(Qt::WA_DeleteOnClose);

  QObject::connect(ask, &hud_message_box::accepted, [=]() {
    forced_tileset_name = game.control.preferred_tileset;
    if (!tilespec_reread(game.control.preferred_tileset, true)) {
      tileset_error(nullptr, LOG_ERROR,
                    _("Can't load requested tileset %s."),
                    game.control.preferred_tileset);
    }
  });
  ask->show();
}

/**
   Ruleset (modpack) has suggested loading certain soundset. Confirm from
   user and load.
 */
void popup_soundset_suggestion_dialog(void)
{
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  QString text;
  QString title;

  title = QString(_("Modpack suggests using %1 soundset."))
              .arg(game.control.preferred_soundset);
  text = QStringLiteral("It might not work with other tilesets.\n"
                        "You are currently using soundset %1.")
             .arg(sound_set_name);
  ask->addButton(_("Keep current soundset"), QMessageBox::RejectRole);
  ask->addButton(_("Load soundset"), QMessageBox::AcceptRole);
  ask->set_text_title(text, title);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(ask, &hud_message_box::accepted, [=]() {
    audio_restart(game.control.preferred_soundset, music_set_name);
  });
  ask->show();
}

/**
   Ruleset (modpack) has suggested loading certain musicset. Confirm from
   user and load.
 */
void popup_musicset_suggestion_dialog(void)
{
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  QString text;
  QString title;

  title = QString(_("Modpack suggests using %1 musicset."))
              .arg(game.control.preferred_musicset);
  text = QStringLiteral("It might not work with other tilesets.\n"
                        "You are currently using musicset %1.")
             .arg(music_set_name);
  ask->addButton(_("Keep current musicset"), QMessageBox::RejectRole);
  ask->addButton(_("Load musicset"), QMessageBox::AcceptRole);
  ask->set_text_title(text, title);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(ask, &hud_message_box::accepted, [=]() {
    audio_restart(sound_set_name, game.control.preferred_musicset);
  });
  ask->show();
}

/**
   Tileset (modpack) has suggested loading certain theme. Confirm from
   user and load.
 */
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  Q_UNUSED(theme_name)
  return false;
}

/**
   This function is called when the client disconnects or the game is
   over.  It should close all dialog windows for that game.
 */
void popdown_all_game_dialogs(void)
{
  int i;
  QList<choice_dialog *> cd_list;
  QList<freeciv::report_widget *> nd_list;

  QApplication::alert(king()->central_wdg);
  cd_list = queen()->game_tab_widget->findChildren<choice_dialog *>();
  for (i = 0; i < cd_list.count(); i++) {
    cd_list[i]->close();
  }
  nd_list =
      queen()->game_tab_widget->findChildren<freeciv::report_widget *>();
  for (i = 0; i < nd_list.count(); i++) {
    nd_list[i]->close();
  }

  popdown_help_dialog();
  popdown_players_report();
  popdown_all_spaceships_dialogs();
  popdown_economy_report();
  popdown_science_report();
  popdown_city_report();
  popdown_endgame_report();
  popdown_unit_sel();
}

/**
   Returns the id of the actor unit currently handled in action selection
   dialog when the action selection dialog is open.
   Returns IDENTITY_NUMBER_ZERO if no action selection dialog is open.
 */
int action_selection_actor_unit(void)
{
  choice_dialog *cd = king()->get_diplo_dialog();

  if (cd != nullptr) {
    return cd->unit_id;
  } else {
    return IDENTITY_NUMBER_ZERO;
  }
}

/**
   Returns id of the target city of the actions currently handled in action
   selection dialog when the action selection dialog is open and it has a
   city target. Returns IDENTITY_NUMBER_ZERO if no action selection dialog
   is open or no city target is present in the action selection dialog.
 */
int action_selection_target_city(void)
{
  choice_dialog *cd = king()->get_diplo_dialog();

  if (cd != nullptr) {
    return cd->target_id[ATK_CITY];
  } else {
    return IDENTITY_NUMBER_ZERO;
  }
}

/**
   Returns id of the target tile of the actions currently handled in action
   selection dialog when the action selection dialog is open and it has a
   tile target. Returns TILE_INDEX_NONE if no action selection dialog is
   open.
 */
int action_selection_target_tile(void)
{
  choice_dialog *cd = king()->get_diplo_dialog();

  if (cd != nullptr) {
    return cd->target_id[ATK_TILE];
  } else {
    return TILE_INDEX_NONE;
  }
}

/**
   Returns id of the target extra of the actions currently handled in action
   selection dialog when the action selection dialog is open and it has an
   extra target. Returns EXTRA_NONE if no action selection dialog is open
   or no extra target is present in the action selection dialog.
 */
int action_selection_target_extra(void)
{
  choice_dialog *cd = king()->get_diplo_dialog();

  if (cd != nullptr) {
    return cd->sub_target_id[ASTK_EXTRA];
  } else {
    return EXTRA_NONE;
  }
}

/**
   Returns id of the target unit of the actions currently handled in action
   selection dialog when the action selection dialog is open and it has a
   unit target. Returns IDENTITY_NUMBER_ZERO if no action selection dialog
   is open or no unit target is present in the action selection dialog.
 */
int action_selection_target_unit(void)
{
  choice_dialog *cd = king()->get_diplo_dialog();

  if (cd != nullptr) {
    return cd->target_id[ATK_UNIT];
  } else {
    return IDENTITY_NUMBER_ZERO;
  }
}

/**
   Updates the action selection dialog with new information.
 */
void action_selection_refresh(struct unit *actor_unit,
                              struct city *target_city,
                              struct unit *target_unit,
                              struct tile *target_tile,
                              struct extra_type *target_extra,
                              const struct act_prob *act_probs)
{
  Q_UNUSED(target_extra)
  choice_dialog *asd;
  Choice_dialog_button *keep_moving_button;
  Choice_dialog_button *wait_button;
  Choice_dialog_button *cancel_button;
  QVariant qv1, qv2;

  asd = king()->get_diplo_dialog();
  if (asd == nullptr) {
    fc_assert_msg(asd != nullptr,
                  "The action selection dialog should have been open");
    return;
  }

  if (actor_unit->id != action_selection_actor_unit()) {
    fc_assert_msg(actor_unit->id == action_selection_actor_unit(),
                  "The action selection dialog is for another actor unit.");
  }

  // Put the actor id in qv1.
  qv1 = actor_unit->id;

  cancel_button = asd->get_identified_button(BUTTON_CANCEL);
  if (cancel_button != nullptr) {
    /* Temporary remove the Cancel button so it won't end up above
     * any added buttons. */
    asd->stack_button(cancel_button);
  }

  wait_button = asd->get_identified_button(BUTTON_WAIT);
  if (wait_button != nullptr) {
    /* Temporary remove the Wait button so it won't end up above
     * any added buttons. */
    asd->stack_button(wait_button);
  }

  keep_moving_button = asd->get_identified_button(BUTTON_MOVE);
  if (keep_moving_button != nullptr) {
    /* Temporary remove the Keep moving button so it won't end up above
     * any added buttons. */
    asd->stack_button(keep_moving_button);
  }

  action_iterate(act)
  {
    QString custom;

    if (action_id_get_actor_kind(act) != AAK_UNIT) {
      // Not relevant.
      continue;
    }

    custom = get_act_sel_action_custom_text(
        action_by_number(act), act_probs[act], actor_unit, target_city);

    // Put the target id in qv2.
    switch (action_id_get_target_kind(act)) {
    case ATK_UNIT:
      if (target_unit != nullptr) {
        qv2 = target_unit->id;
      } else {
        fc_assert_msg(!action_prob_possible(act_probs[act])
                          || target_unit != nullptr,
                      "Action enabled against non existing unit!");

        qv2 = IDENTITY_NUMBER_ZERO;
      }
      break;
    case ATK_CITY:
      if (target_city != nullptr) {
        qv2 = target_city->id;
      } else {
        fc_assert_msg(!action_prob_possible(act_probs[act])
                          || target_city != nullptr,
                      "Action enabled against non existing city!");

        qv2 = IDENTITY_NUMBER_ZERO;
      }
      break;
    case ATK_TILE:
    case ATK_UNITS:
      if (target_tile != nullptr) {
        qv2 = tile_index(target_tile);
      } else {
        fc_assert_msg(!action_prob_possible(act_probs[act])
                          || target_tile != nullptr,
                      "Action enabled against all units on "
                      "non existing tile!");

        qv2 = TILE_INDEX_NONE;
      }
      break;
    case ATK_SELF:
      qv2 = actor_unit->id;
      break;
    case ATK_COUNT:
      fc_assert_msg(ATK_COUNT != action_id_get_target_kind(act),
                    "Bad target kind");
      continue;
    }

    if (asd->get_identified_button(act)) {
      // Update the existing button.
      action_entry_update(asd->get_identified_button(act), act, act_probs,
                          custom, qv1, qv2);
    } else {
      // Add the button (unless its probability is 0).
      action_entry(asd, act, act_probs, custom, qv1, qv2);
    }
  }
  action_iterate_end;

  if (keep_moving_button != nullptr || wait_button != nullptr
      || cancel_button != nullptr) {
    /* Reinsert the "Keep moving" button below any potential
     * buttons recently added. */
    asd->unstack_all_buttons();
  }
}

/**
   Closes the action selection dialog
 */
void action_selection_close(void)
{
  choice_dialog *cd;

  cd = king()->get_diplo_dialog();
  if (cd != nullptr) {
    did_not_decide = true;
    cd->close();
  }
}

/**
   Player has gained a new tech.
 */
void show_tech_gained_dialog(Tech_type_id tech) { Q_UNUSED(tech) }

/**
   Popup dialog for upgrade units
 */
void popup_upgrade_dialog(const std::vector<unit *> &punits)
{
  char buf[512];
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  QString title;
  QVector<int> *punit_ids;

  if (punits.empty()) {
    return;
  }

  punit_ids = new QVector<int>();
  for (const auto punit : punits) {
    punit_ids->push_back(punit->id);
  }

  if (!get_units_upgrade_info(buf, sizeof(buf), punits)) {
    title = _("Upgrade Unit!");
    ask->setStandardButtons(QMessageBox::Ok);
  } else {
    title = _("Upgrade Obsolete Units");
    ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    ask->setDefaultButton(QMessageBox::Cancel);
    ask->button(QMessageBox::Yes)->setText(_("Yes Upgrade"));
  }
  ask->set_text_title(buf, title);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(ask, &hud_message_box::accepted, [=]() {
    std::unique_ptr<QVector<int>> uptr(punit_ids);
    QVector<int> not_ptr = *uptr;
    struct unit *punit;

    for (int id : not_ptr) {
      punit = game_unit_by_number(id);
      if (punit) {
        request_unit_upgrade(punit);
      }
    }
  });
  ask->show();
}

/**
   Set current diplo dialog
 */
void fc_client::set_diplo_dialog(choice_dialog *widget)
{
  opened_dialog = widget;
}

/**
   Get current diplo dialog
 */
choice_dialog *fc_client::get_diplo_dialog() { return opened_dialog; }

/**
   Give a warning when user is about to edit scenario with manually
   set properties.
 */
bool handmade_scenario_warning()
{
  // Just tell the client common code to handle this.
  return false;
}

/**
   Unit wants to get into some transport on given tile.
 */
bool request_transport(struct unit *pcargo, struct tile *ptile)
{
  int tcount;
  hud_unit_loader *hul;
  struct unit_list *potential_transports = unit_list_new();
  struct unit *best_transport = transporter_for_unit_at(pcargo, ptile);

  unit_list_iterate(ptile->units, ptransport)
  {
    if (can_unit_transport(ptransport, pcargo)
        && get_transporter_occupancy(ptransport)
               < get_transporter_capacity(ptransport)) {
      unit_list_append(potential_transports, ptransport);
    }
  }
  unit_list_iterate_end;

  tcount = unit_list_size(potential_transports);

  if (tcount == 0) {
    fc_assert(best_transport == nullptr);
    unit_list_destroy(potential_transports);

    return false; // Unit was not handled here.
  } else if (tcount == 1) {
    // There's exactly one potential transport - use it automatically
    fc_assert(unit_list_get(potential_transports, 0) == best_transport);
    request_unit_load(pcargo, unit_list_get(potential_transports, 0), ptile);

    unit_list_destroy(potential_transports);

    return true;
  }

  hul = new hud_unit_loader(pcargo, ptile);
  hul->show_me();
  unit_list_destroy(potential_transports);
  return true;
}

/**
   Popup detailed information about battle or save information for
   some kind of statistics
 */
void popup_combat_info(int attacker_unit_id, int defender_unit_id,
                       int attacker_hp, int defender_hp,
                       bool make_att_veteran, bool make_def_veteran)
{
  if (king()->qt_settings.show_battle_log) {
    hud_unit_combat *huc = new hud_unit_combat(
        attacker_unit_id, defender_unit_id, attacker_hp, defender_hp,
        make_att_veteran, make_def_veteran, queen()->battlelog_wdg->scale,
        queen()->battlelog_wdg);

    queen()->battlelog_wdg->add_combat_info(huc);
    queen()->battlelog_wdg->show();
  }
}
