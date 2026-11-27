// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

/*
 *  temperature_map[] stores the temperature of each tile
 *  values on tmap can get one of these 4 values
 *  there is 4 extra values as macros combining the 4 basics ones
 */
typedef int temperature_type;

#define TT_FROZEN 1
#define TT_COLD 2
#define TT_TEMPERATE 4
#define TT_TROPICAL 8

#define TT_NFROZEN (TT_COLD | TT_TEMPERATE | TT_TROPICAL)
#define TT_ALL (TT_FROZEN | TT_NFROZEN)
#define TT_NHOT (TT_FROZEN | TT_COLD)
#define TT_HOT (TT_TEMPERATE | TT_TROPICAL)

bool temperature_is_initialized();
bool tmap_is(const struct tile *ptile, temperature_type tt);
bool is_temperature_type_near(const struct tile *ptile, temperature_type tt);
void destroy_tmap();
void create_tmap(bool real);
