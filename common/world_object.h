// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "city.h"
#include "fc_types.h"
#include "map_types.h"
#include "unit.h"

// Qt
#include <QHash>

struct world {
  struct civ_map map;
  QHash<int, const struct city *> *cities;
  QHash<int, const struct unit *> *units;
};
