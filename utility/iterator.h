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
/***********************************************************************
  Iterator base class. "Derived" iterators must have this struct as
  their first member (as a "vtable") and provide implementations of the
  "pure virtual" member functions. See the function comment headers
  below for the expected behaviour of these functions.
***********************************************************************/
struct iterator {
  void (*next)(struct iterator *it);
  void *(*get)(const struct iterator *it);
  bool (*valid)(const struct iterator *it);
};

#define ITERATOR(p) ((struct iterator *) (p))

/***********************************************************************
  Advances the iterator to point to the next item in the sequence.
***********************************************************************/
static inline void iterator_next(struct iterator *it) { it->next(it); }

/***********************************************************************
  Returns the item currently pointed to by the iterator. Note that the
  iterator could point to an item whose value is nullptr; to actually test
  whether the iterator is still valid (e.g. has not gone past the
  end of the sequence), use iterator_valid().
***********************************************************************/
static inline void *iterator_get(const struct iterator *it)
{
  return it->get(it);
}

/***********************************************************************
  Returns TRUE if the iterator points to an item in the sequence.
***********************************************************************/
static inline bool iterator_valid(const struct iterator *it)
{
  return it->valid(it);
}

/***************************************************************************
  Iteration macro for iterators derived from the 'iterator' base class.
  Usually you would define a specific iteration macro for each derived
  iterator type. The meaning of the arguments is as follows:

  TYPE_it - The type of the derived iterator. E.g. 'struct foo_iter'.
  TYPE_a - The real type of the items in the list. The variable with name
           'NAME_a' will be cast to this.
  NAME_a - The name of the variable that will be assigned the current item
           in each iteration. It will be declared for you within the scope
           of the iteration loop.
  FUNC_size - A function that returns the total size in bytes of a
              'TYPE_it'.
  FUNC_init - A "construtor" for 'TYPE_it' objects. It returns a pointer to
              a 'struct iterator' and takes as its first argument a pointer
              to memory large enough to hold a 'TYPE_it' (this amount must
              match the result of FUNC_size()). NB: This function must not
              return nullptr; it must return a valid iterator pointing to the
              first element in the sequence, or an invalid iterator.
  ... - Zero or more extra arguments that 'FUNC_init' expects.
***************************************************************************/
#define generic_iterate(TYPE_it, TYPE_a, NAME_a, FUNC_size, FUNC_init, ...) \
  do {                                                                      \
    char MY_mem_##NAME_a[FUNC_size()];                                      \
    struct iterator *MY_it_##NAME_a;                                        \
    TYPE_a NAME_a;                                                          \
    MY_it_##NAME_a =                                                        \
        FUNC_init((TYPE_it *) (void *) MY_mem_##NAME_a, ##__VA_ARGS__);     \
    for (; iterator_valid(MY_it_##NAME_a); iterator_next(MY_it_##NAME_a)) { \
      NAME_a = (TYPE_a) iterator_get(MY_it_##NAME_a);

#define generic_iterate_end                                                 \
  }                                                                         \
  }                                                                         \
  while (false)

/***************************************************************************
  Iterator init functions cannot return nullptr, so this dummy helper
function can be used to return a "generic invalid iterator" that will just
exit out of generic_iterate. Its size is just sizeof(struct iterator), so it
  will fit into any iterator's allocated stack memory.
***************************************************************************/
struct iterator *invalid_iter_init(struct iterator *it);
