// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

// std
#include <memory>
#include <vector>

// A description of the goal.
struct cm_parameter {
  int minimal_surplus[O_LAST];
  bool max_growth;
  bool require_happy;
  bool allow_disorder;
  bool allow_specialists;

  int factor[O_LAST];
  int happy_factor;

  /**
   * @brief Copy assignment operator
   * @param other Object being copied
   * @return Reference to *this
   */
  cm_parameter &operator=(const cm_parameter &other) = default;
};

// A result which can examined.
struct cm_result {
  bool aborted;
  bool found_a_valid, disorder, happy;

  int surplus[O_LAST];

  int city_radius_sq;
  std::vector<bool> worker_positions;
  citizens specialists[SP_MAX];

  ~cm_result() = default;
};

void cm_init();
void cm_init_citymap();
void cm_free();

std::unique_ptr<cm_result> cm_result_new(struct city *pcity);

/*
 * Will try to meet the requirements and fill out the result. Caller
 * should test result->found_a_valid. cm_query_result() will not change
 * the actual city setting.
 */
void cm_query_result(struct city *pcity,
                     const struct cm_parameter *const parameter,
                     std::unique_ptr<cm_result> &result, bool negative_ok);

/***************** utility methods *************************************/
bool operator==(const struct cm_parameter &p1,
                const struct cm_parameter &p2);
void cm_copy_parameter(struct cm_parameter *dest,
                       const struct cm_parameter *const src);
void cm_init_parameter(struct cm_parameter *dest);
void cm_init_emergency_parameter(struct cm_parameter *dest);

void cm_print_city(const struct city *pcity);
void cm_print_result(const std::unique_ptr<cm_result> &result);

int cm_result_citizens(const std::unique_ptr<cm_result> &result);
int cm_result_specialists(const std::unique_ptr<cm_result> &result);
int cm_result_workers(const std::unique_ptr<cm_result> &result);

void cm_result_from_main_map(std::unique_ptr<cm_result> &result,
                             const struct city *pcity);
