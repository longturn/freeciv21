/***********************************************************************
Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#pragma once

// common
#include "fc_types.h"

// Special value to signal the absence of a server setting.
#define SERVER_SETTING_NONE ((server_setting_id) -1)

// Pure server settings.
server_setting_id server_setting_by_name(const char *name);
bool server_setting_exists(server_setting_id id);

enum sset_type server_setting_type_get(server_setting_id id);

const char *server_setting_name_get(server_setting_id id);

bool server_setting_value_bool_get(server_setting_id id);
int server_setting_value_int_get(server_setting_id id);
unsigned int server_setting_value_bitwise_get(server_setting_id id);

// Special value to signal the absence of a server setting + its value.
#define SSETV_NONE SERVER_SETTING_NONE

ssetv ssetv_from_values(server_setting_id setting, int value);
server_setting_id ssetv_setting_get(ssetv enc);
int ssetv_value_get(ssetv enc);

ssetv ssetv_by_rule_name(const char *name);
const char *ssetv_rule_name(ssetv val);

QString ssetv_human_readable(ssetv val, bool present);

bool ssetv_setting_has_value(ssetv val);
