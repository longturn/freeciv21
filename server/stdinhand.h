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
#pragma once

#include <QString>
#include <QVector>
/* common */
#include "chat.h"       /* SERVER_COMMAND_PREFIX */
#include "connection.h" /* enum cmdlevel */
#include "fc_types.h"

/* server */
#include "commands.h"
#include "console.h"

void stdinhand_init(void);
void stdinhand_turn(void);
void stdinhand_free(void);

void cmd_reply(enum command_id cmd, struct connection *caller,
               enum rfc_status rfc_status, const char *format, ...)
    fc__attribute((__format__(__printf__, 4, 5)));

bool handle_stdin_input(struct connection *caller, char *str);
void set_ai_level_direct(struct player *pplayer, enum ai_level level);
bool read_init_script(struct connection *caller, const char *script_filename,
                      bool from_cmdline, bool check);
QVector<QString> *get_init_script_choices(void);
void show_players(struct connection *caller);

enum rfc_status create_command_newcomer(const char *name, const char *ai,
                                        bool check,
                                        struct nation_type *pnation,
                                        struct player **newplayer, char *buf,
                                        size_t buflen);
enum rfc_status create_command_pregame(const char *name, const char *ai,
                                       bool check, struct player **newplayer,
                                       char *buf, size_t buflen);

bool load_command(struct connection *caller, const char *filename,
                  bool check, bool cmdline_load);
bool start_command(struct connection *caller, bool check, bool notify);

void toggle_ai_player_direct(struct connection *caller,
                             struct player *pplayer);

/* for sernet.c in initing a new connection */
enum cmdlevel access_level_for_next_connection(void);

void notify_if_first_access_level_is_available(void);

bool conn_is_kicked(struct connection *pconn, int *time_remaining);

void set_running_game_access_level(void);

char **freeciv_completion(const char *text, int start, int end);
