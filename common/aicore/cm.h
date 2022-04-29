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

// common
#include "city.h" // CITY_MAP_MAX_SIZE

// A description of the goal.
struct cm_parameter {
  int minimal_surplus[O_LAST];
  bool max_growth;
  bool require_happy;
  bool allow_disorder;
  bool allow_specialists;

  int factor[O_LAST];
  int happy_factor;

  cm_parameter &operator=(const cm_parameter &other)
  {
    max_growth = other.max_growth;
    require_happy = other.require_happy;
    allow_disorder = other.allow_disorder;
    allow_specialists = other.allow_specialists;
    happy_factor = other.happy_factor;

    for (int i = 0; i < O_LAST; ++i) {
      minimal_surplus[i] = other.minimal_surplus[i];
      factor[i] = other.factor[i];
    }
    return *this;
  }
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

std::unique_ptr<cm_result> cm_result_new(city *pcity);

/*
 * Will try to meet the requirements and fill out the result. Caller
 * should test result->found_a_valid. cm_query_result() will not change
 * the actual city setting.
 */
void cm_query_result(city *pcity, const cm_parameter *const parameter,
                     std::unique_ptr<cm_result> &result, bool negative_ok);

/***************** utility methods *************************************/
bool operator==(const cm_parameter &p1, const cm_parameter &p2);
void cm_copy_parameter(cm_parameter *dest, const cm_parameter *const src);
void cm_init_parameter(cm_parameter *dest);
void cm_init_emergency_parameter(cm_parameter *dest);

void cm_print_city(const city *pcity);
void cm_print_result(const std::unique_ptr<cm_result> &result);

int cm_result_citizens(const std::unique_ptr<cm_result> &result);
int cm_result_specialists(const std::unique_ptr<cm_result> &result);
int cm_result_workers(const std::unique_ptr<cm_result> &result);

void cm_result_from_main_map(std::unique_ptr<cm_result> &result,
                             const city *pcity);
