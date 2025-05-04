// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv and Freeciv21 Contributors

#pragma once

// common
#include "fc_types.h"

// Qt
#include <QString>

// Special value to signal the absence of a server setting.
#define SERVER_SETTING_NONE ((server_setting_id) -1)

// Pure server settings.
server_setting_id server_setting_by_name(const char *name);
bool server_setting_exists(server_setting_id id);

enum sset_type server_setting_type_get(server_setting_id id);

const char *server_setting_name_get(server_setting_id id);

bool server_setting_value_bool_get(server_setting_id id);
int server_setting_value_int_get(server_setting_id id);

// Special value to signal the absence of a server setting + its value.
#define SSETV_NONE SERVER_SETTING_NONE

ssetv ssetv_from_values(server_setting_id setting, int value);
server_setting_id ssetv_setting_get(ssetv enc);

ssetv ssetv_by_rule_name(const char *name);
const char *ssetv_rule_name(ssetv val);

QString ssetv_human_readable(ssetv val, bool present);

bool ssetv_setting_has_value(ssetv val);
