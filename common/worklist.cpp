// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "worklist.h"

// utility
#include "log.h"
#include "shared.h"

// common
#include "fc_types.h"
#include "requirements.h"

// std
#include <cstring> // str*, mem*

/**
   Initialize a worklist to be empty.
   For elements, only really need to set [0], but initialize the
   rest to avoid junk values in savefile.
 */
void worklist_init(struct worklist *pwl)
{
  int i;

  pwl->length = 0;

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    // just setting the entry to zero:
    pwl->entries[i].kind = VUT_NONE;
    // all the union pointers should be in the same place:
    pwl->entries[i].value.building = nullptr;
  }
}

/**
   Returns the number of entries in the worklist.  The returned value can
   also be used as the next available worklist index (assuming that
   len < MAX_LEN_WORKLIST).
 */
int worklist_length(const struct worklist *pwl)
{
  fc_assert_ret_val(pwl->length >= 0 && pwl->length <= MAX_LEN_WORKLIST, 0);
  return pwl->length;
}

/**
   Returns whether worklist has no elements.
 */
bool worklist_is_empty(const struct worklist *pwl)
{
  return !pwl || pwl->length == 0;
}

/**
   Fill in the id and is_unit values for the head of the worklist
   if the worklist is non-empty.  Return 1 iff id and is_unit
   are valid.
 */
bool worklist_peek(const struct worklist *pwl, struct universal *prod)
{
  return worklist_peek_ith(pwl, prod, 0);
}

/**
   Fill in the id and is_unit values for the ith element in the
   worklist. If the worklist has fewer than idx elements,
   return FALSE.
 */
bool worklist_peek_ith(const struct worklist *pwl, struct universal *prod,
                       int idx)
{
  // Out of possible bounds.
  if (idx < 0 || pwl->length <= idx) {
    prod->kind = VUT_NONE;
    prod->value.building = nullptr;
    return false;
  }

  *prod = pwl->entries[idx];

  return true;
}

/**
   Remove first element from worklist.
 */
void worklist_advance(struct worklist *pwl) { worklist_remove(pwl, 0); }

/**
   Copy contents from worklist src to worklist dst.
 */
void worklist_copy(struct worklist *dst, const struct worklist *src)
{
  dst->length = src->length;

  memcpy(dst->entries, src->entries, sizeof(struct universal) * src->length);
}

/**
   Remove element from position idx.
 */
void worklist_remove(struct worklist *pwl, int idx)
{
  int i;

  // Don't try to remove something way outside of the worklist.
  if (idx < 0 || pwl->length <= idx) {
    return;
  }

  // Slide everything up one spot.
  for (i = idx; i < pwl->length - 1; i++) {
    pwl->entries[i] = pwl->entries[i + 1];
  }
  // just setting the entry to zero:
  pwl->entries[pwl->length - 1].kind = VUT_NONE;
  // all the union pointers should be in the same place:
  pwl->entries[pwl->length - 1].value.building = nullptr;
  pwl->length--;
}

/**
   Adds the id to the next available slot in the worklist.  'id' is the ID of
   the unit/building to be produced; is_unit specifies whether it's a unit or
   a building.  Returns TRUE if successful.
 */
bool worklist_append(struct worklist *pwl, const struct universal *prod)
{
  int next_index = worklist_length(pwl);

  if (next_index >= MAX_LEN_WORKLIST) {
    return false;
  }

  pwl->entries[next_index] = *prod;
  pwl->length++;

  return true;
}

/**
   Inserts the production at the location idx in the worklist, thus moving
   all subsequent entries down.  'id' specifies the unit/building to
   be produced; is_unit tells whether it's a unit or building.  Returns TRUE
   if successful.
 */
bool worklist_insert(struct worklist *pwl, const struct universal *prod,
                     int idx)
{
  int new_len = MIN(pwl->length + 1, MAX_LEN_WORKLIST), i;

  if (idx < 0 || idx > pwl->length) {
    return false;
  }

  /* move all active values down an index to get room for new id
   * move from [idx .. len - 1] to [idx + 1 .. len].  Any entries at the
   * end are simply lost. */
  for (i = new_len - 2; i >= idx; i--) {
    pwl->entries[i + 1] = pwl->entries[i];
  }

  pwl->entries[idx] = *prod;
  pwl->length = new_len;

  return true;
}

/**
   Return TRUE iff the two worklists are equal.
 */
bool are_worklists_equal(const struct worklist *wlist1,
                         const struct worklist *wlist2)
{
  int i;

  if (wlist1->length != wlist2->length) {
    return false;
  }

  for (i = 0; i < wlist1->length; i++) {
    if (!are_universals_equal(&wlist1->entries[i], &wlist2->entries[i])) {
      return false;
    }
  }

  return true;
}
