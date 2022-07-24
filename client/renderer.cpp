/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "renderer.h"

#include "mapview_common.h"
#include "mapview_g.h"

namespace freeciv {

/**
 * @class renderer
 * @brief Renders the map on widgets
 *
 * This class is used to draw the map. It can handle zoom via the @ref scale
 * property.
 *
 * @property scale By how much the map is scaled before being drawn (a scale
 * of 2 means that everything is 2x bigger)
 * @property origin The position of the top left corner of the view.
 */

/**
 * Constructor.
 */
renderer::renderer(QObject *parent) : QObject(parent) {}

/**
 * Changes the origin of the canvas (the point at the top left of the view).
 */
void renderer::set_origin(const QPointF &origin)
{
  m_origin = origin;
  set_mapview_origin(origin.x(), origin.y());
  flush_dirty(); // Request (or rather, force) a new frame
}

/**
 * Changes the scale of the rendering (zooms in or out).
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
 * Renders the specified region of the visible portion of the map on @c
 * painter.
 * @see @ref render(QPainter&, const QRect&)
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
 * position of
 * @c area is relative to the @ref viewport.
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

} // namespace freeciv
