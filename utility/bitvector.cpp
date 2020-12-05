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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "rand.h"
#include "string_vector.h"

#include "bitvector.h"

/* bv_*  - static bitvectors; used for data which where the length is
           fixed (number of players; flags for enums; ...). They are
           named bv_* and the macros BV_* are defined.
 */

/***********************************************************************/ /**
   Return whether two vectors: vec1 and vec2 have common
   bits. I.e. (vec1 & vec2) != 0.

   Don't call this function directly, use BV_CHECK_MASK macro
   instead. Don't call this function with two different bitvectors.
 ***************************************************************************/
bool bv_check_mask(const unsigned char *vec1, const unsigned char *vec2,
                   size_t size1, size_t size2)
{
  size_t i;
  fc_assert_ret_val(size1 == size2, FALSE);

  for (i = 0; i < size1; i++) {
    if ((vec1[0] & vec2[0]) != 0) {
      return TRUE;
    }
    vec1++;
    vec2++;
  }
  return FALSE;
}

/***********************************************************************/ /**
   Compares elements of two bitvectors. Both vectors are expected to have
   same number of elements, i.e. , size1 must be equal to size2.
 ***************************************************************************/
bool bv_are_equal(const unsigned char *vec1, const unsigned char *vec2,
                  size_t size1, size_t size2)
{
  size_t i;
  fc_assert_ret_val(size1 == size2, FALSE);

  for (i = 0; i < size1; i++) {
    if (vec1[0] != vec2[0]) {
      return FALSE;
    }
    vec1++;
    vec2++;
  }
  return TRUE;
}

/***********************************************************************/ /**
   Set everything that is true in vec_from in vec_to. Stuff that already is
   true in vec_to aren't touched. (Bitwise inclusive OR assignment)

   Both vectors are expected to have same number of elements,
   i.e. , size1 must be equal to size2.

   Don't call this function directly, use BV_SET_ALL_FROM macro
   instead.
 ***************************************************************************/
void bv_set_all_from(unsigned char *vec_to, const unsigned char *vec_from,
                     size_t size_to, size_t size_from)
{
  size_t i;

  fc_assert_ret(size_to == size_from);

  for (i = 0; i < size_to; i++) {
    vec_to[i] |= vec_from[i];
  }
}

/***********************************************************************/ /**
   Clear everything that is true in vec_from in vec_to. Stuff that already
   is false in vec_to aren't touched.

   Both vectors are expected to have same number of elements,
   i.e. , size1 must be equal to size2.

   Don't call this function directly, use BV_CLR_ALL_FROM macro
   instead.
 ***************************************************************************/
void bv_clr_all_from(unsigned char *vec_to, const unsigned char *vec_from,
                     size_t size_to, size_t size_from)
{
  size_t i;

  fc_assert_ret(size_to == size_from);

  for (i = 0; i < size_to; i++) {
    vec_to[i] &= ~vec_from[i];
  }
}
