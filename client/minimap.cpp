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

// client
#include "client_main.h"
#include "minimap_panel.h"
#include "overview_common.h"

// gui-qt
#include "fc_client.h"
#include "mapview.h"
#include "page_game.h"
#include "qtg_cxxside.h"

namespace {
const auto always_visible_margin = 15;
}

/**
   Constructor for minimap
 */
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
  pix = new QPixmap;
}

/**
   Minimap_view destructor
 */
minimap_view::~minimap_view()
{
  delete pix;
  pix = nullptr;
}

/**
   Paint event for minimap
 */
void minimap_view::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Called by close widget, cause widget has been hidden. Updates menu.
 */
void minimap_view::update_menu()
{
  ::king()->menu_bar->minimap_status->setChecked(false);
}

/**
   Minimap is being moved, position is being remembered
 */
void minimap_view::moveEvent(QMoveEvent *event) { position = event->pos(); }

/**
   Minimap is just unhidden, old position is restored
 */
void minimap_view::showEvent(QShowEvent *event)
{
  move(position);
  event->setAccepted(true);
}

namespace {

void overview_pos_nowrap(const struct tileset *t, int *ovr_x, int *ovr_y,
                         int gui_x, int gui_y)
{
  double ntl_x, ntl_y;
  gui_to_natural_pos(t, &ntl_x, &ntl_y, gui_x, gui_y);

  // Now convert straight to overview coordinates.
  *ovr_x = floor((ntl_x - gui_options.overview.map_x0) * OVERVIEW_TILE_SIZE);
  *ovr_y = floor((ntl_y - gui_options.overview.map_y0) * OVERVIEW_TILE_SIZE);
}

} // namespace

/**
   Draws viewport on minimap
 */
void minimap_view::draw_viewport(QPainter *painter)
{
  int x[4], y[4];

  if (!gui_options.overview.map) {
    return;
  }

  overview_pos_nowrap(tileset, &x[0], &y[0], mapview.gui_x0, mapview.gui_y0);
  overview_pos_nowrap(tileset, &x[1], &y[1], mapview.gui_x0 + mapview.width,
                      mapview.gui_y0);
  overview_pos_nowrap(tileset, &x[2], &y[2], mapview.gui_x0 + mapview.width,
                      mapview.gui_y0 + mapview.height);
  overview_pos_nowrap(tileset, &x[3], &y[3], mapview.gui_x0,
                      mapview.gui_y0 + mapview.height);

  if ((current_topo_has_flag(TF_WRAPX)
       && (x[2] - x[0] > NATURAL_WIDTH * OVERVIEW_TILE_SIZE))
      || (current_topo_has_flag(TF_WRAPY)
          && (y[2] - y[0] > NATURAL_HEIGHT * OVERVIEW_TILE_SIZE))) {
    // Don't draw viewport lines if the view wraps around the map.
    return;
  }

  painter->setPen(QColor(Qt::white));

  QVector<QLineF> lines;
  for (int i = 0; i < 4; i++) {
    lines.append(QLineF(x[i] * w_ratio, y[i] * h_ratio,
                        x[(i + 1) % 4] * w_ratio, y[(i + 1) % 4] * h_ratio));

    // Add another line segment if this one wraps around.
    int wrap_src_x = current_topo_has_flag(TF_WRAPX)
                         ? FC_WRAP(x[i], NATURAL_WIDTH * OVERVIEW_TILE_SIZE)
                         : x[i];
    int wrap_src_y = current_topo_has_flag(TF_WRAPY)
                         ? FC_WRAP(y[i], NATURAL_HEIGHT * OVERVIEW_TILE_SIZE)
                         : y[i];

    if (wrap_src_x != x[i] || wrap_src_y != y[i]) {
      int projected_dst_x = x[(i + 1) % 4] + wrap_src_x - x[i];
      int projected_dst_y = y[(i + 1) % 4] + wrap_src_y - y[i];
      lines.append(QLineF(wrap_src_x * w_ratio, wrap_src_y * h_ratio,
                          projected_dst_x * w_ratio,
                          projected_dst_y * h_ratio));
    }

    int wrap_dst_x =
        current_topo_has_flag(TF_WRAPX)
            ? FC_WRAP(x[(i + 1) % 4], NATURAL_WIDTH * OVERVIEW_TILE_SIZE)
            : x[(i + 1) % 4];
    int wrap_dst_y =
        current_topo_has_flag(TF_WRAPY)
            ? FC_WRAP(y[(i + 1) % 4], NATURAL_HEIGHT * OVERVIEW_TILE_SIZE)
            : y[(i + 1) % 4];
    if (wrap_dst_x != x[(i + 1) % 4] || wrap_dst_y != y[(i + 1) % 4]) {
      int projected_src_x = x[i] + wrap_dst_x - x[(i + 1) % 4];
      int projected_src_y = y[i] + wrap_dst_y - y[(i + 1) % 4];
      lines.append(QLineF(projected_src_x * w_ratio,
                          projected_src_y * h_ratio, wrap_dst_x * w_ratio,
                          wrap_dst_y * h_ratio));
    }
  }
  painter->drawLines(lines);
}

/**
   Updates minimap's pixmap
 */
void minimap_view::update_image()
{
  if (isHidden()) {
    return;
  }
  // There might be some map updates lurking around
  mrIdle::idlecb()->runNow();
  update();
}

/**
 * Reimplements QWidget::heightForWidth
 */
int minimap_view::heightForWidth(int width) const
{
  const auto size = gui_options.overview.map->size();
  if (tileset_is_isometric(tileset)) {
    // Traditional iso tilesets have more or less this aspect ratio
    return size.height() * width / size.width() / 2;
  } else {
    return size.height() * width / size.width();
  }
}

/**
 * Reimplements QWidget::sizeHint
 */
QSize minimap_view::sizeHint() const
{
  // The default size is a bit too small...
  return 5 * gui_options.overview.map->size();
}

/**
   Redraws visible map using stored pixmap
 */
void minimap_view::paint(QPainter *painter, QPaintEvent *event)
{
  painter->drawPixmap(1, 1, width() - 1, height() - 1,
                      *gui_options.overview.map);

  painter->setPen(QColor(palette().color(QPalette::HighlightedText)));
  painter->drawRect(0, 0, width() - 1, height() - 1);
  draw_viewport(painter);
}

/**
   Called when minimap has been resized
 */
void minimap_view::resizeEvent(QResizeEvent *event)
{
  auto size = event->size();

  if (x() + size.width() < always_visible_margin) {
    size.setWidth(always_visible_margin - x());
    resize(size);
  }
  if (y() + size.height() < always_visible_margin) {
    size.setHeight(always_visible_margin - y());
    resize(size);
  }

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

/**
   Mouse Handler for minimap_view
   Left button - moves minimap
   Right button - recenters on some point
   For wheel look mouseWheelEvent
 */
void minimap_view::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    cursor = event->pos();
    auto fx = event->pos().x();
    auto fy = event->pos().y();
    fx = qRound(fx / w_ratio);
    fy = qRound(fy / h_ratio);
    fx = qMax(fx, 1);
    fy = qMax(fy, 1);
    fx = qMin(fx, gui_options.overview.width - 1);
    fy = qMin(fy, gui_options.overview.height - 1);
    int x, y;
    overview_to_map_pos(&x, &y, fx, fy);
    auto *ptile = map_pos_to_tile(&(wld.map), x, y);
    fc_assert_ret(ptile);
    center_tile_mapcanvas(ptile);
    update_image();
  }
  event->setAccepted(true);
}

/**
   Return a canvas that is the overview window.
 */
void update_minimap() { queen()->minimap_panel->minimap()->update_image(); }
