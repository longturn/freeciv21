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

#include <algorithm>

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdlib.h> /* qsort() */
#include <string.h>

/* utility */
#include "astring.h"
#include "shared.h"
#include "support.h"

#include "string_vector.h"

/* The string vector structure. */
struct strvec {
  char **vec;
  size_t size;
};

/**********************************************************************/ /**
   Free a string.
 **************************************************************************/
static void string_free(char *string)
{
  if (string) {
    delete[] string;
  }
}

/**********************************************************************/ /**
   Duplicate a string.
 **************************************************************************/
static char *string_duplicate(const char *string)
{
  if (string) {
    return fc_strdup(string);
  }
  return NULL;
}

/**********************************************************************/ /**
   Create a new string vector.
 **************************************************************************/
struct strvec *strvec_new(void)
{
  strvec *psv = new strvec;

  psv->vec = NULL;
  psv->size = 0;

  return psv;
}

/**********************************************************************/ /**
   Destroy a string vector.
 **************************************************************************/
void strvec_destroy(struct strvec *psv)
{
  strvec_clear(psv);
  delete psv;
}

/**********************************************************************/ /**
   Set the size of the vector.
 **************************************************************************/
void strvec_reserve(struct strvec *psv, size_t reserve)
{
  if (reserve == psv->size) {
    return;
  } else if (reserve == 0) {
    strvec_clear(psv);
    return;
  } else if (!psv->vec) {
    /* Initial reserve */
    psv->vec = new char *[reserve] {};
  } else if (reserve > psv->size) {
    /* Expand the vector. */
    auto expanded = new char *[reserve] {};
    std::move(psv->vec, psv->vec + psv->size, expanded);
    delete[] psv->vec;
    psv->vec = expanded;
  } else {
    /* Shrink the vector: free the extra strings. */
    size_t i;

    for (i = psv->size - 1; i >= reserve; i--) {
      string_free(psv->vec[i]);
    }
    auto shrunk = new char *[reserve] {};
    std::move(psv->vec, psv->vec + reserve, shrunk);
    delete[] psv->vec;
    psv->vec = shrunk;
  }
  psv->size = reserve;
}

/**********************************************************************/ /**
   Stores the string vector from a normal vector. If size == -1, it will
   assume it is a NULL terminated vector.
 **************************************************************************/
void strvec_store(struct strvec *psv, const char *const *vec, size_t size)
{
  if (size == (size_t) -1) {
    strvec_clear(psv);
    for (; *vec; vec++) {
      strvec_append(psv, *vec);
    }
  } else {
    size_t i;

    strvec_reserve(psv, size);
    for (i = 0; i < size; i++, vec++) {
      strvec_set(psv, i, *vec);
    }
  }
}

/**********************************************************************/ /**
   Remove all strings from the vector.
 **************************************************************************/
void strvec_clear(struct strvec *psv)
{
  size_t i;
  char **p;

  if (!psv->vec) {
    return;
  }

  for (i = 0, p = psv->vec; i < psv->size; i++, p++) {
    string_free(*p);
  }
  delete[] psv->vec;
  psv->vec = NULL;
  psv->size = 0;
}


/**********************************************************************/ /**
   Insert a string at the start of the vector.
 **************************************************************************/
void strvec_prepend(struct strvec *psv, const char *string)
{
  strvec_reserve(psv, psv->size + 1);
  memmove(psv->vec + 1, psv->vec, (psv->size - 1) * sizeof(char *));
  psv->vec[0] = string_duplicate(string);
}

/**********************************************************************/ /**
   Insert a string at the end of the vector.
 **************************************************************************/
void strvec_append(struct strvec *psv, const char *string)
{
  strvec_reserve(psv, psv->size + 1);
  psv->vec[psv->size - 1] = string_duplicate(string);
}

/**********************************************************************/ /**
   Insert a string at the index of the vector.
 **************************************************************************/
void strvec_insert(struct strvec *psv, size_t svindex, const char *string)
{
  if (svindex <= 0) {
    strvec_prepend(psv, string);
  } else if (svindex >= psv->size) {
    strvec_append(psv, string);
  } else {
    strvec_reserve(psv, psv->size + 1);
    memmove(psv->vec + svindex + 1, psv->vec + svindex,
            (psv->size - svindex - 1) * sizeof(char *));
    psv->vec[svindex] = string_duplicate(string);
  }
}

/**********************************************************************/ /**
   Replace a string at the index of the vector.
   Returns TRUE if the element has been really set.
 **************************************************************************/
bool strvec_set(struct strvec *psv, size_t svindex, const char *string)
{
  if (strvec_index_valid(psv, svindex)) {
    string_free(psv->vec[svindex]);
    psv->vec[svindex] = string_duplicate(string);
    return TRUE;
  }
  return FALSE;
}


/**********************************************************************/ /**
   Returns the size of the vector.
 **************************************************************************/
size_t strvec_size(const struct strvec *psv) { return psv->size; }

/**********************************************************************/ /**
   Returns the datas of the vector.
 **************************************************************************/
const char *const *strvec_data(const struct strvec *psv)
{
  return (const char **) psv->vec;
}

/**********************************************************************/ /**
   Returns TRUE if the index is valid.
 **************************************************************************/
bool strvec_index_valid(const struct strvec *psv, size_t svindex)
{
  return svindex < psv->size;
}

/**********************************************************************/ /**
   Returns the string at the index of the vector.
 **************************************************************************/
const char *strvec_get(const struct strvec *psv, size_t svindex)
{
  return strvec_index_valid(psv, svindex) ? psv->vec[svindex] : NULL;
}

/**********************************************************************/ /**
   Build a localized string with the elements of the string vector. Elements
   will be "or"-separated.

   See also astr_build_or_list(), strvec_to_and_list().
 **************************************************************************/
const char *strvec_to_or_list(const struct strvec *psv, struct astring *astr)
{
  fc_assert_ret_val(NULL != psv, NULL);
  return astr_build_or_list(astr, (const char **) psv->vec, psv->size);
}

/**********************************************************************/ /**
   Build a localized string with the elements of the string vector. Elements
   will be "and"-separated.

   See also astr_build_and_list(), strvec_to_or_list().
 **************************************************************************/
const char *strvec_to_and_list(const struct strvec *psv,
                               struct astring *astr)
{
  fc_assert_ret_val(NULL != psv, NULL);
  return astr_build_and_list(astr, (const char **) psv->vec, psv->size);
}

/**********************************************************************/ /**
   Stores the string vector from a normal vector. If size == -1, it will
   assume it is a NULL terminated vector.
 **************************************************************************/
void qstrvec_store(QVector<QString> *psv, const char *const *vec,
                   size_t size)
{
  if (size == (size_t) -1) {
    psv->clear();
    for (; *vec; vec++) {
      psv->append(*vec);
    }
  } else {
    size_t i;
    psv->resize(size);
    for (i = 0; i < size; i++, vec++) {
      psv->replace(i, *vec);
    }
  }
}
