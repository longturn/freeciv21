/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "citydlg.h"
// Qt
#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidgetAction>

// utility
#include "fc_types.h"
#include "fcintl.h"
#include "player.h"
#include "support.h"

// common
#include "citizens.h"
#include "city.h"
#include "effects.h"
#include "game.h"
#include "tech.h"
#include "worklist.h"

// client
#include "canvas.h"
#include "citydlg_common.h"
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "editor/map_editor.h"
#include "fc_client.h"
#include "fonts.h"
#include "global_worklist.h"
#include "governor.h"
#include "hudwidget.h"
#include "icons.h"
#include "mapctrl_common.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "text.h"
#include "tileset/tilespec.h"
#include "tooltips.h"
#include "top_bar.h"
#include "unitlist.h"
#include "utils/improvement_seller.h"
#include "utils/unit_quick_menu.h"
#include "utils/unit_utils.h"
#include "views/view_cities.h" // hIcon
#include "views/view_map.h"
#include "views/view_map_common.h"
#include "widgets/city/governor_widget.h"

extern QString split_text(const QString &text, bool cut);
extern QString cut_helptext(const QString &text);

/**
   Constructor
 */
unit_list_widget::unit_list_widget(QWidget *parent) : QListWidget(parent)
{
  // Make sure viewportSizeHint is used
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  setWrapping(true);
  setMovement(QListView::Static);

  connect(this, &QListWidget::itemDoubleClicked, this,
          &unit_list_widget::activate);
}

/**
   Reimplemented virtual method.
 */
QSize unit_list_widget::viewportSizeHint() const
{
  if (!m_oneliner) {
    return QSize(1, 5555);
  }
  // Try to put everything on one line
  QSize hint;
  for (int i = 0; i < count(); ++i) {
    hint = hint.expandedTo(sizeHintForIndex(indexFromItem(item(i))));
  }
  hint.setWidth(hint.width() * count());
  return hint;
}

/**
 * Sets the list of units to be displayed.
 */
void unit_list_widget::set_units(unit_list *units)
{
  setUpdatesEnabled(false);
  clear();

  QSize icon_size;
  for (const auto *punit : sorted(units)) {
    auto *item = new QListWidgetItem();
    item->setToolTip(unit_description(punit));
    item->setData(Qt::UserRole, punit->id);

    auto pixmap = create_unit_image(punit);
    icon_size = icon_size.expandedTo(pixmap.size());
    item->setIcon(QIcon(pixmap));
    addItem(item);
  }

  setGridSize(icon_size);
  setIconSize(icon_size);

  setUpdatesEnabled(true);
  updateGeometry();
}

/**
 * Finds the list of currently selected units
 */
std::vector<unit *> unit_list_widget::selected_playable_units() const
{
  if (!can_client_issue_orders()) {
    return {};
  }

  auto units = std::vector<unit *>();
  for (const auto item : selectedItems()) {
    auto id = item->data(Qt::UserRole).toInt();
    const auto unit = game_unit_by_number(id);
    if (unit && unit_owner(unit) == client_player()) {
      units.push_back(unit);
    }
  }
  return units;
}

/**
 * Reimplemented to provide the unit context menu.
 */
void unit_list_widget::contextMenuEvent(QContextMenuEvent *event)
{
  const auto units = selected_playable_units();
  if (units.empty()) {
    return;
  }

  auto menu = new QMenu;
  menu->setAttribute(Qt::WA_DeleteOnClose);
  freeciv::add_quick_unit_actions(menu, units);
  menu->popup(event->globalPos());

  event->accept();
}

/**
 * Activates unit and closes city dialog
 */
void unit_list_widget::activate()
{
  const auto selection = selected_playable_units();

  unit_focus_set(nullptr); // Clear
  for (const auto unit : selection) {
    unit_focus_add(unit);
  }

  if (!selection.empty()) {
    queen()->city_overlay->dont_focus = true;
    popdown_city_dialog();
  }
}

/**
 * Creates the image to represent the given unit in the list
 */
QPixmap unit_list_widget::create_unit_image(const unit *punit)
{
  int happy_cost = 0;
  if (m_show_upkeep) {
    if (auto home = game_city_by_number(punit->homecity)) {
      auto free_unhappy = get_city_bonus(home, EFT_MAKE_CONTENT_MIL);
      happy_cost = city_unit_unhappiness(punit, &free_unhappy);
    }
  }

  double isosize = 0.6;
  if (tileset_hex_height(tileset) > 0 || tileset_hex_width(tileset) > 0) {
    isosize = 0.45;
  }

  auto unit_pixmap = QPixmap();
  if (punit) {
    if (m_show_upkeep) {
      unit_pixmap = QPixmap(tileset_unit_width(get_tileset()),
                            tileset_unit_with_upkeep_height(get_tileset()));
    } else {
      unit_pixmap = QPixmap(tileset_unit_width(get_tileset()),
                            tileset_unit_height(get_tileset()));
    }

    unit_pixmap.fill(Qt::transparent);
    put_unit(punit, &unit_pixmap, QPoint());

    if (m_show_upkeep) {
      put_unit_city_overlays(punit, &unit_pixmap, 0,
                             tileset_unit_layout_offset_y(get_tileset()),
                             punit->upkeep, happy_cost);
    }
  }

  auto img = unit_pixmap.toImage();
  auto crop_rect = zealous_crop_rect(img);
  img = img.copy(crop_rect);

  if (tileset_is_isometric(tileset)) {
    return QPixmap::fromImage(
        img.scaledToHeight(tileset_unit_width(get_tileset()) * isosize,
                           Qt::SmoothTransformation));
  } else {
    return QPixmap::fromImage(img.scaledToHeight(
        tileset_unit_width(get_tileset()), Qt::SmoothTransformation));
  }
}

/**
   Custom progressbar constructor
 */
progress_bar::progress_bar(QWidget *parent) : QProgressBar(parent)
{
  create_region();
  sfont = new QFont;
  pix = nullptr;
}

/**
   Custom progressbar destructor
 */
progress_bar::~progress_bar()
{
  delete pix;
  delete sfont;
}

/**
   Custom progressbar resize event
 */
void progress_bar::resizeEvent(QResizeEvent *event)
{
  Q_UNUSED(event);
  create_region();
}

/**
   Sets pixmap from given universal for custom progressbar
 */
void progress_bar::set_pixmap(struct universal *target)
{
  const QPixmap *sprite;
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
  delete pix;
  if (sprite == nullptr) {
    pix = nullptr;
    return;
  }
  img = sprite->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  tpix = QPixmap::fromImage(cropped_img);
  pix = new QPixmap(tpix.width(), tpix.height());
  pix->fill(Qt::transparent);
  pixmap_copy(pix, &tpix, 0, 0, 0, 0, tpix.width(), tpix.height());
}

/**
   Sets pixmap from given tech number for custom progressbar
 */
void progress_bar::set_pixmap(int n)
{
  const QPixmap *sprite = nullptr;
  if (valid_advance_by_number(n)) {
    sprite = get_tech_sprite(tileset, n);
  }
  delete pix;
  if (sprite == nullptr) {
    pix = nullptr;
    return;
  }
  pix = new QPixmap(sprite->width(), sprite->height());
  pix->fill(Qt::transparent);
  pixmap_copy(pix, sprite, 0, 0, 0, 0, sprite->width(), sprite->height());
  if (isVisible()) {
    update();
  }
}

/**
   Paint event for custom progress bar
 */
void progress_bar::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)
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
  p.setClipRegion(reg);

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

  // draw icon
  if (pix != nullptr) {
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.drawPixmap(
        2, 2, pix_width * static_cast<float>(pix->width()) / pix->height(),
        pix_width, *pix, 0, 0, pix->width(), pix->height());
  }

  // draw text
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
    s2 = text().right(text().size() - i);

    if (2 * f_size >= 2 * height() / 3) {
      if (point_size < 0) {
        sfont->setPixelSize(height() / 4);
      } else {
        sfont->setPointSize(height() / 4);
      }
    }

    j = height() - 2 * f_size;

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

/**
   Creates region with diagonal lines
 */
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

/**
   Draws X on pixmap pointing its useless
 */
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

cityIconInfoLabel::cityIconInfoLabel(QWidget *parent) : QWidget(parent)
{
  auto f = fcFont::instance()->getFont(fonts::default_font);
  QFontMetrics fm(f);

  pixHeight = fm.height();

  initLayout();
}

void cityIconInfoLabel::setCity(city *pciti) { pcity = pciti; }

// inits icons only
void cityIconInfoLabel::initLayout()
{
  QHBoxLayout *l = new QHBoxLayout();
  labs[0].setPixmap(
      (hIcon::i()->get(QStringLiteral("foodplus"))).pixmap(pixHeight));
  l->addWidget(&labs[0]);
  l->addWidget(&labs[1]);

  labs[2].setPixmap(
      (hIcon::i()->get(QStringLiteral("prodplus"))).pixmap(pixHeight));
  l->addWidget(&labs[2]);
  l->addWidget(&labs[3]);

  labs[4].setPixmap(
      (hIcon::i()->get(QStringLiteral("gold"))).pixmap(pixHeight));
  l->addWidget(&labs[4]);
  l->addWidget(&labs[5]);

  labs[6].setPixmap(
      (hIcon::i()->get(QStringLiteral("science"))).pixmap(pixHeight));
  l->addWidget(&labs[6]);
  l->addWidget(&labs[7]);

  labs[8].setPixmap(
      (hIcon::i()->get(QStringLiteral("tradeplus"))).pixmap(pixHeight));
  l->addWidget(&labs[8]);
  l->addWidget(&labs[9]);

  labs[10].setPixmap(
      (hIcon::i()->get(QStringLiteral("resize"))).pixmap(pixHeight));
  l->addWidget(&labs[10]);
  l->addWidget(&labs[11]);

  setLayout(l);
}

void cityIconInfoLabel::updateText()
{
  QString grow_time;
  if (!pcity) {
    return;
  }

  labs[1].setText(QString::number(pcity->surplus[O_FOOD]));
  labs[0].setToolTip(get_city_dialog_output_text(pcity, O_FOOD));
  labs[1].setToolTip(get_city_dialog_output_text(pcity, O_FOOD));

  labs[3].setText(QString::number(pcity->surplus[O_SHIELD]));
  labs[2].setToolTip(get_city_dialog_output_text(pcity, O_SHIELD));
  labs[3].setToolTip(get_city_dialog_output_text(pcity, O_SHIELD));

  labs[5].setText(QString::number(pcity->surplus[O_GOLD]));
  labs[4].setToolTip(get_city_dialog_output_text(pcity, O_GOLD));
  labs[5].setToolTip(get_city_dialog_output_text(pcity, O_GOLD));

  labs[7].setText(QString::number(pcity->surplus[O_SCIENCE]));
  labs[6].setToolTip(get_city_dialog_output_text(pcity, O_SCIENCE));
  labs[7].setToolTip(get_city_dialog_output_text(pcity, O_SCIENCE));

  labs[9].setText(QString::number(pcity->surplus[O_TRADE]));
  labs[8].setToolTip(get_city_dialog_output_text(pcity, O_TRADE));
  labs[9].setToolTip(get_city_dialog_output_text(pcity, O_TRADE));

  if (city_turns_to_grow(pcity) < 1000) {
    grow_time = QString::number(city_turns_to_grow(pcity));
  } else {
    grow_time = QStringLiteral("∞");
  }
  labs[11].setText(grow_time);
  labs[10].setToolTip(get_city_dialog_growth_value(pcity));
  labs[11].setToolTip(get_city_dialog_growth_value(pcity));
}

/**
   city_label is used only for showing citizens icons
   and was created only to catch mouse events
 */
city_label::city_label(QWidget *parent) : QLabel(parent)
{
  type = FEELING_FINAL;
}

void city_label::set_type(int x) { type = x; }
/**
   Mouse handler for city_label
 */
void city_label::mousePressEvent(QMouseEvent *event)
{
  if (!pcity) {
    return;
  }

  if (cma_is_city_under_agent(pcity, nullptr)) {
    return;
  }

  if (!can_client_issue_orders()) {
    return;
  }

  int sprite_width = tileset_small_sprite_width(tileset);
  int sprite_height = tileset_small_sprite_height(tileset);

  int num_citizens = pcity->size;

  // if there's less than CITIZENS_PER_ROW citizens, we may have space on the
  // sides, so account for that
  int horizontal_spacing =
      (this->width() - MIN(CITIZENS_PER_ROW, num_citizens) * sprite_width)
      / 2;

  int citizen_x = (event->x() - horizontal_spacing) / sprite_width;
  int citizen_y = event->y() / sprite_height;

  int citizen_index = citizen_y * CITIZENS_PER_ROW + citizen_x;

  // users can click in empty space beyond the citizen sprites if there are
  // incomplete rows
  if (citizen_index < 0 || citizen_index > num_citizens - 1) {
    return;
  }

  city_rotate_specialist(pcity, citizen_index);
}

QSize city_label::get_pixmap_size() const
{
  auto pix = this->pixmap(Qt::ReturnByValue);
  if (pix.isNull()) {
    return {0, 0};
  } else {
    return pix.size();
  }
}

QSize city_label::minimumSizeHint() const { return this->get_pixmap_size(); }

QSize city_label::sizeHint() const { return this->get_pixmap_size(); }

/**
   Just sets target city for city_label
 */
void city_label::set_city(city *pciti) { pcity = pciti; }

city_info::city_info(QWidget *parent) : QWidget(parent)
{
  QGridLayout *info_grid_layout = new QGridLayout();
  info_grid_layout->setHorizontalSpacing(6);
  info_grid_layout->setVerticalSpacing(0);
  info_grid_layout->setContentsMargins(0, 0, 0, 0);
  info_grid_layout->setColumnStretch(1, 100);
  setLayout(info_grid_layout);

  auto small_font = fcFont::instance()->getFont(fonts::notify_label);

  // We use this to shorten the code below, as it would otherwise be very
  // repetitive. It creates a label for the description and another for the
  // value, and returns the second.
  const auto create_labels = [&](const char *title) {
    auto label = new QLabel(title, this);
    label->setFont(small_font);
    label->setProperty(fonts::notify_label, "true");
    info_grid_layout->addWidget(label, info_grid_layout->rowCount(), 0);

    auto value = new QLabel(this);
    value->setFont(small_font);
    value->setProperty(fonts::notify_label, "true");
    info_grid_layout->addWidget(value, info_grid_layout->rowCount() - 1, 1);
    return std::make_pair(label, value);
  };

  QLabel *dummy;
  std::tie(dummy, m_food) = create_labels(_("<b>Food:</b>"));
  std::tie(dummy, m_granary) = create_labels(_("Granary:"));
  std::tie(dummy, m_size) = create_labels(_("Citizens:"));
  std::tie(dummy, m_growth) = create_labels(_("Change in:"));

  info_grid_layout->addItem(new QSpacerItem(0, 9),
                            info_grid_layout->rowCount(), 0);

  std::tie(dummy, m_production) = create_labels(_("<b>Production:</b>"));
  std::tie(dummy, m_waste) = create_labels(_("Waste:"));

  info_grid_layout->addItem(new QSpacerItem(0, 9),
                            info_grid_layout->rowCount(), 0);

  std::tie(dummy, m_trade) = create_labels(_("<b>Trade:</b>"));
  std::tie(dummy, m_corruption) = create_labels(_("Corruption:"));
  std::tie(dummy, m_gold) = create_labels(_("Gold:"));
  std::tie(dummy, m_science) = create_labels(_("Science:"));
  std::tie(dummy, m_luxury) = create_labels(_("Luxury:"));

  info_grid_layout->addItem(new QSpacerItem(0, 9),
                            info_grid_layout->rowCount(), 0);

  std::tie(dummy, m_culture) = create_labels(_("Culture:"));
  std::tie(dummy, m_pollution) = create_labels(_("Pollution:"));
  std::tie(m_plague_label, m_plague) = create_labels(_("Plague risk:"));
  std::tie(dummy, m_stolen) = create_labels(_("Tech Stolen:"));
  std::tie(dummy, m_airlift) = create_labels(_("Airlift:"));
}

void city_info::update_labels(struct city *pcity)
{
  m_size->setText(
      QString::asprintf("%3d (%s)", pcity->size,
                        qUtf8Printable(get_city_dialog_status_text(pcity))));
  m_size->setToolTip(get_city_dialog_size_text(pcity));

  if (pcity->surplus[O_FOOD] < 0) {
    m_food->setText(QString::asprintf(
        "<b style=\"white-space:pre;\">%3d (<b "
        "style=\"color:red;\">%+4d</b>)</b>",
        pcity->prod[O_FOOD] + pcity->waste[O_FOOD], pcity->surplus[O_FOOD]));
    m_food->setToolTip(get_city_dialog_output_text(pcity, O_FOOD));
    m_growth->setText(get_city_dialog_growth_value(pcity));
    m_growth->setStyleSheet("QLabel { color:red; }");
  } else {
    m_food->setText(QString::asprintf(
        "<b style=\"white-space:pre;\">%3d (%+4d)</b>",
        pcity->prod[O_FOOD] + pcity->waste[O_FOOD], pcity->surplus[O_FOOD]));
    m_food->setToolTip(get_city_dialog_output_text(pcity, O_FOOD));
    m_growth->setText(get_city_dialog_growth_value(pcity));
    m_growth->setStyleSheet("QLabel { white-space:pre; }");
  }

  if (pcity->surplus[O_SHIELD] < 0) {
    m_production->setText(
        QString::asprintf("<b style=\"white-space:pre\">%3d (<b "
                          "style=\"color:red;\">%+4d</b>)</b>",
                          pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD],
                          pcity->surplus[O_SHIELD]));
    m_production->setToolTip(get_city_dialog_output_text(pcity, O_SHIELD));
  } else {
    m_production->setText(
        QString::asprintf("<b style=\"white-space:pre\">%3d (%+4d)</b>",
                          pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD],
                          pcity->surplus[O_SHIELD]));
    m_production->setToolTip(get_city_dialog_output_text(pcity, O_SHIELD));
  }

  if (pcity->surplus[O_TRADE] < 0) {
    m_trade->setText(
        QString::asprintf("<b style=\"white-space:pre\">%3d (<b "
                          "style=\"color:red;\">%+4d</b>)</b>",
                          pcity->prod[O_TRADE] + pcity->waste[O_TRADE],
                          pcity->surplus[O_TRADE]));
    m_trade->setToolTip(get_city_dialog_output_text(pcity, O_TRADE));
  } else {
    m_trade->setText(
        QString::asprintf("<b style=\"white-space:pre\">%3d (%+4d)</b>",
                          pcity->prod[O_TRADE] + pcity->waste[O_TRADE],
                          pcity->surplus[O_TRADE]));
    m_trade->setToolTip(get_city_dialog_output_text(pcity, O_TRADE));
  }

  if (pcity->surplus[O_GOLD] < 0) {
    m_gold->setText(QString::asprintf(
        "<span style=\"white-space:pre\">%3d (<b "
        "style=\"color:red;\">%+4d</b>)</span>",
        pcity->prod[O_GOLD] + pcity->waste[O_GOLD], pcity->surplus[O_GOLD]));
    m_gold->setToolTip(get_city_dialog_output_text(pcity, O_GOLD));
  } else {
    m_gold->setText(QString::asprintf(
        "<span style=\"white-space:pre\">%3d (%+4d)</span>",
        pcity->prod[O_GOLD] + pcity->waste[O_GOLD], pcity->surplus[O_GOLD]));
    m_gold->setToolTip(get_city_dialog_output_text(pcity, O_GOLD));
  }

  if (pcity->surplus[O_LUXURY] < 0) {
    m_luxury->setText(
        QString::asprintf("<span style=\"white-space:pre\">%3d (<b "
                          "style=\"color:red;\">%+4d</b>)</span>",
                          pcity->prod[O_LUXURY] + pcity->waste[O_LUXURY],
                          pcity->surplus[O_LUXURY]));
    m_luxury->setToolTip(get_city_dialog_output_text(pcity, O_LUXURY));
  } else {
    m_luxury->setText(QString::asprintf(
        "<span style=\"white-space:pre\">%3d (%+4d)</span>",
        pcity->prod[O_LUXURY] + pcity->waste[O_LUXURY],
        pcity->surplus[O_LUXURY]));
    m_luxury->setToolTip(get_city_dialog_output_text(pcity, O_LUXURY));
  }

  if (pcity->surplus[O_SCIENCE] < 0) {
    m_science->setText(
        QString::asprintf("<span style=\"white-space:pre\">%3d (<b "
                          "style=\"color:red;\">%+4d</b>)</span>",
                          pcity->prod[O_SCIENCE] + pcity->waste[O_SCIENCE],
                          pcity->surplus[O_SCIENCE]));
    m_science->setToolTip(get_city_dialog_output_text(pcity, O_SCIENCE));
  } else {
    m_science->setText(QString::asprintf(
        "<span style=\"white-space:pre\">%3d (%+4d)</span>",
        pcity->prod[O_SCIENCE] + pcity->waste[O_SCIENCE],
        pcity->surplus[O_SCIENCE]));
    m_science->setToolTip(get_city_dialog_output_text(pcity, O_SCIENCE));
  }

  m_granary->setText(
      QString::asprintf("%3d/%-4d", pcity->food_stock,
                        city_granary_size(city_size_get(pcity))));

  m_corruption->setText(QString::asprintf("%3d", pcity->waste[O_TRADE]));

  m_waste->setText(QString::asprintf("%3d", pcity->waste[O_SHIELD]));

  m_culture->setText(QString::asprintf("%3d", pcity->client.culture));
  m_culture->setToolTip(get_city_dialog_culture_text(pcity));

  m_pollution->setText(QString::asprintf("%3d", pcity->pollution));
  m_pollution->setToolTip(get_city_dialog_pollution_text(pcity));

  if (game.info.illness_on) {
    auto illness =
        city_illness_calc(pcity, nullptr, nullptr, nullptr, nullptr);
    // illness is in tenth of percent
    m_plague->setText(
        QString::asprintf("%3.1f%%", static_cast<float>(illness) / 10.0));
    m_plague->setToolTip(get_city_dialog_illness_text(pcity));
  }
  m_plague_label->setVisible(game.info.illness_on);
  m_plague->setVisible(game.info.illness_on);

  if (pcity->steal > 0) {
    m_stolen->setText(QString::asprintf(
        PL_("%3d time", "%3d times", pcity->steal), pcity->steal));
  } else {
    m_stolen->setText(_("Not stolen"));
  }

  m_airlift->setText(get_city_dialog_airlift_value(pcity));
  m_airlift->setToolTip(get_city_dialog_airlift_text(pcity));
}

/**
   Constructor for city_dialog, sets layouts, policies ...
 */
city_dialog::city_dialog(QWidget *parent) : QWidget(parent)

{
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  QHeaderView *header;

  int h = 2 * fm.height() + 2;
  auto small_font = fcFont::instance()->getFont(fonts::notify_label);
  ui.setupUi(this);

  // Prevent mouse events from going through the panels to the main map
  for (auto child : findChildren<QWidget *>()) {
    child->setAttribute(Qt::WA_NoMousePropagation);
  }

  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  setMouseTracking(true);
  selected_row_p = -1;
  pcity = nullptr;

  // main tab
  ui.lcity_name->setToolTip(_("Click to change city name"));
  ui.buy_button->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("help-donate")));
  connect(ui.buy_button, &QAbstractButton::clicked, this, &city_dialog::buy);
  connect(ui.lcity_name, &QAbstractButton::clicked, this,
          &city_dialog::city_rename);
  citizen_pixmap = nullptr;
  ui.bclose->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("city-close")));
  ui.bclose->setToolTip(_("Close city dialog"));
  connect(ui.bclose, &QAbstractButton::clicked, this, &QWidget::hide);
  ui.next_city_but->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("city-right")));
  ui.next_city_but->setToolTip(_("Show next city"));
  connect(ui.next_city_but, &QAbstractButton::clicked, this,
          &city_dialog::next_city);
  connect(ui.prev_city_but, &QAbstractButton::clicked, this,
          &city_dialog::prev_city);
  ui.prev_city_but->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("city-left")));
  ui.prev_city_but->setToolTip(_("Show previous city"));
  ui.work_next_but->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("go-down")));
  ui.work_prev_but->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("go-up")));
  ui.work_add_but->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("list-add")));
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
  connect(ui.p_table_p->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &city_dialog::item_selected);
  connect(ui.present_units_exp_col_but, &QAbstractButton::clicked, this,
          &city_dialog::present_units_exp_col);

  // governor tab
  ui.cma_table->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  connect(ui.cma_table->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &city_dialog::cma_selected);
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
  ui.label1->setFont(small_font);
  ui.label2->setFont(small_font);
  ui.label4->setFont(small_font);
  ui.label3->setFont(small_font);
  ui.label5->setFont(small_font);
  ui.label6->setFont(small_font);
  lab_table[0] = ui.lab_table1;
  lab_table[1] = ui.lab_table2;
  lab_table[2] = ui.lab_table3;
  lab_table[3] = ui.lab_table4;
  lab_table[4] = ui.lab_table5;
  lab_table[5] = ui.lab_table6;
  for (int x = 0; x < 6; x++) {
    lab_table[5]->set_type(x);
  }

  ui.tabs_right->setTabText(0, _("General"));
  ui.tabs_right->setTabText(1, _("Citizens"));
  ui.tabs_right->setTabText(2, _("Governor"));

  connect(ui.governor, &freeciv::governor_widget::parameters_changed, this,
          &city_dialog::cma_check_agent);

  ui.present_units_list->set_oneliner(true);

  installEventFilter(this);
}

/**
   Changes production to next one or previous
 */
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

/**
   Updates buttons/widgets which should be enabled/disabled
 */
void city_dialog::update_disabled()
{
  const auto can_edit =
      can_client_issue_orders() && city_owner(pcity) == client.conn.playing;
  ui.prev_city_but->setEnabled(can_edit);
  ui.next_city_but->setEnabled(can_edit);
  ui.buy_button->setEnabled(can_edit);
  ui.cma_enable_but->setEnabled(can_edit);
  ui.production_combo_p->setEnabled(can_edit);
  update_prod_buttons();
}

/**
   Update sensitivity of buttons in production tab
 */
void city_dialog::update_prod_buttons()
{
  ui.work_next_but->setEnabled(false);
  ui.work_prev_but->setEnabled(false);
  ui.work_add_but->setEnabled(false);
  ui.work_rem_but->setEnabled(false);

  if (can_client_issue_orders()
      && city_owner(pcity) == client.conn.playing) {
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

/**
   City dialog destructor
 */
city_dialog::~city_dialog()
{
  delete citizen_pixmap;
  removeEventFilter(this);
}

/**
   Hide event
 */
void city_dialog::hideEvent(QHideEvent *event)
{
  if (event->spontaneous()) {
    return;
  }

  if (pcity) {
    if (!dont_focus) {
      unit_focus_update();
    }
    update_map_canvas_visible();
    pcity = nullptr;
  }
  if (king() && queen()) {
    queen()->mapview_wdg->show_all_fcwidgets();
    king()->menu_bar->minimap_status->setEnabled(true);
  }
}

/**
   Show event
 */
void city_dialog::showEvent(QShowEvent *event)
{
  if (event->spontaneous()) {
    return;
  }

  dont_focus = false;
  if (pcity) {
    unit_focus_set(nullptr);
    update_map_canvas_visible();
    king()->menu_bar->minimap_status->setEnabled(false);
  }
}

/**
   Event filter for catching keybaord events
 */
bool city_dialog::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == this) {
    if (event->type() == QEvent::ShortcutOverride) {
      QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
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

/**
   City rename dialog input
 */
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

/**
   Save cma dialog input
 */
void city_dialog::save_cma()
{
  hud_input_box *ask = new hud_input_box(king()->central_wdg);

  ask->set_text_title_definput(_("What should we name the preset?"),
                               _("Name new preset"), _("new preset"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, this, [=]() {
    auto name = ask->input_edit.text();
    if (!name.isEmpty()) {
      const auto params = ui.governor->parameters();
      cmafec_preset_add(qUtf8Printable(name), &params);
      update_cma_tab();
    }
  });
  ask->show();
}

/**
   Enables cma slot, triggered by clicked button or changed cma
 */
void city_dialog::cma_enable()
{
  if (cma_is_city_under_agent(pcity, nullptr)) {
    cma_release_city(pcity);
    return;
  }

  cma_changed();
  update_cma_tab();
}

/**
   Sliders moved and cma has been changed
 */
void city_dialog::cma_changed()
{
  const auto params = ui.governor->parameters();
  cma_put_city_under_agent(pcity, &params);
}

/**
   Double click on some row ( column is unused )
 */
void city_dialog::cma_double_clicked(int row, int column)
{
  Q_UNUSED(column)
  const struct cm_parameter *param;

  if (!can_client_issue_orders()) {
    return;
  }
  param = cmafec_preset_get_parameter(row);

  cma_put_city_under_agent(pcity, param);
  if (cma_is_city_under_agent(pcity, nullptr)) {
    update_cma_tab();
  }
}

/**
   CMA has been selected from list
 */
void city_dialog::cma_selected(const QItemSelection &sl,
                               const QItemSelection &ds)
{
  Q_UNUSED(ds)
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

  if (cma_is_city_under_agent(pcity, nullptr)) {
    cma_put_city_under_agent(pcity, param);
  }

  if (cma_is_city_under_agent(pcity, nullptr)) {
    update_cma_tab();
  } else {
    update_sliders();
  }
}

/**
   Updates cma tab
 */
void city_dialog::update_cma_tab()
{
  QString s;
  QTableWidgetItem *item;
  struct cm_parameter param;
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

  if (cma_is_city_under_agent(pcity, nullptr)) {
    // view->update(); sveinung - update map here ?
    s = QString(cmafec_get_short_descr_of_city(pcity));
    auto icon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    ui.cma_result_pix->setPixmap(icon.pixmap(32));
    // TRANS: %1 is custom string chosen by player
    ui.cma_result->setText(QString(_("<h3>Governor Enabled<br>(%1)</h3>"))
                               .arg(s.toHtmlEscaped()));
  } else {
    auto icon = style()->standardIcon(QStyle::SP_DialogCancelButton);
    ui.cma_result_pix->setPixmap(icon.pixmap(32));
    ui.cma_result->setText(QString(_("<h3>Governor Disabled</h3>")));
  }
  ui.cma_result->setAlignment(Qt::AlignCenter);

  if (cma_is_city_under_agent(pcity, nullptr)) {
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

/**
   Removes selected CMA
 */
void city_dialog::cma_remove()
{
  int i;
  hud_message_box *ask;

  i = ui.cma_table->currentRow();

  if (i == -1 || cmafec_preset_num() == 0) {
    return;
  }

  ask = new hud_message_box(this);
  ask->set_text_title(_("Remove this preset?"), cmafec_preset_get_descr(i));
  ask->setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::No);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, this, [=]() {
    cmafec_preset_remove(i);
    update_cma_tab();
  });
  ask->show();
}

/**
   Received signal about changed qcheckbox - allow disbanding city
 */
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

/**
   Context menu on governor tab in city worklist
 */
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

  cma_menu->popup(QCursor::pos(queen()->screen()));
}

/**
   Context menu on production tab in city worklist
 */
void city_dialog::display_worklist_menu(const QPoint)
{
  QAction *action, *disband, *wl_save, *wl_clear, *wl_empty,
      *submenu_buildings, *submenu_futures, *submenu_units, *submenu_wonders;
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
  submenu_units = options_menu->addAction(_("Show units"));
  submenu_buildings = options_menu->addAction(_("Show buildings"));
  submenu_wonders = options_menu->addAction(_("Show wonders"));
  submenu_futures = options_menu->addAction(_("Show future targets"));
  submenu_futures->setCheckable(true);
  submenu_wonders->setCheckable(true);
  submenu_buildings->setCheckable(true);
  submenu_units->setCheckable(true);
  submenu_futures->setChecked(future_targets);
  connect(submenu_futures, &QAction::triggered, this,
          [=]() { future_targets = !future_targets; });
  submenu_units->setChecked(show_units);
  connect(submenu_units, &QAction::triggered, this,
          [=]() { show_units = !show_units; });
  submenu_buildings->setChecked(show_buildings);
  connect(submenu_buildings, &QAction::triggered, this,
          [=]() { show_buildings = !show_buildings; });
  submenu_wonders->setChecked(show_wonders);
  connect(submenu_wonders, &QAction::triggered, this,
          [=]() { show_wonders = !show_wonders; });
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

  list_menu->popup(QCursor::pos(queen()->screen()));
}

/**
   Enables/disables buy buttons depending on gold
 */
void city_dialog::update_buy_button()
{
  QString str;
  int value;

  ui.buy_button->setEnabled(false);

  if (!client_is_observer() && client.conn.playing != nullptr) {
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

/**
   Fill a pixmap with citizen sprites
 */
void city_dialog::fill_citizens_pixmap(QPixmap *pixmap, QPainter *painter,
                                       const citizen_category *categories,
                                       int num_citizens)
{
  int sprite_width = tileset_small_sprite_width(tileset);
  int sprite_height = tileset_small_sprite_height(tileset);

  QRect sprite_rectangle(0, 0, sprite_width, sprite_height);
  QRect painting_area(0, 0, sprite_width, sprite_height);

  pixmap->fill(Qt::transparent);
  for (int i = 0; i < num_citizens; i++) {
    painting_area.moveTo((i % CITIZENS_PER_ROW) * sprite_width,
                         (i / CITIZENS_PER_ROW) * sprite_height);
    auto pix = get_citizen_sprite(tileset, categories[i], i, pcity);
    painter->begin(citizen_pixmap);
    painter->drawPixmap(painting_area, *pix, sprite_rectangle);
    painter->end();
  }
}

/**
   Redraws citizens for city_label (citizens_label)
 */
void city_dialog::update_citizens()
{
  enum citizen_category categories[MAX_CITY_SIZE];
  int num_citizens =
      get_city_citizen_types(pcity, FEELING_FINAL, categories);

  int num_rows = num_citizens / CITIZENS_PER_ROW;

  // extra incomplete row for leftover citizens
  if (num_citizens % CITIZENS_PER_ROW > 0) {
    num_rows += 1;
  }

  int sprite_width = tileset_small_sprite_width(tileset);
  int sprite_height = tileset_small_sprite_height(tileset);

  int canvas_width = sprite_width * MIN(CITIZENS_PER_ROW, num_citizens);
  int canvas_height = sprite_height * num_rows;

  if (citizen_pixmap) {
    citizen_pixmap->detach();
    delete citizen_pixmap;
  }

  citizen_pixmap = new QPixmap(canvas_width, canvas_height);
  QPainter painter;

  this->fill_citizens_pixmap(citizen_pixmap, &painter, categories,
                             num_citizens);

  ui.citizens_label->set_city(pcity);
  ui.citizens_label->setPixmap(*citizen_pixmap);
  ui.citizens_label->updateGeometry();

  lab_table[FEELING_FINAL]->setPixmap(*citizen_pixmap);
  lab_table[FEELING_FINAL]->updateGeometry();
  lab_table[FEELING_FINAL]->setToolTip(text_happiness_wonders(pcity));

  for (int i = 0; i < FEELING_LAST - 1; i++) {
    lab_table[i]->set_city(pcity);
    num_citizens = get_city_citizen_types(
        pcity, static_cast<citizen_feeling>(i), categories);
    this->fill_citizens_pixmap(citizen_pixmap, &painter, categories,
                               num_citizens);

    lab_table[i]->setPixmap(*citizen_pixmap);
    lab_table[i]->updateGeometry();
  }

  lab_table[FEELING_BASE]->setToolTip(text_happiness_cities(pcity));
  lab_table[FEELING_LUXURY]->setToolTip(text_happiness_luxuries(pcity));
  lab_table[FEELING_EFFECT]->setToolTip(text_happiness_buildings(pcity));
  lab_table[FEELING_NATIONALITY]->setToolTip(
      text_happiness_nationality(pcity));
  lab_table[FEELING_MARTIAL]->setToolTip(text_happiness_units(pcity));
}

/**
   Various refresh after getting new info/reply from server
 */
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
    ui.icon->set_city(pcity->id);
    ui.upkeep->set_city(pcity->id);
  } else {
    popdown_city_dialog();
    ui.icon->set_city(-1);
    ui.upkeep->set_city(-1);
  }

  ui.production_combo_p->blockSignals(false);
  setUpdatesEnabled(true);

  auto scale = queen()->mapview_wdg->scale();
  ui.middleSpacer->changeSize(scale * get_citydlg_canvas_width(),
                              scale * get_citydlg_canvas_height(),
                              QSizePolicy::Expanding,
                              QSizePolicy::Expanding);

  updateGeometry();
  update();
}

void city_dialog::update_sliders()
{
  cm_parameter params;
  if (!cma_is_city_under_agent(pcity, &params)) {
    if (ui.cma_table->currentRow() == -1 || cmafec_preset_num() == 0) {
      return;
    }
    params = *cmafec_preset_get_parameter(ui.cma_table->currentRow());
  }

  ui.governor->set_parameters(params);
}

/**
   Updates nationality table in happiness tab
 */
void city_dialog::update_nation_table()
{
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  QString str;
  QStringList info_list;
  QTableWidgetItem *item;
  char buf[8];
  citizens nationality_i;
  int h;
  int i = 0;

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
          str = QStringLiteral("-");
        } else {
          fc_snprintf(buf, sizeof(buf), "%d", nationality_i);
          str = QString(buf);
        }

        item->setText(str);
        break;

      case 1: {
        auto sprite = get_nation_flag_sprite(
            tileset, nation_of_player(player_slot_get_player(pslot)));

        if (sprite != nullptr) {
          item->setData(Qt::DecorationRole, sprite->scaledToHeight(h));
        } else {
          item->setText(QStringLiteral("FLAG MISSING"));
        }
      } break;

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

/**
   Updates information label ( food, prod ... surpluses ...)
 */
void city_dialog::update_info_label()
{
  ui.info_wdg->update_labels(pcity);
  ui.info_icon_label->setCity(pcity);
  ui.info_icon_label->updateText();
}

/**
   Setups whole city dialog, public function
 */
void city_dialog::setup_ui(struct city *qcity)
{
  if (pcity != qcity) {
    if (pcity) {
      refresh_city_mapcanvas(pcity, pcity->tile, true);
    }
    if (qcity) {
      refresh_city_mapcanvas(qcity, qcity->tile, true);
    }
  }

  pcity = qcity;
  ui.production_combo_p->blockSignals(true);
  refresh();
  ui.production_combo_p->blockSignals(false);
}

/**
   Double clicked item in worklist table in production tab
 */
void city_dialog::dbl_click_p(QTableWidgetItem *item)
{
  Q_UNUSED(item)
  struct worklist queue;
  city_get_queue(pcity, &queue);

  if (selected_row_p < 0 || selected_row_p > worklist_length(&queue)) {
    return;
  }

  worklist_remove(&queue, selected_row_p);
  city_set_queue(pcity, &queue);
}

/**
   Updates layouts for supported and present units in city
 */
void city_dialog::update_units()
{
  struct unit_list *units;
  char buf[256];
  int n;

  if (nullptr != client.conn.playing
      && city_owner(pcity) != client.conn.playing) {
    units = pcity->client.info_units_supported;
  } else {
    units = pcity->units_supported;
  }

  n = unit_list_size(units);
  fc_snprintf(buf, sizeof(buf), _("Supported units: %d"), n);
  ui.supp_units->setText(QString(buf));

  if (nullptr != client.conn.playing
      && city_owner(pcity) != client.conn.playing) {
    units = pcity->client.info_units_present;
  } else {
    units = pcity->tile->units;
  }

  // set direction of the expand/collapse arrow
  if (present_units_exp) {
    ui.present_units_exp_col_but->setArrowType(Qt::DownArrow);
  } else {
    ui.present_units_exp_col_but->setArrowType(Qt::UpArrow);
  }

  n = unit_list_size(units);
  ui.present_units_list->setLayoutDirection(Qt::LeftToRight);
  ui.present_units_list->set_units(units);
  ui.present_units_list->setVisible(n >= 0);
  fc_snprintf(buf, sizeof(buf), _("Present units: %d"), n);
  ui.present_units_label->setText(QString(buf));
}

/**
 * Slot to expand or collapse the present units list
 */
void city_dialog::present_units_exp_col()
{
  if (present_units_exp) {
    ui.present_units_group_box->setSizePolicy(QSizePolicy::MinimumExpanding,
                                              QSizePolicy::Minimum);
    ui.present_units_exp_col_but->setArrowType(Qt::UpArrow);
    ui.present_units_exp_col_but->setToolTip(
        _("Click to expand the present units list"));
    present_units_exp = false;
  } else {
    ui.present_units_group_box->setSizePolicy(QSizePolicy::MinimumExpanding,
                                              QSizePolicy::Expanding);
    ui.present_units_exp_col_but->setArrowType(Qt::DownArrow);
    ui.present_units_exp_col_but->setToolTip(
        _("Click to collapse the present units list"));
    present_units_exp = true;
  }
}

/**
   Selection changed in production tab, in worklist tab
 */
void city_dialog::item_selected(const QItemSelection &sl,
                                const QItemSelection &ds)
{
  Q_UNUSED(ds)
  QModelIndex index;
  QModelIndexList indexes = sl.indexes();

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  selected_row_p = index.row();
  update_prod_buttons();
}

void city_dialog::get_city(bool next)
{
  int size, i, j;
  struct city *other_pcity = nullptr;

  if (nullptr == client.conn.playing) {
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
    int k = next ? j : -j;
    other_pcity =
        city_list_get(client.conn.playing->cities, (i + k + size) % size);
  }
  queen()->mapview_wdg->center_on_tile(other_pcity->tile);
  setup_ui(other_pcity);
}

/**
   Changes city_dialog to next city after pushing next city button
 */
void city_dialog::next_city() { get_city(true); }

/**
   Changes city_dialog to previous city after pushing prev city button
 */
void city_dialog::prev_city() { get_city(false); }

/**
   Updates building improvement/unit
 */
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
      QStringLiteral("(%p%) %2\n%1")
          .arg(city_production_name_translation(pcity), str));

  ui.production_combo_p->updateGeometry();
}

/**
   Buy button. Shows message box asking for confirmation
 */
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

  ask = new hud_message_box(queen()->city_overlay);
  fc_snprintf(buf2, ARRAY_SIZE(buf2),
              PL_("Treasury contains %d gold.", "Treasury contains %d gold.",
                  client_player()->economic.gold),
              client_player()->economic.gold);
  fc_snprintf(buf, ARRAY_SIZE(buf),
              PL_("Buy %s for %d gold?", "Buy %s for %d gold?", value), name,
              value);
  ask->set_text_title(buf, buf2);
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Buy"));
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

/**
   Updates list of improvements
 */
void city_dialog::update_improvements()
{
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  QString str, tooltip;
  QTableWidgetItem *qitem;
  const QPixmap *sprite = nullptr;
  int h, cost, item, targets_used, col, upkeep;
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct worklist queue;

  cost = 0;
  upkeep = 0;

  h = fm.height() + 6;
  targets_used = collect_already_built_targets(targets, pcity);
  name_and_sort_items(targets, targets_used, items, false, pcity);

  for (item = 0; item < targets_used; item++) {
    struct universal target = items[item].item;
    fc_assert_action(VUT_IMPROVEMENT == target.kind, continue);
    upkeep += city_improvement_upkeep(pcity, target.value.building);
  }

  city_get_queue(pcity, &queue);
  ui.p_table_p->setRowCount(worklist_length(&queue));

  for (int i = 0; i < worklist_length(&queue); i++) {
    struct universal target = queue.entries[i];

    tooltip = QLatin1String("");

    if (VUT_UTYPE == target.kind) {
      str = utype_values_translation(target.value.utype);
      cost = utype_build_shield_cost(pcity, target.value.utype);
      tooltip = get_tooltip_unit(target.value.utype, true).trimmed();
      sprite = get_unittype_sprite(get_tileset(), target.value.utype,
                                   direction8_invalid());
    } else if (target.kind == VUT_IMPROVEMENT) {
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
          qitem->setData(Qt::DecorationRole, sprite->scaledToHeight(h));
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

  ui.upkeep->refresh();

  ui.curr_impr->setText(QString(_("Improvements: upkeep %1")).arg(upkeep));
}

/**
   Shows customized table widget with available items to produce
   Shows default targets in overview city page
 */
void city_dialog::show_targets()
{
  production_widget *pw;
  int when = 1;
  pw = new production_widget(this, pcity, future_targets, when,
                             selected_row_p, show_units, false, show_wonders,
                             show_buildings);
  pw->show();
}

/**
   Shows customized table widget with available items to produce
   Shows customized targets in city production page
 */
void city_dialog::show_targets_worklist()
{
  production_widget *pw;
  int when = 4;
  pw = new production_widget(this, pcity, future_targets, when,
                             selected_row_p, show_units, false, show_wonders,
                             show_buildings);
  pw->show();
}

/**
   Clears worklist in production page
 */
void city_dialog::clear_worklist()
{
  struct worklist empty;

  if (!can_client_issue_orders()) {
    return;
  }

  worklist_init(&empty);
  city_set_worklist(pcity, &empty);
}

/**
   Move current item on worklist up
 */
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

/**
   Remove current item on worklist
 */
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

/**
   Move current item on worklist down
 */
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

/**
   Save worklist
 */
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

/**
 * Triggers a governor update if the parameters changed.
 */
void city_dialog::cma_check_agent(const cm_parameter &params)
{
  cm_parameter current;
  if (cma_is_city_under_agent(pcity, &current) && !(current == params)) {
    cma_changed();
    update_cma_tab();
  }
}

/**
   Puts city name and people count on title
 */
void city_dialog::update_title()
{
  QString buf;

  // Defeat keyboard shortcut mnemonics
  ui.lcity_name->setText(
      QString(city_name_get(pcity))
          .replace(QLatin1String("&"), QLatin1String("&&")));

  if (city_unhappy(pcity)) {
    // TRANS: city dialog title
    buf = QString(_("%1 - %2 citizens - DISORDER"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  } else if (city_celebrating(pcity)) {
    // TRANS: city dialog title
    buf = QString(_("%1 - %2 citizens - celebrating"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  } else if (city_happy(pcity)) {
    // TRANS: city dialog title
    buf = QString(_("%1 - %2 citizens - happy"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  } else {
    // TRANS: city dialog title
    buf = QString(_("%1 - %2 citizens"))
              .arg(city_name_get(pcity),
                   population_to_text(city_population(pcity)));
  }

  setWindowTitle(buf);
}

/**
   Pop up (or bring to the front) a dialog for the given city.  It may or
   may not be modal.
 */
void real_city_dialog_popup(struct city *pcity)
{
  // TODO: Change this to a question when the map editor is more feature
  // complete.
  if (queen()->map_editor_wdg->isVisible()) {
    queen()->map_editor_wdg->hide();
  }

  auto *widget = queen()->city_overlay;
  if (!queen()->city_overlay->isVisible()) {
    top_bar_show_map();
    queen()->mapview_wdg->hide_all_fcwidgets();
  }
  queen()->mapview_wdg->center_on_tile(pcity->tile);

  widget->setup_ui(pcity);
  widget->show();
  widget->resize(queen()->mapview_wdg->size());
}

/**
 * Closes the city overlay.
 */
void popdown_city_dialog()
{
  if (queen()) {
    queen()->city_overlay->hide();
  }
}

/**
   Refresh (update) all data for the given city's dialog.
 */
void real_city_dialog_refresh(struct city *pcity)
{
  if (city_dialog_is_open(pcity)) {
    queen()->city_overlay->refresh();
  }
}

/**
   Updates city font
 */
void city_font_update()
{
  QList<QLabel *> l;

  l = queen()->city_overlay->findChildren<QLabel *>();

  auto f = fcFont::instance()->getFont(fonts::notify_label);

  for (auto i : std::as_const(l)) {
    if (i->property(fonts::notify_label).isValid()) {
      i->setFont(f);
    }
  }
}

/**
   Update city dialogs when the given unit's status changes.  This
   typically means updating both the unit's home city (if any) and the
   city in which it is present (if any).
 */
void refresh_unit_city_dialogs(struct unit *punit)
{
  struct city *pcity_sup, *pcity_pre;

  pcity_sup = game_city_by_number(punit->homecity);
  pcity_pre = tile_city(punit->tile);

  real_city_dialog_refresh(pcity_sup);
  real_city_dialog_refresh(pcity_pre);
}

struct city *is_any_city_dialog_open()
{
  // some checks not to iterate cities
  if (!queen()->city_overlay->isVisible()) {
    return nullptr;
  }
  if (client_is_global_observer() || client_is_observer()) {
    return nullptr;
  }

  city_list_iterate(client.conn.playing->cities, pcity)
  {
    if (city_dialog_is_open(pcity)) {
      return pcity;
    }
  }
  city_list_iterate_end;
  return nullptr;
}

/**
   Return whether the dialog for the given city is open.
 */
bool city_dialog_is_open(struct city *pcity)
{
  return queen()->city_overlay->pcity == pcity
         && queen()->city_overlay->isVisible();
}

/**
   City item delegate constructor
 */
city_production_delegate::city_production_delegate(QPoint sh,
                                                   QObject *parent,
                                                   struct city *city)
    : QItemDelegate(parent), pd(sh)
{
  item_height = sh.y();
  pcity = city;
}

/**
   City item delgate paint event
 */
void city_production_delegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
  struct universal *target;
  QString name;
  QVariant qvar;
  QPixmap pix_scaled;
  QRect rect1;
  QRect rect2;
  const QPixmap *sprite;
  QPixmap *free_sprite = nullptr;
  bool useless = false;
  bool is_coinage = false;
  bool is_neutral = false;
  bool is_sea = false;
  bool is_flying = false;
  bool is_unit = true;
  QPixmap pix_dec(option.rect.width(), option.rect.height());
  QStyleOptionViewItem opt;
  QIcon icon = qApp->style()->standardIcon(QStyle::SP_DialogCancelButton);
  struct unit_class *pclass;

  if (!option.rect.isValid()) {
    return;
  }

  qvar = index.data();

  if (!qvar.isValid()) {
    return;
  }

  target = reinterpret_cast<universal *>(qvar.value<void *>());

  if (target == nullptr) {
    free_sprite = new QPixmap;
    *free_sprite = icon.pixmap(100, 100);
    sprite = free_sprite;
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

  if (sprite != nullptr) {
    pix_scaled =
        sprite->scaledToHeight(item_height - 2, Qt::SmoothTransformation);

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
}

/**
   Draws focus for given item
 */
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

/**
   Size hint for city item delegate
 */
QSize city_production_delegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
  Q_UNUSED(option)
  Q_UNUSED(index)
  QSize s;

  s.setWidth(pd.x());
  s.setHeight(pd.y());
  return s;
}

/**
   Production item constructor
 */
production_item::production_item(struct universal *ptarget, QObject *parent)
    : QObject()
{
  setParent(parent);
  target = ptarget;
}

/**
   Production item destructor
 */
production_item::~production_item()
{
  // allocated as renegade in model
  delete target;
}

/**
   Returns stored data
 */
QVariant production_item::data() const
{
  return QVariant::fromValue((void *) target);
}

/**
   Constructor for city production model
 */
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

/**
   Destructor for city production model
 */
city_production_model::~city_production_model()
{
  qDeleteAll(city_target_list);
  city_target_list.clear();
}

/**
   Returns data from model
 */
QVariant city_production_model::data(const QModelIndex &index,
                                     int role) const
{
  if (!index.isValid()) {
    return QVariant();
  }

  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount()
      && (index.column() + index.row() * 3 < city_target_list.count())) {
    int r, c, t, new_index;
    r = index.row();
    c = index.column();
    t = r * 3 + c;
    new_index = t / 3 + rowCount() * c;
    // Exception, shift whole column
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

/**
   Fills model with data
 */
void city_production_model::populate()
{
  production_item *pi;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  struct universal *renegade, *renegate;
  int item, targets_used;
  QString str;
  auto f = fcFont::instance()->getFont(fonts::default_font);
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

      // renagade deleted in production_item destructor
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

  renegate = nullptr;
  pi = new production_item(renegate, this);
  city_target_list << pi;
  sh.setX(2 * sh.y() + sh.x());
  sh.setX(qMin(sh.x(), 250));
}

/**
   Constructor for production widget
   future - show future targets
   show_units - if to show units
   when - where to insert
   curr - current index to insert
   buy - buy if possible
 */
production_widget::production_widget(QWidget *parent, struct city *pcity,
                                     bool future, int when, int curr,
                                     bool show_units, bool buy,
                                     bool show_wonders, bool show_buildings)
    : QTableView()
{
  Q_UNUSED(parent)
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
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &production_widget::prod_selected);
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

  pos = QCursor::pos(queen()->screen());

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

/**
   Mouse press event for production widget
 */
void production_widget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    close();
    return;
  }

  QAbstractItemView::mousePressEvent(event);
}

/**
   Event filter for production widget
 */
bool production_widget::eventFilter(QObject *obj, QEvent *ev)
{
  QRect pw_rect;
  QPoint br;

  if (obj != this) {
    return false;
  }

  if (ev->type() == QEvent::MouseButtonPress) {
    pw_rect.setTopLeft(pos());
    br.setX(pos().x() + width());
    br.setY(pos().y() + height());
    pw_rect.setBottomRight(br);

    if (!pw_rect.contains(QCursor::pos(queen()->screen()))) {
      close();
      return true;
    }
  } else if (ev->type() == QEvent::KeyRelease) {
    auto key_event = dynamic_cast<QKeyEvent *>(ev);
    if (key_event && key_event->key() == Qt::Key_Escape) {
      close();
      return true;
    }
  }

  return false;
}

/**
   Changed selection in production widget
 */
void production_widget::prod_selected(const QItemSelection &sl,
                                      const QItemSelection &ds)
{
  Q_UNUSED(ds)
  Q_UNUSED(sl)
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
  if (target != nullptr) {
    city_get_queue(pw_city, &queue);
    switch (when_change) {
    case 0: // Change current target
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

    case 2: // Insert before
      if (curr_selection < 0 || curr_selection > worklist_length(&queue)) {
        curr_selection = 0;
      }
      curr_selection--;
      curr_selection = qMax(0, curr_selection);
      worklist_insert(&queue, target, curr_selection);
      city_set_queue(pw_city, &queue);
      break;

    case 3: // Insert after
      if (curr_selection < 0 || curr_selection > worklist_length(&queue)) {
        city_queue_insert(pw_city, -1, target);
        break;
      }
      curr_selection++;
      worklist_insert(&queue, target, curr_selection);
      city_set_queue(pw_city, &queue);
      break;

    case 4: // Add last
      city_queue_insert(pw_city, -1, target);
      break;

    default:
      break;
    }
  }
  close();
  destroy();
}

/**
   Destructor for production widget
 */
production_widget::~production_widget()
{
  delete c_p_d;
  delete list_model;
  viewport()->removeEventFilter(fc_tt);
  removeEventFilter(this);
}
