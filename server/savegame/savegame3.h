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
#ifndef FC__SAVEGAME3_H
#define FC__SAVEGAME3_H



void savegame3_load(struct section_file *sfile);
void savegame3_save(struct section_file *sfile, const char *save_reason,
                    bool scenario);



#endif /* FC__SAVEGAME3_H */
