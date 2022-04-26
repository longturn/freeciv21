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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QUrl>

// utility
#include "log.h"
#include "support.h" // bool

// common
#include "city.h"
#include "connection.h"
#include "player.h"

/* client/include */
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "dialogs_g.h"
#include "gui_main_g.h"
#include "menu_g.h"
#include "pages_g.h"
#include "ratesdlg_g.h"
#include "repodlgs_g.h"

// client
#include "client_main.h"
#include "connectdlg_common.h"
#include "options.h"
#include "plrdlg_common.h"
#include "tilespec.h"

#include "update_queue.h"

// forward declaration
#include "gui-qt/canvas.h"
#include "gui-qt/qtg_cxxside.h"

update_queue *update_queue::m_instance = nullptr;

// returns instance of queue
update_queue *update_queue::uq()
{
  if (!m_instance)
    m_instance = new update_queue;
  return m_instance;
}

void update_queue::drop() { NFCN_FREE(m_instance); }

// Extract the update_queue_data from the waiting queue data.
struct update_queue_data *
update_queue::wq_data_extract(struct waiting_queue_data *wq_data)
{
  struct update_queue_data *uq_data = wq_data->uq_data;
  wq_data->uq_data = nullptr;
  return uq_data;
}

// Create a new waiting queue data.
struct waiting_queue_data *
update_queue::wq_data_new(uq_callback_t callback, void *data,
                          uq_free_fn_t free_data_func)
{
  struct waiting_queue_data *wq_data = new waiting_queue_data;
  wq_data->callback = callback;
  wq_data->uq_data = data_new(data, free_data_func);
  return wq_data;
}

// Moves the instances waiting to the request_id to the callback queue.
void update_queue::wq_run_requests(waitingQueue &hash, int request_id)
{
  waitq_list *list;
  if (!hash.contains(request_id)) {
    return;
  }
  list = hash.value(request_id);
  for (auto wq_data : qAsConst(*list)) {
    push(wq_data->callback, wq_data_extract(wq_data));
  }
  hash.remove(request_id);
}

// Free a waiting queue data.
void update_queue::wq_data_destroy(struct waiting_queue_data *wq_data)
{
  fc_assert_ret(nullptr != wq_data);
  if (nullptr != wq_data->uq_data) {
    // May be nullptr, see waiting_queue_data_extract().
    data_destroy(wq_data->uq_data);
  }
  delete wq_data;
}

// Connects the callback to a network event.
void update_queue::wq_add_request(waitingQueue &hash, int request_id,
                                  uq_callback_t callback, void *data,
                                  uq_free_fn_t free_data_func)
{
  waitq_list *wqlist;
  if (!hash.contains(request_id)) {
    waitq_list *wqlist = new waitq_list;
    hash.insert(request_id, wqlist);
  }
  wqlist = hash.value(request_id);
  wqlist->append(wq_data_new(callback, data, free_data_func));
}

// clears content
void update_queue::init()
{
  while (!queue.isEmpty()) {
    updatePair pair = queue.dequeue();
    data_destroy(pair.second);
  }

  for (auto a : qAsConst(wq_processing_started)) {
    for (auto data : qAsConst(*a)) {
      wq_data_destroy(data);
    }

    for (auto a : qAsConst(wq_processing_finished)) {
      for (auto data : qAsConst(*a)) {
        wq_data_destroy(data);
      }
    }
  }
  wq_processing_started.clear();
  wq_processing_finished.clear();
}

update_queue::~update_queue() { init(); }

// Create a new update queue data.
struct update_queue_data *update_queue::data_new(void *data,
                                                 uq_free_fn_t free_data_func)
{
  struct update_queue_data *uq_data = new update_queue_data();

  uq_data->data = data;
  uq_data->free_data_func = free_data_func;
  return uq_data;
}

//  Free a update queue data.
void update_queue::data_destroy(struct update_queue_data *uq_data)
{
  fc_assert_ret(nullptr != uq_data);
  if (nullptr != uq_data->free_data_func) {
    uq_data->free_data_func(uq_data->data);
  }
  delete uq_data;
}

// Freezes the update queue.
void update_queue::freeze(void) { frozen_level++; }

// Unfreezes the update queue by 1
void update_queue::thaw(void)
{
  frozen_level--;
  if (0 == frozen_level && !has_idle_cb && 0 < queue.size()) {
    has_idle_cb = true;
    update_unqueue();
  } else if (0 > frozen_level) {
    qWarning("update_queue::frozen_level < 0");
    frozen_level = 0;
  }
}

// Unfreeze queue
void update_queue::force_thaw(void)
{
  while (is_frozen()) {
    thaw();
  }
}

bool update_queue::is_frozen(void) const { return (0 < frozen_level); }

// Moves the instances waiting to the request_id to the callback queue.
void update_queue::processing_started(int request_id)
{
  wq_run_requests(wq_processing_started, request_id);
}

// Moves the instances waiting to the request_id to the callback queue.
void update_queue::processing_finished(int request_id)
{
  wq_run_requests(wq_processing_finished, request_id);
}

// Unqueue all updates.
void update_queue::update_unqueue()
{
  updatePair pair;
  if (is_frozen() || !tileset_is_fully_loaded()) {
    // Cannot update now, let's add it again.
    has_idle_cb = false;
    return;
  }

  has_idle_cb = false;

  // Invoke callbacks.
  while (!queue.isEmpty()) {
    pair = queue.dequeue();
    auto callback = pair.first;
    auto uq_data = pair.second;
    callback(uq_data->data);
    data_destroy(uq_data);
  }
}

// Add a callback to the update queue. NB: you can only set a callback
// once. Setting a callback twice will put new callback at end.
void update_queue::push(uq_callback_t callback,
                        struct update_queue_data *uq_data)
{
  struct update_queue_data *uqr_data = nullptr;
  for (auto p : qAsConst(queue)) {
    if (p.first == callback)
      uqr_data = p.second;
  }
  auto pr = qMakePair(callback, uqr_data);
  queue.removeAll(pr);
  if (uqr_data) {
    data_destroy(uqr_data);
  }
  queue.enqueue(qMakePair(callback, uq_data));

  if (!has_idle_cb && !is_frozen()) {
    has_idle_cb = true;
    update_unqueue();
  }
}

// Add a callback to the update queue. NB: you can only set a callback
// once. Setting a callback twice will overwrite the previous.
void update_queue::add(uq_callback_t callback, void *data)
{
  push(callback, data_new(data, nullptr));
}

// Add a callback to the update queue. NB: you can only set a callback
// once. Setting a callback twice will overwrite the previous.
void update_queue::add_full(uq_callback_t callback, void *data,
                            uq_free_fn_t free_data_func)
{
  push(callback, data_new(data, free_data_func));
}

// Returns whether this callback is listed in the update queue.
bool update_queue::has_callback(uq_callback_t callback)
{
  for (auto pair : qAsConst(queue)) {
    if (pair.first == callback)
      return true;
  }
  return false;
}

bool update_queue::has_callback_full(uq_callback_t callback,
                                     const void **data,
                                     uq_free_fn_t *free_data_func)
{
  struct update_queue_data *uq_data = nullptr;
  for (auto p : qAsConst(queue)) {
    if (p.first == callback)
      uq_data = p.second;
  }
  if (uq_data) {
    if (nullptr != data) {
      *data = uq_data->data;
    }
    if (nullptr != free_data_func) {
      *free_data_func = uq_data->free_data_func;
    }
    return true;
  }
  return false;
}

// Connects the callback to the start of the processing (in server side) of
// the request.
void update_queue::connect_processing_started_full(
    int request_id, uq_callback_t callback, void *data,
    uq_free_fn_t free_data_func)
{
  wq_add_request(wq_processing_started, request_id, callback, data,
                 free_data_func);
}

// Connects the callback to the end of the processing (in server side) of
// the request.
void update_queue::connect_processing_finished(int request_id,
                                               uq_callback_t callback,
                                               void *data)
{
  wq_add_request(wq_processing_finished, request_id, callback, data,
                 nullptr);
}

/**
 * Connects the callback to the end of the processing (in server side) of
 * the request. The callback will be called only once for this request.
 */
void update_queue::connect_processing_finished_unique(int request_id,
                                                      uq_callback_t callback,
                                                      void *data)
{
  if (wq_processing_finished.contains(request_id)) {
    for (const auto &d : *wq_processing_finished[request_id]) {
      if (d->callback == callback && d->uq_data->data == data) {
        // Already present
        return;
      }
    }
  }
  wq_add_request(wq_processing_finished, request_id, callback, data,
                 nullptr);
}

// Connects the callback to the end of the processing (in server side) of
// the request.
void update_queue::connect_processing_finished_full(
    int request_id, uq_callback_t callback, void *data,
    uq_free_fn_t free_data_func)
{
  wq_add_request(wq_processing_finished, request_id, callback, data,
                 free_data_func);
}

// Connects the callback to the start of the processing (in server side) of
// the request.
void update_queue::connect_processing_started(int request_id,
                                              uq_callback_t callback,
                                              void *data)
{
  wq_add_request(wq_processing_started, request_id, callback, data, nullptr);
}

// Set the client page.
static void set_client_page_callback(void *data)
{
  enum client_pages page = static_cast<client_pages>(FC_PTR_TO_INT(data));

  qtg_real_set_client_page(page);
}

// Set the client page.
void set_client_page(client_pages page)
{
  log_debug("Requested page: %s.", client_pages_name(page));

  update_queue::uq()->add(set_client_page_callback, FC_INT_TO_PTR(page));
}

// Start server and then, set the client page.
void client_start_server_and_set_page(client_pages page)
{
  log_debug("Requested server start + page: %s.", client_pages_name(page));

  if (client_start_server(client_url().userName())) {
    update_queue::uq()->connect_processing_finished(
        client.conn.client.last_request_id_used, set_client_page_callback,
        FC_INT_TO_PTR(page));
  }
}

// Returns the next client page.
client_pages get_client_page()
{
  const void *data;

  if (update_queue::uq()->has_callback_full(set_client_page_callback, &data,
                                            nullptr)) {
    return static_cast<client_pages>(FC_PTR_TO_INT(data));
  } else {
    return qtg_get_current_client_page();
  }
}

// Returns whether there's page switching already in progress.
bool update_queue_is_switching_page()
{
  return update_queue::uq()->has_callback(set_client_page_callback);
}

// Update the menus.
static void menus_update_callback(void *data)
{
  if (FC_PTR_TO_INT(data)) {
    real_menus_init();
  }
  real_menus_update();
}

// Request the menus to be initialized and updated.
void menus_init(void)
{
  update_queue::uq()->add(menus_update_callback, FC_INT_TO_PTR(true));
}

// Request the menus to be updated.
void menus_update(void)
{
  if (!update_queue::uq()->has_callback(menus_update_callback)) {
    update_queue::uq()->add(menus_update_callback, FC_INT_TO_PTR(false));
  }
}

// Update multipliers/policy dialog.
void multipliers_dialog_update()
{
  update_queue::uq()->add(real_multipliers_dialog_update, nullptr);
}

// Update cities gui.
static void cities_update_callback(void *data)
{
#ifdef FREECIV_DEBUG
#define NEED_UPDATE(city_update, action)                                    \
  if (city_update & need_update) {                                          \
    action;                                                                 \
    need_update = static_cast<city_updates>(need_update & ~city_update);    \
  }
#else // FREECIV_DEBUG
#define NEED_UPDATE(city_update, action)                                    \
  if (city_update & need_update) {                                          \
    action;                                                                 \
  }
#endif // FREECIV_DEBUG

  cities_iterate(pcity)
  {
    enum city_updates need_update = pcity->client.need_updates;

    if (CU_NO_UPDATE == need_update) {
      continue;
    }

    // Clear all updates.
    pcity->client.need_updates = CU_NO_UPDATE;

    NEED_UPDATE(CU_UPDATE_REPORT, real_city_report_update_city(pcity));
    NEED_UPDATE(CU_UPDATE_DIALOG, qtg_real_city_dialog_refresh(pcity));
    NEED_UPDATE(CU_POPUP_DIALOG, qtg_real_city_dialog_popup(pcity));

#ifdef FREECIV_DEBUG
    if (CU_NO_UPDATE != need_update) {
      qCritical("Some city updates not handled "
                "for city %s (id %d): %d left.",
                city_name_get(pcity), pcity->id, need_update);
    }
#endif // FREECIV_DEBUG
  }
  cities_iterate_end;
#undef NEED_UPDATE
}

// Request the city dialog to be popped up for the city.
void popup_city_dialog(city *pcity)
{
  pcity->client.need_updates =
      static_cast<city_updates>(static_cast<int>(pcity->client.need_updates)
                                | static_cast<int>(CU_POPUP_DIALOG));
  update_queue::uq()->add(cities_update_callback, nullptr);
}

// Request the city dialog to be updated for the city.
void refresh_city_dialog(city *pcity)
{
  pcity->client.need_updates =
      static_cast<city_updates>(static_cast<int>(pcity->client.need_updates)
                                | static_cast<int>(CU_UPDATE_DIALOG));
  update_queue::uq()->add(cities_update_callback, nullptr);
}

// Request the city to be updated in the city report.
void city_report_dialog_update_city(struct city *pcity)
{
  pcity->client.need_updates =
      static_cast<city_updates>(static_cast<int>(pcity->client.need_updates)
                                | static_cast<int>(CU_UPDATE_REPORT));
  update_queue::uq()->add(cities_update_callback, nullptr);
}

// Update the connection list in the start page.
void conn_list_dialog_update(void)
{
  update_queue::uq()->add(qtg_real_conn_list_dialog_update, nullptr);
}

// Update the nation report.
void players_dialog_update()
{
  update_queue::uq()->add(real_players_dialog_update, nullptr);
}

// Update the city report.
void city_report_dialog_update()
{
  update_queue::uq()->add(real_city_report_dialog_update, nullptr);
}

// Update the science report.
void science_report_dialog_update()
{
  update_queue::uq()->add(real_science_report_dialog_update, nullptr);
}

// Update the economy report.
void economy_report_dialog_update()
{
  update_queue::uq()->add(real_economy_report_dialog_update, nullptr);
}

// Update the units report.
void units_report_dialog_update()
{
  update_queue::uq()->add(real_units_report_dialog_update, nullptr);
}

// Update the units report.
void unit_select_dialog_update(void)
{
  update_queue::uq()->add(unit_select_dialog_update_real, nullptr);
}
