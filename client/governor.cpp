/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
  Copyright (c) 1996-2020 Freeciv21 & Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  0R (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include <QElapsedTimer>

// utility
#include "bugs.h"
#include "fciconv.h"

// common
#include "city.h"
#include "dataio.h"
#include "featured_text.h"
#include "nation.h"
#include "specialist.h"
// client
#include "attribute.h"
#include "citydlg_common.h"
#include "client_main.h"
#include "climisc.h"

// include
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "mapctrl_g.h"

#include "governor.h"

#define log_apply_result log_debug
#define log_handle_city log_debug
#define log_handle_city2 log_debug
#define log_results_are_equal log_debug

#define SHOW_APPLY_RESULT_ON_SERVER_ERRORS false
#define ALWAYS_APPLY_AT_SERVER false
#define MAX_LEN_PRESET_NAME 80
#define SAVED_PARAMETER_SIZE 32

#define CMA_NUM_PARAMS 5

#define SPECLIST_TAG preset
#define SPECLIST_TYPE struct cma_preset
#include "speclist.h"

static struct preset_list *preset_list = nullptr;

static void city_remove(int city_id)
{
  attr_city_set(ATTR_CITY_CMA_PARAMETER, city_id, 0, nullptr);
}

struct cma_preset {
  char *descr;
  struct cm_parameter parameter;
};

static struct {
  int apply_result_ignored, apply_result_applied, refresh_forced;
} stats;

governor *governor::m_instance = nullptr;

// yolo class
class cma_yoloswag {
public:
  cma_yoloswag();
  ~cma_yoloswag();
  void put_city_under_agent(struct city *pcity,
                            const struct cm_parameter *const parameter);
  void release_city(struct city *pcity);
  bool is_city_under_agent(const struct city *pcity,
                           struct cm_parameter *parameter);
  bool get_parameter(enum attr_city attr, int city_id,
                     struct cm_parameter *parameter);
  void set_parameter(enum attr_city attr, int city_id,
                     const struct cm_parameter *parameter);
  void handle_city(struct city *pcity);
  int get_request();
  void result_came_from_server(int request);

private:
  struct city *check_city(int city_id, struct cm_parameter *parameter);
  bool apply_result_on_server(struct city *pcity,
                              std::unique_ptr<cm_result> &&result);
  std::unique_ptr<cm_result> cma_result_got;
  int last_request;
  struct city *xcity;
};

// gimb means "governor is my bitch"
Q_GLOBAL_STATIC(cma_yoloswag, gimb)

// deletes governor
void governor::drop()
{
  delete m_instance;
  m_instance = nullptr;
}

governor::~governor() = default;

// instance for governor
governor *governor::i()
{
  if (m_instance == nullptr) {
    m_instance = new governor;
  }
  return m_instance;
}

// register new event and run it if hot
void governor::add_city_changed(struct city *pcity)
{
  scity_changed.insert(pcity);
  run();
};

void governor::add_city_new(struct city *pcity)
{
  scity_changed.insert(pcity);
  run();
};

void governor::add_city_remove(struct city *pcity)
{
  scity_remove.insert(pcity);
  run();
};

// run all events
void governor::run()
{
  if (superhot < 1 || !client.conn.playing) {
    return;
  }

  for (auto *pcity : qAsConst(scity_changed)) {
    // dont check city if its not ours, asan says
    // city was removed, but city still points to something
    // uncomment and check whats happening when city is conquered
    bool dontCont = false;
    city_list_iterate(client.conn.playing->cities, wtf)
    {
      if (wtf == pcity) {
        dontCont = true;
      }
    }
    city_list_iterate_end;

    if (!dontCont) {
      continue;
    }
    if (pcity) {
      gimb->handle_city(pcity);
    }
  }
  scity_changed.clear();
  for (auto *pcity : qAsConst(scity_remove)) {
    if (pcity) {
      attr_city_set(ATTR_CITY_CMAFE_PARAMETER, pcity->id, 0, nullptr);
      city_remove(pcity->id);
    }
  }
  scity_remove.clear();
  update_turn_done_button_state();
}

inline bool operator==(const struct cm_result &result1,
                       const struct cm_result &result2)
{
  if ((result1.disorder != result2.disorder)
      || (result1.happy != result2.happy)) {
    return false;
  }

  specialist_type_iterate(sp)
  {
    if (result1.specialists[sp] != result2.specialists[sp]) {
      return false;
    }
  }
  specialist_type_iterate_end;

  output_type_iterate(ot)
  {
    if (result1.surplus[ot] != result2.surplus[ot]) {
      return false;
    }
  }
  output_type_iterate_end;

  fc_assert_ret_val(result1.city_radius_sq == result2.city_radius_sq, false);
  city_map_iterate(result1.city_radius_sq, cindex, x, y)
  {
    if (is_city_center_index(cindex)) {
      continue;
    }

    if (result1.worker_positions[cindex]
        != result2.worker_positions[cindex]) {
      log_results_are_equal("worker_positions");
      return false;
    }
  }
  city_map_iterate_end;

  return true;
}

// yet another abstraction layer
int cities_results_request() { return gimb->get_request(); }
int cma_yoloswag::get_request() { return last_request; }
void cma_got_result(int citynr) { gimb->result_came_from_server(citynr); }

// yolo constructor
cma_yoloswag::cma_yoloswag()
{
  last_request = -9999;
  cma_result_got = nullptr;
  xcity = nullptr;
}

// cma results returned from server to check if everything is legit
void cma_yoloswag::result_came_from_server(int last_request_id)
{
  struct city *pcity = xcity;
  last_request = last_request_id;
  bool success;

  if (last_request_id < 0) {
    return;
  }
  if (last_request_id != 0) {
    int city_id = pcity->id;

    if (pcity != check_city(city_id, nullptr)) {
      qDebug("apply_result_on_server(city %d) !check_city()!", city_id);
      return;
    }
  }

  // Return.
  auto state_result = cm_result_new(pcity);
  cm_result_from_main_map(state_result, pcity);

  success = (cma_result_got && *state_result == *cma_result_got);
  if (!success) {
#if SHOW_APPLY_RESULT_ON_SERVER_ERRORS
    qCritical("apply_result_on_server(city %d=\"%s\") no match!", pcity->id,
              city_name_get(pcity));

    log_test("apply_result_on_server(city %d=\"%s\") have:", pcity->id,
             city_name_get(pcity));
    cm_print_city(pcity);
    cm_print_result(state_result);

    log_test("apply_result_on_server(city %d=\"%s\") want:", pcity->id,
             city_name_get(pcity));
    cm_print_result(cma_result_got);
#endif // SHOW_APPLY_RESULT_ON_SERVER_ERRORS
  }
  cma_result_got = nullptr;
  log_apply_result("apply_result_on_server() return %d.", (int) success);
  last_request = -9999;
  xcity = nullptr;
}

cma_yoloswag::~cma_yoloswag() = default;

/**
 * Change the actual city setting to the given result. Returns TRUE iff
 * the actual data matches the calculated one.
 */
bool cma_yoloswag::apply_result_on_server(
    struct city *pcity, std::unique_ptr<cm_result> &&result)
{
  int first_request_id = 0, last_request_id = 0, i;
  int city_radius_sq = city_map_radius_sq_get(pcity);
  struct tile *pcenter = city_tile(pcity);

  fc_assert_ret_val(result->found_a_valid, false);
  auto current_state = cm_result_new(pcity);
  cm_result_from_main_map(current_state, pcity);

  if (*current_state == *result && !ALWAYS_APPLY_AT_SERVER) {
    stats.apply_result_ignored++;
    return true;
  }
  // Do checks
  if (city_size_get(pcity) != cm_result_citizens(result)) {
    qCritical("apply_result_on_server(city %d=\"%s\") bad result!",
              pcity->id, city_name_get(pcity));
    cm_print_city(pcity);
    cm_print_result(result);
    return false;
  }

  stats.apply_result_applied++;

  log_apply_result("apply_result_on_server(city %d=\"%s\")", pcity->id,
                   city_name_get(pcity));

  connection_do_buffer(&client.conn);

  // Remove all surplus workers
  city_tile_iterate_skip_center(city_radius_sq, pcenter, ptile, idx, x, y)
  {
    if (tile_worked(ptile) == pcity && !result->worker_positions[idx]) {
      log_apply_result("Removing worker at {%d,%d}.", x, y);

      last_request_id = dsend_packet_city_make_specialist(
          &client.conn, pcity->id, ptile->index);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }
  city_tile_iterate_skip_center_end;

  // Change the excess non-default specialists to default.
  specialist_type_iterate(sp)
  {
    if (sp == DEFAULT_SPECIALIST) {
      continue;
    }

    for (i = 0; i < pcity->specialists[sp] - result->specialists[sp]; i++) {
      log_apply_result("Change specialist from %d to %d.", sp,
                       DEFAULT_SPECIALIST);
      last_request_id =
          city_change_specialist(pcity, sp, DEFAULT_SPECIALIST);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }
  specialist_type_iterate_end;

  // now all surplus people are DEFAULT_SPECIALIST

  // Set workers
  /* FIXME: This code assumes that any toggled worker will turn into a
   * DEFAULT_SPECIALIST! */
  city_tile_iterate_skip_center(city_radius_sq, pcenter, ptile, idx, x, y)
  {
    if (nullptr == tile_worked(ptile) && result->worker_positions[idx]) {
      log_apply_result("Putting worker at {%d,%d}.", x, y);
      fc_assert_action(city_can_work_tile(pcity, ptile), break);

      last_request_id = dsend_packet_city_make_worker(
          &client.conn, pcity->id, ptile->index);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }
  city_tile_iterate_skip_center_end;

  /* Set all specialists except DEFAULT_SPECIALIST (all the unchanged
   * ones remain as DEFAULT_SPECIALIST). */
  specialist_type_iterate(sp)
  {
    if (sp == DEFAULT_SPECIALIST) {
      continue;
    }

    for (i = 0; i < result->specialists[sp] - pcity->specialists[sp]; i++) {
      log_apply_result("Changing specialist from %d to %d.",
                       DEFAULT_SPECIALIST, sp);
      last_request_id =
          city_change_specialist(pcity, DEFAULT_SPECIALIST, sp);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }
  specialist_type_iterate_end;

  if (last_request_id == 0 || ALWAYS_APPLY_AT_SERVER) {
    /*
     * If last_request is 0 no change request was send. But it also
     * means that the results are different or the fc_results_are_equal()
     * test at the start of the function would be true. So this
     * means that the client has other results for the same
     * allocation of citizen than the server. We just send a
     * PACKET_CITY_REFRESH to bring them in sync.
     */
    first_request_id = dsend_packet_city_refresh(&client.conn, pcity->id);
    last_request_id = first_request_id;
    stats.refresh_forced++;
  }

  connection_do_unbuffer(&client.conn);

  cma_result_got = std::move(result);
  last_request = last_request_id;
  xcity = pcity;
  return true;
}

void cma_yoloswag::put_city_under_agent(
    struct city *pcity, const struct cm_parameter *const parameter)
{
  log_debug("cma_put_city_under_agent(city %d=\"%s\")", pcity->id,
            city_name_get(pcity));
  fc_assert_ret(city_owner(pcity) == client.conn.playing);
  cma_set_parameter(ATTR_CITY_CMA_PARAMETER, pcity->id, parameter);
  governor::i()->add_city_changed(pcity);
  log_debug("cma_put_city_under_agent: return");
}

void cma_yoloswag::release_city(struct city *pcity)
{
  attr_city_set(ATTR_CITY_CMA_PARAMETER, pcity->id, 0, nullptr);
  refresh_city_dialog(pcity);
  city_report_dialog_update_city(pcity);
}

bool cma_yoloswag::is_city_under_agent(const struct city *pcity,
                                       struct cm_parameter *parameter)
{
  struct cm_parameter my_parameter;

  if (!cma_get_parameter(ATTR_CITY_CMA_PARAMETER, pcity->id,
                         &my_parameter)) {
    return false;
  }

  if (parameter) {
    *parameter = my_parameter;
  }
  return true;
}
bool cma_yoloswag::get_parameter(enum attr_city attr, int city_id,
                                 struct cm_parameter *parameter)
{
  size_t len;
  char buffer[SAVED_PARAMETER_SIZE];
  struct data_in din;
  int version, dummy;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. Always add new parameters at the end.
   */

  len = attr_city_get(attr, city_id, sizeof(buffer), buffer);
  if (len == 0) {
    return false;
  }

  dio_input_init(&din, buffer, len);
  fc_assert_ret_val(dio_get_uint8_raw(&din, &version), false);
  fc_assert_ret_val(version == 2, false);

  /* Initialize the parameter (includes some AI-only fields that aren't
   * touched below). */
  cm_init_parameter(parameter);

  output_type_iterate(i)
  {
    fc_assert_ret_val(
        dio_get_sint16_raw(&din, &parameter->minimal_surplus[i]), false);
    fc_assert_ret_val(dio_get_sint16_raw(&din, &parameter->factor[i]),
                      false);
  }
  output_type_iterate_end;

  fc_assert_ret_val(dio_get_sint16_raw(&din, &parameter->happy_factor),
                    false);
  fc_assert_ret_val(dio_get_uint8_raw(&din, &dummy),
                    false); // Dummy value; used to be factor_target.
  fc_assert_ret_val(dio_get_bool8_raw(&din, &parameter->require_happy),
                    false);

  // Optional fields
  dio_get_bool8_raw(&din, &parameter->max_growth);
  dio_get_bool8_raw(&din, &parameter->allow_disorder);
  dio_get_bool8_raw(&din, &parameter->allow_specialists);

  return true;
}

void cma_yoloswag::set_parameter(enum attr_city attr, int city_id,
                                 const struct cm_parameter *parameter)
{
  char buffer[SAVED_PARAMETER_SIZE];
  struct raw_data_out dout;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. */

  dio_output_init(&dout, buffer, sizeof(buffer));

  dio_put_uint8_raw(&dout, 2);

  output_type_iterate(i)
  {
    dio_put_sint16_raw(&dout, parameter->minimal_surplus[i]);
    dio_put_sint16_raw(&dout, parameter->factor[i]);
  }
  output_type_iterate_end;

  dio_put_sint16_raw(&dout, parameter->happy_factor);
  dio_put_uint8_raw(&dout, 0); // Dummy value; used to be factor_target.
  dio_put_bool8_raw(&dout, parameter->require_happy);

  dio_put_bool8_raw(&dout, parameter->max_growth);
  dio_put_bool8_raw(&dout, parameter->allow_disorder);
  dio_put_bool8_raw(&dout, parameter->allow_specialists);

  fc_assert(dio_output_used(&dout) == SAVED_PARAMETER_SIZE);

  attr_city_set(attr, city_id, SAVED_PARAMETER_SIZE, buffer);
}

/**
   Returns TRUE if the city is valid for CMA. Fills parameter if TRUE
   is returned. Parameter can be nullptr.
 */
struct city *cma_yoloswag::check_city(int city_id,
                                      struct cm_parameter *parameter)
{
  struct city *pcity = game_city_by_number(city_id);
  struct cm_parameter dummy;

  if (!parameter) {
    parameter = &dummy;
  }

  if (!pcity
      || !cma_get_parameter(ATTR_CITY_CMA_PARAMETER, city_id, parameter)) {
    return nullptr;
  }

  if (city_owner(pcity) != client.conn.playing) {
    cma_release_city(pcity);
    return nullptr;
  }

  return pcity;
}

/**
   The given city has changed. handle_city ensures that either the city
   follows the set CMA goal or that the CMA detaches itself from the
   city.
 */
void cma_yoloswag::handle_city(struct city *pcity)
{
  auto result = cm_result_new(pcity);
  bool handled;
  int i, city_id = pcity->id;

  log_handle_city("handle_city(city %d=\"%s\") pos=(%d,%d) owner=%s",
                  pcity->id, city_name_get(pcity), TILE_XY(pcity->tile),
                  nation_rule_name(nation_of_city(pcity)));

  log_handle_city2("START handle city %d=\"%s\"", pcity->id,
                   city_name_get(pcity));

  handled = false;
  for (i = 0; i < 5; i++) {
    struct cm_parameter parameter;

    log_handle_city2("  try %d", i);

    if (pcity != check_city(city_id, &parameter)) {
      handled = true;
      break;
    }

    cm_query_result(pcity, &parameter, result, false);
    if (!result->found_a_valid) {
      log_handle_city2("  no valid found result");

      cma_release_city(pcity);

      create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                   _("The citizen governor can't fulfill the requirements "
                     "for %s. Passing back control."),
                   city_link(pcity));
      handled = true;
      break;
    } else {
      if (!apply_result_on_server(pcity, std::move(result))) {
        log_handle_city2("  doesn't cleanly apply");
        if (pcity == check_city(city_id, nullptr) && i == 0) {
          create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                       _("The citizen governor has gotten confused dealing "
                         "with %s.  You may want to have a look."),
                       city_link(pcity));
        }
      } else {
        log_handle_city2("  ok");
        // Everything ok
        handled = true;
        break;
      }
      result = nullptr;
    }
  }

  if (!handled) {
    fc_assert_ret(pcity == check_city(city_id, nullptr));
    log_handle_city2("  not handled");

    create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                 _("The citizen governor has gotten confused dealing "
                   "with %s.  You may want to have a look."),
                 city_link(pcity));

    cma_release_city(pcity);

    qCInfo(bugs_category,
           "handle_city() CMA: %s has changed multiple times.",
           city_name_get(pcity));
  }

  log_handle_city2("END handle city=(%d)", city_id);
}

/**
   Put city under governor control
 */
void cma_put_city_under_agent(struct city *pcity,
                              const struct cm_parameter *const parameter)
{
  gimb->put_city_under_agent(pcity, parameter);
}

/**
   Release city from governor control.
 */
void cma_release_city(struct city *pcity) { gimb->release_city(pcity); }

/**
   Check whether city is under governor control, and fill parameter if it is.
 */
bool cma_is_city_under_agent(const struct city *pcity,
                             struct cm_parameter *parameter)
{
  return gimb->is_city_under_agent(pcity, parameter);
}

/**
   Get the parameter.

   Don't bother to cm_init_parameter, since we set all the fields anyway.
   But leave the comment here so we can find this place when searching
   for all the creators of a parameter.
 */
bool cma_get_parameter(enum attr_city attr, int city_id,
                       struct cm_parameter *parameter)
{
  return gimb->get_parameter(attr, city_id, parameter);
}

/**
   Set attribute block for city from parameter.
 */
void cma_set_parameter(enum attr_city attr, int city_id,
                       const struct cm_parameter *parameter)
{
  gimb->set_parameter(attr, city_id, parameter);
}

/**
   Sets the front-end parameter.
 */
void cmafec_set_fe_parameter(struct city *pcity,
                             const struct cm_parameter *const parameter)
{
  cma_set_parameter(ATTR_CITY_CMAFE_PARAMETER, pcity->id, parameter);
}

/**
   Return the front-end parameter for the given city. Returns a dummy
   parameter if no parameter was set.
 */
void cmafec_get_fe_parameter(struct city *pcity, struct cm_parameter *dest)
{
  struct cm_parameter parameter;

  if (cma_is_city_under_agent(pcity, &parameter)) {
    cm_copy_parameter(dest, &parameter);
  } else {
    // Create a dummy parameter to return.
    cm_init_parameter(dest);
    if (!cma_get_parameter(ATTR_CITY_CMAFE_PARAMETER, pcity->id, dest)) {
      // We haven't seen this city before; store the dummy.
      cmafec_set_fe_parameter(pcity, dest);
    }
  }
}

/**
   Adds a preset.
 */
void cmafec_preset_add(const char *descr_name, const cm_parameter *pparam)
{
  struct cma_preset *ppreset = new cma_preset;

  if (preset_list == nullptr) {
    preset_list = preset_list_new();
  }

  cm_copy_parameter(&ppreset->parameter, pparam);
  ppreset->descr = new char[MAX_LEN_PRESET_NAME];
  (void) fc_strlcpy(ppreset->descr, descr_name, MAX_LEN_PRESET_NAME);
  preset_list_prepend(preset_list, ppreset);
}

/**
   Removes a preset.
 */
void cmafec_preset_remove(int idx)
{
  struct cma_preset *ppreset;

  fc_assert_ret(idx >= 0 && idx < cmafec_preset_num());

  ppreset = preset_list_get(preset_list, idx);
  preset_list_remove(preset_list, ppreset);

  delete[] ppreset->descr;
  delete ppreset;
}

/**
   Returns the indexed preset's description.
 */
char *cmafec_preset_get_descr(int idx)
{
  struct cma_preset *ppreset;

  fc_assert_ret_val(idx >= 0 && idx < cmafec_preset_num(), nullptr);

  ppreset = preset_list_get(preset_list, idx);
  return ppreset->descr;
}

/**
   Returns the indexed preset's parameter.
 */
const struct cm_parameter *cmafec_preset_get_parameter(int idx)
{
  struct cma_preset *ppreset;

  fc_assert_ret_val(idx >= 0 && idx < cmafec_preset_num(), nullptr);

  ppreset = preset_list_get(preset_list, idx);
  return &ppreset->parameter;
}

/**
   Returns the index of the preset which matches the given
   parameter. Returns -1 if no preset could be found.
 */
int cmafec_preset_get_index_of_parameter(
    const struct cm_parameter *const parameter)
{
  int i;

  for (i = 0; i < preset_list_size(preset_list); i++) {
    struct cma_preset *ppreset = preset_list_get(preset_list, i);
    if (ppreset->parameter == *parameter) {
      return i;
    }
  }
  return -1;
}

/**
   Returns the total number of presets.
 */
int cmafec_preset_num() { return preset_list_size(preset_list); }

/**
   Return short description of city governor preset
 */
const char *cmafec_get_short_descr_of_city(const struct city *pcity)
{
  struct cm_parameter parameter;

  if (!cma_is_city_under_agent(pcity, &parameter)) {
    return _("none");
  } else {
    return cmafec_get_short_descr(&parameter);
  }
}

/**
   Returns the description of the matching preset or "custom" if no
   preset could be found.
 */
const char *
cmafec_get_short_descr(const struct cm_parameter *const parameter)
{
  int idx = cmafec_preset_get_index_of_parameter(parameter);

  if (idx == -1) {
    return _("custom");
  } else {
    return cmafec_preset_get_descr(idx);
  }
}

/**
   Create default cma presets for a new user (or without configuration file)
 */
void create_default_cma_presets()
{
  int i;
  struct cm_parameter parameters[CMA_NUM_PARAMS] = {
      {// very happy
       .minimal_surplus = {0, 0, 0, -20, 0, 0},
       .require_happy = false,
       .allow_disorder = false,
       .allow_specialists = true,
       .factor = {10, 5, 0, 4, 0, 4},
       .happy_factor = 25},
      {// prefer food
       .minimal_surplus = {-20, 0, 0, -20, 0, 0},
       .require_happy = false,
       .allow_disorder = false,
       .allow_specialists = true,
       .factor = {25, 5, 0, 4, 0, 4},
       .happy_factor = 0},
      {// prefer prod
       .minimal_surplus = {0, -20, 0, -20, 0, 0},
       .require_happy = false,
       .allow_disorder = false,
       .allow_specialists = true,
       .factor = {10, 25, 0, 4, 0, 4},
       .happy_factor = 0},
      {// prefer gold
       .minimal_surplus = {0, 0, 0, -20, 0, 0},
       .require_happy = false,
       .allow_disorder = false,
       .allow_specialists = true,
       .factor = {10, 5, 0, 25, 0, 4},
       .happy_factor = 0},
      {// prefer science
       .minimal_surplus = {0, 0, 0, -20, 0, 0},
       .require_happy = false,
       .allow_disorder = false,
       .allow_specialists = true,
       .factor = {10, 5, 0, 4, 0, 25},
       .happy_factor = 0}};
  const char *names[CMA_NUM_PARAMS] = {
      N_("?cma:Very happy"), N_("?cma:Prefer food"),
      N_("?cma:Prefer production"), N_("?cma:Prefer gold"),
      N_("?cma:Prefer science")};

  for (i = CMA_NUM_PARAMS - 1; i >= 0; i--) {
    cmafec_preset_add(Q_(names[i]), &parameters[i]);
  }
}
