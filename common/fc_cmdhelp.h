/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

struct cmdhelp;

struct cmdhelp *cmdhelp_new(const char *cmdname);
void cmdhelp_destroy(struct cmdhelp *pcmdhelp);
void cmdhelp_add(struct cmdhelp *pcmdhelp, const char *shortarg,
                 const char *longarg, const char *helpstr, ...)
    fc__attribute((__format__(__printf__, 4, 5)));
void cmdhelp_display(struct cmdhelp *pcmdhelp, bool sort, bool gui_options,
                     bool report_bugs);
