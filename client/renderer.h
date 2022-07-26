/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRegion>
#include <QSize>

namespace freeciv {

class map_updates_handler;

class renderer : public QObject {
  Q_OBJECT
  Q_PROPERTY(QPointF origin READ origin WRITE set_origin);
  Q_PROPERTY(double scale READ scale WRITE set_scale);

public:
  explicit renderer(QObject *parent = nullptr);
  virtual ~renderer() = default;

  /// The origin of the view (the point at the top left corner)
  QPointF origin() const { return m_origin; }
  void set_origin(const QPointF &origin);

  /// The scale (zoom) at which rendering is performed
  double scale() const { return m_scale; }
  void set_scale(double scale);

  /// The current dimensions of the viewport
  QSize viewport_size() const { return m_viewport_size; }
  void set_viewport_size(const QSize &size);

  void render(QPainter &painter, const QRegion &region) const;
  void render(QPainter &painter, const QRect &area) const;

signals:
  void repaint_needed(const QRegion &where);

private slots:
  void unqueue_updates();

private:
  QPointF m_origin;
  double m_scale = 1.0;
  QSize m_viewport_size;
  map_updates_handler *m_updates;
};

} // namespace freeciv
