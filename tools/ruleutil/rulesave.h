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
#ifndef FC__RULESAVE_H
#define FC__RULESAVE_H



struct rule_data {
  char *nationlist;
  char *nationlist_saved;
};

bool save_ruleset(const char *path, const char *name,
                  struct rule_data *data);



#endif /* FC__RULESAVE_H */
