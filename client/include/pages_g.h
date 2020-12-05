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
#ifndef FC__PAGES_G_H
#define FC__PAGES_G_H

/**************************************************************************
  Toplevel window pages modes.
**************************************************************************/
#define SPECENUM_NAME client_pages
#define SPECENUM_VALUE0 PAGE_MAIN     /* Main menu, aka intro page.  */
#define SPECENUM_VALUE1 PAGE_START    /* Start new game page.  */
#define SPECENUM_VALUE2 PAGE_SCENARIO /* Start new scenario page. */
#define SPECENUM_VALUE3 PAGE_LOAD     /* Load saved game page. */
#define SPECENUM_VALUE4 PAGE_NETWORK  /* Connect to network page.  */
#define SPECENUM_VALUE5 PAGE_GAME     /* In game page. */
#include "specenum_gen.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, real_set_client_page, enum client_pages page)
GUI_FUNC_PROTO(enum client_pages, get_current_client_page, void)
GUI_FUNC_PROTO(void, update_start_page, void)

/* Actually defined in update_queue.c */
void set_client_page(enum client_pages page);
void client_start_server_and_set_page(enum client_pages page);
enum client_pages get_client_page(void);

#endif /* FC__PAGES_G_H */
