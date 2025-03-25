// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// std
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
