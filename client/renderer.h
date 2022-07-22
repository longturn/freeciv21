/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QObject>
#include <QPainter>
#include <QRect>
#include <QRegion>
#include <QSize>

namespace freeciv {

class renderer : public QObject {
  Q_OBJECT
  Q_PROPERTY(double scale READ scale WRITE set_scale);

public:
  explicit renderer(QObject *parent = nullptr);
  virtual ~renderer() = default;

  /// The scale (zoom) at which rendering is performed
  double scale() const { return m_scale; }
  void set_scale(double scale);

  /// The current dimensions of the viewport
  QSize viewport_size() const { return m_viewport_size; }
  void set_viewport_size(const QSize &size);

  void render(QPainter &painter, const QRegion &region) const;
  void render(QPainter &painter, const QRect &area) const;

private:
  double m_scale = 1.0;
  QSize m_viewport_size;
};

} // namespace freeciv
