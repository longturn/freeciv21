/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

/*
 * The purpose of this module is to manage the citizens of a city. The
 * caller has to provide a goal (struct cm_parameter) which determines
 * in which way the citizens are allocated and placed. The module will
 * also avoid disorder.
 *
 * The plan defines a minimal surplus. The module will try to get the
 * required surplus. If there are citizens free after allocation of
 * the minimal surplus these citizens will get arranged to maximize
 * the weighted sum over the surplus of each type.
 */

/* utility */
#include "support.h" /* bool type */

/* common */
#include "city.h" /* CITY_MAP_MAX_SIZE */

/* A description of the goal. */
struct cm_parameter {
  int minimal_surplus[O_LAST];
  bool max_growth;
  bool require_happy;
  bool allow_disorder;
  bool allow_specialists;

  int factor[O_LAST];
  int happy_factor;
};

/* A result which can examined. */
struct cm_result {
  bool aborted;
  bool found_a_valid, disorder, happy;

  int surplus[O_LAST];

  int city_radius_sq;
  bool *worker_positions;
  citizens specialists[SP_MAX];
};

void cm_init(void);
void cm_init_citymap(void);
void cm_free(void);

struct cm_result *cm_result_new(struct city *pcity);
void cm_result_destroy(struct cm_result *result);

/*
 * Will try to meet the requirements and fill out the result. Caller
 * should test result->found_a_valid. cm_query_result() will not change
 * the actual city setting.
 */
void cm_query_result(struct city *pcity,
                     const struct cm_parameter *const parameter,
                     struct cm_result *result, bool negative_ok);

/***************** utility methods *************************************/
bool operator==(const struct cm_parameter &p1,
                const struct cm_parameter &p2);
void cm_copy_parameter(struct cm_parameter *dest,
                       const struct cm_parameter *const src);
void cm_init_parameter(struct cm_parameter *dest);
void cm_init_emergency_parameter(struct cm_parameter *dest);

void cm_print_city(const struct city *pcity);
void cm_print_result(const struct cm_result *result);

int cm_result_citizens(const struct cm_result *result);
int cm_result_specialists(const struct cm_result *result);
int cm_result_workers(const struct cm_result *result);

void cm_result_from_main_map(struct cm_result *result,
                             const struct city *pcity);


