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
#ifndef FC__LOCALLUACONF_H
#define FC__LOCALLUACONF_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* Lua headers want to define VERSION to lua version */
#undef VERSION

#if defined(HAVE_MKSTEMP) && defined(FREECIV_HAVE_UNISTD_H)
#define LUA_USE_MKSTEMP
#endif
#if defined(HAVE_POPEN) && defined(HAVE_PCLOSE)
#define LUA_USE_POPEN
#endif
#if defined(HAVE__LONGJMP) && defined(HAVE__SETJMP)
#define LUA_USE_ULONGJMP
#endif
#if defined(HAVE_GMTIME_R) && defined(HAVE_LOCALTIME_R)
#define LUA_USE_GMTIME_R
#endif
#if defined(HAVE_FSEEKO)
#define LUA_USE_FSEEKO
#endif

#endif /* FC__LOCALLUACONF_H */
