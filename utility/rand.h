/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <random>

#define fc_rand(_size) fc_rand_debug((_size), "fc_rand", __LINE__, __FILE__)

std::uint_fast32_t fc_rand_debug(std::uint_fast32_t size,
                                 const char *called_as, int line,
                                 const char *file);

void fc_srand(std::uint_fast32_t seed);
void fc_rand_seed(std::mt19937 &gen);

bool fc_rand_is_init();
void fc_rand_set_init(bool init);

std::mt19937 &fc_rand_state();
void fc_rand_set_state(const std::mt19937 &state);

/*===*/

#define fc_randomly(_seed, _size)                                           \
  fc_randomly_debug((_seed), (_size), "fc_randomly", __LINE__, __FILE__)

std::uint_fast32_t fc_randomly_debug(std::uint_fast32_t seed,
                                     std::uint_fast32_t size,
                                     const char *called_as, int line,
                                     const char *file);
