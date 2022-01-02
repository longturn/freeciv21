/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "unitreport.h"
// Qt
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
// common
#include "movement.h"
#include "text.h"
// client
#include "client_main.h"
#include "climisc.h"
#include "mapview_common.h"
#include "sprite.h"
// gui-qt
#include "canvas.h"
#include "fc_client.h"
#include "fonts.h"
#include "goto.h"
#include "hudwidget.h"
#include "page_game.h"

namespace /* anonymous */ {

/**
 * \brief A QTableWidgetItem that can be sorted based on a custom value
 */
class sortable_list_widget_item : public QTableWidgetItem {
  int m_sort_value;

public:
  /// \brief Constructor.
  sortable_list_widget_item(const QString &text, int sort_value)
      : QTableWidgetItem(text, UserType), m_sort_value(sort_value)
  {
  }

  /// \brief Reimplemented public function.
  bool operator<(const QTableWidgetItem &other) const override
  {
    if (const auto sortable =
            dynamic_cast<const sortable_list_widget_item *>(&other)) {
      return m_sort_value < sortable->m_sort_value;
    } else {
      return QTableWidgetItem::operator<(other);
    }
  }
};

} // anonymous namespace

units_reports *units_reports::m_instance = nullptr;

units_waiting::units_waiting(QWidget *parent)
{
  setParent(parent);
  setAttribute(Qt::WA_DeleteOnClose);
  waiting_units = new QTableWidget(this);
  QVBoxLayout *vb = new QVBoxLayout(this);

  QStringList headersLabels;
  headersLabels << _("Type") << _("Location") << _("Mp") << _("Time left");
  waiting_units->setColumnCount(headersLabels.count());
  waiting_units->setHorizontalHeaderLabels(headersLabels);

  waiting_units->setProperty("showGrid", "false");
  waiting_units->setProperty("selectionBehavior", "SelectRows");
  waiting_units->setEditTriggers(QAbstractItemView::NoEditTriggers);
  waiting_units->verticalHeader()->setVisible(false);
  waiting_units->horizontalHeader()->setVisible(true);
  waiting_units->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  waiting_units->setSelectionMode(QAbstractItemView::SingleSelection);
  waiting_units->horizontalHeader()->resizeSections(QHeaderView::Stretch);
  waiting_units->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  waiting_units->setProperty("unit_wait_table", true);
  waiting_units->setSortingEnabled(true);
  vb->addWidget(waiting_units, Qt::AlignHCenter);
  setLayout(vb);
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &units_waiting::update_units);
  connect(waiting_units, &QTableWidget::cellClicked, this,
          &units_waiting::clicked);
  timer->start(1000);

  update_units();
}

units_waiting::~units_waiting() {}

void units_waiting::clicked(int x, int y)
{
  Q_UNUSED(y);

  QTableWidgetItem *item = waiting_units->item(x, 0);
  struct unit *punit =
      static_cast<unit *>(item->data(Qt::UserRole).value<void *>());

  if (punit) {
    unit_focus_set(punit);
    center_tile_mapcanvas(punit->tile);
  }
}

void units_waiting::update_units()
{
  int units_count = 0;

  waiting_units->clearContents();

  if (!client_has_player()) {
    return;
  }

  unit_list_iterate(client_player()->units, punit)
  {
    if (!can_unit_move_now(punit) && punit->ssa_controller == SSA_NONE) {
      QTableWidgetItem *item =
          new QTableWidgetItem(utype_name_translation(punit->utype));
      item->setData(Qt::UserRole,
                    QVariant::fromValue(static_cast<void *>(punit)));
      waiting_units->setItem(units_count, 0, item);

      int pcity_near_dist;
      struct city *pcity_near = get_nearest_city(punit, &pcity_near_dist);
      waiting_units->setItem(
          units_count, 1,
          new sortable_list_widget_item(
              get_nearest_city_text(pcity_near, pcity_near_dist),
              pcity_near_dist));

      waiting_units->setItem(units_count, 2,
                             new sortable_list_widget_item(
                                 move_points_text(punit->moves_left, false),
                                 punit->moves_left));

      time_t dt = time(NULL) - punit->action_timestamp;
      if (dt < 0 && !can_unit_move_now(punit)) {
        char buf[64];
        format_time_duration(-dt, buf, sizeof(buf));
        waiting_units->setItem(units_count, 3,
                               new sortable_list_widget_item(buf, dt));
      }

      ++units_count;
    }
  }
  unit_list_iterate_end;
  waiting_units->setRowCount(units_count);
  waiting_units->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);

  if (units_reports::exists() && units_count
      && units_reports::instance()->isVisible()) {
    show();
  } else {
    hide();
  }

  auto max_height = parentWidget()->height() - y();
  auto want = sizeHint();
  setFixedSize(want.width(), std::min(want.height(), max_height));
}

void units_waiting::showEvent(QShowEvent *event) { update_units(); }

/**
   Unit item constructor (single item for units report)
 */
unittype_item::unittype_item(QWidget *parent, struct unit_type *ut)
    : QFrame(parent)
{
  int isize;
  QFontMetrics *fm;
  QHBoxLayout *hbox;
  QHBoxLayout *hbox_top;
  QHBoxLayout *hbox_upkeep;
  QImage cropped_img;
  QImage img;
  QLabel *lab;
  QPixmap pix;
  QRect crop;
  QSizePolicy size_fixed_policy(QSizePolicy::Maximum, QSizePolicy::Maximum,
                                QSizePolicy::Slider);
  QSpacerItem *spacer;
  QVBoxLayout *vbox;
  QVBoxLayout *vbox_main;
  const QPixmap *spr;

  setParent(parent);
  utype = ut;
  init_img();
  unit_scroll = 0;
  setSizePolicy(size_fixed_policy);
  auto f = fcFont::instance()->getFont(fonts::default_font);
  fm = new QFontMetrics(f);
  isize = fm->height() * 2 / 3;
  vbox_main = new QVBoxLayout();
  hbox = new QHBoxLayout();
  vbox = new QVBoxLayout();
  hbox_top = new QHBoxLayout();
  upgrade_button.setText(QStringLiteral("★"));
  upgrade_button.setVisible(false);
  connect(&upgrade_button, &QAbstractButton::pressed, this,
          &unittype_item::upgrade_units);
  hbox_top->addWidget(&upgrade_button, 0, Qt::AlignLeft);
  label_info_unit.setTextFormat(Qt::PlainText);
  hbox_top->addWidget(&label_info_unit);
  vbox_main->addLayout(hbox_top);
  vbox->addWidget(&label_info_active);
  vbox->addWidget(&label_info_inbuild);
  hbox->addWidget(&label_pix);
  hbox->addLayout(vbox);
  vbox_main->addLayout(hbox);
  hbox_upkeep = new QHBoxLayout;
  hbox_upkeep->addWidget(&shield_upkeep);
  lab = new QLabel(QLatin1String(""));
  spr = tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.shield",
                                    "citybar.shields", "", "", false);
  img = spr->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  pix = QPixmap::fromImage(cropped_img);
  lab->setPixmap(pix.scaledToHeight(isize));
  hbox_upkeep->addWidget(lab);
  spacer = new QSpacerItem(0, isize, QSizePolicy::Expanding,
                           QSizePolicy::Minimum);
  hbox_upkeep->addSpacerItem(spacer);
  hbox_upkeep->addWidget(&gold_upkeep);
  spr = get_tax_sprite(tileset, O_GOLD);
  lab = new QLabel(QLatin1String(""));
  lab->setPixmap(spr->scaledToHeight(isize));
  hbox_upkeep->addWidget(lab);
  spacer = new QSpacerItem(0, isize, QSizePolicy::Expanding,
                           QSizePolicy::Minimum);
  hbox_upkeep->addSpacerItem(spacer);
  hbox_upkeep->addWidget(&food_upkeep);
  lab = new QLabel(QLatin1String(""));
  spr = tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "citybar.food",
                                    "citybar.food", "", "", false);
  img = spr->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  pix = QPixmap::fromImage(cropped_img);
  lab->setPixmap(pix.scaledToHeight(isize));
  hbox_upkeep->addWidget(lab);
  vbox_main->addLayout(hbox_upkeep);
  setLayout(vbox_main);
  entered = false;
  delete fm;
}

/**
   Unit item destructor
 */
unittype_item::~unittype_item() = default;

/**
   Sets unit type pixmap to label
 */
void unittype_item::init_img()
{
  auto sp = get_unittype_sprite(get_tileset(), utype, direction8_invalid());
  label_pix.setPixmap(*sp);
}

/**
   Popup question if to upgrade units
 */
void unittype_item::upgrade_units()
{
  QString b, c;
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  int price;
  const struct unit_type *upgrade;
  const Unit_type_id type = utype_number(utype);

  upgrade = can_upgrade_unittype(client_player(), utype);
  price = unit_upgrade_price(client_player(), utype, upgrade);
  b = QString::asprintf(PL_("Treasury contains %d gold.",
                            "Treasury contains %d gold.",
                            client_player()->economic.gold),
                        client_player()->economic.gold);
  c = QString::asprintf(PL_("Upgrade as many %s to %s as possible "
                            "for %d gold each?\n%s",
                            "Upgrade as many %s to %s as possible "
                            "for %d gold each?\n%s",
                            price),
                        utype_name_translation(utype),
                        utype_name_translation(upgrade), price,
                        b.toUtf8().data());
  ask->set_text_title(c, _("Upgrade Obsolete Units"));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted,
          [=]() { dsend_packet_unit_type_upgrade(&client.conn, type); });
  ask->show();
}

/**
   Mouse entered widget
 */
void unittype_item::enterEvent(QEvent *event)
{
  Q_UNUSED(event)
  entered = true;
  update();
}

/**
   Paint event for unittype item ( draws background from theme )
 */
void unittype_item::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)
  QRect rfull;
  QPainter p;

  if (entered) {
    rfull = QRect(1, 1, width() - 2, height() - 2);
    p.begin(this);
    p.setPen(QColor(palette().color(QPalette::Highlight)));
    p.drawRect(rfull);
    p.end();
  }
}

/**
   Mouse left widget
 */
void unittype_item::leaveEvent(QEvent *event)
{
  Q_UNUSED(event)
  entered = false;
  update();
}

/**
   Mouse wheel event - cycles via units for given unittype
 */
void unittype_item::wheelEvent(QWheelEvent *event)
{
  int unit_count = 0;

  if (client_is_observer() && client.conn.playing == NULL) {
    return;
  }

  unit_list_iterate(client_player()->units, punit)
  {
    if (punit->utype != utype) {
      continue;
    }
    if (ACTIVITY_IDLE == punit->activity
        || ACTIVITY_SENTRY == punit->activity) {
      if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        unit_count++;
      }
    }
  }
  unit_list_iterate_end;

  if (event->angleDelta().y() < 0) {
    unit_scroll--;
  } else {
    unit_scroll++;
  }
  if (unit_scroll < 0) {
    unit_scroll = unit_count;
  } else if (unit_scroll > unit_count) {
    unit_scroll = 0;
  }

  unit_count = 0;

  unit_list_iterate(client_player()->units, punit)
  {
    if (punit->utype != utype) {
      continue;
    }
    if (ACTIVITY_IDLE == punit->activity
        || ACTIVITY_SENTRY == punit->activity) {
      if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        unit_count++;
        if (unit_count == unit_scroll) {
          unit_focus_set_and_select(punit);
        }
      }
    }
  }
  unit_list_iterate_end;
  event->accept();
}

/**
   Class representing list of unit types ( unit_items )
 */
units_reports::units_reports() : fcwidget()
{
  layout = new QHBoxLayout;
  scroll_layout = new QHBoxLayout(this);
  init_layout();
  setParent(queen()->mapview_wdg);
  cw = new close_widget(this);
  cw->setFixedSize(12, 12);
  setVisible(false);
  uw = new units_waiting(queen()->mapview_wdg);
  uw->setVisible(false);
  int x, y;
  x = width();
  y = height();
  queen()->mapview_wdg->find_place(0, 4, x, y, x, y, 0, true);
  move(x, y);
}

/**
   Destructor for unit_report
 */
units_reports::~units_reports()
{
  qDeleteAll(unittype_list);
  unittype_list.clear();
  uw->close();
  uw->deleteLater();
  delete cw;
}

/**
   Adds one unit to list
 */
void units_reports::add_item(unittype_item *item)
{
  unittype_list.append(item);
}

/**
  Check is instance exists
 */
bool units_reports::exists() { return static_cast<bool>(m_instance); }

/**
   Returns instance of units_reports
 */
units_reports *units_reports::instance()
{
  if (!m_instance) {
    m_instance = new units_reports;
  }
  return m_instance;
}

/**
   Deletes units_reports instance
 */
void units_reports::drop() { NFCN_FREE(m_instance); }

/**
   Called when close button was pressed
 */
void units_reports::update_menu()
{
  was_destroyed = true;
  drop();
}

/**
   Initiazlizes layout ( layout needs to be changed after adding units )
 */
void units_reports::init_layout()
{
  QSizePolicy size_fixed_policy(QSizePolicy::Maximum, QSizePolicy::Maximum,
                                QSizePolicy::Slider);

  scroll = new QScrollArea(this);
  setSizePolicy(size_fixed_policy);
  scroll->setWidgetResizable(true);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_widget.setLayout(layout);
  scroll->setWidget(&scroll_widget);
  scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  scroll->setProperty("city_scroll", true);
  scroll_layout->addWidget(scroll);
  setLayout(scroll_layout);
}

/**
   Paint event
 */
void units_reports::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  cw->put_to_corner();
}

/**
   Resize event
 */
void units_reports::resizeEvent(QResizeEvent *event)
{
  Q_UNUSED(event);
  uw->move(pos().x(), height() + pos().y());
}

void units_reports::hideEvent(QHideEvent *event)
{
  Q_UNUSED(event);
  destroy();
}

/**
   Updates units
 */
void units_reports::update_units(bool show)
{
  struct urd_info {
    int active_count;
    int building_count;
    int upkeep[O_LAST];
  };
  bool upgradable;
  int i, j;
  int output;
  int total_len = 0;
  struct urd_info *info;
  struct urd_info *unit_array = new struct urd_info[utype_count()]();
  struct urd_info unit_totals;
  Unit_type_id utype_id;
  unittype_item *ui = nullptr;

  clear_layout();
  memset(&unit_totals, '\0', sizeof(unit_totals));
  // Count units.
  players_iterate(pplayer)
  {
    if (client_has_player() && pplayer != client_player()) {
      continue;
    }
    unit_list_iterate(pplayer->units, punit)
    {
      info = unit_array + utype_index(unit_type_get(punit));
      if (0 != punit->homecity) {
        for (output = 0; output < O_LAST; output++) {
          info->upkeep[output] += punit->upkeep[output];
        }
      }
      info->active_count++;
    }
    unit_list_iterate_end;
    city_list_iterate(pplayer->cities, pcity)
    {
      if (VUT_UTYPE == pcity->production.kind) {
        int num_units;
        info = unit_array + utype_index(pcity->production.value.utype);
        // Account for build slots in city
        (void) city_production_build_units(pcity, true, &num_units);
        // Unit is in progress even if it won't be done this turn
        num_units = MAX(num_units, 1);
        info->building_count += num_units;
      }
    }
    city_list_iterate_end;
  }
  players_iterate_end;

  unit_type_iterate(utype)
  {
    utype_id = utype_index(utype);
    info = unit_array + utype_id;
    upgradable = client_has_player()
                 && nullptr != can_upgrade_unittype(client_player(), utype);
    if (0 == info->active_count && 0 == info->building_count) {
      continue; // We don't need a row for this type.
    }
    ui = new unittype_item(this, utype);
    ui->label_info_active.setText("⚔:"
                                  + QString::number(info->active_count));
    ui->label_info_inbuild.setText("⚒:"
                                   + QString::number(info->building_count));
    ui->label_info_unit.setText(utype_name_translation(utype));
    if (upgradable) {
      ui->upgrade_button.setVisible(true);
    } else {
      ui->upgrade_button.setVisible(false);
    }
    ui->shield_upkeep.setText(QString::number(info->upkeep[O_SHIELD]));
    ui->food_upkeep.setText(QString::number(info->upkeep[O_FOOD]));
    ui->gold_upkeep.setText(QString::number(info->upkeep[O_GOLD]));
    add_item(ui);
  }
  unit_type_iterate_end;

  setUpdatesEnabled(false);
  hide();
  i = unittype_list.count();
  for (j = 0; j < i; j++) {
    ui = unittype_list[j];
    layout->addWidget(ui, 0, Qt::AlignVCenter);
    total_len = total_len + ui->sizeHint().width() + 18;
  }

  total_len =
      total_len + contentsMargins().left() + contentsMargins().right();
  if (show) {
    setVisible(true);
  }
  setUpdatesEnabled(true);
  setFixedWidth(qMin(total_len, queen()->mapview_wdg->width()));
  if (ui != nullptr) {
    setFixedHeight(ui->height() + 60);
  }
  layout->update();
  updateGeometry();
  delete[] unit_array;

  uw->setVisible(true);
}

/**
   Mouse press event -activates unit and closes dialog
 */
void units_reports::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    event->accept();
    close();
  }
}

/**
   Cleans layout - run it before layout initialization
 */
void units_reports::clear_layout()
{
  int i = unittype_list.count();
  unittype_item *ui;
  int j;

  setUpdatesEnabled(false);
  setMouseTracking(false);

  for (j = 0; j < i; j++) {
    ui = unittype_list[j];
    layout->removeWidget(ui);
    delete ui;
  }

  while (!unittype_list.empty()) {
    unittype_list.removeFirst();
  }

  setMouseTracking(true);
  setUpdatesEnabled(true);
}

/**
   Closes units report
 */
void popdown_units_report()
{
  if (units_reports::exists()) {
    units_reports::instance()->drop();
  }
}

/**
   Toggles units report, bool used for compatibility with sidebar callback
 */
void toggle_units_report(bool x)
{
  Q_UNUSED(x);
  if (units_reports::exists()
      && queen()->game_tab_widget->currentIndex() == 0) {
    units_reports::instance()->drop();
  } else {
    units_report_dialog_popup(true);
  }
}

/**
   Update the units report.
 */
void real_units_report_dialog_update(void *unused)
{
  Q_UNUSED(unused)
  if (units_reports::exists()) {
    units_reports::instance()->update_units();
  }
}

/**
   Display the units report.  Optionally raise it.
   Typically triggered by F2.
 */
void units_report_dialog_popup(bool raise)
{
  Q_UNUSED(raise)
  queen()->game_tab_widget->setCurrentIndex(0);
  units_reports::instance()->update_units(true);
}
