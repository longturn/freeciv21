// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"
#include "path_finding.h"
#include "tile.h"
#include "unit.h"
#include "unittype.h"

/*
 * WAGs: how hard to avoid tall stacks of units.
 * Pass as fearfulness values to adv_avoid_risks.
 */
#define NORMAL_STACKING_FEARFULNESS ((double) PF_TURN_FACTOR / 36.0)

struct adv_risk_cost {
  double base_value;
  double fearfulness;
  double enemy_zoc_cost;
};

bool adv_follow_path(struct unit *punit, const PFPath &path,
                     struct tile *ptile);

bool adv_unit_execute_path(struct unit *punit, const PFPath &path);

int adv_could_unit_move_to_tile(struct unit *punit, struct tile *dst_tile);

bool adv_danger_at(struct unit *punit, struct tile *ptile);

void adv_avoid_risks(struct pf_parameter *parameter,
                     struct adv_risk_cost *risk_cost, struct unit *punit,
                     const double fearfulness);

int adv_unittype_att_rating(const struct unit_type *punittype, int veteran,
                            int moves_left, int hp);
int adv_unit_att_rating(const struct unit *punit);
int adv_unit_def_rating_basic(const struct unit *punit);
int adv_unit_def_rating_basic_squared(const struct unit *punit);
