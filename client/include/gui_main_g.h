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
#pragma once

// Forward declarations
class QTcpSocket;

// utility
#include "support.h" // bool type

// common
#include "fc_types.h"

void ui_main();
void ui_exit();

void insert_client_build_info(char *outbuf, size_t outlen);

extern const char *client_string;

// Actually defined in update_queue.c
void gui_update_allfonts();
void conn_list_dialog_update();
