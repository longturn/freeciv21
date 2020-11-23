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

#ifndef FC__AGENTS_H
#define FC__AGENTS_H

#include "support.h" /* bool type */

#include "attribute.h"
#include "fc_types.h"
#include "city.h"
#include <QSet>

/*
 * Besides callback for convenience client/agents/agents also
 * implements a "flattening" of the call stack i.e. to ensure that
 * every agent is only called once at any time.
 */

/* Don't use the very last level unless you know what you're doing */
#define LAST_AGENT_LEVEL 99

#define MAX_AGENT_NAME_LEN 10

enum callback_type { CB_NEW, CB_REMOVE, CB_CHANGE, CB_LAST };

class governor {
  ~governor();
  static governor *i();
  static void drop();
  bool hot() { return superhot; };
  void freeze() { --superhot; };
  void unfreeze() { ++superhot; };
  void add_city_changed(struct city *pcity);
  void add_city_new(struct city *pcity);
  void add_city_remove(struct city *pcity);
private:
  governor() { superhot = 1; };
  static governor *m_instance;
  QSet<struct city*> city_changed;
  QSet<struct city*> city_new;
  QSet<struct city*> city_remove;
  int superhot;
};

struct agent {
  char name[MAX_AGENT_NAME_LEN];
  int level;

  void (*city_callbacks[CB_LAST])(int);
};

void agents_init(void);
void agents_free(void);
void register_agent(const struct agent *agent);
bool agents_busy(void);

/* called from client/packhand.c */
void agents_disconnect(void);
void agents_processing_started(void);
void agents_processing_finished(void);
void agents_freeze_hint(void);
void agents_thaw_hint(void);
void agents_game_joined(void);

void agents_city_changed(struct city *pcity);
void agents_city_new(struct city *pcity);
void agents_city_remove(struct city *pcity);

/* called from agents */
void cause_a_city_changed_for_agent(const char *name_of_calling_agent,
                                    struct city *pcity);
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

#endif /* FC__AGENTS_H */
