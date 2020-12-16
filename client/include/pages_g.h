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

#include "megaenums.h"
DECLARE_ENUM_WITH_TYPE(client_pages, int32_t, PAGE_MAIN = 0, PAGE_START = 1,
                       PAGE_SCENARIO = 2, PAGE_LOAD = 3, PAGE_NETWORK = 4,
                       PAGE_GAME = 5);

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, real_set_client_page, enum client_pages page)
GUI_FUNC_PROTO(enum client_pages, get_current_client_page, void)
GUI_FUNC_PROTO(void, update_start_page, void)

/* Actually defined in update_queue.c */
void set_client_page(enum client_pages page);
void client_start_server_and_set_page(enum client_pages page);
enum client_pages get_client_page(void);


