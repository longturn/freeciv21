// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "bitvector.h"

// common
#include "fc_types.h"
#include "name_translation.h" // struct name_translation
#include "requirements.h"     // struct requirement_vector

// Qt
#include <QtContainerFwd> // QVector<QString>

// std
#include <cstddef> // size_t

#define MAX_NUM_USER_TECH_FLAGS (TECH_USER_LAST - TECH_USER_1 + 1)

enum tech_req { AR_ONE = 0, AR_TWO = 1, AR_ROOT = 2, AR_SIZE };

struct tech_class {
  int idx;
  struct name_translation name;
  bool ruledit_disabled;
  int cost_pct;
};

struct advance {
  Tech_type_id item_number;
  struct name_translation name;
  char graphic_str[MAX_LEN_NAME]; // which named sprite to use
  char graphic_alt[MAX_LEN_NAME]; // alternate icon name
  struct tech_class *tclass;

  struct advance *require[AR_SIZE];
  bool inherited_root_req;

  /* Required to start researching this tech. For shared research it must
   * be fulfilled for at least one player that shares the research. */
  struct requirement_vector research_reqs;

  bv_tech_flags flags;
  QVector<QString> *helptext;

  /*
   * Message displayed to the first player to get a bonus tech
   */
  char *bonus_message;

  /* Cost of advance in bulbs. It may be specified in ruleset, or
   * calculated in techs_precalc_data(). However, this value wouldn't
   * be right if game.info.tech_cost_style is TECH_COST_CIV1CIV2. */
  double cost;

  /*
   * Number of requirements this technology has _including_
   * itself. Precalculated at server then send to client.
   */
  int num_reqs;
};

BV_DEFINE(bv_techs, A_LAST);

/* General advance/technology accessor functions. */
Tech_type_id advance_count();
Tech_type_id advance_index(const struct advance *padvance);
Tech_type_id advance_number(const struct advance *padvance);

struct advance *advance_by_number(const Tech_type_id atype);

struct advance *valid_advance(struct advance *padvance);
struct advance *valid_advance_by_number(const Tech_type_id atype);

struct advance *advance_by_rule_name(const char *name);
struct advance *advance_by_translated_name(const char *name);

const char *advance_rule_name(const struct advance *padvance);
const char *advance_name_translation(const struct advance *padvance);

void tech_classes_init();
struct tech_class *tech_class_by_number(const int idx);
#define tech_class_index(_ptclass_) (_ptclass_)->idx
const char *tech_class_name_translation(const struct tech_class *ptclass);
const char *tech_class_rule_name(const struct tech_class *ptclass);
struct tech_class *tech_class_by_rule_name(const char *name);

#define tech_class_iterate(_p)                                              \
  {                                                                         \
    int _i_##_p;                                                            \
    for (_i_##_p = 0; _i_##_p < game.control.num_tech_classes; _i_##_p++) { \
      struct tech_class *_p = tech_class_by_number(_i_##_p);

#define tech_class_iterate_end                                              \
  }                                                                         \
  }

#define tech_class_re_active_iterate(_p)                                    \
  tech_class_iterate(_p)                                                    \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define tech_class_re_active_iterate_end                                    \
  }                                                                         \
  }                                                                         \
  tech_class_iterate_end;

void user_tech_flags_init();
void user_tech_flags_free();
void set_user_tech_flag_name(enum tech_flag_id id, const char *name,
                             const char *helptxt);
const char *tech_flag_helptxt(enum tech_flag_id id);

/* General advance/technology flag accessor routines */
bool advance_has_flag(Tech_type_id tech, enum tech_flag_id flag);

// Ancillary routines
Tech_type_id advance_required(const Tech_type_id tech,
                              enum tech_req require);
struct advance *advance_requires(const struct advance *padvance,
                                 enum tech_req require);

bool techs_have_fixed_costs();

bool is_future_tech(Tech_type_id tech);

// Initialization
void techs_init();
void techs_free();

void techs_precalc_data();

// Iteration

/* This iterates over almost all technologies.  It includes non-existent
 * technologies, but not A_FUTURE. */
#define advance_index_iterate(_start, _index)                               \
  {                                                                         \
    Tech_type_id _index = (_start);                                         \
    for (; _index < advance_count(); _index++) {

#define advance_index_iterate_end                                           \
  }                                                                         \
  }

const struct advance *advance_array_last();

#define advance_iterate(_start, _p)                                         \
  {                                                                         \
    struct advance *_p = advance_by_number(_start);                         \
    if (nullptr != _p) {                                                    \
      for (; _p <= advance_array_last(); _p++) {

#define advance_iterate_end                                                 \
  }                                                                         \
  }                                                                         \
  }

#define advance_re_active_iterate(_p)                                       \
  advance_iterate(A_FIRST, _p)                                              \
  {                                                                         \
    if (_p->require[AR_ONE] != A_NEVER) {

#define advance_re_active_iterate_end                                       \
  }                                                                         \
  }                                                                         \
  advance_iterate_end;

/* Advance requirements iterator.
 * Iterates over 'goal' and all its requirements (including root_reqs),
 * recursively. */
struct advance_req_iter;

size_t advance_req_iter_sizeof();
struct iterator *advance_req_iter_init(struct advance_req_iter *it,
                                       const struct advance *goal);

#define advance_req_iterate(_goal, _padvance)                               \
  generic_iterate(struct advance_req_iter, const struct advance *,          \
                  _padvance, advance_req_iter_sizeof,                       \
                  advance_req_iter_init, _goal)
#define advance_req_iterate_end generic_iterate_end

/* Iterates over all the root requirements of 'goal'.
 * (Not including 'goal' itself, unless it is the special case of a
 * self-root-req technology.) */
struct advance_root_req_iter;

size_t advance_root_req_iter_sizeof();
struct iterator *advance_root_req_iter_init(struct advance_root_req_iter *it,
                                            const struct advance *goal);

#define advance_root_req_iterate(_goal, _padvance)                          \
  generic_iterate(struct advance_root_req_iter, const struct advance *,     \
                  _padvance, advance_root_req_iter_sizeof,                  \
                  advance_root_req_iter_init, _goal)
#define advance_root_req_iterate_end generic_iterate_end
