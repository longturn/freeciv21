/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <cstddef>

/*
 * If 4 byte wide signed int is used this gives 20 object types with
 * 100 million keys each.
 */
enum attr_object_type_start_keys { ATTR_CITY_START = 1 * 100 * 1000 * 1000 };

enum attr_city {
  ATTR_CITY_CMA_PARAMETER = ATTR_CITY_START,
  ATTR_CITY_CMAFE_PARAMETER
};

/*
 * Generic methods.
 */
void attribute_init();
void attribute_free();
void attribute_flush();
void attribute_restore();
void attribute_set(int key, int id, int x, int y, size_t data_length,
                   const void *const data);
size_t attribute_get(int key, int id, int x, int y, size_t max_data_length,
                     void *data);

/*
 * Special methods for cities.
 */
void attr_city_set(enum attr_city what, int city_id, size_t data_length,
                   const void *const data);
size_t attr_city_get(enum attr_city what, int city_id,
                     size_t max_data_length, void *data);
