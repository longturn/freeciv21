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

// Forward declarations
class QPainter;
class QPointF;
class QRect;
class QTextDocument;

struct city;

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
};

/**
 * A simple city bar that draws simple text on the map.
 */
class simple_citybar_painter : public citybar_painter {
public:
  explicit simple_citybar_painter();
  virtual ~simple_citybar_painter();

  QRect paint(QPainter &painter, const QPointF &position,
              const city *pcity) const override;

private:
  mutable QTextDocument *m_document;
  mutable QTextDocument *m_dark_document;
};
