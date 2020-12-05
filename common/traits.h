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
#ifndef FC__TRAITS_H
#define FC__TRAITS_H



#define SPECENUM_NAME trait
#define SPECENUM_VALUE0 TRAIT_EXPANSIONIST
#define SPECENUM_VALUE0NAME "Expansionist"
#define SPECENUM_VALUE1 TRAIT_TRADER
#define SPECENUM_VALUE1NAME "Trader"
#define SPECENUM_VALUE2 TRAIT_AGGRESSIVE
#define SPECENUM_VALUE2NAME "Aggressive"
#define SPECENUM_VALUE3 TRAIT_BUILDER
#define SPECENUM_VALUE3NAME "Builder"
#define SPECENUM_COUNT TRAIT_COUNT
#include "specenum_gen.h"

#define TRAIT_DEFAULT_VALUE 50
#define TRAIT_MAX_VALUE (TRAIT_DEFAULT_VALUE * TRAIT_DEFAULT_VALUE)
#define TRAIT_MAX_VALUE_SR (TRAIT_DEFAULT_VALUE)

struct ai_trait {
  int val; /* Value assigned in the beginning */
  int mod; /* This is modification that changes during game. */
};

struct trait_limits {
  int min;
  int max;
  int fixed;
};



#endif /* FC__TRAITS_H */
