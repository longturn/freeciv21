/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "renderer.h"

#include "map_updates_handler.h"
#include "mapview_common.h"
#include "mapview_g.h"

namespace freeciv {

/**
 * @class renderer
 * @brief Renders the map on widgets
 *
 * This class is used to draw the map. The position of the mapview is given
 * by @ref origin and zoom is set via the @ref scale property.
 */

/**
 * Constructor.
 */
renderer::renderer(QObject *parent)
    : QObject(parent), m_updates(new map_updates_handler(this))
{
  connect(m_updates, &map_updates_handler::repaint_needed, this,
          &renderer::unqueue_updates, Qt::QueuedConnection);
}

/**
 * Changes the origin of the canvas (the point at the top left of the view).
 */
void renderer::set_origin(const QPointF &origin)
{
  m_origin = origin;
  set_mapview_origin(origin.x(), origin.y());
  emit repaint_needed(QRect(QPoint(), m_viewport_size));
}

/**
 * Changes the scale of the rendering (zooms in or out). A scale of 2 means
 * that everything is 2x bigger.
 */
void renderer::set_scale(double scale)
{
  m_scale = scale;

  // When zoomed in, we pretend that the canvas is smaller than it actually
  // is. This makes text look bad, but everything else is drawn correctly.
  map_canvas_resized(m_viewport_size.width() / m_scale,
                     m_viewport_size.height() / m_scale);
}

/**
 * Instructs the renderer to draw a viewport with a different size.
 */
void renderer::set_viewport_size(const QSize &size)
{
  m_viewport_size = size;

  // When zoomed in, we pretend that the canvas is smaller than it actually
  // is. This makes text look bad, but everything else is drawn correctly.
  map_canvas_resized(m_viewport_size.width() / m_scale,
                     m_viewport_size.height() / m_scale);
}

/**
 * Renders the specified @c region of the visible portion of the map on @c
 * painter.
 * @overload
 */
void renderer::render(QPainter &painter, const QRegion &region) const
{
  for (const auto &rect : region) {
    render(painter, rect);
  }
}

/**
 * Renders the specified area of the visible portion of the map on @c
 * painter. This is meant to be used directly from @c paintEvent, so the
 * position of @c area is relative to the viewport.
 * @overload
 */
void renderer::render(QPainter &painter, const QRect &area) const
{
  if (scale() != 1) {
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
  }

  auto mapview_rect =
      QRectF(area.left() / scale(), area.top() / scale(),
             area.width() / scale(), area.height() / scale());
  painter.drawPixmap(area, *mapview.store, mapview_rect);
}

/**
 * Processes all pending map updates and writes them to the map buffer.
 */
void renderer::unqueue_updates()
{
  const auto rects = update_rects();

  /* This code "pops" the lists of tile updates off of the static array and
   * stores them locally.  This allows further updates to be queued within
   * the function itself (namely, within update_map_canvas). */
  const auto updates = m_updates->list();
  const auto full = m_updates->full();

  m_updates->clear();

  if (!map_is_empty()) {
    if (full) {
      update_map_canvas(0, 0, mapview.store_width, mapview.store_height);
      emit repaint_needed(QRect(0, 0, mapview.store_width * scale(),
                                mapview.store_height * scale()));
    } else {
      QRectF to_update;

      for (const auto [tile, upd_types] : updates) {
        for (const auto [type, rect] : rects) {
          if (upd_types & type) {
            float xl, yt;
            (void) tile_to_canvas_pos(&xl, &yt, tile);
            to_update |= rect.translated(xl, yt);
          }
        }
      }

      if (to_update.intersects(
              QRectF(0, 0, mapview.width, mapview.height))) {
        const auto aligned = to_update.toAlignedRect();
        update_map_canvas(aligned.x(), aligned.y(), aligned.width(),
                          aligned.height());

        const auto scaled_aligned =
            QRectF(aligned.x() * scale(), aligned.y() * scale(),
                   aligned.width() * scale(), aligned.height() * scale())
                .toAlignedRect();
        emit repaint_needed(scaled_aligned);
      }
    }
  }
}

} // namespace freeciv
