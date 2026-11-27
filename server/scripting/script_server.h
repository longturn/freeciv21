// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "luascript_types.h" // IWYU pragma: keep -- compile fails without it

struct section_file;
struct connection;

void script_server_remove_exported_object(void *object);

// Script functions.
bool script_server_init();
void script_server_free();

bool script_server_do_string(struct connection *caller, const char *str);
bool script_server_do_file(struct connection *caller, const char *filename);
bool script_server_load_file(const char *filename, char **buf);

bool script_server_unsafe_do_string(struct connection *caller,
                                    const char *str);
bool script_server_unsafe_do_file(struct connection *caller,
                                  const char *filename);

/* Script state i/o. */
void script_server_state_load(struct section_file *file);
void script_server_state_save(struct section_file *file);

// Signals.
void script_server_signal_emit(const char *signal_name, ...);

// Functions
bool script_server_call(const char *func_name, ...);
