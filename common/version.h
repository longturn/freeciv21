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

/* This is only used in version.c, and only if IS_BETA_VERSION is true.
 * The month[] array is defined in version.c (index: 1 == Jan, 2 == Feb,
 * ...).
 */
#ifndef NEXT_RELEASE_MONTH
#define NEXT_RELEASE_MONTH (month[FREECIV_RELEASE_MONTH])
#endif

/* version informational strings */
const char *freeciv_name_version(void);
const char *word_version(void);
const char *fc_git_revision(void);
const char *fc_comparable_version(void);
const char *freeciv_datafile_version(void);

const char *freeciv_motto(void);

/* If returns NULL, not a beta version. */
const char *beta_message(void);




