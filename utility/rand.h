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

/* Utility */
#include "support.h" /* bool type */

typedef uint32_t RANDOM_TYPE;

typedef struct {
  RANDOM_TYPE v[56];
  int j, k, x;
  bool is_init; /* initially 0 for static storage */
} RANDOM_STATE;

#define fc_rand(_size)                                                      \
  fc_rand_debug((_size), "fc_rand", __FC_LINE__, __FILE__)

RANDOM_TYPE fc_rand_debug(RANDOM_TYPE size, const char *called_as, int line,
                          const char *file);

void fc_srand(RANDOM_TYPE seed);

void fc_rand_uninit();
bool fc_rand_is_init();
RANDOM_STATE fc_rand_state();
void fc_rand_set_state(RANDOM_STATE &state);

void test_random1(int n);

/*===*/

#define fc_randomly(_seed, _size)                                           \
  fc_randomly_debug((_seed), (_size), "fc_randomly", __FC_LINE__, __FILE__)

RANDOM_TYPE fc_randomly_debug(RANDOM_TYPE seed, RANDOM_TYPE size,
                              const char *called_as, int line,
                              const char *file);
