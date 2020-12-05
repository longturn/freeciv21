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
#ifndef FC__MPDB_H
#define FC__MPDB_H

/* common */
#include "fc_types.h"

/* modinst */
#include "download.h"

struct install_info {
  char name[MAX_LEN_NAME];
  enum modpack_type type;
  char version[MAX_LEN_NAME];
};

#define SPECLIST_TAG install_info
#define SPECLIST_TYPE struct install_info
#include "speclist.h"

#define install_info_list_iterate(ii_list, item)                            \
  TYPED_LIST_ITERATE(struct install_info, ii_list, item)
#define install_info_list_iterate_end LIST_ITERATE_END

/* Backward compatibility to pre-sqlite database versions */
void load_install_info_list(const char *filename);

void create_mpdb(const char *filename, bool scenario_db);
void open_mpdb(const char *filename, bool scenario_db);
void close_mpdbs(void);

bool mpdb_update_modpack(const char *name, enum modpack_type type,
                         const char *version);
const char *mpdb_installed_version(const char *name, enum modpack_type type);

#endif /* FC__MPDB_H */
