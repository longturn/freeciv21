// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

#define SPECENUM_NAME trait
#define SPECENUM_VALUE0 TRAIT_EXPANSIONIST
#define SPECENUM_VALUE0NAME "Expansionist"
#define SPECENUM_VALUE1 TRAIT_TRADER
#define SPECENUM_VALUE1NAME "Trader"
#define SPECENUM_VALUE2 TRAIT_AGGRESSIVE
#define SPECENUM_VALUE2NAME "Aggressive"
#define SPECENUM_VALUE3 TRAIT_BUILDER
#define SPECENUM_VALUE3NAME "Builder"
#define SPECENUM_COUNT TRAIT_COUNT
#include "specenum_gen.h" // IWYU pragma: keep

#define TRAIT_DEFAULT_VALUE 50
#define TRAIT_MAX_VALUE (TRAIT_DEFAULT_VALUE * TRAIT_DEFAULT_VALUE)
#define TRAIT_MAX_VALUE_SR (TRAIT_DEFAULT_VALUE)

struct ai_trait {
  int val; // Value assigned in the beginning
  int mod; // This is modification that changes during game.
};

struct trait_limits {
  int min;
  int max;
  int fixed;
};
