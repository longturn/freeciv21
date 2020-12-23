/**************************************************************************
                  Copyright (c) 2020 Freeciv21 contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

#include <memory>

// Forward declarations
class QPainter;
class QPointF;
class QRect;
class QString;
class QStringList;
class QTextDocument;

struct city;
struct option;

/**
 * Abstraction for city bars of various styles.
 */
class citybar_painter {
public:
  virtual ~citybar_painter() = default;

  /**
   * Draws a city bar under the given `position`. Returns the bounding box
   * of the touched area.
   */
  virtual QRect paint(QPainter &painter, const QPointF &position,
                      const city *pcity) const = 0;

  static QStringList available();
  static const QVector<QString> *available_vector(const option *);
  static void option_changed(option *opt);
  static bool set_current(const QString &name);
  static citybar_painter *current() { return s_current.get(); }

private:
  static std::unique_ptr<citybar_painter> s_current;
};

/**
 * A simple city bar that draws simple text on the map.
 */
class simple_citybar_painter : public citybar_painter {
public:
  QRect paint(QPainter &painter, const QPointF &position,
              const city *pcity) const override;
};

/**
 * The traditional box-based city bar.
 */
class traditional_citybar_painter : public citybar_painter {
public:
  QRect paint(QPainter &painter, const QPointF &position,
              const city *pcity) const override;
};

/**
 * A polished city bar, more like certain commercial game.
 */
class polished_citybar_painter : public citybar_painter {
public:
  QRect paint(QPainter &painter, const QPointF &position,
              const city *pcity) const override;
};
