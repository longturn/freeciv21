/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

/* utility */
#include "rgbcolor.h"
// gui-qt
#include "colors.h"
#include "colors_g.h"
#include "qtg_cxxside.h"

/************************************************************************/ /**
   Allocate a color (adjusting it for our colormap if necessary on paletted
   systems) and return a pointer to it.
 ****************************************************************************/
struct color *qtg_color_alloc(int r, int g, int b)
{
  struct color *pcolor = new color;

  pcolor->qcolor.setRgb(r, g, b);

  return pcolor;
}

/************************************************************************/ /**
   Free a previously allocated color.  See qtg_color_alloc.
 ****************************************************************************/
void qtg_color_free(struct color *pcolor) { delete pcolor; }

/************************************************************************/ /**
   Return a number indicating the perceptual brightness of this color
   relative to others (larger is brighter).
 ****************************************************************************/
int color_brightness_score(struct color *pcolor)
{
  /* QColor has color space conversions, but nothing giving a perceptually
   * even color space */
  struct rgbcolor *prgb = rgbcolor_new(
      pcolor->qcolor.red(), pcolor->qcolor.green(), pcolor->qcolor.blue());
  int score = rgbcolor_brightness_score(prgb);

  rgbcolor_destroy(prgb);
  return score;
}
