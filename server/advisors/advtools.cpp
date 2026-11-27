// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "advtools.h"

// common
#include "fc_types.h"

// std
#include <cmath>

/**
   Amortize means gradually paying off a cost or debt over time. In freeciv
   terms this means we calculate how much less worth something is to us
   depending on how long it will take to complete.

   This is based on a global interest rate as defined by the MORT value.
 */
adv_want amortize(adv_want benefit, int delay)
{
  double discount = 1.0 - 1.0 / (static_cast<double>(MORT));

  /* Note there's no rounding here.  We could round but it would probably
   * be better just to return (and take) a double for the benefit. */
  return benefit * pow(discount, delay);
}
