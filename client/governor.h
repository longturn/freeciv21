/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "attribute.h"
#include <QSet>

class governor {
public:
  ~governor();
  static governor *i();
  static void drop();
  bool hot() { return superhot; };
  void freeze() { --superhot; };
  void unfreeze() { ++superhot; run();};
  void add_city_changed(struct city *pcity);
  void add_city_new(struct city *pcity);
  void add_city_remove(struct city *pcity);
private:
  governor() { superhot = 1; };
  void run();
  static governor *m_instance;
  QSet<struct city*> scity_changed;
  QSet<struct city*> scity_remove;
  int superhot;
};

void cma_init(void);
bool cma_apply_result(struct city *pcity, const struct cm_result *result);
void cma_put_city_under_agent(struct city *pcity,
                              const struct cm_parameter *const parameter);
void cma_release_city(struct city *pcity);
bool cma_is_city_under_agent(const struct city *pcity,
                             struct cm_parameter *parameter);

/***************** utility methods *************************************/
bool cma_get_parameter(enum attr_city attr, int city_id,
                       struct cm_parameter *parameter);
void cma_set_parameter(enum attr_city attr, int city_id,
                       const struct cm_parameter *parameter);

int cities_results_request();
void cma_got_result(int);

void cmafec_init(void);
void cmafec_free(void);

void cmafec_set_fe_parameter(struct city *pcity,
                             const struct cm_parameter *const parameter);
void cmafec_get_fe_parameter(struct city *pcity, struct cm_parameter *dest);

const char *
cmafec_get_short_descr(const struct cm_parameter *const parameter);
const char *cmafec_get_short_descr_of_city(const struct city *pcity);
const char *
cmafec_get_result_descr(struct city *pcity, const struct cm_result *result,
                        const struct cm_parameter *const parameter);

/*
 * Preset handling
 */
void cmafec_preset_add(const char *descr_name, struct cm_parameter *pparam);
void cmafec_preset_remove(int idx);
int cmafec_preset_get_index_of_parameter(
    const struct cm_parameter *const parameter);
char *cmafec_preset_get_descr(int idx);
const struct cm_parameter *cmafec_preset_get_parameter(int idx);
int cmafec_preset_num(void);
void create_default_cma_presets(void);

