// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

struct base_type {
  Base_type_id item_number;
  enum base_gui_type gui_type;
  int border_sq;
  int vision_main_sq;
  int vision_invis_sq;
  int vision_subs_sq;

  bv_base_flags flags;

  struct extra_type *self;
};

#define BASE_NONE -1

// General base accessor functions.
Base_type_id base_count();
Base_type_id base_number(const struct base_type *pbase);

struct base_type *base_by_number(const Base_type_id id);

struct extra_type *base_extra_get(const struct base_type *pbase);

// Functions to operate on a base flag.
bool base_has_flag(const struct base_type *pbase, enum base_flag_id flag);
bool is_base_flag_card_near(const struct tile *ptile,
                            enum base_flag_id flag);
bool is_base_flag_near_tile(const struct tile *ptile,
                            enum base_flag_id flag);
bool base_flag_is_retired(enum base_flag_id flag);
bool base_has_flag_for_utype(const struct base_type *pbase,
                             enum base_flag_id flag,
                             const struct unit_type *punittype);

// Ancillary functions
bool can_build_base(const struct unit *punit, const struct base_type *pbase,
                    const struct tile *ptile);

struct base_type *get_base_by_gui_type(enum base_gui_type type,
                                       const struct unit *punit,
                                       const struct tile *ptile);

bool territory_claiming_base(const struct base_type *pbase);

// Initialization and iteration
void base_type_init(struct extra_type *pextra, int idx);
void base_types_free();

#define base_deps_iterate(_reqs, _dep)                                      \
  {                                                                         \
    requirement_vector_iterate(_reqs, preq)                                 \
    {                                                                       \
      if (preq->source.kind == VUT_EXTRA && preq->present                   \
          && is_extra_caused_by(preq->source.value.extra, EC_BASE)) {       \
        struct base_type *_dep = extra_base_get(preq->source.value.extra);

#define base_deps_iterate_end                                               \
  }                                                                         \
  }                                                                         \
  requirement_vector_iterate_end;                                           \
  }
