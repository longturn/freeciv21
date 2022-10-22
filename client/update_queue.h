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
#pragma once

#include <QHash>
#include <QPair>
#include <QQueue>

typedef void (*uq_callback_t)(void *data);
typedef void (*uq_free_fn_t)(void *data);

// Type of data listed in 'wq_processing_started' and
// 'wq_processing_finished.'
struct waiting_queue_data {
  uq_callback_t callback;
  void *data;
  uq_free_fn_t free_data_func;

  bool operator==(const waiting_queue_data &other) const
  {
    return callback == other.callback && data == other.data
           && free_data_func == other.free_data_func;
  }
};

typedef QList<waiting_queue_data> waitq_list;
typedef QHash<int, waitq_list> waitingQueue;

class update_queue {
  static update_queue *m_instance;
  QQueue<waiting_queue_data> queue;
  waitingQueue wq_processing_finished;
  bool has_idle_cb = {false};

public:
  static update_queue *uq();
  static void drop();
  ~update_queue();
  void init();
  void add(uq_callback_t callback);
  void processing_finished(int request_id);
  bool has_callback(uq_callback_t callback);
  bool has_callback_full(uq_callback_t cb, const void **data,
                         uq_free_fn_t *free_fn);
  void connect_processing_finished(int request_id, uq_callback_t cb,
                                   void *data);
  void connect_processing_finished_unique(int request_id, uq_callback_t cb,
                                          void *data);
  void connect_processing_finished_full(int request_id, uq_callback_t cb,
                                        void *data, uq_free_fn_t free_func);

private:
  update_queue() = default;
  void update_unqueue();
  void push(const waiting_queue_data &wq);
  void wq_run_requests(waitingQueue &hash, int request_id);
  void wq_data_destroy(waiting_queue_data &wq_data);
  void wq_add_request(waitingQueue &hash, int request_id, uq_callback_t cb,
                      void *data, uq_free_fn_t free_fn);
};

bool update_queue_is_switching_page(void);
void players_dialog_update();
