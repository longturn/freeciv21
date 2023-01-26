/*
    Copyright (c) 1996-2023 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
 */

// utility
#include "log.h"
#include "shared.h"

// common
#include "player.h"
#include "rgbcolor.h"
#include "terrain.h"

/* client/include */
#include "colors_g.h"

// client
#include "tileset/tilespec.h"

#include "colors_common.h"

struct color_system {
  struct rgbcolor **stdcolors;
};

/**
   Called when the client first starts to allocate the default colors.

   Currently this must be called in ui_main, generally after UI
   initialization.
 */
struct color_system *color_system_read(struct section_file *file)
{
  struct color_system *colors = new color_system;
  enum color_std stdcolor;

  colors->stdcolors = new rgbcolor *[COLOR_LAST]();

  for (stdcolor = color_std_begin(); stdcolor != color_std_end();
       stdcolor = color_std_next(stdcolor)) {
    struct rgbcolor *prgbcolor = nullptr;

    if (rgbcolor_load(file, &prgbcolor, "colors.%s0",
                      color_std_name(stdcolor))) {
      *(colors->stdcolors + stdcolor) = prgbcolor;
    } else {
      qCritical("Color %s: %s", color_std_name(stdcolor), secfile_error());
      *(colors->stdcolors + stdcolor) = rgbcolor_new(0, 0, 0);
    }
  }

  return colors;
}

/**
   Called when the client first starts to free any allocated colors.
 */
void color_system_free(struct color_system *colors)
{
  enum color_std stdcolor;

  for (stdcolor = color_std_begin(); stdcolor != color_std_end();
       stdcolor = color_std_next(stdcolor)) {
    rgbcolor_destroy(*(colors->stdcolors + stdcolor));
  }

  delete[] colors->stdcolors;
  delete colors;
}

/**
   Return a pointer to the given "standard" color.
 */
QColor get_color(const struct tileset *t, enum color_std stdcolor)
{
  struct color_system *colors = get_color_system(t);

  fc_assert_ret_val(colors != nullptr, Qt::black);

  auto &color = *(colors->stdcolors + stdcolor);
  return QColor(color->r, color->g, color->b);
}

/**
   Return whether the player has a color assigned yet.
   Should only be FALSE in pregame.
 */
bool player_has_color(const struct tileset *t, const struct player *pplayer)
{
  Q_UNUSED(t)
  fc_assert_ret_val(pplayer != nullptr, false);

  return pplayer->rgb != nullptr;
}

/**
   Return the color of the player.
   In pregame, callers should check player_has_color() before calling
   this.
 */
QColor get_player_color(const struct tileset *t,
                        const struct player *pplayer)
{
  Q_UNUSED(t)
  fc_assert_ret_val(pplayer != nullptr, Qt::black);
  fc_assert_ret_val(pplayer->rgb != nullptr, Qt::black);

  return QColor(pplayer->rgb->r, pplayer->rgb->g, pplayer->rgb->b);
}

/**
   Return a pointer to the given "terrain" color.
 */
QColor get_terrain_color(const struct tileset *t,
                         const struct terrain *pterrain)
{
  Q_UNUSED(t)
  fc_assert_ret_val(pterrain != nullptr, Qt::black);
  fc_assert_ret_val(pterrain->rgb != nullptr, Qt::black);

  return QColor(pterrain->rgb->r, pterrain->rgb->g, pterrain->rgb->b);
}

/**
   Find the colour from 'candidates' with the best perceptual contrast from
   'subject'.
 */
QColor color_best_contrast(const QColor &subject, const QColor *candidates,
                           int ncandidates)
{
  int sbright = color_brightness_score(subject), bestdiff = 0;

  fc_assert_ret_val(candidates != nullptr, Qt::black);
  fc_assert_ret_val(ncandidates > 0, Qt::black);

  QColor best;
  for (int i = 0; i < ncandidates; i++) {
    int cbright = color_brightness_score(candidates[i]);
    int diff = ABS(sbright - cbright);

    if (i == 0 || diff > bestdiff) {
      best = candidates[i];
      bestdiff = diff;
    }
  }

  return best;
}

/**
 * Return a number indicating the perceptual brightness of this color
 * relative to others (larger is brighter).
 */
int color_brightness_score(const QColor &color)
{
  /* QColor has color space conversions, but nothing giving a perceptually
   * even color space */
  rgbcolor prgb{color.red(), color.green(), color.blue()};
  return rgbcolor_brightness_score(&prgb);
}
