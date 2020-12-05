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
#ifndef FC__BITVECTOR_H
#define FC__BITVECTOR_H

#include <stdlib.h> /* size_t */
#include <string.h> /* memset */

/* utility */
#include "log.h"
#include "support.h" /* bool, fc__attribute */

/* Yields TRUE iff the bit bit_no is set in val. */
#define TEST_BIT(val, bit_no)                                               \
  (((val) & (1u << (bit_no))) == (1u << (bit_no)))

/* Static bitvectors. */
#define _BV_BYTES(bits) ((((bits) -1) / 8) + 1)
#define _BV_BYTE_INDEX(bits) ((bits) / 8)
#define _BV_BITMASK(bit) (1u << ((bit) &0x7))
#ifdef FREECIV_DEBUG
#define _BV_ASSERT(bv, bit)                                                 \
  fc_assert((bit) >= 0 && (bit) < (signed int) sizeof((bv).vec) * 8)
#else
#define _BV_ASSERT(bv, bit) (void) 0
#endif

template <class BV> inline bool BV_ISSET(const BV &bv, int bit)
{
  fc_assert_ret_val(bit >= 0 && bit < (signed int) sizeof(bv.vec) * 8,
                    false);
  return (bv.vec[_BV_BYTE_INDEX(bit)] & _BV_BITMASK(bit)) != 0;
}

#define BV_SET(bv, bit)                                                     \
  do {                                                                      \
    _BV_ASSERT(bv, bit);                                                    \
    (bv).vec[_BV_BYTE_INDEX(bit)] |= _BV_BITMASK(bit);                      \
  } while (FALSE)
#define BV_CLR(bv, bit)                                                     \
  do {                                                                      \
    _BV_ASSERT(bv, bit);                                                    \
    (bv).vec[_BV_BYTE_INDEX(bit)] &= ~_BV_BITMASK(bit);                     \
  } while (FALSE)
#define BV_SET_VAL(bv, bit, val)                                            \
  do {                                                                      \
    if (val) {                                                              \
      BV_SET(bv, bit);                                                      \
    } else {                                                                \
      BV_CLR(bv, bit);                                                      \
    }                                                                       \
  } while (FALSE);
#define BV_CLR_ALL(bv)                                                      \
  do {                                                                      \
    memset((bv).vec, 0, sizeof((bv).vec));                                  \
  } while (FALSE)
#define BV_SET_ALL(bv)                                                      \
  do {                                                                      \
    memset((bv).vec, 0xff, sizeof((bv).vec));                               \
  } while (FALSE)

bool bv_check_mask(const unsigned char *vec1, const unsigned char *vec2,
                   size_t size1, size_t size2);
#define BV_CHECK_MASK(vec1, vec2)                                           \
  bv_check_mask((vec1).vec, (vec2).vec, sizeof((vec1).vec),                 \
                sizeof((vec2).vec))
#define BV_ISSET_ANY(vec) BV_CHECK_MASK(vec, vec)

bool bv_are_equal(const unsigned char *vec1, const unsigned char *vec2,
                  size_t size1, size_t size2);
#define BV_ARE_EQUAL(vec1, vec2)                                            \
  bv_are_equal((vec1).vec, (vec2).vec, sizeof((vec1).vec),                  \
               sizeof((vec2).vec))

void bv_set_all_from(unsigned char *vec_to, const unsigned char *vec_from,
                     size_t size_to, size_t size_from);
#define BV_SET_ALL_FROM(vec_to, vec_from)                                   \
  bv_set_all_from((vec_to).vec, (vec_from).vec, sizeof((vec_to).vec),       \
                  sizeof((vec_from).vec))

void bv_clr_all_from(unsigned char *vec_to, const unsigned char *vec_from,
                     size_t size_to, size_t size_from);
#define BV_CLR_ALL_FROM(vec_to, vec_from)                                   \
  bv_clr_all_from((vec_to).vec, (vec_from).vec, sizeof((vec_to).vec),       \
                  sizeof((vec_from).vec))

/* Used to make a BV typedef. Such types are usually called "bv_foo". */
#define BV_DEFINE(name, bits)                                               \
  typedef struct {                                                          \
    unsigned char vec[_BV_BYTES(bits)];                                     \
  } name

#endif /* FC__BITVECTOR_H */
