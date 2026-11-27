// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

#define MORT 24

adv_want amortize(adv_want benefit, int delay);

/*
 * To prevent integer overflows the product "power * hp * firepower"
 * is divided by POWER_DIVIDER.
 *
 */
#define POWER_DIVIDER (POWER_FACTOR * 3)
