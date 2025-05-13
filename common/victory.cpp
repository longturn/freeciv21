// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "victory.h"

// common
#include "fc_types.h"
#include "game.h"

/**
   Whether victory condition is enabled
 */
bool victory_enabled(enum victory_condition_type victory)
{
  return (game.info.victory_conditions & (1 << victory));
}
