/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QImageReader>
#include <QPainter>
// utility
#include "log.h"
#include <cmath>
// gui-qt
#include "colors.h"
#include "fc_client.h"
#include "qtg_cxxside.h"
#include "sprite.h"

/**
   Load the given graphics file into a sprite.  This function loads an
   entire image file, which may later be broken up into individual sprites
   with crop_sprite.
 */
QPixmap *qtg_load_gfxfile(const char *filename)
{
  QPixmap *entire = new QPixmap;

  if (QPixmapCache::find(QString(filename), entire)) {
    return entire;
  }
  entire->load(QString(filename));
  QPixmapCache::insert(QString(filename), *entire);

  return entire;
}

/**
   Create a new sprite by cropping and taking only the given portion of
   the image.

   source gives the sprite that is to be cropped.

   x,y, width, height gives the rectangle to be cropped.  The pixel at
   position of the source sprite will be at (0,0) in the new sprite, and
   the new sprite will have dimensions (width, height).

   mask gives an additional mask to be used for clipping the new
   sprite. Only the transparency value of the mask is used in
   crop_sprite. The formula is: dest_trans = src_trans *
   mask_trans. Note that because the transparency is expressed as an
   integer it is common to divide it by 256 afterwards.

   mask_offset_x, mask_offset_y is the offset of the mask relative to the
   origin of the source image.  The pixel at (mask_offset_x,mask_offset_y)
   in the mask image will be used to clip pixel (0,0) in the source image
   which is pixel (-x,-y) in the new image.
 */
QPixmap *qtg_crop_sprite(const QPixmap *source, int x, int y, int width,
                         int height, const QPixmap *mask, int mask_offset_x,
                         int mask_offset_y)
{
  QPainter p;
  QRectF source_rect;
  QRectF dest_rect;
  QPixmap *cropped;

  fc_assert_ret_val(source, NULL);

  if (!width || !height) {
    return NULL;
  }
  cropped = new QPixmap(width, height);
  cropped->fill(Qt::transparent);
  source_rect = QRectF(x, y, width, height);
  dest_rect = QRectF(0, 0, width, height);

  p.begin(cropped);
  p.setRenderHint(QPainter::Antialiasing);
  p.drawPixmap(dest_rect, *source, source_rect);
  p.end();

  if (mask) {
    int mw = mask->width();
    int mh = mask->height();

    source_rect = QRectF(0, 0, mw, mh);
    dest_rect = QRectF(mask_offset_x - x, mask_offset_y - y, mw, mh);
    p.begin(cropped);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawPixmap(dest_rect, *mask, source_rect);
    p.end();
  }

  return cropped;
}

/**
   Find the dimensions of the sprite.
 */
void qtg_get_sprite_dimensions(const QPixmap *sprite, int *width,
                               int *height)
{
  *width = sprite->width();
  *height = sprite->height();
}

/**
   Free a sprite and all associated image data.
 */
void qtg_free_sprite(QPixmap *s) { delete s; }

/**
   Create a new sprite with the given height, width and color.
 */
QPixmap *qtg_create_sprite(int width, int height, const QColor *pcolor)
{
  QPixmap *created = new QPixmap;

  created = new QPixmap(width, height);

  created->fill(*pcolor);

  return created;
}
