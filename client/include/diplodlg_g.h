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
#include "shared.h"

#include "diptreaty.h"

#include "gui_proto_constructor.h"

void handle_diplomacy_init_meeting(int counterpart,
               int initiated_from);
void handle_diplomacy_cancel_meeting(int counterpart,
               int initiated_from);
void handle_diplomacy_create_clause(int counterpart,
               int giver, enum clause_type type, int value);
void handle_diplomacy_remove_clause( int counterpart,
               int giver, enum clause_type type, int value);
void handle_diplomacy_accept_treaty( int counterpart,
               bool I_accepted, bool other_accepted);

void close_all_diplomacy_dialogs(void);

