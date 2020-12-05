/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#ifndef FC__BASE_H
#define FC__BASE_H



/* utility */
#include "bitvector.h"

/* common */
#include "fc_types.h"
#include "requirements.h"

struct strvec; /* Actually defined in "utility/string_vector.h". */

/* Used in the network protocol. */
#define SPECENUM_NAME base_gui_type
#define SPECENUM_VALUE0 BASE_GUI_FORTRESS
#define SPECENUM_VALUE0NAME "Fortress"
#define SPECENUM_VALUE1 BASE_GUI_AIRBASE
#define SPECENUM_VALUE1NAME "Airbase"
#define SPECENUM_VALUE2 BASE_GUI_OTHER
#define SPECENUM_VALUE2NAME "Other"
#include "specenum_gen.h"

/* Used in the network protocol. */
#define SPECENUM_NAME base_flag_id
/* Unit inside are not considered aggressive if base is close to city */
#define SPECENUM_VALUE0 BF_NOT_AGGRESSIVE
/* TRANS: this and following strings are 'base flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("NoAggressive")

#define SPECENUM_COUNT BF_COUNT
#define SPECENUM_BITVECTOR bv_base_flags
#include "specenum_gen.h"

struct extra_type;

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

/* General base accessor functions. */
Base_type_id base_count(void);
Base_type_id base_number(const struct base_type *pbase);

struct base_type *base_by_number(const Base_type_id id);

struct extra_type *base_extra_get(const struct base_type *pbase);

/* Functions to operate on a base flag. */
bool base_has_flag(const struct base_type *pbase, enum base_flag_id flag);
bool is_base_flag_card_near(const struct tile *ptile,
                            enum base_flag_id flag);
bool is_base_flag_near_tile(const struct tile *ptile,
                            enum base_flag_id flag);
bool base_flag_is_retired(enum base_flag_id flag);
bool base_has_flag_for_utype(const struct base_type *pbase,
                             enum base_flag_id flag,
                             const struct unit_type *punittype);

/* Ancillary functions */
bool can_build_base(const struct unit *punit, const struct base_type *pbase,
                    const struct tile *ptile);
bool player_can_build_base(const struct base_type *pbase,
                           const struct player *pplayer,
                           const struct tile *ptile);

struct base_type *get_base_by_gui_type(enum base_gui_type type,
                                       const struct unit *punit,
                                       const struct tile *ptile);

bool territory_claiming_base(const struct base_type *pbase);

/* Initialization and iteration */
void base_type_init(struct extra_type *pextra, int idx);
void base_types_free(void);

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



#endif /* FC__BASE_H */
