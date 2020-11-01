/**************************************************************************
  /\ ___ /\        Copyright (c) 1996-2020 ＦＲＥＥＣＩＶ ２１ and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                  If not, see https://www.gnu.org/licenses/.
**************************************************************************/

#include "unitselect.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QtMath>
// common
#include "movement.h"
// client
#include "client_main.h"
#include "control.h"
#include "mapview_common.h"
#include "tilespec.h"
// gui-qt
#include "canvas.h"
#include "fonts.h"
#include "mapview.h"
#include "page_game.h"
#include "qtg_cxxside.h"

/***********************************************************************/ /**
   Contructor for units_select
 ***************************************************************************/
units_select::units_select(struct tile *ptile, QWidget *parent)
{
  QPoint p, final_p;

  setParent(parent);
  utile = ptile;
  pix = NULL;
  show_line = 0;
  highligh_num = -1;
  ufont.setItalic(true);
  info_font = *fc_font::instance()->get_font(fonts::notify_label);
  update_units();
  h_pix = NULL;
  create_pixmap();
  p = mapFromGlobal(QCursor::pos());
  cw = new close_widget(this);
  setMouseTracking(true);
  final_p.setX(p.x());
  final_p.setY(p.y());
  if (p.x() + width() > parentWidget()->width()) {
    final_p.setX(parentWidget()->width() - width());
  }
  if (p.y() - height() < 0) {
    final_p.setY(height());
  }
  move(final_p.x(), final_p.y() - height());
  setFocus();
  /* Build fails with qt5 connect style for static functions
   * Qt5.2 so dont update */
  QTimer::singleShot(10, this, SLOT(update_img()));
}

/***********************************************************************/ /**
   Destructor for unit select
 ***************************************************************************/
units_select::~units_select()
{
  delete h_pix;
  delete pix;
  delete cw;
}

/***********************************************************************/ /**
   Create pixmap of whole widget except borders (pix)
 ***************************************************************************/
void units_select::create_pixmap()
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
  struct canvas *unit_pixmap;
  struct unit *punit;
  float isosize;

  if (pix != NULL) {
    delete pix;
    pix = NULL;
  };
  isosize = 0.7;
  if (tileset_hex_height(tileset) > 0 || tileset_hex_width(tileset) > 0) {
    isosize = 0.5;
  }

  update_units();
  if (unit_list.count() > 0) {
    if (!tileset_is_isometric(tileset)) {
      item_size.setWidth(tileset_unit_width(tileset));
      item_size.setHeight(tileset_unit_width(tileset));
    } else {
      item_size.setWidth(tileset_unit_width(tileset) * isosize);
      item_size.setHeight(tileset_unit_width(tileset) * isosize);
    }
    more = false;
    if (h_pix != nullptr) {
      delete h_pix;
    }
    h_pix = new QPixmap(item_size.width(), item_size.height());
    h_pix->fill(palette().color(QPalette::HighlightedText));
    if (unit_count < 5) {
      row_count = 1;
      pix = new QPixmap((unit_list.size()) * item_size.width(),
                        item_size.height());
    } else if (unit_count < 9) {
      row_count = 2;
      pix = new QPixmap(4 * item_size.width(), 2 * item_size.height());
    } else {
      row_count = 3;
      if (unit_count > 12) {
        more = true;
      }
      pix = new QPixmap(4 * item_size.width(), 3 * item_size.height());
    }
    pix->fill(Qt::transparent);
    for (auto punit : qAsConst(unit_list)) {
      unit_pixmap = qtg_canvas_create(tileset_unit_width(tileset),
                                      tileset_unit_height(tileset));
      unit_pixmap->map_pixmap.fill(Qt::transparent);
      put_unit(punit, unit_pixmap, 1.0, 0, 0);
      img = unit_pixmap->map_pixmap.toImage();
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
      qtg_canvas_free(unit_pixmap);
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
      if (i % 4 == 0) {
        x = 0;
        y = y + item_size.height();
      }
      punit = unit_list.at(i);
      Q_ASSERT(punit != NULL);

      if (i == highligh_num) {
        p.drawPixmap(x, y, *h_pix);
        p.drawPixmap(x, y, *tmp_pix);
      } else {
        p.drawPixmap(x, y, *tmp_pix);
      }

      if (client_is_global_observer()
          || unit_owner(punit) == client.conn.playing) {
        int rate, f;
        QString str;

        rate = unit_type_get(punit)->move_rate;
        f = ((punit->fuel) - 1);
        str = QString(move_points_text(punit->moves_left, false));
        if (utype_fuel(unit_type_get(punit))) {
          str = str + "("
                + QString(
                    move_points_text((rate * f) + punit->moves_left, false))
                + ")";
        }
        /* TRANS: MP = Movement points */
        str = QString(_("MP:")) + str;
        p.drawText(x, y + item_size.height() - 4, str);
      }

      x = x + item_size.width();
      delete tmp_pix;
    }
    p.end();
    setFixedWidth(pix->width() + 20);
    setFixedHeight(pix->height() + 2 * (fm.height() + 6));
    qDeleteAll(pix_list.begin(), pix_list.end());
  }
}

/***********************************************************************/ /**
   Event for mouse moving around units_select
 ***************************************************************************/
void units_select::mouseMoveEvent(QMouseEvent *event)
{
  int a, b;
  int old_h;
  QFontMetrics fm(info_font);

  old_h = highligh_num;
  highligh_num = -1;
  if (event->x() > width() - 11 || event->y() > height() - fm.height() - 5
      || event->y() < fm.height() + 3 || event->x() < 11) {
    /** do nothing if mouse is on border, just skip next if */
  } else if (row_count > 0) {
    a = (event->x() - 10) / item_size.width();
    b = (event->y() - fm.height() - 3) / item_size.height();
    highligh_num = b * 4 + a;
  }
  if (old_h != highligh_num) {
    create_pixmap();
    update();
  }
}

/***********************************************************************/ /**
   Mouse pressed event for units_select.
   Left Button - chooses units
   Right Button - closes widget
 ***************************************************************************/
void units_select::mousePressEvent(QMouseEvent *event)
{
  struct unit *punit;
  if (event->button() == Qt::RightButton) {
    was_destroyed = true;
    close();
    destroy();
  }
  if (event->button() == Qt::LeftButton && highligh_num != -1) {
    update_units();
    if (highligh_num >= unit_list.count()) {
      return;
    }
    punit = unit_list.at(highligh_num);
    unit_focus_set(punit);
    was_destroyed = true;
    close();
    destroy();
  }
}

/***********************************************************************/ /**
   Update image, because in constructor theme colors
   are uninitialized in QPainter
 ***************************************************************************/
void units_select::update_img()
{
  create_pixmap();
  update();
}

/***********************************************************************/ /**
   Redirected paint event
 ***************************************************************************/
void units_select::paint(QPainter *painter, QPaintEvent *event)
{
  QFontMetrics fm(info_font);
  int h, i;
  int *f_size;
  QPen pen;
  QString str, str2;
  struct unit *punit;
  int point_size = info_font.pointSize();
  int pixel_size = info_font.pixelSize();

  if (point_size < 0) {
    f_size = &pixel_size;
  } else {
    f_size = &point_size;
  }
  if (highligh_num != -1 && highligh_num < unit_list.count()) {
    punit = unit_list.at(highligh_num);
    /* TRANS: HP - hit points */
    str2 = QString(_("%1 HP:%2/%3"))
               .arg(QString(unit_activity_text(punit)),
                    QString::number(punit->hp),
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
  if (pix != NULL) {
    painter->drawPixmap(10, h + 3, *pix);
    pen.setColor(palette().color(QPalette::Text));
    painter->setPen(pen);
    painter->setFont(info_font);
    painter->drawText(10, h, str);
    if (highligh_num != -1 && highligh_num < unit_list.count()) {
      painter->drawText(10, height() - 5, str2);
    }
    /* draw scroll */
    if (more) {
      int maxl = ((unit_count - 1) / 4) + 1;
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
  cw->put_to_corner();
}

/***********************************************************************/ /**
   Paint event, redirects to paint(...)
 ***************************************************************************/
void units_select::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/***********************************************************************/ /**
   Function from abstract fcwidget to update menu, its not needed
   cause widget is easy closable via right mouse click
 ***************************************************************************/
void units_select::update_menu()
{
  was_destroyed = true;
  close();
  destroy();
}

/***********************************************************************/ /**
   Updates unit list on tile
 ***************************************************************************/
void units_select::update_units()
{
  int i = 1;
  struct unit_list *punit_list;

  if (utile == nullptr)
    return;
  unit_count = 0;
  if (utile == NULL) {
    struct unit *punit = head_of_units_in_focus();
    if (punit) {
      utile = unit_tile(punit);
    }
  }
  unit_list.clear();
  if (utile != nullptr) {
    punit_list = utile->units;
    if (punit_list != nullptr) {
      unit_list_iterate(utile->units, punit)
      {
        unit_count++;
        if (i > show_line * 4)
          unit_list.push_back(punit);
        i++;
      }
      unit_list_iterate_end;
    }
  }
  if (unit_list.count() == 0) {
    close();
  }
}

/***********************************************************************/ /**
   Close event for units_select, restores focus to map
 ***************************************************************************/
void units_select::closeEvent(QCloseEvent *event)
{
  queen()->mapview_wdg->setFocus();
  QWidget::closeEvent(event);
}

/***********************************************************************/ /**
   Mouse wheel event for units_select
 ***************************************************************************/
void units_select::wheelEvent(QWheelEvent *event)
{
  int nr;

  if (!more && utile == NULL) {
    return;
  }
  nr = qCeil(static_cast<qreal>(unit_list_size(utile->units)) / 4) - 3;
  if (event->delta() < 0) {
    show_line++;
    show_line = qMin(show_line, nr);
  } else {
    show_line--;
    show_line = qMax(0, show_line);
  }
  update_units();
  create_pixmap();
  update();
  event->accept();
}

/***********************************************************************/ /**
   Keyboard handler for units_select
 ***************************************************************************/
void units_select::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape) {
    was_destroyed = true;
    close();
    destroy();
  }
  QWidget::keyPressEvent(event);
}

/************************************************************************/ /**
   Shows/closes unit selection widget
 ****************************************************************************/
void toggle_unit_sel_widget(struct tile *ptile)
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != NULL) {
    unit_sel->close();
    delete unit_sel;
    unit_sel = new units_select(ptile, queen()->mapview_wdg);
    unit_sel->show();
  } else {
    unit_sel = new units_select(ptile, queen()->mapview_wdg);
    unit_sel->show();
  }
}

/************************************************************************/ /**
   Update unit selection widget if open
 ****************************************************************************/
void update_unit_sel()
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != NULL) {
    unit_sel->update_units();
    unit_sel->create_pixmap();
    unit_sel->update();
  }
}

/************************************************************************/ /**
   Closes unit selection widget.
 ****************************************************************************/
void popdown_unit_sel()
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != nullptr) {
    unit_sel->close();
    delete unit_sel;
    unit_sel = nullptr;
  }
}
