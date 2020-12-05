/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#ifndef FC__SAVEMAIN_H
#define FC__SAVEMAIN_H

/* utility */
#include "support.h"



struct section_file;

void savegame_load(struct section_file *sfile);
void savegame_save(struct section_file *sfile, const char *save_reason,
                   bool scenario);

void save_game(const char *orig_filename, const char *save_reason,
               bool scenario);

void save_system_close(void);



#endif /* FC__SAVEMAIN_H */
