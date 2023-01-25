/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QBitmap>
#include <QColor>
#include <QPixmap>

#include <map>

namespace freeciv {

class colorizer : public QObject {
  Q_OBJECT

public:
  explicit colorizer(const QPixmap &base, int hue_to_replace,
                     QObject *parent = nullptr);
  virtual ~colorizer() = default;

  /// Returns the base pixmap used by this colorizer
  QPixmap base() const { return m_base; }

  const QPixmap *pixmap(const QColor &color) const;

private:
  QPixmap m_base;
  QImage m_base_image;
  int m_hue_to_replace;
  mutable std::map<QRgb, QPixmap> m_cache;
};

} // namespace freeciv
