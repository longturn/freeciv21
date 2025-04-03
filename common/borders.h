// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

bool is_border_source(struct tile *ptile);
int tile_border_source_radius_sq(struct tile *ptile);
int tile_border_source_strength(struct tile *ptile);
int tile_border_strength(struct tile *ptile, struct tile *source);
