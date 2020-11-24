/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "minimap.h"
// Qt
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QToolTip>
#include <QWheelEvent>
// client
#include "client_main.h"
#include "overview_common.h"
// gui-qt
#include "fc_client.h"
#include "mapview.h"
#include "menu.h"
#include "page_game.h"
#include "qtg_cxxside.h"

/**********************************************************************/ /**
   Constructor for minimap
 **************************************************************************/
minimap_view::minimap_view(QWidget *parent) : fcwidget()
{
  setParent(parent);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  w_ratio = 0.0;
  h_ratio = 0.0;
  // Dark magic: This call is required for the widget to work.
  resize(0, 0);
  background = QBrush(QColor(0, 0, 0));
  setCursor(Qt::CrossCursor);
  rw = new resize_widget(this);
  rw->put_to_corner();
  pix = new QPixmap;
  scale_factor = 1.0;
  connect(&thread, &minimap_thread::rendered_image, this,
          &minimap_view::update_pixmap);
}

/**********************************************************************/ /**
   Minimap_view destructor
 **************************************************************************/
minimap_view::~minimap_view()
{
  if (pix) {
    delete pix;
  }
}

/**********************************************************************/ /**
   Paint event for minimap
 **************************************************************************/
void minimap_view::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**********************************************************************/ /**
   Sets scaling factor for minimap
 **************************************************************************/
void minimap_view::scale(double factor)
{
  scale_factor *= factor;
  if (scale_factor < 1) {
    scale_factor = 1.0;
  };
  update_image();
}

/**********************************************************************/ /**
   Called by close widget, cause widget has been hidden. Updates menu.
 **************************************************************************/
void minimap_view::update_menu()
{
  ::king()->menu_bar->minimap_status->setChecked(false);
}

/**********************************************************************/ /**
   Minimap is being moved, position is being remembered
 **************************************************************************/
void minimap_view::moveEvent(QMoveEvent *event) { position = event->pos(); }

/**********************************************************************/ /**
   Minimap is just unhidden, old position is restored
 **************************************************************************/
void minimap_view::showEvent(QShowEvent *event)
{
  move(position);
  event->setAccepted(true);
}

/**********************************************************************/ /**
   Draws viewport on minimap
 **************************************************************************/
void minimap_view::draw_viewport(QPainter *painter)
{
  int i, x[4], y[4];
  int src_x, src_y, dst_x, dst_y;

  if (!gui_options.overview.map) {
    return;
  }

  gui_to_overview_pos(tileset, &x[0], &y[0], mapview.gui_x0, mapview.gui_y0);
  gui_to_overview_pos(tileset, &x[1], &y[1], mapview.gui_x0 + mapview.width,
                      mapview.gui_y0);
  gui_to_overview_pos(tileset, &x[2], &y[2], mapview.gui_x0 + mapview.width,
                      mapview.gui_y0 + mapview.height);
  gui_to_overview_pos(tileset, &x[3], &y[3], mapview.gui_x0,
                      mapview.gui_y0 + mapview.height);
  painter->setPen(QColor(Qt::white));

  if (scale_factor > 1) {
    for (i = 0; i < 4; i++) {
      scale_point(x[i], y[i]);
    }
  }

  for (i = 0; i < 4; i++) {
    src_x = x[i] * w_ratio;
    src_y = y[i] * h_ratio;
    dst_x = x[(i + 1) % 4] * w_ratio;
    dst_y = y[(i + 1) % 4] * h_ratio;
    painter->drawLine(src_x, src_y, dst_x, dst_y);
  }
}

/**********************************************************************/ /**
   Scales point from real overview coords to scaled overview coords.
 **************************************************************************/
void minimap_view::scale_point(int &x, int &y)
{
  int ax, bx;
  int dx, dy;

  gui_to_overview_pos(tileset, &ax, &bx, mapview.gui_x0 + mapview.width / 2,
                      mapview.gui_y0 + mapview.height / 2);
  x = qRound(x * scale_factor);
  y = qRound(y * scale_factor);
  dx = qRound(ax * scale_factor - gui_options.overview.width / 2);
  dy = qRound(bx * scale_factor - gui_options.overview.height / 2);
  x = x - dx;
  y = y - dy;
}

/**********************************************************************/ /**
   Scales point from scaled overview coords to real overview coords.
 **************************************************************************/
void unscale_point(double scale_factor, int &x, int &y)
{
  int ax, bx;
  int dx, dy;

  gui_to_overview_pos(tileset, &ax, &bx, mapview.gui_x0 + mapview.width / 2,
                      mapview.gui_y0 + mapview.height / 2);
  dx = qRound(ax * scale_factor - gui_options.overview.width / 2);
  dy = qRound(bx * scale_factor - gui_options.overview.height / 2);
  x = x + dx;
  y = y + dy;
  x = qRound(x / scale_factor);
  y = qRound(y / scale_factor);
}

/**********************************************************************/ /**
   Sets minimap scale to default
 **************************************************************************/
void minimap_view::reset() { scale_factor = 1; }

/**********************************************************************/ /**
   Slot for updating pixmap from thread's image
 **************************************************************************/
void minimap_view::update_pixmap(const QImage &image)
{
  *pix = QPixmap::fromImage(image);
  update();
}

/**********************************************************************/ /**
   Minimap thread's contructor
 **************************************************************************/
minimap_thread::minimap_thread(QObject *parent)
    : QThread(parent), mini_width(20), mini_height(20), scale(1.0f),
      restart(false), abort(false)
{
}

/**********************************************************************/ /**
   Minimap thread's desctructor
 **************************************************************************/
minimap_thread::~minimap_thread()
{
  mutex.lock();
  abort = true;
  condition.wakeOne();
  mutex.unlock();
  wait();
}

/**********************************************************************/ /**
   Starts thread
 **************************************************************************/
void minimap_thread::render(double scale_factor, int width, int height)
{
  restart = false;
  abort = false;
  mini_width = width;
  mini_height = height;
  scale = scale_factor;
  if (!isRunning()) {
    start(LowPriority);
  } else {
    restart = true;
    condition.wakeOne();
  }
}

/**********************************************************************/ /**
   Updates minimap's image in thread
 **************************************************************************/
void minimap_thread::run()
{
  forever
  {
    QImage tpix;
    QImage gpix;
    QImage image(QSize(mini_width, mini_height), QImage::Format_RGB32);
    QImage bigger_pix(gui_options.overview.width * 2,
                      gui_options.overview.height * 2, QImage::Format_RGB32);
    int delta_x, delta_y;
    int x, y, ix, iy;
    float wf, hf;
    QPixmap *src, *dst;

    if (abort)
      return;
    mutex.lock();
    if (gui_options.overview.map != nullptr) {
      if (scale > 1) {
        /* move minimap now,
           scale later and draw without looking for origin */
        src = &gui_options.overview.map->map_pixmap;
        dst = &gui_options.overview.window->map_pixmap;
        x = gui_options.overview.map_x0;
        y = gui_options.overview.map_y0;
        ix = gui_options.overview.width - x;
        iy = gui_options.overview.height - y;
        pixmap_copy(dst, src, 0, 0, ix, iy, x, y);
        pixmap_copy(dst, src, 0, y, ix, 0, x, iy);
        pixmap_copy(dst, src, x, 0, 0, iy, ix, y);
        pixmap_copy(dst, src, x, y, 0, 0, ix, iy);
        tpix = gui_options.overview.window->map_pixmap.toImage();
        wf = static_cast<float>(gui_options.overview.width) / scale;
        hf = static_cast<float>(gui_options.overview.height) / scale;
        x = 0;
        y = 0;
        unscale_point(scale, x, y);
        bigger_pix.fill(Qt::black);
        delta_x = gui_options.overview.width / 2;
        delta_y = gui_options.overview.height / 2;
        image_copy(&bigger_pix, &tpix, 0, 0, delta_x, delta_y,
                   gui_options.overview.width, gui_options.overview.height);
        gpix = bigger_pix.copy(delta_x + x, delta_y + y, wf, hf);
        image = gpix.scaled(mini_width, mini_height, Qt::IgnoreAspectRatio,
                            Qt::FastTransformation);
      } else {
        tpix = gui_options.overview.map->map_pixmap.toImage();
        image = tpix.scaled(mini_width, mini_height, Qt::IgnoreAspectRatio,
                            Qt::FastTransformation);
      }
    }
    emit rendered_image(image);
    if (!restart)
      condition.wait(&mutex);
    restart = false;
    mutex.unlock();
  }
}

/**********************************************************************/ /**
   Updates minimap's pixmap
 **************************************************************************/
void minimap_view::update_image()
{
  if (isHidden()) {
    return;
  }
  // There might be some map updates lurking around
  mr_idle::idlecb()->run_now();
  thread.render(scale_factor, width(), height());
}

/**********************************************************************/ /**
   Redraws visible map using stored pixmap
 **************************************************************************/
void minimap_view::paint(QPainter *painter, QPaintEvent *event)
{
  int x, y, ix, iy;

  x = gui_options.overview.map_x0 * w_ratio;
  y = gui_options.overview.map_y0 * h_ratio;
  ix = pix->width() - x;
  iy = pix->height() - y;

  if (scale_factor > 1) {
    painter->drawPixmap(0, 0, *pix, 0, 0, pix->width(), pix->height());
  } else {
    painter->drawPixmap(ix, iy, *pix, 0, 0, x, y);
    painter->drawPixmap(ix, 0, *pix, 0, y, x, iy);
    painter->drawPixmap(0, iy, *pix, x, 0, ix, y);
    painter->drawPixmap(0, 0, *pix, x, y, ix, iy);
  }
  painter->setPen(QColor(palette().color(QPalette::Highlight)));
  painter->drawRect(1, 1, width() - 1, height() - 1);
  draw_viewport(painter);
  rw->put_to_corner();
}

/**********************************************************************/ /**
   Called when minimap has been resized
 **************************************************************************/
void minimap_view::resizeEvent(QResizeEvent *event)
{
  QSize size;
  size = event->size();

  if (C_S_RUNNING <= client_state() && size.width() > 0
      && size.height() > 0) {
    w_ratio = static_cast<float>(width()) / gui_options.overview.width;
    h_ratio = static_cast<float>(height()) / gui_options.overview.height;
    king()->qt_settings.minimap_width =
        static_cast<float>(size.width()) / mapview.width;
    king()->qt_settings.minimap_height =
        static_cast<float>(size.height()) / mapview.height;
  }
  update_image();
}

/**********************************************************************/ /**
   Wheel event for minimap - zooms it in or out
 **************************************************************************/
void minimap_view::wheelEvent(QWheelEvent *event)
{
  if (event->delta() > 0) {
    zoom_in();
  } else {
    zoom_out();
  }
  event->accept();
}

/**********************************************************************/ /**
   Sets scale factor to scale minimap 20% up
 **************************************************************************/
void minimap_view::zoom_in()
{
  if (scale_factor < double(gui_options.overview.width) / 8) {
    scale(1.2);
  }
}

/**********************************************************************/ /**
   Sets scale factor to scale minimap 20% down
 **************************************************************************/
void minimap_view::zoom_out() { scale(0.833); }

/**********************************************************************/ /**
   Mouse Handler for minimap_view
   Left button - moves minimap
   Right button - recenters on some point
   For wheel look mouseWheelEvent
 **************************************************************************/
void minimap_view::mousePressEvent(QMouseEvent *event)
{
  int fx, fy;
  int x, y;

  if (event->button() == Qt::LeftButton) {
    if (king()->interface_locked) {
      return;
    }
    cursor = event->globalPos() - geometry().topLeft();
  }
  if (event->button() == Qt::RightButton) {
    cursor = event->pos();
    fx = event->pos().x();
    fy = event->pos().y();
    fx = qRound(fx / w_ratio);
    fy = qRound(fy / h_ratio);
    if (scale_factor > 1) {
      unscale_point(scale_factor, fx, fy);
    }
    fx = qMax(fx, 1);
    fy = qMax(fy, 1);
    fx = qMin(fx, gui_options.overview.width - 1);
    fy = qMin(fy, gui_options.overview.height - 1);
    overview_to_map_pos(&x, &y, fx, fy);
    center_tile_mapcanvas(map_pos_to_tile(&(wld.map), x, y));
    update_image();
  }
  event->setAccepted(true);
}

/**********************************************************************/ /**
   Called when mouse button was pressed. Used to moving minimap.
 **************************************************************************/
void minimap_view::mouseMoveEvent(QMouseEvent *event)
{
  if (king()->interface_locked) {
    return;
  }
  if (event->buttons() & Qt::LeftButton) {
    QPoint p, r;
    p = event->pos();
    r = mapTo(queen()->mapview_wdg, p);
    p = r - p;
    move(event->globalPos() - cursor);
    setCursor(Qt::SizeAllCursor);
    king()->qt_settings.minimap_x =
        static_cast<float>(p.x()) / mapview.width;
    king()->qt_settings.minimap_y =
        static_cast<float>(p.y()) / mapview.height;
  }
}

/**********************************************************************/ /**
   Called when mouse button unpressed. Restores cursor.
 **************************************************************************/
void minimap_view::mouseReleaseEvent(QMouseEvent *event)
{
  setCursor(Qt::CrossCursor);
}

/**********************************************************************/ /**
   Return a canvas that is the overview window.
 **************************************************************************/
void update_minimap() { queen()->minimapview_wdg->update_image(); }

/**********************************************************************/ /**
   Called when the map size changes. This may be used to change the
   size of the GUI element holding the overview canvas. The
   overview.width and overview.height are updated if this function is
   called.
   It's used for first creation of overview only, later overview stays the
   same size, scaled by qt-specific function.
 **************************************************************************/
void overview_size_changed(void)
{
  queen()->minimapview_wdg->resize(0, 0);
  queen()->minimapview_wdg->move(
      king()->qt_settings.minimap_x * mapview.width,
      king()->qt_settings.minimap_y * mapview.height);
  queen()->minimapview_wdg->resize(
      king()->qt_settings.minimap_width * mapview.width,
      king()->qt_settings.minimap_height * mapview.height);
}
