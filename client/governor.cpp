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

#include <QElapsedTimer>

/* utility */
#include "bugs.h"
#include "capability.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h" /* for MIN() */
#include "specialist.h"
#include "support.h"

// common
#include "city.h"
#include "dataio.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "packets.h"
#include "specialist.h"

/* client */
#include "attribute.h"
#include "client_main.h"
#include "climisc.h"
#include "packhand.h"

/* include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "mapctrl_g.h"
#include "messagewin_g.h"

#include "governor.h"

#define log_request_ids(...) /* log_test(__VA_ARGS__) */
#define log_todo_lists(...)  /* log_test(__VA_ARGS__) */
#define log_meta_callback(...) log_debug(__VA_ARGS__)
#define log_debug_freeze(...) /* log_test(__VA_ARGS__) */
#define log_apply_result log_debug
#define log_handle_city log_debug
#define log_handle_city2 log_debug
#define log_results_are_equal log_debug

#define SHOW_TIME_STATS FALSE
#define SHOW_APPLY_RESULT_ON_SERVER_ERRORS FALSE
#define ALWAYS_APPLY_AT_SERVER FALSE
#define RESULT_COLUMNS 10
#define BUFFER_SIZE 100
#define MAX_LEN_PRESET_NAME 80
#define SAVED_PARAMETER_SIZE 29
#define MAX_AGENTS 10

#define SPECLIST_TAG preset
#define SPECLIST_TYPE struct cma_preset
#include "speclist.h"

#define preset_list_iterate(presetlist, ppreset)                            \
  TYPED_LIST_ITERATE(struct cma_preset, presetlist, ppreset)
#define preset_list_iterate_end LIST_ITERATE_END

static void city_changed(int city_id);
static void city_remove(int city_id)
{
  attr_city_set(ATTR_CITY_CMA_PARAMETER, city_id, 0, NULL);
}

governor *governor::m_instance = 0;

void governor::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

governor::~governor() {}

governor *governor::i()
{
  if (!m_instance)
    m_instance = new governor;
  return m_instance;
}

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

void governor::run()
{
  if (superhot < 1)
    return;

  for (auto pcity : scity_changed) {
    if (pcity) {
      city_changed(pcity->id);
    }
  }
  scity_changed.clear();
  for (auto pcity : scity_remove) {
    if (pcity) {
      attr_city_set(ATTR_CITY_CMAFE_PARAMETER, pcity->id, 0, NULL);
      city_remove(pcity->id);
    }
  }
  scity_remove.clear();
  update_turn_done_button_state();
}

static struct preset_list *preset_list = NULL;

struct cma_preset {
  char *descr;
  struct cm_parameter parameter;
};

static struct {
  civtimer *wall_timer;
  int apply_result_ignored, apply_result_applied, refresh_forced;
} stats;

class cma_yoloswag {
public:
  cma_yoloswag();
  ~cma_yoloswag();
  bool apply_result(struct city *pcity, const struct cm_result *result);
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
  void city_changed(int city_id);
  struct city *check_city(int city_id, struct cm_parameter *parameter);
  bool apply_result_on_server(struct city *pcity,
                              const struct cm_result *result);
  struct cm_result *cma_state_result;
  const struct cm_result *cma_result_got;
  int last_request;
  struct city *xcity;
};

// gimb means "governor is my bitch"
Q_GLOBAL_STATIC(cma_yoloswag, gimb)

inline bool operator==(const struct cm_result &result1,
                       const struct cm_result &result2)
{
#define T(x)                                                                 \
  if (result1.x != result2.x) {                                              \
    log_results_are_equal(#x);                                               \
    return FALSE;                                                            \
  }

  T(disorder);
  T(happy);

  specialist_type_iterate(sp) { T(specialists[sp]); }
  specialist_type_iterate_end;

  output_type_iterate(ot) { T(surplus[ot]); }
  output_type_iterate_end;

  fc_assert_ret_val(result1.city_radius_sq == result2.city_radius_sq, FALSE);
  city_map_iterate(result1.city_radius_sq, cindex, x, y)
  {
    if (is_free_worked_index(cindex)) {
      continue;
    }

    if (result1.worker_positions[cindex]
        != result2.worker_positions[cindex]) {
      log_results_are_equal("worker_positions");
      return FALSE;
    }
  }
  city_map_iterate_end;

  return TRUE;
#undef T
}

int cities_results_request() { return gimb->get_request(); }

void cma_got_result(int citynr) { gimb->result_came_from_server(citynr); }

cma_yoloswag::cma_yoloswag()
{
  cma_state_result = nullptr;
  last_request = -9999;
  cma_result_got = nullptr;
  xcity = nullptr;
}

int cma_yoloswag::get_request() { return last_request; }

void cma_yoloswag::result_came_from_server(int last_request_id)
{
  struct city *pcity = xcity;
  last_request = last_request_id;
  bool success;

  if (last_request_id < 0)
    return;
  if (last_request_id != 0) {
    int city_id = pcity->id;

    if (pcity != check_city(city_id, NULL)) {
      qDebug("apply_result_on_server(city %d) !check_city()!", city_id);
      return;
    }
  }

  /* Return. */
  cm_result_from_main_map(cma_state_result, pcity);

  success = (*cma_state_result == *cma_result_got);
  if (!success) {

#if SHOW_APPLY_RESULT_ON_SERVER_ERRORS
    qCritical("apply_result_on_server(city %d=\"%s\") no match!", pcity->id,
              city_name_get(pcity));

    log_test("apply_result_on_server(city %d=\"%s\") have:", pcity->id,
             city_name_get(pcity));
    cm_print_city(pcity);
    cm_print_result(cma_state_result);

  log_test("apply_result_on_server(city %d=\"%s\") want:", pcity->id,
             city_name_get(pcity));
  cm_print_result(cma_result_got);
#endif /* SHOW_APPLY_RESULT_ON_SERVER_ERRORS */
 }
 cm_result_destroy(cma_state_result);
 cm_result_destroy(const_cast<cm_result *>(cma_result_got));
 log_apply_result("apply_result_on_server() return %d.", (int) success);
 cma_state_result = nullptr;
 last_request = -9999;
 xcity = nullptr;
}

cma_yoloswag::~cma_yoloswag() {}

void cma_yoloswag::city_changed(int city_id)
{
 struct city *pcity = game_city_by_number(city_id);
  if (pcity) {
   handle_city(pcity);
 }
}

bool cma_yoloswag::apply_result(struct city *pcity,
                            const struct cm_result *result)
{
 fc_assert(!cma_is_city_under_agent(pcity, NULL));
 if (result->found_a_valid) {
   return apply_result_on_server(pcity, result);
 } else {
   return false;
 }
}


/************************************************************************/ /**
 Change the actual city setting to the given result. Returns TRUE iff
 the actual data matches the calculated one.
****************************************************************************/
bool cma_yoloswag::apply_result_on_server(struct city *pcity,
                                       const struct cm_result *result)
{
 int first_request_id = 0, last_request_id = 0, i;
 int city_radius_sq = city_map_radius_sq_get(pcity);
 struct cm_result *current_state = cm_result_new(pcity);
 struct tile *pcenter = city_tile(pcity);

 fc_assert_ret_val(result->found_a_valid, FALSE);
 cm_result_from_main_map(current_state, pcity);

 if (*current_state == *result && !ALWAYS_APPLY_AT_SERVER) {
   stats.apply_result_ignored++;
   return TRUE;
 }
  /* Do checks */
  if (city_size_get(pcity) != cm_result_citizens(result)) {
   qCritical("apply_result_on_server(city %d=\"%s\") bad result!",
             pcity->id, city_name_get(pcity));
   cm_print_city(pcity);
   cm_print_result(result);
   return FALSE;
 }

  stats.apply_result_applied++;

  log_apply_result("apply_result_on_server(city %d=\"%s\")", pcity->id,
                   city_name_get(pcity));

  connection_do_buffer(&client.conn);

 /* Remove all surplus workers */
 city_tile_iterate_skip_free_worked(city_radius_sq, pcenter, ptile, idx, x,
                                    y)
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
  city_tile_iterate_skip_free_worked_end;

  /* Change the excess non-default specialists to default. */
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

  /* now all surplus people are DEFAULT_SPECIALIST */

  /* Set workers */
  /* FIXME: This code assumes that any toggled worker will turn into a
   * DEFAULT_SPECIALIST! */
  city_tile_iterate_skip_free_worked(city_radius_sq, pcenter, ptile, idx, x,
                                     y)
  {
    if (NULL == tile_worked(ptile) && result->worker_positions[idx]) {
      log_apply_result("Putting worker at {%d,%d}.", x, y);
      fc_assert_action(city_can_work_tile(pcity, ptile), break);

      last_request_id = dsend_packet_city_make_worker(
          &client.conn, pcity->id, ptile->index);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }
  city_tile_iterate_skip_free_worked_end;

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
    first_request_id = last_request_id =
        dsend_packet_city_refresh(&client.conn, pcity->id);
    stats.refresh_forced++;
  }

  connection_do_unbuffer(&client.conn);

  cma_result_got = result;
  cma_state_result = current_state;
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
  attr_city_set(ATTR_CITY_CMA_PARAMETER, pcity->id, 0, NULL);
  refresh_city_dialog(pcity);
  city_report_dialog_update_city(pcity);
}

bool cma_yoloswag::is_city_under_agent(const struct city *pcity,
                                    struct cm_parameter *parameter)
{
  struct cm_parameter my_parameter;

  if (!cma_get_parameter(ATTR_CITY_CMA_PARAMETER, pcity->id,
                         &my_parameter)) {
    return FALSE;
  }

  if (parameter) {
    memcpy(parameter, &my_parameter, sizeof(struct cm_parameter));
  }
  return TRUE;
}
bool cma_yoloswag::get_parameter(enum attr_city attr, int city_id,
                              struct cm_parameter *parameter)
{
  size_t len;
  char buffer[SAVED_PARAMETER_SIZE];
  struct data_in din;
  int version, dummy;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. */

  len = attr_city_get(attr, city_id, sizeof(buffer), buffer);
  if (len == 0) {
    return FALSE;
  }
  fc_assert_ret_val(len == SAVED_PARAMETER_SIZE, FALSE);

  dio_input_init(&din, buffer, len);

  dio_get_uint8_raw(&din, &version);
  fc_assert_ret_val(version == 2, FALSE);

  /* Initialize the parameter (includes some AI-only fields that aren't
   * touched below). */
  cm_init_parameter(parameter);

  output_type_iterate(i)
  {
    dio_get_sint16_raw(&din, &parameter->minimal_surplus[i]);
    dio_get_sint16_raw(&din, &parameter->factor[i]);
  }
  output_type_iterate_end;

  dio_get_sint16_raw(&din, &parameter->happy_factor);
  dio_get_uint8_raw(&din,
                    &dummy); /* Dummy value; used to be factor_target. */
  dio_get_bool8_raw(&din, &parameter->require_happy);

  return TRUE;
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
  dio_put_uint8_raw(&dout, 0); /* Dummy value; used to be factor_target. */
  dio_put_bool8_raw(&dout, parameter->require_happy);

  fc_assert(dio_output_used(&dout) == SAVED_PARAMETER_SIZE);

  attr_city_set(attr, city_id, SAVED_PARAMETER_SIZE, buffer);
}

/************************************************************************/ /**
   Returns TRUE if the city is valid for CMA. Fills parameter if TRUE
   is returned. Parameter can be NULL.
 ****************************************************************************/
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
    return NULL;
  }

  if (city_owner(pcity) != client.conn.playing) {
    cma_release_city(pcity);
    return NULL;
  }

  return pcity;
}

/************************************************************************/ /**
   The given city has changed. handle_city ensures that either the city
   follows the set CMA goal or that the CMA detaches itself from the
   city.
 ****************************************************************************/
void cma_yoloswag::handle_city(struct city *pcity)
{
  struct cm_result *result = cm_result_new(pcity);
  bool handled;
  int i, city_id = pcity->id;

  log_handle_city("handle_city(city %d=\"%s\") pos=(%d,%d) owner=%s",
                  pcity->id, city_name_get(pcity), TILE_XY(pcity->tile),
                  nation_rule_name(nation_of_city(pcity)));

  log_handle_city2("START handle city %d=\"%s\"", pcity->id,
                   city_name_get(pcity));

  handled = FALSE;
  for (i = 0; i < 5; i++) {
    struct cm_parameter parameter;

    log_handle_city2("  try %d", i);

    if (pcity != check_city(city_id, &parameter)) {
      handled = TRUE;
      break;
    }

    cm_query_result(pcity, &parameter, result, FALSE);
    if (!result->found_a_valid) {
      log_handle_city2("  no valid found result");

      cma_release_city(pcity);

      create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                   _("The citizen governor can't fulfill the requirements "
                     "for %s. Passing back control."),
                   city_link(pcity));
      handled = TRUE;
      break;
    } else {
      if (!apply_result_on_server(pcity, result)) {
        log_handle_city2("  doesn't cleanly apply");
        if (pcity == check_city(city_id, NULL) && i == 0) {
          create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                       _("The citizen governor has gotten confused dealing "
                         "with %s.  You may want to have a look."),
                       city_link(pcity));
        }
      } else {
        log_handle_city2("  ok");
        /* Everything ok */
        handled = TRUE;
        break;
      }
    }
  }

  if (!handled) {
    fc_assert_ret(pcity == check_city(city_id, NULL));
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

static void city_changed(int city_id)
{
  struct city *pcity = game_city_by_number(city_id);

  if (pcity) {
    gimb->handle_city(pcity);
  }
}

/************************************************************************/ /**
   Apply result on server if it's valid
 ****************************************************************************/
bool cma_apply_result(struct city *pcity, const struct cm_result *result)
{
  return gimb->apply_result(pcity, result);
}

/************************************************************************/ /**
   Put city under governor control
 ****************************************************************************/
void cma_put_city_under_agent(struct city *pcity,
                              const struct cm_parameter *const parameter)
{
  gimb->put_city_under_agent(pcity, parameter);
}

/************************************************************************/ /**
   Release city from governor control.
 ****************************************************************************/
void cma_release_city(struct city *pcity) { gimb->release_city(pcity); }

/************************************************************************/ /**
   Check whether city is under governor control, and fill parameter if it is.
 ****************************************************************************/
bool cma_is_city_under_agent(const struct city *pcity,
                             struct cm_parameter *parameter)
{
  return gimb->is_city_under_agent(pcity, parameter);
}

/************************************************************************/ /**
   Get the parameter.

   Don't bother to cm_init_parameter, since we set all the fields anyway.
   But leave the comment here so we can find this place when searching
   for all the creators of a parameter.
 ****************************************************************************/
bool cma_get_parameter(enum attr_city attr, int city_id,
                       struct cm_parameter *parameter)
{
  return gimb->get_parameter(attr, city_id, parameter);
}

/************************************************************************/ /**
   Set attribute block for city from parameter.
 ****************************************************************************/
void cma_set_parameter(enum attr_city attr, int city_id,
                       const struct cm_parameter *parameter)
{
  gimb->set_parameter(attr, city_id, parameter);
}



/**********************************************************************/ /**
   Initialize the presets if there are no presets loaded on startup.
 **************************************************************************/
void cmafec_init(void)
{

  if (preset_list == NULL) {
    preset_list = preset_list_new();
  }
}

/**********************************************************************/ /**
   Free resources allocated for presets system.
 **************************************************************************/
void cmafec_free(void)
{
  while (cmafec_preset_num() > 0) {
    cmafec_preset_remove(0);
  }
  preset_list_destroy(preset_list);
}

/**********************************************************************/ /**
   Sets the front-end parameter.
 **************************************************************************/
void cmafec_set_fe_parameter(struct city *pcity,
                             const struct cm_parameter *const parameter)
{
  cma_set_parameter(ATTR_CITY_CMAFE_PARAMETER, pcity->id, parameter);
}

/**********************************************************************/ /**
   Return the front-end parameter for the given city. Returns a dummy
   parameter if no parameter was set.
 **************************************************************************/
void cmafec_get_fe_parameter(struct city *pcity, struct cm_parameter *dest)
{
  struct cm_parameter parameter;

  /* our fe_parameter could be stale. our agents parameter is uptodate */
  if (cma_is_city_under_agent(pcity, &parameter)) {
    cm_copy_parameter(dest, &parameter);
    cmafec_set_fe_parameter(pcity, dest);
  } else {
    /* Create a dummy parameter to return. */
    cm_init_parameter(dest);
    if (!cma_get_parameter(ATTR_CITY_CMAFE_PARAMETER, pcity->id, dest)) {
      /* We haven't seen this city before; store the dummy. */
      cmafec_set_fe_parameter(pcity, dest);
    }
  }
}

/**********************************************************************/ /**
   Adds a preset.
 **************************************************************************/
void cmafec_preset_add(const char *descr_name, struct cm_parameter *pparam)
{
  struct cma_preset *ppreset = new cma_preset;

  if (preset_list == NULL) {
    preset_list = preset_list_new();
  }

  cm_copy_parameter(&ppreset->parameter, pparam);
  ppreset->descr = new char[MAX_LEN_PRESET_NAME];
  (void) fc_strlcpy(ppreset->descr, descr_name, MAX_LEN_PRESET_NAME);
  preset_list_prepend(preset_list, ppreset);
}

/**********************************************************************/ /**
   Removes a preset.
 **************************************************************************/
void cmafec_preset_remove(int idx)
{
  struct cma_preset *ppreset;

  fc_assert_ret(idx >= 0 && idx < cmafec_preset_num());

  ppreset = preset_list_get(preset_list, idx);
  preset_list_remove(preset_list, ppreset);

  delete[] ppreset->descr;
  delete ppreset;
}

/**********************************************************************/ /**
   Returns the indexed preset's description.
 **************************************************************************/
char *cmafec_preset_get_descr(int idx)
{
  struct cma_preset *ppreset;

  fc_assert_ret_val(idx >= 0 && idx < cmafec_preset_num(), NULL);

  ppreset = preset_list_get(preset_list, idx);
  return ppreset->descr;
}

/**********************************************************************/ /**
   Returns the indexed preset's parameter.
 **************************************************************************/
const struct cm_parameter *cmafec_preset_get_parameter(int idx)
{
  struct cma_preset *ppreset;

  fc_assert_ret_val(idx >= 0 && idx < cmafec_preset_num(), NULL);

  ppreset = preset_list_get(preset_list, idx);
  return &ppreset->parameter;
}

/**********************************************************************/ /**
   Returns the index of the preset which matches the given
   parameter. Returns -1 if no preset could be found.
 **************************************************************************/
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

/**********************************************************************/ /**
   Returns the total number of presets.
 **************************************************************************/
int cmafec_preset_num(void) { return preset_list_size(preset_list); }

/**********************************************************************/ /**
   Return short description of city governor preset
 **************************************************************************/
const char *cmafec_get_short_descr_of_city(const struct city *pcity)
{
  struct cm_parameter parameter;

  if (!cma_is_city_under_agent(pcity, &parameter)) {
    return _("none");
  } else {
    return cmafec_get_short_descr(&parameter);
  }
}

/**********************************************************************/ /**
   Returns the description of the matching preset or "custom" if no
   preset could be found.
 **************************************************************************/
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

/**********************************************************************/ /**
   Return string describing when city is assumed to grow.
 **************************************************************************/
static const char *get_city_growth_string(struct city *pcity, int surplus)
{
  int stock, cost, turns;
  static char buffer[50];

  if (surplus == 0) {
    fc_snprintf(buffer, sizeof(buffer), _("never"));
    return buffer;
  }

  stock = pcity->food_stock;
  cost = city_granary_size(city_size_get(pcity));

  stock += surplus;

  if (stock >= cost) {
    turns = 1;
  } else if (surplus > 0) {
    turns = ((cost - stock - 1) / surplus) + 1 + 1;
  } else {
    if (stock < 0) {
      turns = -1;
    } else {
      turns = (stock / surplus);
    }
  }
  fc_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns", turns),
              turns);
  return buffer;
}

/**********************************************************************/ /**
   Return string describing when city is assumed to finish current production
 **************************************************************************/
static const char *get_prod_complete_string(struct city *pcity, int surplus)
{
  int stock, cost, turns;
  static char buffer[50];

  if (surplus <= 0) {
    fc_snprintf(buffer, sizeof(buffer), _("never"));
    return buffer;
  }

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_strlcpy(
        buffer,
        improvement_name_translation(pcity->production.value.building),
        sizeof(buffer));
    return buffer;
  }
  stock = pcity->shield_stock + surplus;
  cost = city_production_build_shield_cost(pcity);

  if (stock >= cost) {
    turns = 1;
  } else if (surplus > 0) {
    turns = ((cost - stock - 1) / surplus) + 1 + 1;
  } else {
    if (stock < 0) {
      turns = -1;
    } else {
      turns = (stock / surplus);
    }
  }
  fc_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns", turns),
              turns);
  return buffer;
}

/**********************************************************************/ /**
   Return string describing result
 **************************************************************************/
const char *
cmafec_get_result_descr(struct city *pcity, const struct cm_result *result,
                        const struct cm_parameter *const parameter)
{
  int j;
  char buf[RESULT_COLUMNS][BUFFER_SIZE];
  char citizen_types[BUFFER_SIZE];
  static char buffer[600];

  /* TRANS: "W" is worker citizens, as opposed to specialists;
   * %s will represent the specialist types, for instance "E/S/T" */
  fc_snprintf(citizen_types, BUFFER_SIZE, _("People (W/%s)"),
              specialists_abbreviation_string());

  if (!result->found_a_valid) {
    for (j = 0; j < RESULT_COLUMNS; j++)
      fc_snprintf(buf[j], BUFFER_SIZE, "---");
  } else {
    output_type_iterate(o)
    {
      fc_snprintf(buf[o], BUFFER_SIZE, "%+3d", result->surplus[o]);
    }
    output_type_iterate_end;

    fc_snprintf(buf[6], BUFFER_SIZE, "%d/%s%s",
                city_size_get(pcity) - cm_result_specialists(result),
                specialists_string(result->specialists),
                /* TRANS: preserve leading space */
                result->happy ? _(" happy") : "");

    fc_snprintf(buf[7], BUFFER_SIZE, "%s",
                get_city_growth_string(pcity, result->surplus[O_FOOD]));
    fc_snprintf(buf[8], BUFFER_SIZE, "%s",
                get_prod_complete_string(pcity, result->surplus[O_SHIELD]));
    fc_snprintf(buf[9], BUFFER_SIZE, "%s",
                cmafec_get_short_descr(parameter));
  }

  fc_snprintf(buffer, sizeof(buffer),
              _("Name: %s\n"
                "Food:       %10s Gold:    %10s\n"
                "Production: %10s Luxury:  %10s\n"
                "Trade:      %10s Science: %10s\n"
                "\n"
                "%*s%s: %s\n"
                "          City grows: %s\n"
                "Production completed: %s"),
              buf[9], buf[O_FOOD], buf[O_GOLD], buf[O_SHIELD], buf[O_LUXURY],
              buf[O_TRADE], buf[O_SCIENCE],
              MAX(0, 20 - (int) get_internal_string_length(citizen_types)),
              "", citizen_types, buf[6], buf[7], buf[8]);

  log_debug("\n%s", buffer);
  return buffer;
}

/**********************************************************************/ /**
   Create default cma presets for a new user (or without configuration file)
 **************************************************************************/
void create_default_cma_presets(void)
{
  int i;
  struct cm_parameter parameters[] = {
      {/* very happy */
       .minimal_surplus = {0, 0, 0, -20, 0, 0},
       .require_happy = FALSE,
       .allow_disorder = FALSE,
       .allow_specialists = TRUE,
       .factor = {10, 5, 0, 4, 0, 4},
       .happy_factor = 25},
      {/* prefer food */
       .minimal_surplus = {-20, 0, 0, -20, 0, 0},
       .require_happy = FALSE,
       .allow_disorder = FALSE,
       .allow_specialists = TRUE,
       .factor = {25, 5, 0, 4, 0, 4},
       .happy_factor = 0},
      {/* prefer prod */
       .minimal_surplus = {0, -20, 0, -20, 0, 0},
       .require_happy = FALSE,
       .allow_disorder = FALSE,
       .allow_specialists = TRUE,
       .factor = {10, 25, 0, 4, 0, 4},
       .happy_factor = 0},
      {/* prefer gold */
       .minimal_surplus = {0, 0, 0, -20, 0, 0},
       .require_happy = FALSE,
       .allow_disorder = FALSE,
       .allow_specialists = TRUE,
       .factor = {10, 5, 0, 25, 0, 4},
       .happy_factor = 0},
      {/* prefer science */
       .minimal_surplus = {0, 0, 0, -20, 0, 0},
       .require_happy = FALSE,
       .allow_disorder = FALSE,
       .allow_specialists = TRUE,
       .factor = {10, 5, 0, 4, 0, 25},
       .happy_factor = 0}};
  const char *names[ARRAY_SIZE(parameters)] = {
      N_("?cma:Very happy"), N_("?cma:Prefer food"),
      N_("?cma:Prefer production"), N_("?cma:Prefer gold"),
      N_("?cma:Prefer science")};

  for (i = ARRAY_SIZE(parameters) - 1; i >= 0; i--) {
    cmafec_preset_add(Q_(names[i]), &parameters[i]);
  }
}
