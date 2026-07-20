// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "support.h"

// Qt
#include <QVector>

struct fc_lua;

void luascript_func_init(struct fc_lua *fcl);
void luascript_func_free(struct fc_lua *fcl);

bool luascript_func_check(struct fc_lua *fcl,
                          QVector<QString> *missing_func_required,
                          QVector<QString> *missing_func_optional);
void luascript_func_add_valist(struct fc_lua *fcl, const char *func_name,
                               bool required, int nargs, int nreturns,
                               va_list args);
void luascript_func_add(struct fc_lua *fcl, const char *func_name,
                        bool required, int nargs, int nreturns, ...);
bool luascript_func_call_valist(struct fc_lua *fcl, const char *func_name,
                                va_list args);
bool luascript_func_call(struct fc_lua *fcl, const char *func_name, ...);

bool luascript_func_is_required(struct fc_lua *fcl, const char *func_name);
