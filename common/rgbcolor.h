/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// utility
#include "shared.h"

struct section_file;

/* Used for the color system in the client and the definition of the terrain
 * colors used in the overview/map images. The values are read from the
 * rulesets. */
struct color;

/* An RGBcolor contains the R,G,B bitvalues for a color.  The color itself
 * holds the color structure for this color but may be nullptr (it's
 * allocated on demand at runtime). */
struct rgbcolor {
  int r, g, b;
};

// get 'struct color_list' and related functions:
#define SPECLIST_TAG rgbcolor
#define SPECLIST_TYPE struct rgbcolor
#include "speclist.h"

#define rgbcolor_list_iterate(rgbcolorlist, prgbcolor)                      \
  TYPED_LIST_ITERATE(struct rgbcolor, rgbcolorlist, prgbcolor)
#define rgbcolor_list_iterate_end LIST_ITERATE_END

/* Check the RGB color values. If a value is not in the interval [0, 255]
 * clip it to the interval boundaries. */
#define CHECK_RGBCOLOR(_str, _c, _colorname)                                \
  {                                                                         \
    int _color_save = _c;                                                   \
                                                                            \
    _c = CLIP(0, _c, 255);                                                  \
    if (_c != _color_save) {                                                \
      qCritical("Invalid value for '%s' in color definition '%s' (%d). "    \
                "Setting it to '%d'.",                                      \
                _colorname, _str, _color_save, _c);                         \
    }                                                                       \
  }
#define rgbcolor_check(_str, _r, _g, _b)                                    \
  {                                                                         \
    CHECK_RGBCOLOR(_str, _r, "red");                                        \
    CHECK_RGBCOLOR(_str, _g, "green");                                      \
    CHECK_RGBCOLOR(_str, _b, "blue");                                       \
  }

struct rgbcolor *rgbcolor_new(int r, int g, int b);
struct rgbcolor *rgbcolor_copy(const struct rgbcolor *prgbcolor);
bool rgbcolors_are_equal(const struct rgbcolor *c1,
                         const struct rgbcolor *c2);
void rgbcolor_destroy(struct rgbcolor *prgbcolor);

bool rgbcolor_load(struct section_file *file, struct rgbcolor **prgbcolor,
                   const char *path, ...)
    fc__attribute((__format__(__printf__, 3, 4)));
void rgbcolor_save(struct section_file *file,
                   const struct rgbcolor *prgbcolor, const char *path, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

bool rgbcolor_to_hex(const struct rgbcolor *prgbcolor, char *hex,
                     size_t hex_len);
bool rgbcolor_from_hex(struct rgbcolor **prgbcolor, const char *hex);

int rgbcolor_brightness_score(struct rgbcolor *prgbcolor);
