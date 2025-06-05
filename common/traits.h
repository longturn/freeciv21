// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

struct ai_trait {
  int val; // Value assigned in the beginning
  int mod; // This is modification that changes during game.
};

struct trait_limits {
  int min;
  int max;
  int fixed;
};
