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
#ifndef FC__META_H
#define FC__META_H


#include "support.h" /* bool type */

// Forward declarations
class QString;

#define DEFAULT_META_SERVER_NO_SEND TRUE
#define DEFAULT_META_SERVER_ADDR FREECIV_META_URL
#define METASERVER_REFRESH_INTERVAL (3 * 60)
#define METASERVER_MIN_UPDATE_INTERVAL 7 /* not too short, not too long */

enum meta_flag { META_INFO, META_REFRESH, META_GOODBYE };

const char *default_meta_patches_string(void);
const char *default_meta_message_string(void);

const char *get_meta_patches_string(void);
const char *get_meta_message_string(void);
const char *get_user_meta_message_string(void);

void maybe_automatic_meta_message(const char *automatic);

void set_meta_patches_string(const char *string);
void set_meta_message_string(const char *string);
void set_user_meta_message_string(const char *string);

QString meta_addr_port();

void server_close_meta(void);
bool server_open_meta(bool persistent);
bool is_metaserver_open(void);

bool send_server_info_to_metaserver(enum meta_flag flag);

#endif /* FC__META_H */
