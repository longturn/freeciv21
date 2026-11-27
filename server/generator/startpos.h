// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"
#include "map_types.h"

bool create_start_positions(enum map_startpos mode,
                            struct unit_type *initial_unit);
