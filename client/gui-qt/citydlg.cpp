/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "citydlg.h"
// Qt
#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidgetAction>
// utility
#include "fcintl.h"
#include "support.h"
// common
#include "citizens.h"
#include "city.h"
#include "game.h"
// agents
#include "cma_core.h"
#include "cma_fec.h"
// client
#include "citydlg_common.h"
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "global_worklist.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "mapview_g.h"
#include "sprite.h"
#include "text.h"
#include "tilespec.h"
// gui-qt
#include "canvas.h"
#include "colors.h"
#include "fc_client.h"
#include "fonts.h"
#include "hudwidget.h"
#include "icons.h"
#include "mapview.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "tooltips.h"

extern QApplication *qapp;
city_dialog *city_dialog::m_instance = 0;
extern QString split_text(const QString &text, bool cut);
extern QString cut_helptext(const QString &text);

/************************************************************************/ /**
   Custom progressbar constructor
 ****************************************************************************/
progress_bar::progress_bar(QWidget *parent) : QProgressBar(parent)
{
  m_timer.start();
  startTimer(50);
  create_region();
  sfont = new QFont;
  m_animate_step = 0;
  pix = nullptr;
}

/************************************************************************/ /**
   Custom progressbar destructor
 ****************************************************************************/
progress_bar::~progress_bar()
{
  if (pix != nullptr) {
    delete pix;
  }
  delete sfont;
}

/************************************************************************/ /**
   Custom progressbar resize event
 ****************************************************************************/
void progress_bar::resizeEvent(QResizeEvent *event) { create_region(); }

/************************************************************************/ /**
   Sets pixmap from given universal for custom progressbar
 ****************************************************************************/
void progress_bar::set_pixmap(struct universal *target)
{
  struct sprite *sprite;
  QImage cropped_img;
  QImage img;
  QPixmap tpix;
  QRect crop;

  if (VUT_UTYPE == target->kind) {
    sprite = get_unittype_sprite(get_tileset(), target->value.utype,
                                 direction8_invalid());
  } else {
    sprite = get_building_sprite(tileset, target->value.building);
  }
  if (pix != nullptr) {
    delete pix;
  }
  if (sprite == nullptr) {
    pix = nullptr;
    return;
  }
  img = sprite->pm->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  tpix = QPixmap::fromImage(cropped_img);
  pix = new QPixmap(tpix.width(), tpix.height());
  pix->fill(Qt::transparent);
  pixmap_copy(pix, &tpix, 0, 0, 0, 0, tpix.width(), tpix.height());
}

/************************************************************************/ /**
   Sets pixmap from given tech number for custom progressbar
 ****************************************************************************/
void progress_bar::set_pixmap(int n)
{
  struct sprite *sprite;

  if (valid_advance_by_number(n)) {
    sprite = get_tech_sprite(tileset, n);
  } else {
    sprite = nullptr;
  }
  if (pix != nullptr) {
    delete pix;
  }
  if (sprite == nullptr) {
    pix = nullptr;
    return;
  }
  pix = new QPixmap(sprite->pm->width(), sprite->pm->height());
  pix->fill(Qt::transparent);
  pixmap_copy(pix, sprite->pm, 0, 0, 0, 0, sprite->pm->width(),
              sprite->pm->height());
  if (isVisible()) {
    update();
  }
}

/************************************************************************/ /**
   Timer event used to animate progress
 ****************************************************************************/
void progress_bar::timerEvent(QTimerEvent *event)
{
  if ((value() != minimum() && value() < maximum())
      || (0 == minimum() && 0 == maximum())) {
    m_animate_step = m_timer.elapsed() / 50;
    update();
  }
}

/************************************************************************/ /**
   Paint event for custom progress bar
 ****************************************************************************/
void progress_bar::paintEvent(QPaintEvent *event)
{
  QPainter p;
  QLinearGradient g, gx;
  QColor c;
  QRect r, rx, r2;
  int max;
  int f_size;
  int pix_width = 0;
  int point_size = sfont->pointSize();
  int pixel_size = sfont->pixelSize();

  if (pix != nullptr) {
    pix_width = height() - 4;
  }
  if (point_size < 0) {
    f_size = pixel_size;
  } else {
    f_size = point_size;
  }

  rx.setX(0);
  rx.setY(0);
  rx.setWidth(width());
  rx.setHeight(height());
  p.begin(this);
  p.drawLine(rx.topLeft(), rx.topRight());
  p.drawLine(rx.bottomLeft(), rx.bottomRight());

  max = maximum();

  if (max == 0) {
    max = 1;
  }

  r = QRect(0, 0, width() * value() / max, height());

  gx = QLinearGradient(0, 0, 0, height());
  c = QColor(palette().color(QPalette::Highlight));
  gx.setColorAt(0, c);
  gx.setColorAt(0.5, QColor(40, 40, 40));
  gx.setColorAt(1, c);
  p.fillRect(r, QBrush(gx));
  p.setClipRegion(reg.translated(m_animate_step % 32, 0));

  g = QLinearGradient(0, 0, width(), height());
  c.setAlphaF(0.1);
  g.setColorAt(0, c);
  c.setAlphaF(0.9);
  g.setColorAt(1, c);
  p.fillRect(r, QBrush(g));

  p.setClipping(false);
  r2 = QRect(width() * value() / max, 0, width(), height());
  c = palette().color(QPalette::Window);
  p.fillRect(r2, c);

  /* draw icon */
  if (pix != nullptr) {
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.drawPixmap(
        2, 2, pix_width * static_cast<float>(pix->width()) / pix->height(),
        pix_width, *pix, 0, 0, pix->width(), pix->height());
  }

  /* draw text */
  c = palette().color(QPalette::Text);
  p.setPen(c);
  sfont->setCapitalization(QFont::AllUppercase);
  sfont->setBold(true);
  p.setFont(*sfont);

  if (text().contains('\n')) {
    QString s1, s2;
    int i, j;

    i = text().indexOf('\n');
    s1 = text().left(i);
    s2 = text().right(text().count() - i);

    if (2 * f_size >= 2 * height() / 3) {
      if (point_size < 0) {
        sfont->setPixelSize(height() / 4);
      } else {
        sfont->setPointSize(height() / 4);
      }
    }

    j = height() - 2 * f_size;
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    QFontMetrics fm(*sfont);

    if (fm.horizontalAdvance(s1) > rx.width()) {
      s1 = fm.elidedText(s1, Qt::ElideRight, rx.width());
    }

    i = rx.width() - fm.horizontalAdvance(s1) + pix_width;
    i = qMax(0, i);
    p.drawText(i / 2, j / 3 + f_size, s1);

    if (fm.horizontalAdvance(s2) > rx.width()) {
      s2 = fm.elidedText(s2, Qt::ElideRight, rx.width());
    }

    i = rx.width() - fm.horizontalAdvance(s2) + pix_width;
    i = qMax(0, i);

    p.drawText(i / 2, height() - j / 3, s2);
  } else {
    QString s;
    int i, j;
    s = text();
    j = height() - f_size;
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    QFontMetrics fm(*sfont);

    if (fm.horizontalAdvance(s) > rx.width()) {
      s = fm.elidedText(s, Qt::ElideRight, rx.width());
    }

    i = rx.width() - fm.horizontalAdvance(s) + pix_width;
    i = qMax(0, i);
    p.drawText(i / 2, j / 2 + f_size, s);
  }
  p.end();
}

/************************************************************************/ /**
   Creates region with diagonal lines
 ****************************************************************************/
void progress_bar::create_region()
{
  int offset;
  QRect r(-50, 0, width() + 50, height());
  int chunk_width = 16;
  int size = width() + 50;
  reg = QRegion();

  for (offset = 0; offset < (size * 2); offset += (chunk_width * 2)) {
    QPolygon a;

    a.setPoints(4, r.x(), r.y() + offset, r.x() + r.width(),
                (r.y() + offset) - size, r.x() + r.width(),
                (r.y() + offset + chunk_width) - size, r.x(),
                r.y() + offset + chunk_width);
    reg += QRegion(a);
  }
}

/************************************************************************/ /**
   Draws X on pixmap pointing its useless
 ****************************************************************************/
static void pixmap_put_x(QPixmap *pix)
{
  QPen pen(QColor(0, 0, 0));
  QPainter p;

  pen.setWidth(2);
  p.begin(pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(pen);
  p.drawLine(0, 0, pix->width(), pix->height());
  p.drawLine(pix->width(), 0, 0, pix->height());
  p.end();
}

/************************************************************************/ /**
   Improvement item constructor
 ****************************************************************************/
impr_item::impr_item(QWidget *parent, const impr_type *building,
                     struct city *city)
    : QLabel(parent)
{
  setParent(parent);
  pcity = city;
  impr = building;
  impr_pixmap = nullptr;
  struct sprite *sprite;
  sprite = get_building_sprite(tileset, building);

  if (sprite != nullptr) {
    impr_pixmap =
        qtg_canvas_create(sprite->pm->width(), sprite->pm->height());
    impr_pixmap->map_pixmap.fill(Qt::transparent);
    pixmap_copy(&impr_pixmap->map_pixmap, sprite->pm, 0, 0, 0, 0,
                sprite->pm->width(), sprite->pm->height());
  } else {
    impr_pixmap = qtg_canvas_create(10, 10);
    impr_pixmap->map_pixmap.fill(Qt::red);
  }

  setFixedWidth(impr_pixmap->map_pixmap.width() + 4);
  setFixedHeight(impr_pixmap->map_pixmap.height());
  setToolTip(get_tooltip_improvement(building, city, true).trimmed());
}

/************************************************************************/ /**
   Improvement item destructor
 ****************************************************************************/
impr_item::~impr_item()
{
  if (impr_pixmap) {
    canvas_free(impr_pixmap);
  }
}

/************************************************************************/ /**
   Sets pixmap to improvemnt item
 ****************************************************************************/
void impr_item::init_pix()
{
  setPixmap(impr_pixmap->map_pixmap);
  update();
}

/************************************************************************/ /**
   Mouse enters widget
 ****************************************************************************/
void impr_item::enterEvent(QEvent *event)
{
  struct sprite *sprite;
  QPainter p;

  if (impr_pixmap) {
    canvas_free(impr_pixmap);
  }

  sprite = get_building_sprite(tileset, impr);
  if (impr && sprite) {
    impr_pixmap =
        qtg_canvas_create(sprite->pm->width(), sprite->pm->height());
    impr_pixmap->map_pixmap.fill(
        QColor(palette().color(QPalette::Highlight)));
    pixmap_copy(&impr_pixmap->map_pixmap, sprite->pm, 0, 0, 0, 0,
                sprite->pm->width(), sprite->pm->height());
  } else {
    impr_pixmap = qtg_canvas_create(10, 10);
    impr_pixmap->map_pixmap.fill(
        QColor(palette().color(QPalette::Highlight)));
  }

  init_pix();
}

/************************************************************************/ /**
   Mouse leaves widget
 ****************************************************************************/
void impr_item::leaveEvent(QEvent *event)
{
  struct sprite *sprite;

  if (impr_pixmap) {
    canvas_free(impr_pixmap);
  }

  sprite = get_building_sprite(tileset, impr);
  if (impr && sprite) {
    impr_pixmap =
        qtg_canvas_create(sprite->pm->width(), sprite->pm->height());
    impr_pixmap->map_pixmap.fill(Qt::transparent);
    pixmap_copy(&impr_pixmap->map_pixmap, sprite->pm, 0, 0, 0, 0,
                sprite->pm->width(), sprite->pm->height());
  } else {
    impr_pixmap = qtg_canvas_create(10, 10);
    impr_pixmap->map_pixmap.fill(Qt::red);
  }

  init_pix();
}

/************************************************************************/ /**
   Improvement list constructor
 ****************************************************************************/
impr_info::impr_info() : QFrame()
{
  layout = new QHBoxLayout(this);
  init_layout();
}

/************************************************************************/ /**
   Inits improvement list constructor
 ****************************************************************************/
void impr_info::init_layout()
{
  QSizePolicy size_fixed_policy(QSizePolicy::Fixed,
                                QSizePolicy::MinimumExpanding,
                                QSizePolicy::Slider);

  setSizePolicy(size_fixed_policy);
  setLayout(layout);
}

/************************************************************************/ /**
   Improvement list destructor
 ****************************************************************************/
impr_info::~impr_info() {}

/************************************************************************/ /**
   Adds improvement item to list
 ****************************************************************************/
void impr_info::add_item(impr_item *item) { impr_list.append(item); }

/************************************************************************/ /**
   Clears layout on improvement list
 ****************************************************************************/
void impr_info::clear_layout()
{
  int i = impr_list.count();
  impr_item *ui;
  int j;
  setUpdatesEnabled(false);
  setMouseTracking(false);

  for (j = 0; j < i; j++) {
    ui = impr_list[j];
    layout->removeWidget(ui);
    delete ui;
  }

  while (!impr_list.empty()) {
    impr_list.removeFirst();
  }

  setMouseTracking(true);
  setUpdatesEnabled(true);
}

/************************************************************************/ /**
   Updates list of improvements
 ****************************************************************************/
void impr_info::update_buildings()
{
  int i = impr_list.count();
  int j;
  int h = 0;
  impr_item *ui;

  setUpdatesEnabled(false);
  hide();

  for (j = 0; j < i; j++) {
    ui = impr_list[j];
    h = ui->height();
    layout->addWidget(ui, 0, Qt::AlignVCenter);
  }
  layout->setContentsMargins(0, 0, 0, 0);
  if (impr_list.count() > 0) {
    parentWidget()->parentWidget()->setFixedHeight(h + 12);
  } else {
    parentWidget()->parentWidget()->setFixedHeight(0);
  }

  show();
  setUpdatesEnabled(true);
  layout->update();
  updateGeometry();
}

/************************************************************************/ /**
   Double click event on improvement item
 ****************************************************************************/
void impr_item::mouseDoubleClickEvent(QMouseEvent *event)
{
  hud_message_box *ask;
  char buf[256];
  int price;
  const int impr_id = improvement_number(impr);
  const int city_id = pcity->id;

  if (!can_client_issue_orders()) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    ask = new hud_message_box(city_dialog::instance());
    if (test_player_sell_building_now(client.conn.playing, pcity, impr)
        != TR_SUCCESS) {
      return;
    }

    price = impr_sell_gold(impr);
    fc_snprintf(buf, ARRAY_SIZE(buf),
                PL_("Sell %s for %d gold?", "Sell %s for %d gold?", price),
                city_improvement_name_translation(pcity, impr), price);

    ask->set_text_title(buf, (_("Sell improvement?")));
    ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    ask->setAttribute(Qt::WA_DeleteOnClose);
    connect(ask, &hud_message_box::accepted, [=]() {
      struct city *pcity = game_city_by_number(city_id);
      if (!pcity) {
        return;
      }
      city_sell_improvement(pcity, impr_id);
    });
    ask->show();
  }
}

/************************************************************************/ /**
   Class representing one unit, allows context menu, holds pixmap for it
 ****************************************************************************/
unit_item::unit_item(QWidget *parent, struct unit *punit, bool supp,
                     int hppy_cost)
    : QLabel()
{
  happy_cost = hppy_cost;
  QImage cropped_img;
  QImage img;
  QRect crop;
  qunit = punit;
  struct canvas *unit_pixmap;
  struct tileset *tmp;
  float isosize;

  setParent(parent);
  supported = supp;

  tmp = nullptr;
  if (unscaled_tileset) {
    tmp = tileset;
    tileset = unscaled_tileset;
  }
  isosize = 0.6;
  if (tileset_hex_height(tileset) > 0 || tileset_hex_width(tileset) > 0) {
    isosize = 0.45;
  }

  if (punit) {
    if (supported) {
      unit_pixmap =
          qtg_canvas_create(tileset_unit_width(get_tileset()),
                            tileset_unit_with_upkeep_height(get_tileset()));
    } else {
      unit_pixmap = qtg_canvas_create(tileset_unit_width(get_tileset()),
                                      tileset_unit_height(get_tileset()));
    }

    unit_pixmap->map_pixmap.fill(Qt::transparent);
    put_unit(punit, unit_pixmap, 0, 0);

    if (supported) {
      put_unit_city_overlays(punit, unit_pixmap, 0,
                             tileset_unit_layout_offset_y(get_tileset()),
                             punit->upkeep, happy_cost);
    }
  } else {
    unit_pixmap = qtg_canvas_create(10, 10);
    unit_pixmap->map_pixmap.fill(Qt::transparent);
  }

  img = unit_pixmap->map_pixmap.toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  if (tileset_is_isometric(tileset)) {
    unit_img = cropped_img.scaledToHeight(tileset_unit_width(get_tileset())
                                              * isosize,
                                          Qt::SmoothTransformation);
  } else {
    unit_img = cropped_img.scaledToHeight(tileset_unit_width(get_tileset()),
                                          Qt::SmoothTransformation);
  }
  canvas_free(unit_pixmap);
  if (tmp != nullptr) {
    tileset = tmp;
  }

  create_actions();
  setFixedWidth(unit_img.width() + 4);
  setFixedHeight(unit_img.height());
  setToolTip(unit_description(qunit));
}

/************************************************************************/ /**
   Sets pixmap for unit_item class
 ****************************************************************************/
void unit_item::init_pix()
{
  setPixmap(QPixmap::fromImage(unit_img));
  update();
}

/************************************************************************/ /**
   Destructor for unit item
 ****************************************************************************/
unit_item::~unit_item() {}

/************************************************************************/ /**
   Context menu handler
 ****************************************************************************/
void unit_item::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu;

  if (!can_client_issue_orders()) {
    return;
  }

  if (unit_owner(qunit) != client_player()) {
    return;
  }

  menu = new QMenu(king()->central_wdg);
  menu->addAction(activate);
  menu->addAction(activate_and_close);

  if (sentry) {
    menu->addAction(sentry);
  }

  if (fortify) {
    menu->addAction(fortify);
  }

  if (change_home) {
    menu->addAction(change_home);
  }

  if (load) {
    menu->addAction(load);
  }

  if (unload) {
    menu->addAction(unload);
  }

  if (unload_trans) {
    menu->addAction(unload_trans);
  }

  if (disband_action) {
    menu->addAction(disband_action);
  }

  if (upgrade) {
    menu->addAction(upgrade);
  }

  menu->popup(event->globalPos());
}

/************************************************************************/ /**
   Initializes context menu
 ****************************************************************************/
void unit_item::create_actions()
{
  struct unit_list *qunits;

  if (unit_owner(qunit) != client_player() || !can_client_issue_orders()) {
    return;
  }

  qunits = unit_list_new();
  unit_list_append(qunits, qunit);
  activate = new QAction(_("Activate unit"), this);
  connect(activate, &QAction::triggered, this, &unit_item::activate_unit);
  activate_and_close = new QAction(_("Activate and close dialog"), this);
  connect(activate_and_close, &QAction::triggered, this,
          &unit_item::activate_and_close_dialog);

  if (can_unit_do_activity(qunit, ACTIVITY_SENTRY)) {
    sentry = new QAction(_("Sentry unit"), this);
    connect(sentry, &QAction::triggered, this, &unit_item::sentry_unit);
  } else {
    sentry = NULL;
  }

  if (can_unit_do_activity(qunit, ACTIVITY_FORTIFYING)) {
    fortify = new QAction(_("Fortify unit"), this);
    connect(fortify, &QAction::triggered, this, &unit_item::fortify_unit);
  } else {
    fortify = NULL;
  }
  if (unit_can_do_action(qunit, ACTION_DISBAND_UNIT)) {
    disband_action = new QAction(_("Disband unit"), this);
    connect(disband_action, &QAction::triggered, this, &unit_item::disband);
  } else {
    disband_action = NULL;
  }

  if (can_unit_change_homecity(qunit)) {
    change_home =
        new QAction(action_id_name_translation(ACTION_HOME_CITY), this);
    connect(change_home, &QAction::triggered, this,
            &unit_item::change_homecity);
  } else {
    change_home = NULL;
  }

  if (units_can_load(qunits)) {
    load = new QAction(_("Load"), this);
    connect(load, &QAction::triggered, this, &unit_item::load_unit);
  } else {
    load = NULL;
  }

  if (units_can_unload(qunits)) {
    unload = new QAction(_("Unload"), this);
    connect(unload, &QAction::triggered, this, &unit_item::unload_unit);
  } else {
    unload = NULL;
  }

  if (units_are_occupied(qunits)) {
    unload_trans = new QAction(_("Unload All From Transporter"), this);
    connect(unload_trans, &QAction::triggered, this, &unit_item::unload_all);
  } else {
    unload_trans = NULL;
  }

  if (units_can_upgrade(qunits)) {
    upgrade = new QAction(_("Upgrade Unit"), this);
    connect(upgrade, &QAction::triggered, this, &unit_item::upgrade_unit);
  } else {
    upgrade = NULL;
  }

  unit_list_destroy(qunits);
}

/************************************************************************/ /**
   Popups MessageBox  for disbanding unit and disbands it
 ****************************************************************************/
void unit_item::disband()
{
  struct unit_list *punits;
  struct unit *punit = player_unit_by_number(client_player(), qunit->id);

  if (punit == nullptr) {
    return;
  }

  punits = unit_list_new();
  unit_list_append(punits, punit);
  popup_disband_dialog(punits);
  unit_list_destroy(punits);
}

/************************************************************************/ /**
   Loads unit into some tranport
 ****************************************************************************/
void unit_item::load_unit()
{
  qtg_request_transport(qunit, unit_tile(qunit));
}

/************************************************************************/ /**
   Unloads unit
 ****************************************************************************/
void unit_item::unload_unit() { request_unit_unload(qunit); }

/************************************************************************/ /**
   Unloads all units from transporter
 ****************************************************************************/
void unit_item::unload_all() { request_unit_unload_all(qunit); }

/************************************************************************/ /**
   Upgrades unit
 ****************************************************************************/
void unit_item::upgrade_unit()
{
  struct unit_list *qunits;
  qunits = unit_list_new();
  unit_list_append(qunits, qunit);
  popup_upgrade_dialog(qunits);
  unit_list_destroy(qunits);
}

/************************************************************************/ /**
   Changes homecity for given unit
 ****************************************************************************/
void unit_item::change_homecity()
{
  if (qunit) {
    request_unit_change_homecity(qunit);
  }
}

/************************************************************************/ /**
   Activates unit and closes city dialog
 ****************************************************************************/
void unit_item::activate_and_close_dialog()
{
  if (qunit) {
    unit_focus_set(qunit);
    qtg_popdown_all_city_dialogs();
  }
}

/************************************************************************/ /**
   Activates unit in city dialog
 ****************************************************************************/
void unit_item::activate_unit()
{
  if (qunit) {
    unit_focus_set(qunit);
  }
}

/************************************************************************/ /**
   Fortifies unit in city dialog
 ****************************************************************************/
void unit_item::fortify_unit()
{
  if (qunit) {
    request_unit_fortify(qunit);
  }
}

/************************************************************************/ /**
   Mouse entered widget
 ****************************************************************************/
void unit_item::enterEvent(QEvent *event)
{
  QImage temp_img(unit_img.size(), QImage::Format_ARGB32_Premultiplied);
  QPainter p;

  p.begin(&temp_img);
  p.fillRect(0, 0, unit_img.width(), unit_img.height(),
             QColor(palette().color(QPalette::Highlight)));
  p.drawImage(0, 0, unit_img);
  p.end();

  setPixmap(QPixmap::fromImage(temp_img));
  update();
}

/************************************************************************/ /**
   Mouse left widget
 ****************************************************************************/
void unit_item::leaveEvent(QEvent *event) { init_pix(); }

/************************************************************************/ /**
   Mouse press event -activates unit and closes dialog
 ****************************************************************************/
void unit_item::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
    if (qunit) {
      unit_focus_set(qunit);
      qtg_popdown_all_city_dialogs();
    }
  }
}

/************************************************************************/ /**
   Sentries unit in city dialog
 ****************************************************************************/
void unit_item::sentry_unit()
{
  if (qunit) {
    request_unit_sentry(qunit);
  }
}

/************************************************************************/ /**
   Class representing list of units ( unit_item 's)
 ****************************************************************************/
unit_info::unit_info() : QFrame()
{
  layout = new QHBoxLayout(this);
  init_layout();
  supports = false;
}
void unit_info::set_supp(bool s) { supports = s; }

/************************************************************************/ /**
   Destructor for unit_info
 ****************************************************************************/
unit_info::~unit_info()
{
  qDeleteAll(unit_list);
  unit_list.clear();
}

/************************************************************************/ /**
   Adds one unit to list
 ****************************************************************************/
void unit_info::add_item(unit_item *item) { unit_list.append(item); }

/************************************************************************/ /**
   Initiazlizes layout ( layout needs to be changed after adding units )
 ****************************************************************************/
void unit_info::init_layout()
{
  QSizePolicy size_fixed_policy(QSizePolicy::Fixed,
                                QSizePolicy::MinimumExpanding,
                                QSizePolicy::Slider);
  setSizePolicy(size_fixed_policy);
  setLayout(layout);
}

/************************************************************************/ /**
   Updates units
 ****************************************************************************/
void unit_info::update_units()
{
  int i = unit_list.count();
  int j;
  unit_item *ui;

  setUpdatesEnabled(false);
  hide();

  for (j = 0; j < i; j++) {
    ui = unit_list[j];
    layout->addWidget(ui, 0, Qt::AlignVCenter);
  }
  layout->setContentsMargins(0, 0, 0, 0);

  if (unit_list.count() > 0) {
    parentWidget()->parentWidget()->setFixedHeight(ui->height() + 12);
  } else {
    parentWidget()->parentWidget()->setFixedHeight(0);
  }
  show();
  setUpdatesEnabled(true);
  layout->update();
  updateGeometry();
}

/************************************************************************/ /**
   Cleans layout - run it before layout initialization
 ****************************************************************************/
void unit_info::clear_layout()
{
  int i = unit_list.count();
  unit_item *ui;
  int j;
  setUpdatesEnabled(false);
  setMouseTracking(false);

  for (j = 0; j < i; j++) {
    ui = unit_list[j];
    layout->removeWidget(ui);
    delete ui;
  }

  while (!unit_list.empty()) {
    unit_list.removeFirst();
  }

  setMouseTracking(true);
  setUpdatesEnabled(true);
}

/************************************************************************/ /**
   city_label is used only for showing citizens icons
   and was created only to catch mouse events
 ****************************************************************************/
city_label::city_label(QWidget *parent) : QLabel(parent), pcity(nullptr)
{
  type = FEELING_FINAL;
}

void city_label::set_type(int x) { type = x; }
/************************************************************************/ /**
   Mouse handler for city_label
 ****************************************************************************/
void city_label::mousePressEvent(QMouseEvent *event)
{
  int citnum, i;
  int w = tileset_small_sprite_width(tileset) / king()->map_scale;
  int num_citizens;

  if (!pcity)
    return;
  if (cma_is_city_under_agent(pcity, NULL)) {
    return;
  }
  num_citizens = pcity->size;
  i = 1 + (num_citizens * 5 / 200);
  w = w / i;
  citnum = event->x() / w;

  if (!can_client_issue_orders()) {
    return;
  }

  city_rotate_specialist(pcity, citnum);
}

/************************************************************************/ /**
   Just sets target city for city_label
 ****************************************************************************/
void city_label::set_city(city *pciti) { pcity = pciti; }

city_info::city_info(QWidget *parent) : QWidget(parent)
{
  int info_nr;
  int iter;
  QFont *small_font;
  QLabel *ql;
  QStringList info_list;

  QGridLayout *info_grid_layout = new QGridLayout();
  small_font = fc_font::instance()->get_font(fonts::notify_label);
  info_list << _("Food:") << _("Prod:") << _("Trade:") << _("Gold:")
            << _("Luxury:") << _("Science:") << _("Granary:")
            << _("Change in:") << _("Corruption:") << _("Waste:")
            << _("Culture:") << _("Pollution:") << _("Plague risk:")
            << _("Tech Stolen:") << _("Airlift:");
  info_nr = info_list.count();
  setFont(*small_font);
  info_grid_layout->setSpacing(0);
  info_grid_layout->setContentsMargins(0, 0, 0, 0);

  for (iter = 0; iter < info_nr; iter++) {
    ql = new QLabel(info_list[iter], this);
    ql->setFont(*small_font);
    ql->setProperty(fonts::notify_label, "true");
    info_grid_layout->addWidget(ql, iter, 0);
    qlt[iter] = new QLabel(this);
    qlt[iter]->setFont(*small_font);
    qlt[iter]->setProperty(fonts::notify_label, "true");
    info_grid_layout->addWidget(qlt[iter], iter, 1);
    info_grid_layout->setRowStretch(iter, 0);
  }

  setLayout(info_grid_layout);
}
void city_info::update_labels(struct city *pcity)
{
  int illness = 0;
  char buffer[512];
  char buf[2 * NUM_INFO_FIELDS][512];
  int granaryturns;

  enum {
    FOOD = 0,
    SHIELD = 2,
    TRADE = 4,
    GOLD = 6,
    LUXURY = 8,
    SCIENCE = 10,
    GRANARY = 12,
    GROWTH = 14,
    CORRUPTION = 16,
    WASTE = 18,
    CULTURE = 20,
    POLLUTION = 22,
    ILLNESS = 24,
    STEAL = 26,
    AIRLIFT = 28,
  };

  /* fill the buffers with the necessary info */
  fc_snprintf(buf[FOOD], sizeof(buf[FOOD]), "%3d (%+4d)",
              pcity->prod[O_FOOD], pcity->surplus[O_FOOD]);
  fc_snprintf(buf[SHIELD], sizeof(buf[SHIELD]), "%3d (%+4d)",
              pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD],
              pcity->surplus[O_SHIELD]);
  fc_snprintf(buf[TRADE], sizeof(buf[TRADE]), "%3d (%+4d)",
              pcity->surplus[O_TRADE] + pcity->waste[O_TRADE],
              pcity->surplus[O_TRADE]);
  fc_snprintf(buf[GOLD], sizeof(buf[GOLD]), "%3d (%+4d)",
              pcity->prod[O_GOLD], pcity->surplus[O_GOLD]);
  fc_snprintf(buf[LUXURY], sizeof(buf[LUXURY]), "%3d",
              pcity->prod[O_LUXURY]);
  fc_snprintf(buf[SCIENCE], sizeof(buf[SCIENCE]), "%3d",
              pcity->prod[O_SCIENCE]);
  fc_snprintf(buf[GRANARY], sizeof(buf[GRANARY]), "%4d/%-4d",
              pcity->food_stock, city_granary_size(city_size_get(pcity)));

  get_city_dialog_output_text(pcity, O_FOOD, buf[FOOD + 1],
                              sizeof(buf[FOOD + 1]));
  get_city_dialog_output_text(pcity, O_SHIELD, buf[SHIELD + 1],
                              sizeof(buf[SHIELD + 1]));
  get_city_dialog_output_text(pcity, O_TRADE, buf[TRADE + 1],
                              sizeof(buf[TRADE + 1]));
  get_city_dialog_output_text(pcity, O_GOLD, buf[GOLD + 1],
                              sizeof(buf[GOLD + 1]));
  get_city_dialog_output_text(pcity, O_SCIENCE, buf[SCIENCE + 1],
                              sizeof(buf[SCIENCE + 1]));
  get_city_dialog_output_text(pcity, O_LUXURY, buf[LUXURY + 1],
                              sizeof(buf[LUXURY + 1]));
  get_city_dialog_culture_text(pcity, buf[CULTURE + 1],
                               sizeof(buf[CULTURE + 1]));
  get_city_dialog_pollution_text(pcity, buf[POLLUTION + 1],
                                 sizeof(buf[POLLUTION + 1]));
  get_city_dialog_illness_text(pcity, buf[ILLNESS + 1],
                               sizeof(buf[ILLNESS + 1]));

  granaryturns = city_turns_to_grow(pcity);

  if (granaryturns == 0) {
    /* TRANS: city growth is blocked.  Keep short. */
    fc_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("blocked"));
  } else if (granaryturns == FC_INFINITY) {
    /* TRANS: city is not growing.  Keep short. */
    fc_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("never"));
  } else {
    /* A negative value means we'll have famine in that many turns.
       But that's handled down below. */
    /* TRANS: city growth turns.  Keep short. */
    fc_snprintf(buf[GROWTH], sizeof(buf[GROWTH]),
                PL_("%d turn", "%d turns", abs(granaryturns)),
                abs(granaryturns));
  }

  fc_snprintf(buf[CORRUPTION], sizeof(buf[CORRUPTION]), "%4d",
              pcity->waste[O_TRADE]);
  fc_snprintf(buf[WASTE], sizeof(buf[WASTE]), "%4d", pcity->waste[O_SHIELD]);
  fc_snprintf(buf[CULTURE], sizeof(buf[CULTURE]), "%4d",
              pcity->client.culture);
  fc_snprintf(buf[POLLUTION], sizeof(buf[POLLUTION]), "%4d",
              pcity->pollution);

  if (!game.info.illness_on) {
    fc_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), " -.-");
  } else {
    illness = city_illness_calc(pcity, NULL, NULL, NULL, NULL);
    /* illness is in tenth of percent */
    fc_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), "%4.1f%%",
                (float) illness / 10.0);
  }
  if (pcity->steal) {
    fc_snprintf(buf[STEAL], sizeof(buf[STEAL]), _("%d times"), pcity->steal);
  } else {
    fc_snprintf(buf[STEAL], sizeof(buf[STEAL]), _("Not stolen"));
  }

  get_city_dialog_airlift_value(pcity, buf[AIRLIFT], sizeof(buf[AIRLIFT]));
  get_city_dialog_airlift_text(pcity, buf[AIRLIFT + 1],
                               sizeof(buf[AIRLIFT + 1]));

  get_city_dialog_output_text(pcity, O_FOOD, buffer, sizeof(buffer));

  for (int i = 0; i < NUM_INFO_FIELDS; i++) {
    int j = 2 * i;

    qlt[i]->setText(QString(buf[2 * i]));

    if (j != GROWTH && j != GRANARY && j != WASTE && j != CORRUPTION
        && j != STEAL) {
      qlt[i]->setToolTip("<pre>" + QString(buf[2 * i + 1]).toHtmlEscaped()
                         + "</pre>");
    }
  }
}

governor_sliders::governor_sliders(QWidget *parent) : QGroupBox(parent)
{
  QStringList str_list;
  QSlider *slider;
  QLabel *some_label;
  QGridLayout *slider_grid = new QGridLayout;

  str_list << _("Food") << _("Shield") << _("Trade") << _("Gold")
           << _("Luxury") << _("Science") << _("Celebrate");
  some_label = new QLabel(_("Minimal Surplus"));
  some_label->setFont(*fc_font::instance()->get_font(fonts::notify_label));
  some_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  slider_grid->addWidget(some_label, 0, 0, 1, 3);
  some_label = new QLabel(_("Priority"));
  some_label->setFont(*fc_font::instance()->get_font(fonts::notify_label));
  some_label->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  slider_grid->addWidget(some_label, 0, 3, 1, 3);

  for (int i = 0; i < str_list.count(); i++) {
    some_label = new QLabel(str_list.at(i));
    slider_grid->addWidget(some_label, i + 1, 0, 1, 1);
    some_label = new QLabel("0");
    some_label->setMinimumWidth(25);

    if (i != str_list.count() - 1) {
      slider = new QSlider(Qt::Horizontal);
      slider->setPageStep(1);
      slider->setFocusPolicy(Qt::TabFocus);
      slider_tab[2 * i] = slider;
      slider->setRange(-20, 20);
      slider->setSingleStep(1);
      slider_grid->addWidget(some_label, i + 1, 1, 1, 1);
      slider_grid->addWidget(slider, i + 1, 2, 1, 1);
      slider->setProperty("FC", QVariant::fromValue((void *) some_label));

      connect(slider, &QAbstractSlider::valueChanged, this,
              &governor_sliders::cma_slider);
    } else {
      cma_celeb_checkbox = new QCheckBox;
      slider_grid->addWidget(cma_celeb_checkbox, i + 1, 2, 1, 1);
      connect(cma_celeb_checkbox, &QCheckBox::stateChanged, this,
              &governor_sliders::cma_celebrate_changed);
    }

    some_label = new QLabel("0");
    some_label->setMinimumWidth(25);
    slider = new QSlider(Qt::Horizontal);
    slider->setFocusPolicy(Qt::TabFocus);
    slider->setRange(0, 25);
    slider_tab[2 * i + 1] = slider;
    slider->setProperty("FC", QVariant::fromValue((void *) some_label));
    slider_grid->addWidget(some_label, i + 1, 3, 1, 1);
    slider_grid->addWidget(slider, i + 1, 4, 1, 1);
    connect(slider, &QAbstractSlider::valueChanged, this,
            &governor_sliders::cma_slider);
  }
  setLayout(slider_grid);
}

/************************************************************************/ /**
   CMA options on slider has been changed
 ****************************************************************************/
void governor_sliders::cma_slider(int value)
{
  QVariant qvar;
  QSlider *slider;
  QLabel *label;

  slider = qobject_cast<QSlider *>(sender());
  qvar = slider->property("FC");

  if (qvar.isNull() || !qvar.isValid()) {
    return;
  }

  label = reinterpret_cast<QLabel *>(qvar.value<void *>());
  label->setText(QString::number(value));

  city_dialog::instance()->cma_check_agent();
}

/************************************************************************/ /**
   CMA option 'celebrate' qcheckbox state has been changed
 ****************************************************************************/
void governor_sliders::cma_celebrate_changed(int val)
{
  city_dialog::instance()->cma_check_agent();
}

/************************************************************************/ /**
   Updates sliders ( cma params )
 ****************************************************************************/
void governor_sliders::update_sliders(struct cm_parameter &param)
{
  int output;
  QVariant qvar;
  QLabel *label;

  for (output = O_FOOD; output < 2 * O_LAST; output++) {
    slider_tab[output]->blockSignals(true);
  }

  for (output = O_FOOD; output < O_LAST; output++) {
    qvar = slider_tab[2 * output + 1]->property("FC");
    label = reinterpret_cast<QLabel *>(qvar.value<void *>());
    label->setText(QString::number(param.factor[output]));
    slider_tab[2 * output + 1]->setValue(param.factor[output]);
    qvar = slider_tab[2 * output]->property("FC");
    label = reinterpret_cast<QLabel *>(qvar.value<void *>());
    label->setText(QString::number(param.minimal_surplus[output]));
    slider_tab[2 * output]->setValue(param.minimal_surplus[output]);
  }

  slider_tab[2 * O_LAST + 1]->blockSignals(true);
  qvar = slider_tab[2 * O_LAST + 1]->property("FC");
  label = reinterpret_cast<QLabel *>(qvar.value<void *>());
  label->setText(QString::number(param.happy_factor));
  slider_tab[2 * O_LAST + 1]->setValue(param.happy_factor);
  slider_tab[2 * O_LAST + 1]->blockSignals(false);
  cma_celeb_checkbox->blockSignals(true);
  cma_celeb_checkbox->setChecked(param.require_happy);
  cma_celeb_checkbox->blockSignals(false);

  for (output = O_FOOD; output < 2 * O_LAST; output++) {
    slider_tab[output]->blockSignals(false);
  }
}

/************************************************************************/ /**
   Constructor for city_dialog, sets layouts, policies ...
 ****************************************************************************/
city_dialog::city_dialog(QWidget *parent)
    : qfc_dialog(parent), future_targets(false), show_units(true),
      show_wonders(true), show_buildings(true)
{
  QFont f = QApplication::font();
  QFont *small_font;
  QFontMetrics fm(f);
  QHeaderView *header;

  int h = 2 * fm.height() + 2;
  small_font = fc_font::instance()->get_font(fonts::notify_label);
  ui.setupUi(this);

  setMouseTracking(true);
  selected_row_p = -1;
  pcity = NULL;

  // main tab
  ui.lcity_name->setToolTip(_("Click to change city name"));
  ui.buy_button->setIcon(fc_icons::instance()->get_icon("help-donate"));
  connect(ui.buy_button, &QAbstractButton::clicked, this, &city_dialog::buy);
  connect(ui.lcity_name, &QAbstractButton::clicked, this,
          &city_dialog::city_rename);
  citizen_pixmap = NULL;
  ui.supported_units->set_supp(true);
  ui.scroll2->setWidgetResizable(true);
  ui.scroll2->setMaximumHeight(
      tileset_unit_with_upkeep_height(get_tileset()) + 6
      + ui.scroll2->horizontalScrollBar()->height());
  ui.scroll2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui.scroll->setWidgetResizable(true);
  ui.scroll->setMaximumHeight(tileset_unit_height(get_tileset()) + 6
                              + ui.scroll->horizontalScrollBar()->height());
  ui.scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_height = ui.scroll->horizontalScrollBar()->height();
  ui.scroll3->setWidgetResizable(true);
  ui.scroll3->setMaximumHeight(
      tileset_unit_height(tileset) + 6
      + ui.scroll3->horizontalScrollBar()->height());
  ui.scroll3->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui.scroll->setProperty("city_scroll", true);
  ui.scroll2->setProperty("city_scroll", true);
  ui.scroll3->setProperty("city_scroll", true);
  ui.bclose->setIcon(fc_icons::instance()->get_icon("city-close"));
  ui.bclose->setIconSize(QSize(56, 56));
  ui.bclose->setToolTip(_("Close city dialog"));
  connect(ui.bclose, &QAbstractButton::clicked, this, &QWidget::hide);
  ui.next_city_but->setIcon(fc_icons::instance()->get_icon("city-right"));
  ui.next_city_but->setIconSize(QSize(56, 56));
  ui.next_city_but->setToolTip(_("Show next city"));
  connect(ui.next_city_but, &QAbstractButton::clicked, this,
          &city_dialog::next_city);
  connect(ui.prev_city_but, &QAbstractButton::clicked, this,
          &city_dialog::prev_city);
  ui.prev_city_but->setIcon(fc_icons::instance()->get_icon("city-left"));
  ui.prev_city_but->setIconSize(QSize(56, 56));
  ui.prev_city_but->setToolTip(_("Show previous city"));
  ui.work_next_but->setIcon(fc_icons::instance()->get_icon("go-down"));
  ui.work_prev_but->setIcon(fc_icons::instance()->get_icon("go-up"));
  ui.work_add_but->setIcon(fc_icons::instance()->get_icon("list-add"));
  ui.work_rem_but->setIcon(
      style()->standardIcon(QStyle::SP_DialogDiscardButton));
  ui.production_combo_p->setToolTip(_("Click to change current production"));
  ui.production_combo_p->setFixedHeight(h);
  ui.p_table_p->setMinimumWidth(160);
  ui.p_table_p->setContextMenuPolicy(Qt::CustomContextMenu);
  header = ui.p_table_p->horizontalHeader();
  header->setStretchLastSection(true);
  connect(ui.p_table_p, &QWidget::customContextMenuRequested, this,
          &city_dialog::display_worklist_menu);
  connect(ui.production_combo_p, &progress_bar::clicked, this,
          &city_dialog::show_targets);
  connect(ui.work_add_but, &QAbstractButton::clicked, this,
          &city_dialog::show_targets_worklist);
  connect(ui.work_prev_but, &QAbstractButton::clicked, this,
          &city_dialog::worklist_up);
  connect(ui.work_next_but, &QAbstractButton::clicked, this,
          &city_dialog::worklist_down);
  connect(ui.work_rem_but, &QAbstractButton::clicked, this,
          &city_dialog::worklist_del);
  connect(ui.p_table_p, &QTableWidget::itemDoubleClicked, this,
          &city_dialog::dbl_click_p);
  connect(
      ui.p_table_p->selectionModel(),
      SIGNAL(
          selectionChanged(const QItemSelection &, const QItemSelection &)),
      SLOT(item_selected(const QItemSelection &, const QItemSelection &)));
  setSizeGripEnabled(true);

  /* governor tab */
  ui.qgbox->setTitle(_("Presets:"));
  ui.qsliderbox->setTitle(_("Governor settings"));

  ui.cma_table->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  connect(
      ui.cma_table->selectionModel(),
      SIGNAL(
          selectionChanged(const QItemSelection &, const QItemSelection &)),
      SLOT(cma_selected(const QItemSelection &, const QItemSelection &)));
  connect(ui.cma_table, &QWidget::customContextMenuRequested, this,
          &city_dialog::cma_context_menu);
  connect(ui.cma_table, &QTableWidget::cellDoubleClicked, this,
          &city_dialog::cma_double_clicked);

  ui.cma_enable_but->setFocusPolicy(Qt::TabFocus);
  connect(ui.cma_enable_but, &QAbstractButton::pressed, this,
          &city_dialog::cma_enable);

  ui.bsavecma->setText(_("Save"));
  ui.bsavecma->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  connect(ui.bsavecma, &QAbstractButton::pressed, this,
          &city_dialog::save_cma);

  ui.nationality_group->setTitle(_("Nationality"));
  ui.happiness_group->setTitle(_("Happiness"));
  ui.label1->setText(_("Cities:"));
  ui.label2->setText(_("Luxuries:"));
  ui.label3->setText(_("Buildings:"));
  ui.label4->setText(_("Nationality:"));
  ui.label5->setText(_("Units:"));
  ui.label6->setText(_("Wonders:"));
  ui.label1->setFont(*small_font);
  ui.label2->setFont(*small_font);
  ui.label4->setFont(*small_font);
  ui.label3->setFont(*small_font);
  ui.label5->setFont(*small_font);
  ui.label6->setFont(*small_font);
  lab_table[0] = ui.lab_table1;
  lab_table[1] = ui.lab_table2;
  lab_table[2] = ui.lab_table3;
  lab_table[3] = ui.lab_table4;
  lab_table[4] = ui.lab_table5;
  lab_table[5] = ui.lab_table6;
  for (int x = 0; x < 6; x++) {
    lab_table[5]->set_type(x);
  }

  ui.tab3->setLayout(ui.tabLayout3);
  ui.tabWidget->setTabText(2, _("Happiness"));
  ui.tabWidget->setTabText(1, _("Governor"));
  ui.tabWidget->setTabText(0, _("General"));
  ui.tab2->setLayout(ui.tabLayout2);
  ui.tab->setLayout(ui.tabLayout);
  setLayout(ui.vlayout);
  ui.tabWidget->setCurrentIndex(0);

  installEventFilter(this);
}

/************************************************************************/ /**
   Changes production to next one or previous
 ****************************************************************************/
void city_dialog::change_production(bool next)
{

  cid cprod;
  int i, pos;
  int item, targets_used;
  QList<cid> prod_list;
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct universal univ;

  pos = 0;
  cprod = cid_encode(pcity->production);
  targets_used = collect_eventually_buildable_targets(targets, pcity, false);
  name_and_sort_items(targets, targets_used, items, false, pcity);

  for (item = 0; item < targets_used; item++) {
    if (can_city_build_now(pcity, &items[item].item)) {
      prod_list << cid_encode(items[item].item);
    }
  }

  for (i = 0; i < prod_list.size(); i++) {
    if (prod_list.at(i) == cprod) {
      if (next) {
        pos = i + 1;
      } else {
        pos = i - 1;
      }
    }
  }
  if (pos == prod_list.size()) {
    pos = 0;
  }
  if (pos == -1) {
    pos = prod_list.size() - 1;
  }
  univ = cid_decode(static_cast<cid>(prod_list.at(pos)));
  city_change_production(pcity, &univ);
}

/************************************************************************/ /**
   Updates buttons/widgets which should be enabled/disabled
 ****************************************************************************/
void city_dialog::update_disabled()
{
  if (NULL == client.conn.playing
      || city_owner(pcity) != client.conn.playing) {
    ui.prev_city_but->setDisabled(true);
    ui.next_city_but->setDisabled(true);
    ui.buy_button->setDisabled(true);
    ui.cma_enable_but->setDisabled(true);
    ui.production_combo_p->setDisabled(true);
    ui.current_units->setDisabled(true);
    ui.supported_units->setDisabled(true);
    if (!client_is_observer()) {
    }
  } else {
    ui.prev_city_but->setEnabled(true);
    ui.next_city_but->setEnabled(true);
    ui.buy_button->setEnabled(true);
    ui.cma_enable_but->setEnabled(true);
    ui.production_combo_p->setEnabled(true);
    ui.current_units->setEnabled(true);
    ui.supported_units->setEnabled(true);
  }

  if (can_client_issue_orders()) {
    ui.cma_enable_but->setEnabled(true);
  } else {
    ui.cma_enable_but->setDisabled(true);
  }

  update_prod_buttons();
}

/************************************************************************/ /**
   Update sensitivity of buttons in production tab
 ****************************************************************************/
void city_dialog::update_prod_buttons()
{
  ui.work_next_but->setDisabled(true);
  ui.work_prev_but->setDisabled(true);
  ui.work_add_but->setDisabled(true);
  ui.work_rem_but->setDisabled(true);

  if (client.conn.playing && city_owner(pcity) == client.conn.playing) {
    ui.work_add_but->setEnabled(true);

    if (selected_row_p >= 0 && selected_row_p < ui.p_table_p->rowCount()) {
      ui.work_rem_but->setEnabled(true);
    }

    if (selected_row_p >= 0
        && selected_row_p < ui.p_table_p->rowCount() - 1) {
      ui.work_next_but->setEnabled(true);
    }

    if (selected_row_p > 0 && selected_row_p < ui.p_table_p->rowCount()) {
      ui.work_prev_but->setEnabled(true);
    }
  }
}

/************************************************************************/ /**
   City dialog destructor
 ****************************************************************************/
city_dialog::~city_dialog()
{
  if (citizen_pixmap) {
    citizen_pixmap->detach();
    delete citizen_pixmap;
  }

  ui.cma_table->clear();
  ui.p_table_p->clear();
  ui.nationality_table->clear();
  ui.current_units->clear_layout();
  ui.supported_units->clear_layout();
  removeEventFilter(this);
}

/************************************************************************/ /**
   Hide event
 ****************************************************************************/
void city_dialog::hideEvent(QHideEvent *event)
{
  if (pcity) {
    key_city_hide_open(pcity);
    unit_focus_update();
    map_canvas_resized(mapview.width, mapview.height);
  }
  king()->qt_settings.city_geometry = saveGeometry();
}

/************************************************************************/ /**
   Show event
 ****************************************************************************/
void city_dialog::showEvent(QShowEvent *event)
{
  if (!king()->qt_settings.city_geometry.isNull()) {
    restoreGeometry(king()->qt_settings.city_geometry);
  } else {
    QList<QScreen *> screens = QGuiApplication::screens();
    QRect rect = screens[0]->availableGeometry();

    resize((rect.width() * 4) / 5, (rect.height() * 5) / 6);
  }
  if (pcity) {
    key_city_show_open(pcity);
    unit_focus_set(nullptr);
    map_canvas_resized(mapview.width, mapview.height);
  }
}

/************************************************************************/ /**
   Show event
 ****************************************************************************/
void city_dialog::closeEvent(QCloseEvent *event)
{
  king()->qt_settings.city_geometry = saveGeometry();
}

/************************************************************************/ /**
   Event filter for catching keybaord events
 ****************************************************************************/
bool city_dialog::eventFilter(QObject *obj, QEvent *event)
{

  if (obj == this) {
    if (event->type() == QEvent::KeyPress) {
    }

    if (event->type() == QEvent::ShortcutOverride) {
      QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
      if (key_event->key() == Qt::Key_Right) {
        next_city();
        event->setAccepted(true);
        return true;
      }
      if (key_event->key() == Qt::Key_Left) {
        prev_city();
        event->setAccepted(true);
        return true;
      }
      if (key_event->key() == Qt::Key_Up) {
        change_production(true);
        event->setAccepted(true);
        return true;
      }
      if (key_event->key() == Qt::Key_Down) {
        change_production(false);
        event->setAccepted(true);
        return true;
      }
    }
  }
  return QObject::eventFilter(obj, event);
}

/************************************************************************/ /**
   City rename dialog input
 ****************************************************************************/
void city_dialog::city_rename()
{
  hud_input_box *ask;
  const int city_id = pcity->id;

  if (!can_client_issue_orders()) {
    return;
  }

  ask = new hud_input_box(king()->central_wdg);
  ask->set_text_title_definput(_("What should we rename the city to?"),
                               _("Rename City"), city_name_get(pcity));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, this, [=]() {
    struct city *pcity = game_city_by_number(city_id);
    QByteArray ask_bytes;

    if (!pcity) {
      return;
    }

    ask_bytes = ask->input_edit.text().toLocal8Bit();
    ::city_rename(pcity, ask_bytes.data());
  });
  ask->show();
}

/************************************************************************/ /**
   Save cma dialog input
 ****************************************************************************/
void city_dialog::save_cma()
{
  hud_input_box *ask = new hud_input_box(king()->central_wdg);

  ask->set_text_title_definput(_("What should we name the preset?"),
                               _("Name new preset"), _("new preset"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, this, [=]() {
    struct cm_parameter param;
    QByteArray ask_bytes = ask->input_edit.text().toLocal8Bit();
    QString text = ask_bytes.data();
    if (!text.isEmpty()) {
      param.allow_disorder = false;
      param.allow_specialists = true;
      param.require_happy = ui.qsliderbox->cma_celeb_checkbox->isChecked();
      param.happy_factor =
          ui.qsliderbox->slider_tab[2 * O_LAST + 1]->value();

      for (int i = O_FOOD; i < O_LAST; i++) {
        param.minimal_surplus[i] = ui.qsliderbox->slider_tab[2 * i]->value();
        param.factor[i] = ui.qsliderbox->slider_tab[2 * i + 1]->value();
      }

      ask_bytes = text.toLocal8Bit();
      cmafec_preset_add(ask_bytes.data(), &param);
      update_cma_tab();
    }
  });
  ask->show();
}

/************************************************************************/ /**
   Enables cma slot, triggered by clicked button or changed cma
 ****************************************************************************/
void city_dialog::cma_enable()
{
  if (cma_is_city_under_agent(pcity, NULL)) {
    cma_release_city(pcity);
    return;
  }

  cma_changed();
  update_cma_tab();
}

/************************************************************************/ /**
   Sliders moved and cma has been changed
 ****************************************************************************/
void city_dialog::cma_changed()
{
  struct cm_parameter param;

  param.allow_disorder = false;
  param.allow_specialists = true;
  param.require_happy = ui.qsliderbox->cma_celeb_checkbox->isChecked();
  param.happy_factor = ui.qsliderbox->slider_tab[2 * O_LAST + 1]->value();

  for (int i = O_FOOD; i < O_LAST; i++) {
    param.minimal_surplus[i] = ui.qsliderbox->slider_tab[2 * i]->value();
    param.factor[i] = ui.qsliderbox->slider_tab[2 * i + 1]->value();
  }

  cma_put_city_under_agent(pcity, &param);
}

/************************************************************************/ /**
   Double click on some row ( column is unused )
 ****************************************************************************/
void city_dialog::cma_double_clicked(int row, int column)
{
  const struct cm_parameter *param;

  if (!can_client_issue_orders()) {
    return;
  }
  param = cmafec_preset_get_parameter(row);
  if (cma_is_city_under_agent(pcity, NULL)) {
    cma_release_city(pcity);
  }

  cma_put_city_under_agent(pcity, param);
  update_cma_tab();
}

/************************************************************************/ /**
   CMA has been selected from list
 ****************************************************************************/
void city_dialog::cma_selected(const QItemSelection &sl,
                               const QItemSelection &ds)
{
  const struct cm_parameter *param;
  QModelIndex index;
  QModelIndexList indexes = sl.indexes();

  if (indexes.isEmpty() || ui.cma_table->signalsBlocked()) {
    return;
  }

  index = indexes.at(0);
  int ind = index.row();

  if (ui.cma_table->currentRow() == -1 || cmafec_preset_num() == 0) {
    return;
  }

  param = cmafec_preset_get_parameter(ind);
  update_sliders();

  if (cma_is_city_under_agent(pcity, NULL)) {
    cma_release_city(pcity);
    cma_put_city_under_agent(pcity, param);
  }
}

/************************************************************************/ /**
   Updates cma tab
 ****************************************************************************/
void city_dialog::update_cma_tab()
{
  QString s;
  QTableWidgetItem *item;
  struct cm_parameter param;
  QPixmap pix;
  int i;

  ui.cma_table->clear();
  ui.cma_table->setRowCount(0);

  for (i = 0; i < cmafec_preset_num(); i++) {
    item = new QTableWidgetItem;
    item->setText(cmafec_preset_get_descr(i));
    ui.cma_table->insertRow(i);
    ui.cma_table->setItem(i, 0, item);
  }

  if (cmafec_preset_num() == 0) {
    ui.cma_table->insertRow(0);
    item = new QTableWidgetItem;
    item->setText(_("No governor defined"));
    ui.cma_table->setItem(0, 0, item);
  }

  if (cma_is_city_under_agent(pcity, NULL)) {
    // view->update(); sveinung - update map here ?
    s = QString(cmafec_get_short_descr_of_city(pcity));
    pix = style()->standardPixmap(QStyle::SP_DialogApplyButton);
    pix = pix.scaled(2 * pix.width(), 2 * pix.height(),
                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui.cma_result_pix->setPixmap(pix);
    /* TRANS: %1 is custom string chosen by player */
    ui.cma_result->setText(QString(_("<h3>Governor Enabled<br>(%1)</h3>"))
                               .arg(s.toHtmlEscaped()));
    ui.cma_result->setAlignment(Qt::AlignCenter);
  } else {
    pix = style()->standardPixmap(QStyle::SP_DialogCancelButton);
    pix = pix.scaled(1.6 * pix.width(), 1.6 * pix.height(),
                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui.cma_result_pix->setPixmap(pix);
    ui.cma_result->setText(QString(_("<h3>Governor Disabled</h3>")));
    ui.cma_result->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  }

  if (cma_is_city_under_agent(pcity, NULL)) {
    cmafec_get_fe_parameter(pcity, &param);
    i = cmafec_preset_get_index_of_parameter(
        const_cast<struct cm_parameter *const>(&param));
    if (i >= 0 && i < ui.cma_table->rowCount()) {
      ui.cma_table->blockSignals(true);
      ui.cma_table->setCurrentCell(i, 0);
      ui.cma_table->blockSignals(false);
    }

    ui.cma_enable_but->setText(_("Disable"));
  } else {
    ui.cma_enable_but->setText(_("Enable"));
  }
  update_sliders();
}

/************************************************************************/ /**
   Removes selected CMA
 ****************************************************************************/
void city_dialog::cma_remove()
{
  int i;
  hud_message_box *ask;

  i = ui.cma_table->currentRow();

  if (i == -1 || cmafec_preset_num() == 0) {
    return;
  }

  ask = new hud_message_box(city_dialog::instance());
  ask->set_text_title(_("Remove this preset?"), cmafec_preset_get_descr(i));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, this, [=]() {
    cmafec_preset_remove(i);
    update_cma_tab();
  });
  ask->show();
}

/************************************************************************/ /**
   Received signal about changed qcheckbox - allow disbanding city
 ****************************************************************************/
void city_dialog::disband_state_changed(bool allow_disband)
{
  bv_city_options new_options;

  BV_CLR_ALL(new_options);

  if (allow_disband) {
    BV_SET(new_options, CITYO_DISBAND);
  } else {
    BV_CLR(new_options, CITYO_DISBAND);
  }

  if (!client_is_observer()) {
    dsend_packet_city_options_req(&client.conn, pcity->id, new_options);
  }
}

/************************************************************************/ /**
   Context menu on governor tab in city worklist
 ****************************************************************************/
void city_dialog::cma_context_menu(const QPoint)
{
  QMenu *cma_menu = new QMenu(this);
  QAction *cma_del_item;

  cma_menu->setAttribute(Qt::WA_DeleteOnClose);
  cma_del_item = cma_menu->addAction(_("Remove Governor"));
  connect(cma_menu, &QMenu::triggered, this, [=](QAction *act) {
    if (act == cma_del_item) {
      cma_remove();
    }
  });

  cma_menu->popup(QCursor::pos());
}

/************************************************************************/ /**
   Context menu on production tab in city worklist
 ****************************************************************************/
void city_dialog::display_worklist_menu(const QPoint)
{
  QAction *action;
  QAction *disband;
  QAction *wl_save;
  QAction *wl_clear;
  QAction *wl_empty;
  QMap<QString, cid> list;
  QMap<QString, cid>::const_iterator map_iter;
  QMenu *change_menu;
  QMenu *insert_menu;
  QMenu *list_menu;
  QMenu *options_menu;
  int city_id = pcity->id;

  if (!can_client_issue_orders()) {
    return;
  }
  list_menu = new QMenu(this);
  change_menu = list_menu->addMenu(_("Change worklist"));
  insert_menu = list_menu->addMenu(_("Insert worklist"));
  wl_clear = list_menu->addAction(_("Clear"));
  connect(wl_clear, &QAction::triggered, this, &city_dialog::clear_worklist);
  list.clear();

  global_worklists_iterate(pgwl)
  {
    list.insert(global_worklist_name(pgwl), global_worklist_id(pgwl));
  }
  global_worklists_iterate_end;

  if (list.count() == 0) {
    wl_empty = change_menu->addAction(_("(no worklists defined)"));
    insert_menu->addAction(wl_empty);
  }

  map_iter = list.constBegin();

  while (map_iter != list.constEnd()) {
    action = change_menu->addAction(map_iter.key());
    action->setData(map_iter.value());

    action = insert_menu->addAction(map_iter.key());
    action->setData(map_iter.value());

    ++map_iter;
  }

  wl_save = list_menu->addAction(_("Save worklist"));
  connect(wl_save, &QAction::triggered, this, &city_dialog::save_worklist);
  options_menu = list_menu->addMenu(_("Options"));
  disband = options_menu->addAction(_("Allow disbanding city"));
  disband->setCheckable(true);
  disband->setChecked(is_city_option_set(pcity, CITYO_DISBAND));
  connect(disband, &QAction::triggered, this,
          &city_dialog::disband_state_changed);

  connect(change_menu, &QMenu::triggered, this, [=](QAction *act) {
    QVariant id = act->data();
    struct city *pcity = game_city_by_number(city_id);
    const struct worklist *worklist;

    if (!pcity) {
      return;
    }

    fc_assert_ret(id.type() == QVariant::Int);
    worklist = global_worklist_get(global_worklist_by_id(id.toInt()));
    city_set_queue(pcity, worklist);
  });

  connect(insert_menu, &QMenu::triggered, this, [=](QAction *act) {
    QVariant id = act->data();
    struct city *pcity = game_city_by_number(city_id);
    const struct worklist *worklist;

    if (!pcity) {
      return;
    }

    fc_assert_ret(id.type() == QVariant::Int);
    worklist = global_worklist_get(global_worklist_by_id(id.toInt()));
    city_queue_insert_worklist(pcity, selected_row_p + 1, worklist);
  });

  list_menu->popup(QCursor::pos());
}

/************************************************************************/ /**
   Enables/disables buy buttons depending on gold
 ****************************************************************************/
void city_dialog::update_buy_button()
{
  QString str;
  int value;

  ui.buy_button->setDisabled(true);

  if (!client_is_observer() && client.conn.playing != NULL) {
    value = pcity->client.buy_cost;
    str = QString(PL_("Buy (%1 gold)", "Buy (%1 gold)", value))
              .arg(QString::number(value));

    if (client.conn.playing->economic.gold >= value && value != 0) {
      ui.buy_button->setEnabled(true);
    }
  } else {
    str = QString(_("Buy"));
  }

  ui.buy_button->setText(str);
}

/************************************************************************/ /**
   Redraws citizens for city_label (citizens_label)
 ****************************************************************************/
void city_dialog::update_citizens()
{
  enum citizen_category categories[MAX_CITY_SIZE];
  int i, j, width, height;
  QPainter p;
  QPixmap *pix;
  int num_citizens =
      get_city_citizen_types(pcity, FEELING_FINAL, categories);
  int w = tileset_small_sprite_width(tileset) / king()->map_scale;
  int h = tileset_small_sprite_height(tileset) / king()->map_scale;

  i = 1 + (num_citizens * 5 / 200);
  w = w / i;
  QRect source_rect(0, 0, w, h);
  QRect dest_rect(0, 0, w, h);
  width = w * num_citizens;
  height = h;

  if (citizen_pixmap) {
    citizen_pixmap->detach();
    delete citizen_pixmap;
  }

  citizen_pixmap = new QPixmap(width, height);

  for (j = 0, i = 0; i < num_citizens; i++, j++) {
    dest_rect.moveTo(i * w, 0);
    pix = get_citizen_sprite(tileset, categories[j], j, pcity)->pm;
    p.begin(citizen_pixmap);
    p.drawPixmap(dest_rect, *pix, source_rect);
    p.end();
  }

  ui.citizens_label->set_city(pcity);
  ui.citizens_label->setPixmap(*citizen_pixmap);

  lab_table[FEELING_FINAL]->setPixmap(*citizen_pixmap);
  lab_table[FEELING_FINAL]->setToolTip(text_happiness_wonders(pcity));

  for (int k = 0; k < FEELING_LAST - 1; k++) {
    lab_table[k]->set_city(pcity);
    num_citizens = get_city_citizen_types(
        pcity, static_cast<citizen_feeling>(k), categories);

    for (j = 0, i = 0; i < num_citizens; i++, j++) {
      dest_rect.moveTo(i * w, 0);
      pix = get_citizen_sprite(tileset, categories[j], j, pcity)->pm;
      p.begin(citizen_pixmap);
      p.drawPixmap(dest_rect, *pix, source_rect);
      p.end();
    }

    lab_table[k]->setPixmap(*citizen_pixmap);

    switch (k) {
    case FEELING_BASE:
      lab_table[k]->setToolTip(text_happiness_cities(pcity));
      break;

    case FEELING_LUXURY:
      lab_table[k]->setToolTip(text_happiness_luxuries(pcity));
      break;

    case FEELING_EFFECT:
      lab_table[k]->setToolTip(text_happiness_buildings(pcity));
      break;

    case FEELING_NATIONALITY:
      lab_table[k]->setToolTip(text_happiness_nationality(pcity));
      break;

    case FEELING_MARTIAL:
      lab_table[k]->setToolTip(text_happiness_units(pcity));
      break;

    default:
      break;
    }
  }
}

/************************************************************************/ /**
   Various refresh after getting new info/reply from server
 ****************************************************************************/
void city_dialog::refresh()
{
  setUpdatesEnabled(false);
  ui.production_combo_p->blockSignals(true);

  if (pcity) {
    update_title();
    update_info_label();
    update_buy_button();
    update_citizens();
    update_building();
    update_improvements();
    update_units();
    update_nation_table();
    update_cma_tab();
    update_disabled();
    key_city_show_open(pcity);
  } else {
    key_city_hide_open(pcity);
    destroy_city_dialog();
  }

  ui.production_combo_p->blockSignals(false);
  setUpdatesEnabled(true);
  updateGeometry();
  update();
}

void city_dialog::update_sliders()
{
  struct cm_parameter param;
  const struct cm_parameter *cparam;

  if (!cma_is_city_under_agent(pcity, &param)) {
    if (ui.cma_table->currentRow() == -1 || cmafec_preset_num() == 0) {
      return;
    }
    cparam = cmafec_preset_get_parameter(ui.cma_table->currentRow());
    cm_copy_parameter(&param, cparam);
  }
  ui.qsliderbox->update_sliders(param);
}
/************************************************************************/ /**
   Updates nationality table in happiness tab
 ****************************************************************************/
void city_dialog::update_nation_table()
{
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  QPixmap *pix = NULL;
  QPixmap pix_scaled;
  QString str;
  QStringList info_list;
  QTableWidgetItem *item;
  char buf[8];
  citizens nationality_i;
  int h;
  int i = 0;
  struct sprite *sprite;

  h = fm.height() + 6;
  ui.nationality_table->clear();
  ui.nationality_table->setRowCount(0);
  info_list.clear();
  info_list << _("#") << _("Flag") << _("Nation");
  ui.nationality_table->setHorizontalHeaderLabels(info_list);

  citizens_iterate(pcity, pslot, nationality)
  {
    ui.nationality_table->insertRow(i);

    for (int j = 0; j < ui.nationality_table->columnCount(); j++) {
      item = new QTableWidgetItem;

      switch (j) {
      case 0:
        nationality_i = citizens_nation_get(pcity, pslot);

        if (nationality_i == 0) {
          str = "-";
        } else {
          fc_snprintf(buf, sizeof(buf), "%d", nationality_i);
          str = QString(buf);
        }

        item->setText(str);
        break;

      case 1:
        sprite = get_nation_flag_sprite(
            tileset, nation_of_player(player_slot_get_player(pslot)));

        if (sprite != NULL) {
          pix = sprite->pm;
          pix_scaled = pix->scaledToHeight(h);
          item->setData(Qt::DecorationRole, pix_scaled);
        } else {
          item->setText("FLAG MISSING");
        }
        break;

      case 2:
        item->setText(
            nation_adjective_for_player(player_slot_get_player(pslot)));
        break;

      default:
        break;
      }
      ui.nationality_table->setItem(i, j, item);
    }
    i++;
  }
  citizens_iterate_end;
  ui.nationality_table->horizontalHeader()->setStretchLastSection(false);
  ui.nationality_table->resizeColumnsToContents();
  ui.nationality_table->resizeRowsToContents();
  ui.nationality_table->horizontalHeader()->setStretchLastSection(true);
}

/************************************************************************/ /**
   Updates information label ( food, prod ... surpluses ...)
 ****************************************************************************/
void city_dialog::update_info_label()
{
  ui.info_wdg->update_labels(pcity);
  ui.cma_city_info->update_labels(pcity);
}

/************************************************************************/ /**
   Setups whole city dialog, public function
 ****************************************************************************/
void city_dialog::setup_ui(struct city *qcity)
{
  QPixmap q_pix = *get_icon_sprite(tileset, ICON_CITYDLG)->pm;
  QIcon q_icon = ::QIcon(q_pix);

  setWindowIcon(q_icon);
  pcity = qcity;
  ui.production_combo_p->blockSignals(true);
  refresh();
  ui.production_combo_p->blockSignals(false);
}

/****************************************************************************
  Returns instance of city_dialog
****************************************************************************/
city_dialog *city_dialog::instance()
{
  if (!m_instance) {
    m_instance = new city_dialog(queen()->mapview_wdg);
  }
  return m_instance;
}

/****************************************************************************
  Deletes city_dialog instance
****************************************************************************/
void city_dialog::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

bool city_dialog::exist() { return m_instance ? true : false; }

/************************************************************************/ /**
   Removes selected item from city worklist
 ****************************************************************************/
void city_dialog::delete_prod() { display_worklist_menu(QCursor::pos()); }

/************************************************************************/ /**
   Double clicked item in worklist table in production tab
 ****************************************************************************/
void city_dialog::dbl_click_p(QTableWidgetItem *item)
{
  struct worklist queue;
  city_get_queue(pcity, &queue);

  if (selected_row_p < 0 || selected_row_p > worklist_length(&queue)) {
    return;
  }

  worklist_remove(&queue, selected_row_p);
  city_set_queue(pcity, &queue);
}

/************************************************************************/ /**
   Updates layouts for supported and present units in city
 ****************************************************************************/
void city_dialog::update_units()
{
  unit_item *uic;
  struct unit_list *units;
  char buf[256];
  int n;
  int happy_cost;
  int free_unhappy = get_city_bonus(pcity, EFT_MAKE_CONTENT_MIL);
  ui.supported_units->setUpdatesEnabled(false);
  ui.supported_units->clear_layout();

  if (NULL != client.conn.playing
      && city_owner(pcity) != client.conn.playing) {
    units = pcity->client.info_units_supported;
  } else {
    units = pcity->units_supported;
  }

  unit_list_iterate(units, punit)
  {
    happy_cost = city_unit_unhappiness(punit, &free_unhappy);
    uic = new unit_item(this, punit, true, happy_cost);
    uic->init_pix();
    ui.supported_units->add_item(uic);
  }
  unit_list_iterate_end;
  n = unit_list_size(units);
  fc_snprintf(buf, sizeof(buf), _("Supported units %d"), n);
  ui.supp_units->setText(QString(buf));
  ui.supported_units->update_units();
  ui.supported_units->setUpdatesEnabled(true);
  ui.current_units->setUpdatesEnabled(true);
  ui.current_units->clear_layout();

  if (NULL != client.conn.playing
      && city_owner(pcity) != client.conn.playing) {
    units = pcity->client.info_units_present;
  } else {
    units = pcity->tile->units;
  }

  unit_list_iterate(units, punit)
  {
    uic = new unit_item(this, punit, false);
    uic->init_pix();
    ui.current_units->add_item(uic);
  }
  unit_list_iterate_end;

  n = unit_list_size(units);
  fc_snprintf(buf, sizeof(buf), _("Present units %d"), n);
  ui.curr_units->setText(QString(buf));

  ui.current_units->update_units();
  ui.current_units->setUpdatesEnabled(true);
}

/************************************************************************/ /**
   Selection changed in production tab, in worklist tab
 ****************************************************************************/
void city_dialog::item_selected(const QItemSelection &sl,
                                const QItemSelection &ds)
{
  QModelIndex index;
  QModelIndexList indexes = sl.indexes();

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  selected_row_p = index.row();
  update_prod_buttons();
}

/************************************************************************/ /**
   Changes city_dialog to next city after pushing next city button
 ****************************************************************************/
void city_dialog::next_city()
{
  int size, i, j;
  struct city *other_pcity = NULL;

  if (NULL == client.conn.playing) {
    return;
  }

  size = city_list_size(client.conn.playing->cities);

  if (size == 1) {
    return;
  }

  for (i = 0; i < size; i++) {
    if (pcity == city_list_get(client.conn.playing->cities, i)) {
      break;
    }
  }

  for (j = 1; j < size; j++) {
    other_pcity =
        city_list_get(client.conn.playing->cities, (i + j + size) % size);
  }
  center_tile_mapcanvas(other_pcity->tile);
  key_city_hide_open(pcity);
  qtg_real_city_dialog_popup(other_pcity);
}

/************************************************************************/ /**
   Changes city_dialog to previous city after pushing prev city button
 ****************************************************************************/
void city_dialog::prev_city()
{
  int size, i, j;
  struct city *other_pcity = NULL;

  if (NULL == client.conn.playing) {
    return;
  }

  size = city_list_size(client.conn.playing->cities);

  if (size == 1) {
    return;
  }

  for (i = 0; i < size; i++) {
    if (pcity == city_list_get(client.conn.playing->cities, i)) {
      break;
    }
  }

  for (j = 1; j < size; j++) {
    other_pcity =
        city_list_get(client.conn.playing->cities, (i - j + size) % size);
  }

  center_tile_mapcanvas(other_pcity->tile);
  key_city_hide_open(pcity);
  qtg_real_city_dialog_popup(other_pcity);
}

/************************************************************************/ /**
   Updates building improvement/unit
 ****************************************************************************/
void city_dialog::update_building()
{
  char buf[32];
  QString str;
  int cost = city_production_build_shield_cost(pcity);

  get_city_dialog_production(pcity, buf, sizeof(buf));
  ui.production_combo_p->setRange(0, cost);
  ui.production_combo_p->set_pixmap(&pcity->production);
  if (pcity->shield_stock >= cost) {
    ui.production_combo_p->setValue(cost);
  } else {
    ui.production_combo_p->setValue(pcity->shield_stock);
  }
  ui.production_combo_p->setAlignment(Qt::AlignCenter);
  str = QString(buf);
  str = str.simplified();

  ui.production_combo_p->setFormat(
      QString("(%p%) %2\n%1")
          .arg(city_production_name_translation(pcity), str));

  ui.production_combo_p->updateGeometry();
}

/************************************************************************/ /**
   Buy button. Shows message box asking for confirmation
 ****************************************************************************/
void city_dialog::buy()
{
  char buf[1024], buf2[1024];
  const char *name = city_production_name_translation(pcity);
  int value = pcity->client.buy_cost;
  hud_message_box *ask;
  int city_id = pcity->id;

  if (!can_client_issue_orders()) {
    return;
  }

  ask = new hud_message_box(city_dialog::instance());
  fc_snprintf(buf2, ARRAY_SIZE(buf2),
              PL_("Treasury contains %d gold.", "Treasury contains %d gold.",
                  client_player()->economic.gold),
              client_player()->economic.gold);
  fc_snprintf(buf, ARRAY_SIZE(buf),
              PL_("Buy %s for %d gold?", "Buy %s for %d gold?", value), name,
              value);
  ask->set_text_title(buf, buf2);
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, this, [=]() {
    struct city *pcity = game_city_by_number(city_id);

    if (!pcity) {
      return;
    }

    city_buy_production(pcity);
  });
  ask->show();
}

/************************************************************************/ /**
   Updates list of improvements
 ****************************************************************************/
void city_dialog::update_improvements()
{
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  QPixmap *pix = NULL;
  QPixmap pix_scaled;
  QString str, tooltip;
  QTableWidgetItem *qitem;
  struct sprite *sprite;
  int h, cost, item, targets_used, col, upkeep;
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct worklist queue;
  impr_item *ii;

  upkeep = 0;
  ui.city_buildings->setUpdatesEnabled(false);
  ui.city_buildings->clear_layout();

  h = fm.height() + 6;
  targets_used = collect_already_built_targets(targets, pcity);
  name_and_sort_items(targets, targets_used, items, false, pcity);

  for (item = 0; item < targets_used; item++) {
    struct universal target = items[item].item;

    ii = new impr_item(this, target.value.building, pcity);
    ii->init_pix();
    ui.city_buildings->add_item(ii);

    fc_assert_action(VUT_IMPROVEMENT == target.kind, continue);
    sprite = get_building_sprite(tileset, target.value.building);
    upkeep = upkeep + city_improvement_upkeep(pcity, target.value.building);
    if (sprite != nullptr) {
      pix = sprite->pm;
      pix_scaled = pix->scaledToHeight(h);
    }
  }

  city_get_queue(pcity, &queue);
  ui.p_table_p->setRowCount(worklist_length(&queue));

  for (int i = 0; i < worklist_length(&queue); i++) {
    struct universal target = queue.entries[i];

    tooltip = "";

    if (VUT_UTYPE == target.kind) {
      str = utype_values_translation(target.value.utype);
      cost = utype_build_shield_cost(pcity, target.value.utype);
      tooltip = get_tooltip_unit(target.value.utype, true).trimmed();
      sprite = get_unittype_sprite(get_tileset(), target.value.utype,
                                   direction8_invalid());
    } else {
      str = city_improvement_name_translation(pcity, target.value.building);
      sprite = get_building_sprite(tileset, target.value.building);
      tooltip = get_tooltip_improvement(target.value.building, pcity, true)
                    .trimmed();

      if (improvement_has_flag(target.value.building, IF_GOLD)) {
        cost = -1;
      } else {
        cost = impr_build_shield_cost(pcity, target.value.building);
      }
    }

    for (col = 0; col < 3; col++) {
      qitem = new QTableWidgetItem();
      qitem->setToolTip(tooltip);

      switch (col) {
      case 0:
        if (sprite) {
          pix = sprite->pm;
          pix_scaled = pix->scaledToHeight(h);
          qitem->setData(Qt::DecorationRole, pix_scaled);
        }
        break;

      case 1:
        if (str.contains('[') && str.contains(']')) {
          int ii, ij;

          ii = str.lastIndexOf('[');
          ij = str.lastIndexOf(']');
          if (ij > ii) {
            str = str.remove(ii, ij - ii + 1);
          }
        }
        qitem->setText(str);
        break;

      case 2:
        qitem->setTextAlignment(Qt::AlignRight);
        qitem->setText(QString::number(cost));
        break;
      }
      ui.p_table_p->setItem(i, col, qitem);
    }
  }

  ui.p_table_p->horizontalHeader()->setStretchLastSection(false);
  ui.p_table_p->resizeColumnsToContents();
  ui.p_table_p->resizeRowsToContents();
  ui.p_table_p->horizontalHeader()->setStretchLastSection(true);

  ui.city_buildings->update_buildings();
  ui.city_buildings->setUpdatesEnabled(true);
  ui.city_buildings->setUpdatesEnabled(true);

  ui.curr_impr->setText(QString(_("Improvements - upkeep %1")).arg(upkeep));
}

/************************************************************************/ /**
   Slot executed when user changed production in customized table widget
 ****************************************************************************/
void city_dialog::production_changed(int index)
{
  cid id;
  QVariant qvar;

  if (can_client_issue_orders()) {
    struct universal univ;

    id = qvar.toInt();
    univ = cid_production(id);
    city_change_production(pcity, &univ);
  }
}

/************************************************************************/ /**
   Shows customized table widget with available items to produce
   Shows default targets in overview city page
 ****************************************************************************/
void city_dialog::show_targets()
{
  production_widget *pw;
  int when = 1;
  pw = new production_widget(this, pcity, future_targets, when,
                             selected_row_p, show_units, false, show_wonders,
                             show_buildings);
  pw->show();
}

/************************************************************************/ /**
   Shows customized table widget with available items to produce
   Shows customized targets in city production page
 ****************************************************************************/
void city_dialog::show_targets_worklist()
{
  production_widget *pw;
  int when = 4;
  pw = new production_widget(this, pcity, future_targets, when,
                             selected_row_p, show_units, false, show_wonders,
                             show_buildings);
  pw->show();
}

/************************************************************************/ /**
   Clears worklist in production page
 ****************************************************************************/
void city_dialog::clear_worklist()
{
  struct worklist empty;

  if (!can_client_issue_orders()) {
    return;
  }

  worklist_init(&empty);
  city_set_worklist(pcity, &empty);
}

/************************************************************************/ /**
   Move current item on worklist up
 ****************************************************************************/
void city_dialog::worklist_up()
{
  QModelIndex index;
  struct worklist queue;
  struct universal *target;

  if (selected_row_p < 1 || selected_row_p >= ui.p_table_p->rowCount()) {
    return;
  }

  target = new universal;
  city_get_queue(pcity, &queue);
  worklist_peek_ith(&queue, target, selected_row_p);
  worklist_remove(&queue, selected_row_p);
  worklist_insert(&queue, target, selected_row_p - 1);
  city_set_queue(pcity, &queue);
  index = ui.p_table_p->model()->index(selected_row_p - 1, 0);
  ui.p_table_p->setCurrentIndex(index);
  delete target;
}

/************************************************************************/ /**
   Remove current item on worklist
 ****************************************************************************/
void city_dialog::worklist_del()
{
  QTableWidgetItem *item;

  if (selected_row_p < 0 || selected_row_p >= ui.p_table_p->rowCount()) {
    return;
  }

  item = ui.p_table_p->item(selected_row_p, 0);
  dbl_click_p(item);
  update_prod_buttons();
}

/************************************************************************/ /**
   Move current item on worklist down
 ****************************************************************************/
void city_dialog::worklist_down()
{
  QModelIndex index;
  struct worklist queue;
  struct universal *target;

  if (selected_row_p < 0 || selected_row_p >= ui.p_table_p->rowCount() - 1) {
    return;
  }

  target = new universal;
  city_get_queue(pcity, &queue);
  worklist_peek_ith(&queue, target, selected_row_p);
  worklist_remove(&queue, selected_row_p);
  worklist_insert(&queue, target, selected_row_p + 1);
  city_set_queue(pcity, &queue);
  index = ui.p_table_p->model()->index(selected_row_p + 1, 0);
  ui.p_table_p->setCurrentIndex(index);
  delete target;
}

/************************************************************************/ /**
   Save worklist
 ****************************************************************************/
void city_dialog::save_worklist()
{
  hud_input_box *ask = new hud_input_box(king()->central_wdg);
  int city_id = pcity->id;

  ask->set_text_title_definput(_("What should we name new worklist?"),
                               _("Save current worklist"),
                               _("New worklist"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, [=]() {
    struct global_worklist *gw;
    struct worklist queue;
    QByteArray ask_bytes;
    QString text;
    struct city *pcity = game_city_by_number(city_id);

    ask_bytes = ask->input_edit.text().toLocal8Bit();
    text = ask_bytes.data();
    if (!text.isEmpty()) {
      ask_bytes = text.toLocal8Bit();
      gw = global_worklist_new(ask_bytes.data());
      city_get_queue(pcity, &queue);
      global_worklist_set(gw, &queue);
    }
  });
  ask->show();
}

void city_dialog::cma_check_agent()
{
  if (cma_is_city_under_agent(pcity, NULL)) {
    cma_changed();
    update_cma_tab();
  }
}
/************************************************************************/ /**
   Puts city name and people count on title
 ****************************************************************************/
void city_dialog::update_title()
{
  QString buf;

  // Defeat keyboard shortcut mnemonics
  ui.lcity_name->setText(QString(city_name_get(pcity)).replace("&", "&&"));

  if (city_unhappy(pcity)) {
    /* TRANS: city dialog title */
    buf = QString(_("%1 - %2 citizens - DISORDER"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  } else if (city_celebrating(pcity)) {
    /* TRANS: city dialog title */
    buf = QString(_("%1 - %2 citizens - celebrating"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  } else if (city_happy(pcity)) {
    /* TRANS: city dialog title */
    buf = QString(_("%1 - %2 citizens - happy"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  } else {
    /* TRANS: city dialog title */
    buf = QString(_("%1 - %2 citizens"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  }

  setWindowTitle(buf);
}

/************************************************************************/ /**
   Pop up (or bring to the front) a dialog for the given city.  It may or
   may not be modal.
 ****************************************************************************/
void qtg_real_city_dialog_popup(struct city *pcity)
{
  city_dialog::instance()->setup_ui(pcity);
  city_dialog::instance()->show();
  city_dialog::instance()->activateWindow();
  city_dialog::instance()->raise();
}

/************************************************************************/ /**
   Closes city dialog
 ****************************************************************************/
void destroy_city_dialog()
{
  // Only tyrans destroy cities instead building
  if (king()->current_page() >= PAGE_GAME)
    city_dialog::instance()->drop();
}

/************************************************************************/ /**
   Close the dialog for the given city.
 ****************************************************************************/
void qtg_popdown_city_dialog(struct city *pcity)
{
  city_dialog::instance()->hide();
}

/************************************************************************/ /**
   Close the dialogs for all cities.
 ****************************************************************************/
void qtg_popdown_all_city_dialogs() { destroy_city_dialog(); }

/************************************************************************/ /**
   Refresh (update) all data for the given city's dialog.
 ****************************************************************************/
void qtg_real_city_dialog_refresh(struct city *pcity)
{
  if (qtg_city_dialog_is_open(pcity)) {
    city_dialog::instance()->refresh();
  }
}

/************************************************************************/ /**
   Updates city font
 ****************************************************************************/
void city_font_update()
{
  QList<QLabel *> l;
  QFont *f;

  l = city_dialog::instance()->findChildren<QLabel *>();

  f = fc_font::instance()->get_font(fonts::notify_label);

  for (int i = 0; i < l.size(); ++i) {
    if (l.at(i)->property(fonts::notify_label).isValid()) {
      l.at(i)->setFont(*f);
    }
  }
}

/************************************************************************/ /**
   Update city dialogs when the given unit's status changes.  This
   typically means updating both the unit's home city (if any) and the
   city in which it is present (if any).
 ****************************************************************************/
void qtg_refresh_unit_city_dialogs(struct unit *punit)
{

  struct city *pcity_sup, *pcity_pre;

  pcity_sup = game_city_by_number(punit->homecity);
  pcity_pre = tile_city(punit->tile);

  qtg_real_city_dialog_refresh(pcity_sup);
  qtg_real_city_dialog_refresh(pcity_pre);
}

struct city *is_any_city_dialog_open()
{
  // some checks not to iterate cities
  if (!city_dialog::exist())
    return nullptr;
  if (!city_dialog::instance()->isVisible())
    return nullptr;
  if (client_is_global_observer() || client_is_observer())
    return nullptr;

  city_list_iterate(client.conn.playing->cities, pcity)
  {
    if (qtg_city_dialog_is_open(pcity))
      return pcity;
  }
  city_list_iterate_end;
  return nullptr;
}

/************************************************************************/ /**
   Return whether the dialog for the given city is open.
 ****************************************************************************/
bool qtg_city_dialog_is_open(struct city *pcity)
{
  if (!city_dialog::exist())
    return false;
  if (city_dialog::instance()->pcity == pcity
      && city_dialog::instance()->isVisible()) {
    return true;
  }

  return false;
}

/************************************************************************/ /**
   City item delegate constructor
 ****************************************************************************/
city_production_delegate::city_production_delegate(QPoint sh,
                                                   QObject *parent,
                                                   struct city *city)
    : QItemDelegate(parent), pd(sh)
{
  item_height = sh.y();
  pcity = city;
}

/************************************************************************/ /**
   City item delgate paint event
 ****************************************************************************/
void city_production_delegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
  struct universal *target;
  QString name;
  QVariant qvar;
  QPixmap *pix;
  QPixmap pix_scaled;
  QRect rect1;
  QRect rect2;
  struct sprite *sprite;
  bool useless = false;
  bool is_coinage = false;
  bool is_neutral = false;
  bool is_sea = false;
  bool is_flying = false;
  bool is_unit = true;
  QPixmap pix_dec(option.rect.width(), option.rect.height());
  QStyleOptionViewItem opt;
  color col;
  QIcon icon = qapp->style()->standardIcon(QStyle::SP_DialogCancelButton);
  bool free_sprite = false;
  struct unit_class *pclass;

  if (!option.rect.isValid()) {
    return;
  }

  qvar = index.data();

  if (!qvar.isValid()) {
    return;
  }

  target = reinterpret_cast<universal *>(qvar.value<void *>());

  if (target == NULL) {
    col.qcolor = Qt::white;
    sprite = qtg_create_sprite(100, 100, &col);
    free_sprite = true;
    *sprite->pm = icon.pixmap(100, 100);
    name = _("Cancel");
    is_unit = false;
  } else if (VUT_UTYPE == target->kind) {
    name = utype_name_translation(target->value.utype);
    is_neutral = utype_has_flag(target->value.utype, UTYF_CIVILIAN);
    pclass = utype_class(target->value.utype);
    if (!uclass_has_flag(pclass, UCF_TERRAIN_DEFENSE)
        && !utype_can_do_action_result(target->value.utype, ACTRES_FORTIFY)
        && !uclass_has_flag(pclass, UCF_ZOC)) {
      is_sea = true;
    }

    if ((utype_fuel(target->value.utype)
         && !uclass_has_flag(pclass, UCF_TERRAIN_DEFENSE)
         && !utype_can_do_action_result(target->value.utype, ACTRES_PILLAGE)
         && !utype_can_do_action_result(target->value.utype, ACTRES_FORTIFY)
         && !uclass_has_flag(pclass, UCF_ZOC))
        /* FIXME: Assumed to be flying since only missiles can do suicide
         * attacks in classic-like rulesets. This isn't true for all
         * rulesets. Not a high priority to fix since all is_flying and
         * is_sea is used for is to set a color. */
        || utype_is_consumed_by_action_result(ACTRES_ATTACK,
                                              target->value.utype)) {
      if (is_sea) {
        is_sea = false;
      }
      is_flying = true;
    }

    sprite = get_unittype_sprite(get_tileset(), target->value.utype,
                                 direction8_invalid());
  } else {
    is_unit = false;
    name = improvement_name_translation(target->value.building);
    sprite = get_building_sprite(tileset, target->value.building);
    useless = is_improvement_redundant(pcity, target->value.building);
    is_coinage = improvement_has_flag(target->value.building, IF_GOLD);
  }

  if (sprite != NULL) {
    pix = sprite->pm;
    pix_scaled =
        pix->scaledToHeight(item_height - 2, Qt::SmoothTransformation);

    if (useless) {
      pixmap_put_x(&pix_scaled);
    }
  }

  opt = QItemDelegate::setOptions(index, option);
  painter->save();
  opt.displayAlignment = Qt::AlignLeft;
  opt.textElideMode = Qt::ElideMiddle;
  QItemDelegate::drawBackground(painter, opt, index);
  rect1 = option.rect;
  rect1.setWidth(pix_scaled.width() + 4);
  rect2 = option.rect;
  rect2.setLeft(option.rect.left() + rect1.width());
  rect2.setTop(rect2.top()
               + (rect2.height() - painter->fontMetrics().height()) / 2);
  QItemDelegate::drawDisplay(painter, opt, rect2, name);

  if (is_unit) {
    if (is_sea) {
      pix_dec.fill(QColor(0, 0, 255, 80));
    } else if (is_flying) {
      pix_dec.fill(QColor(220, 0, 0, 80));
    } else if (is_neutral) {
      pix_dec.fill(QColor(0, 120, 0, 40));
    } else {
      pix_dec.fill(QColor(0, 0, 150, 40));
    }

    QItemDelegate::drawDecoration(painter, option, option.rect, pix_dec);
  }

  if (is_coinage) {
    pix_dec.fill(QColor(255, 255, 0, 70));
    QItemDelegate::drawDecoration(painter, option, option.rect, pix_dec);
  }

  if (!pix_scaled.isNull()) {
    QItemDelegate::drawDecoration(painter, opt, rect1, pix_scaled);
  }

  drawFocus(painter, opt, option.rect);

  painter->restore();

  if (free_sprite) {
    qtg_free_sprite(sprite);
  }
}

/************************************************************************/ /**
   Draws focus for given item
 ****************************************************************************/
void city_production_delegate::drawFocus(QPainter *painter,
                                         const QStyleOptionViewItem &option,
                                         const QRect &rect) const
{
  QPixmap pix(option.rect.width(), option.rect.height());

  if ((option.state & QStyle::State_MouseOver) == 0 || !rect.isValid()) {
    return;
  }

  pix.fill(QColor(50, 50, 50, 50));
  QItemDelegate::drawDecoration(painter, option, option.rect, pix);
}

/************************************************************************/ /**
   Size hint for city item delegate
 ****************************************************************************/
QSize city_production_delegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
  QSize s;

  s.setWidth(pd.x());
  s.setHeight(pd.y());
  return s;
}

/************************************************************************/ /**
   Production item constructor
 ****************************************************************************/
production_item::production_item(struct universal *ptarget, QObject *parent)
    : QObject()
{
  setParent(parent);
  target = ptarget;
}

/************************************************************************/ /**
   Production item destructor
 ****************************************************************************/
production_item::~production_item()
{
  /* allocated as renegade in model */
  if (target != NULL) {
    delete target;
  }
}

/************************************************************************/ /**
   Returns stored data
 ****************************************************************************/
QVariant production_item::data() const
{
  return QVariant::fromValue((void *) target);
}

/************************************************************************/ /**
   Sets data for item, must be declared.
 ****************************************************************************/
bool production_item::setData() { return false; }

/************************************************************************/ /**
   Constructor for city production model
 ****************************************************************************/
city_production_model::city_production_model(struct city *pcity, bool f,
                                             bool su, bool sw, bool sb,
                                             QObject *parent)
    : QAbstractListModel(parent)
{
  show_units = su;
  show_wonders = sw;
  show_buildings = sb;
  mcity = pcity;
  future_t = f;
  populate();
}

/************************************************************************/ /**
   Destructor for city production model
 ****************************************************************************/
city_production_model::~city_production_model()
{
  qDeleteAll(city_target_list);
  city_target_list.clear();
}

/************************************************************************/ /**
   Returns data from model
 ****************************************************************************/
QVariant city_production_model::data(const QModelIndex &index,
                                     int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount()
      && (index.column() + index.row() * 3 < city_target_list.count())) {
    int r, c, t, new_index;
    r = index.row();
    c = index.column();
    t = r * 3 + c;
    new_index = t / 3 + rowCount() * c;
    /* Exception, shift whole column */
    if ((c == 2) && city_target_list.count() % 3 == 1) {
      new_index = t / 3 + rowCount() * c - 1;
    }
    if (role == Qt::ToolTipRole) {
      return get_tooltip(city_target_list[new_index]->data());
    }

    return city_target_list[new_index]->data();
  }

  return QVariant();
}

/************************************************************************/ /**
   Fills model with data
 ****************************************************************************/
void city_production_model::populate()
{
  production_item *pi;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  struct universal *renegade;
  int item, targets_used;
  QString str;
  QFont f = *fc_font::instance()->get_font(fonts::default_font);
  QFontMetrics fm(f);

  sh.setY(fm.height() * 2);
  sh.setX(0);

  qDeleteAll(city_target_list);
  city_target_list.clear();

  targets_used =
      collect_eventually_buildable_targets(targets, mcity, future_t);
  name_and_sort_items(targets, targets_used, items, false, mcity);

  for (item = 0; item < targets_used; item++) {
    if (future_t || can_city_build_now(mcity, &items[item].item)) {
      renegade = new universal(items[item].item);

      /* renagade deleted in production_item destructor */
      if (VUT_UTYPE == renegade->kind) {
        str = utype_name_translation(renegade->value.utype);
        sh.setX(qMax(sh.x(), fm.horizontalAdvance(str)));

        if (show_units) {
          pi = new production_item(renegade, this);
          city_target_list << pi;
        }
      } else {
        str = improvement_name_translation(renegade->value.building);
        sh.setX(qMax(sh.x(), fm.horizontalAdvance(str)));

        if ((is_wonder(renegade->value.building) && show_wonders)
            || (is_improvement(renegade->value.building) && show_buildings)
            || (improvement_has_flag(renegade->value.building, IF_GOLD))
            || (is_special_improvement(renegade->value.building)
                && show_buildings)) {
          pi = new production_item(renegade, this);
          city_target_list << pi;
        }
      }
    }
  }

  renegade = NULL;
  pi = new production_item(renegade, this);
  city_target_list << pi;
  sh.setX(2 * sh.y() + sh.x());
  sh.setX(qMin(sh.x(), 250));
}

/************************************************************************/ /**
   Sets data in model
 ****************************************************************************/
bool city_production_model::setData(const QModelIndex &index,
                                    const QVariant &value, int role)
{
  if (!index.isValid() || role != Qt::DisplayRole || role != Qt::ToolTipRole)
    return false;

  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount()) {
    bool change = city_target_list[index.row()]->setData();
    return change;
  }

  return false;
}

/************************************************************************/ /**
   Constructor for production widget
   future - show future targets
   show_units - if to show units
   when - where to insert
   curr - current index to insert
   buy - buy if possible
 ****************************************************************************/
production_widget::production_widget(QWidget *parent, struct city *pcity,
                                     bool future, int when, int curr,
                                     bool show_units, bool buy,
                                     bool show_wonders, bool show_buildings)
    : QTableView()
{
  QPoint pos, sh;
  auto temp = QGuiApplication::screens();
  int desk_width = temp[0]->availableGeometry().width();
  int desk_height = temp[0]->availableGeometry().height();
  fc_tt = new fc_tooltip(this);
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowFlags(Qt::Popup);
  verticalHeader()->setVisible(false);
  horizontalHeader()->setVisible(false);
  setProperty("showGrid", false);
  curr_selection = curr;
  sh_units = show_units;
  pw_city = pcity;
  buy_it = buy;
  when_change = when;
  list_model = new city_production_model(pw_city, future, show_units,
                                         show_wonders, show_buildings, this);
  sh = list_model->sh;
  c_p_d = new city_production_delegate(sh, this, pw_city);
  setItemDelegate(c_p_d);
  setModel(list_model);
  viewport()->installEventFilter(fc_tt);
  installEventFilter(this);
  connect(
      selectionModel(),
      SIGNAL(
          selectionChanged(const QItemSelection &, const QItemSelection &)),
      SLOT(prod_selected(const QItemSelection &, const QItemSelection &)));
  resizeRowsToContents();
  resizeColumnsToContents();
  setFixedWidth(3 * sh.x() + 6);
  setFixedHeight(list_model->rowCount() * sh.y() + 6);

  if (width() > desk_width) {
    setFixedWidth(desk_width);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  } else {
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }

  if (height() > desk_height) {
    setFixedHeight(desk_height);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  } else {
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }

  pos = QCursor::pos();

  if (pos.x() + width() > desk_width) {
    pos.setX(desk_width - width());
  } else if (pos.x() - width() < 0) {
    pos.setX(0);
  }

  if (pos.y() + height() > desk_height) {
    pos.setY(desk_height - height());
  } else if (pos.y() - height() < 0) {
    pos.setY(0);
  }

  move(pos);
  setMouseTracking(true);
  setFocus();
}

/************************************************************************/ /**
   Mouse press event for production widget
 ****************************************************************************/
void production_widget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    close();
    return;
  }

  QAbstractItemView::mousePressEvent(event);
}

/************************************************************************/ /**
   Event filter for production widget
 ****************************************************************************/
bool production_widget::eventFilter(QObject *obj, QEvent *ev)
{
  QRect pw_rect;
  QPoint br;

  if (obj != this)
    return false;

  if (ev->type() == QEvent::MouseButtonPress) {
    pw_rect.setTopLeft(pos());
    br.setX(pos().x() + width());
    br.setY(pos().y() + height());
    pw_rect.setBottomRight(br);

    if (!pw_rect.contains(QCursor::pos())) {
      close();
    }
  }

  return false;
}

/************************************************************************/ /**
   Changed selection in production widget
 ****************************************************************************/
void production_widget::prod_selected(const QItemSelection &sl,
                                      const QItemSelection &ds)
{
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  QModelIndex index;
  QVariant qvar;
  struct worklist queue;
  struct universal *target;

  if (indexes.isEmpty() || client_is_observer()) {
    return;
  }
  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  if (!qvar.isValid()) {
    return;
  }
  target = reinterpret_cast<universal *>(qvar.value<void *>());
  if (target != NULL) {
    city_get_queue(pw_city, &queue);
    switch (when_change) {
    case 0: /* Change current target */
      city_change_production(pw_city, target);
      if (city_can_buy(pw_city) && buy_it) {
        city_buy_production(pw_city);
      }
      break;

    case 1: /* Change current (selected on list)*/
      if (curr_selection < 0 || curr_selection > worklist_length(&queue)) {
        city_change_production(pw_city, target);
      } else {
        worklist_remove(&queue, curr_selection);
        worklist_insert(&queue, target, curr_selection);
        city_set_queue(pw_city, &queue);
      }
      break;

    case 2: /* Insert before */
      if (curr_selection < 0 || curr_selection > worklist_length(&queue)) {
        curr_selection = 0;
      }
      curr_selection--;
      curr_selection = qMax(0, curr_selection);
      worklist_insert(&queue, target, curr_selection);
      city_set_queue(pw_city, &queue);
      break;

    case 3: /* Insert after */
      if (curr_selection < 0 || curr_selection > worklist_length(&queue)) {
        city_queue_insert(pw_city, -1, target);
        break;
      }
      curr_selection++;
      worklist_insert(&queue, target, curr_selection);
      city_set_queue(pw_city, &queue);
      break;

    case 4: /* Add last */
      city_queue_insert(pw_city, -1, target);
      break;

    default:
      break;
    }
  }
  close();
  destroy();
}

/************************************************************************/ /**
   Destructor for production widget
 ****************************************************************************/
production_widget::~production_widget()
{
  delete c_p_d;
  delete list_model;
  viewport()->removeEventFilter(fc_tt);
  removeEventFilter(this);
}
