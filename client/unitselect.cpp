/*
  /\ ___ /\        Copyright (c) 1996-2023 ＦＲＥＥＣＩＶ ２１ and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                  If not, see https://www.gnu.org/licenses/.
 */

#include "unitselect.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWidgetAction>
#include <QtMath>
// common
#include "movement.h"
// client
#include "canvas.h"
#include "client_main.h"
#include "control.h"
#include "fonts.h"
#include "page_game.h"
#include "tileset/tilespec.h"
#include "utils/unit_utils.h"
#include "views/view_map.h"
#include "views/view_map_common.h"

/**
   Contructor for units_select
 */
units_select::units_select(struct tile *ptile, QWidget *parent)
    : QMenu(parent)
{
  m_widget = new units_select_widget(ptile, this);

  auto action = new QWidgetAction(this);
  action->setDefaultWidget(m_widget);
  addAction(action);

  popup(mapFromGlobal(QCursor::pos(queen()->screen())));
}

/**
   Updates unit list on tile
 */
void units_select::update_units() { m_widget->update_units(); }

/**
   Close event for units_select, restores focus to map
 */
void units_select::closeEvent(QCloseEvent *event)
{
  queen()->mapview_wdg->setFocus();
  QMenu::closeEvent(event);
}

/**
   Create pixmap of whole widget except borders (pix)
 */
void units_select::create_pixmap() { m_widget->create_pixmap(); }

/**
   Constructor for units_select_widget
 */
units_select_widget::units_select_widget(struct tile *ptile, QWidget *parent)
    : QWidget(parent)
{
  utile = ptile;
  pix = nullptr;
  show_line = 0;
  highligh_num = -1;
  ufont.setItalic(true);
  info_font = fcFont::instance()->getFont(fonts::notify_label);
  update_units();
  h_pix = nullptr;
  create_pixmap();
  setMouseTracking(true);
}

/**
   Destructor for units_select_widget
 */
units_select_widget::~units_select_widget()
{
  delete h_pix;
  delete pix;
}

/**
   Create pixmap of whole widget except borders (pix)
 */
void units_select_widget::create_pixmap()
{
  int a;
  int x, y, i;
  QFontMetrics fm(info_font);
  QImage cropped_img;
  QImage img;
  QList<QPixmap *> pix_list;
  QPainter p;
  QPen pen;
  QPixmap pixc;
  QPixmap *pixp;
  QPixmap *tmp_pix;
  QRect crop;
  QPixmap *unit_pixmap;
  const unit *punit;
  float isosize;

  delete pix;
  isosize = 0.7;
  if (tileset_hex_height(tileset) > 0 || tileset_hex_width(tileset) > 0) {
    isosize = 0.5;
  }

  update_units();
  if (!unit_list.empty()) {
    if (!tileset_is_isometric(tileset)) {
      item_size.setWidth(tileset_unit_width(tileset));
      item_size.setHeight(tileset_unit_width(tileset));
    } else {
      item_size.setWidth(tileset_unit_width(tileset) * isosize);
      item_size.setHeight(tileset_unit_width(tileset) * isosize);
    }
    more = false;
    delete h_pix;
    h_pix = new QPixmap(item_size.width(), item_size.height());
    h_pix->fill(palette().color(QPalette::HighlightedText));

    // Determine the layout. 5 columns up to 25 units, then 6.
    column_count = qMin(unit_count, unit_count <= 25 ? 5 : 6);
    // Up to 6 rows visible at the same time.
    row_count = qMin((unit_count + column_count - 1) / column_count, 6);
    // And whether we go over.
    more = unit_count > row_count * column_count;

    pix = new QPixmap(column_count * item_size.width(),
                      row_count * item_size.height());
    pix->fill(Qt::transparent);
    for (auto *punit : std::as_const(unit_list)) {
      unit_pixmap = new QPixmap(tileset_unit_width(tileset),
                                tileset_unit_height(tileset));
      unit_pixmap->fill(Qt::transparent);
      put_unit(punit, unit_pixmap, QPoint());
      img = unit_pixmap->toImage();
      crop = zealous_crop_rect(img);
      cropped_img = img.copy(crop);
      if (!tileset_is_isometric(tileset)) {
        img = cropped_img.scaled(
            tileset_unit_width(tileset), tileset_unit_width(tileset),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
      } else {
        img = cropped_img.scaled(tileset_unit_width(tileset) * isosize,
                                 tileset_unit_width(tileset) * isosize,
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
      }
      pixc = QPixmap::fromImage(img);
      pixp = new QPixmap(pixc);
      pix_list.push_back(pixp);
      delete unit_pixmap;
    }
    a = qMin(item_size.width() / 4, 12);
    x = 0, y = -item_size.height(), i = -1;
    p.begin(pix);
    ufont.setPixelSize(a);
    p.setFont(ufont);
    pen.setColor(palette().color(QPalette::Text));
    p.setPen(pen);

    while (!pix_list.isEmpty()) {
      tmp_pix = pix_list.takeFirst();
      i++;
      if (i % column_count == 0) {
        x = 0;
        y = y + item_size.height();
      }
      punit = unit_list.at(i);
      Q_ASSERT(punit != nullptr);

      if (i == highligh_num) {
        p.drawPixmap(x, y, *h_pix);
        p.drawPixmap(x, y, *tmp_pix);
      } else {
        p.drawPixmap(x, y, *tmp_pix);
      }

      if (client_is_global_observer()
          || unit_owner(punit) == client.conn.playing) {
        auto str = QString(move_points_text(punit->moves_left, false));
        if (utype_fuel(unit_type_get(punit))) {
          // TRANS: T for turns
          str += " " + QString(_("(%1T)")).arg(punit->fuel - 1);
        }
        // TRANS: MP = Movement points
        str = QString(_("MP:")) + str;
        p.drawText(x, y + item_size.height() - 4, str);
      }

      x = x + item_size.width();
      delete tmp_pix;
    }
    p.end();
    setFixedWidth(pix->width() + 20);
    setFixedHeight(pix->height() + 3 * fm.height() + 2 * 6);
    qDeleteAll(pix_list.begin(), pix_list.end());
  }
}

/**
   Event for mouse moving around units_select_widget
 */
void units_select_widget::mouseMoveEvent(QMouseEvent *event)
{
  int a, b;
  int old_h;
  QFontMetrics fm(info_font);

  old_h = highligh_num;
  highligh_num = -1;
  auto pos = event->position();
  if (pos.x() > width() - 11 || pos.y() > height() - fm.height() - 5
      || pos.y() < fm.height() + 3 || pos.x() < 11) {
    /** do nothing if mouse is on border, just skip next if */
  } else if (row_count > 0) {
    a = (pos.x() - 10) / item_size.width();
    b = (pos.y() - fm.height() - 3) / item_size.height();
    highligh_num = b * column_count + a;
  }
  if (old_h != highligh_num) {
    create_pixmap();
    update();
  }
}

/**
   Mouse pressed event for units_select_widget.
   Left Button - chooses units
   Right Button - closes widget
 */
void units_select_widget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && highligh_num != -1) {
    update_units();
    if (highligh_num >= unit_list.size()) {
      return;
    }
    unit_focus_set(unit_list.at(highligh_num));
  }
  QWidget::mousePressEvent(event);
}

/**
   Redirected paint event
 */
void units_select_widget::paint(QPainter *painter, QPaintEvent *event)
{
  Q_UNUSED(event)
  QFontMetrics fm(info_font);
  int h, i;
  int *f_size;
  QPen pen;
  QString str, str2, unit_name;
  int point_size = info_font.pointSize();
  int pixel_size = info_font.pixelSize();

  if (point_size < 0) {
    f_size = &pixel_size;
  } else {
    f_size = &point_size;
  }
  if (highligh_num != -1 && highligh_num < unit_list.size()) {
    auto punit = unit_list.at(highligh_num);
    // TRANS: HP - hit points
    unit_name = unit_name_translation(punit);
    str2 = QString(_("%1 HP:%2/%3"))
               .arg(unit_activity_text(punit), QString::number(punit->hp),
                    QString::number(unit_type_get(punit)->hp));
  }
  str = QString(PL_("%1 unit", "%1 units", unit_list_size(utile->units)))
            .arg(unit_list_size(utile->units));
  for (i = *f_size; i > 4; i--) {
    if (point_size < 0) {
      info_font.setPixelSize(i);
    } else {
      info_font.setPointSize(i);
    }
    QFontMetrics qfm(info_font);
    if (10 + qfm.horizontalAdvance(str2) < width()) {
      break;
    }
  }
  h = fm.height();
  if (pix != nullptr) {
    painter->drawPixmap(10, h + 3, *pix);
    pen.setColor(palette().color(QPalette::Text));
    painter->setPen(pen);
    painter->setFont(info_font);
    painter->drawText(10, h, str);
    if (highligh_num != -1 && highligh_num < unit_list.size()) {
      painter->drawText(10, height() - 5 - h, unit_name);
      painter->drawText(10, height() - 5, str2);
    }
    // draw scroll
    if (more) {
      int maxl = ((unit_count - 1) / column_count) + 1;
      float page_height = 3.0f / maxl;
      float page_start = (static_cast<float>(show_line)) / maxl;
      pen.setColor(palette().color(QPalette::HighlightedText));
      painter->setBrush(palette().color(QPalette::HighlightedText).darker());
      painter->setPen(palette().color(QPalette::HighlightedText).darker());
      painter->drawRect(pix->width() + 10, h, 8, h + pix->height());
      painter->setPen(pen);
      painter->drawRoundedRect(pix->width() + 10,
                               h + page_start * pix->height(), 8,
                               h + page_height * pix->height(), 2, 2);
    }
  }
  if (point_size < 0) {
    info_font.setPixelSize(*f_size);
  } else {
    info_font.setPointSize(*f_size);
  }
}

/**
   Paint event, redirects to paint(...)
 */
void units_select_widget::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event); // Draw background

  QPainter painter;
  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Updates unit list on tile
 */
void units_select_widget::update_units()
{
  int i = 1;
  struct unit_list *punit_list;

  if (utile == nullptr) {
    return;
  }
  unit_count = 0;
  if (utile == nullptr) {
    struct unit *punit = head_of_units_in_focus();
    if (punit) {
      utile = unit_tile(punit);
    }
  }
  unit_list.clear();
  if (utile != nullptr) {
    punit_list = utile->units;
    if (punit_list != nullptr) {
      for (auto *punit : sorted(utile->units)) {
        unit_count++;
        if (i > show_line * column_count) {
          unit_list.push_back(punit);
        }
        i++;
      }
    }
  }
  if (unit_list.empty()) {
    close();
  }
}
/**
   Mouse wheel event for units_select_widget
 */
void units_select_widget::wheelEvent(QWheelEvent *event)
{
  if (!more && utile == nullptr) {
    return;
  }

  // The number of hidden lines. This is the number of rows needed to show
  // all units, minus what is shown without scrolling.
  auto nr = (unit_list_size(utile->units) + column_count - 1) / column_count
            - row_count;

  // We scroll one full row per scroll event. The angle delta determines the
  // direction in which we scroll. We don't scroll when it's 0.
  const auto delta = event->angleDelta().y();
  if (delta < 0) {
    show_line++;
    show_line = qMin(show_line, nr);
  } else if (delta > 0) {
    show_line--;
    show_line = qMax(0, show_line);
  }
  update_units();
  create_pixmap();
  update();
  event->accept();
}

/**
   Shows/closes unit selection widget
 */
void toggle_unit_sel_widget(struct tile *ptile)
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != nullptr) {
    unit_sel->close();
    delete unit_sel;
  }

  unit_sel = new units_select(ptile, queen()->mapview_wdg);
}

/**
   Update unit selection widget if open
 */
void update_unit_sel()
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != nullptr) {
    unit_sel->update_units();
    unit_sel->create_pixmap();
    unit_sel->update();
  }
}

/**
   Closes unit selection widget.
 */
void popdown_unit_sel()
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != nullptr) {
    unit_sel->close();
    delete unit_sel;
    unit_sel = nullptr;
  }
}
