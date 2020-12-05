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
#ifndef FC__CHAT_H
#define FC__CHAT_H

/* Definitions related to interpreting chat messages.
 * Behaviour generally can't be changed at whim because client and
 * server are assumed to agree on these details. */



/* Special characters in chatlines. */

/* The character to mark chatlines as server commands */
/* FIXME this is still hard-coded in a lot of places */
#define SERVER_COMMAND_PREFIX '/'
#define CHAT_ALLIES_PREFIX '.'
#define CHAT_DIRECT_PREFIX ':'



#endif /* FC__CHAT_H */
