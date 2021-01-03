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

typedef void (*uq_callback_t)(void *data);
typedef void (*uq_free_fn_t)(void *data);
#define UQ_FREEDATA(fn) ((uq_free_fn_t) fn)

/* General update queue. */
void update_queue_init();
void update_queue_free();

void update_queue_freeze();
void update_queue_thaw();
void update_queue_force_thaw();
bool update_queue_is_frozen();

void update_queue_processing_started(int request_id);
void update_queue_processing_finished(int request_id);

/* User interface. */
void update_queue_add(uq_callback_t callback, void *data);
void update_queue_add_full(uq_callback_t callback, void *data,
                           uq_free_fn_t free_data_func);
bool update_queue_has_callback(uq_callback_t callback);
bool update_queue_has_callback_full(uq_callback_t callback,
                                    const void **data,
                                    uq_free_fn_t *free_data_func);

void update_queue_connect_processing_started(int request_id,
                                             uq_callback_t callback,
                                             void *data);
void update_queue_connect_processing_started_full(
    int request_id, uq_callback_t callback, void *data,
    uq_free_fn_t free_data_func);
void update_queue_connect_processing_finished(int request_id,
                                              uq_callback_t callback,
                                              void *data);
void update_queue_connect_processing_finished_full(
    int request_id, uq_callback_t callback, void *data,
    uq_free_fn_t free_data_func);

bool update_queue_is_switching_page();
