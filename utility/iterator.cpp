// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "iterator.h"

/**
   'next' function implementation for an "invalid" iterator.
 */
static void invalid_iter_next(struct iterator *it)
{ // Do nothing.
}

/**
   'get' function implementation for an "invalid" iterator.
 */
static void *invalid_iter_get(const struct iterator *it) { return nullptr; }

/**
   'valid' function implementation for an "invalid" iterator.
 */
static bool invalid_iter_valid(const struct iterator *it) { return false; }

/**
   Initializes the iterator vtable so that generic_iterate assumes that
   the iterator is invalid.
 */
struct iterator *invalid_iter_init(struct iterator *it)
{
  it->next = invalid_iter_next;
  it->get = invalid_iter_get;
  it->valid = invalid_iter_valid;
  return it;
}
