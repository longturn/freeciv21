/**************************************************************************
    Copyright (c) 1996-2020 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
**************************************************************************/

// utility
#include "fcintl.h"
#include "log.h"

// common
#include "city.h"
#include "effects.h"
#include "game.h"

#include "specialist.h"

struct specialist specialists[SP_MAX];
int default_specialist;

/**
   Initialize data for specialists.
 */
void specialists_init()
{
  int i;

  for (i = 0; i < ARRAY_SIZE(specialists); i++) {
    struct specialist *p = &specialists[i];

    p->item_number = i;
    p->ruledit_disabled = false;

    requirement_vector_init(&p->reqs);
  }
}

/**
   Free data for specialists.
 */
void specialists_free()
{
  int i;

  for (i = 0; i < ARRAY_SIZE(specialists); i++) {
    struct specialist *p = &specialists[i];

    requirement_vector_free(&p->reqs);
    delete p->helptext;
    p->helptext = nullptr;
  }
}

/**
   Return the number of specialist_types.
 */
Specialist_type_id specialist_count()
{
  return game.control.num_specialist_types;
}

/**
   Return the specialist index.

   Currently same as specialist_number(), paired with specialist_count()
   indicates use as an array index.
 */
Specialist_type_id specialist_index(const struct specialist *sp)
{
  fc_assert_ret_val(nullptr != sp, 0);
  return sp - specialists;
}

/**
   Return the specialist item_number.
 */
Specialist_type_id specialist_number(const struct specialist *sp)
{
  fc_assert_ret_val(nullptr != sp, 0);
  return sp->item_number;
}

/**
   Return the specialist pointer for the given number.
 */
struct specialist *specialist_by_number(const Specialist_type_id id)
{
  if (id < 0 || id >= game.control.num_specialist_types) {
    return nullptr;
  }
  return &specialists[id];
}

/**
   Return the specialist type with the given (untranslated!) rule name.
   Returns nullptr if none match.
 */
struct specialist *specialist_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  specialist_type_iterate(i)
  {
    struct specialist *sp = specialist_by_number(i);
    if (0 == fc_strcasecmp(specialist_rule_name(sp), qname)) {
      return sp;
    }
  }
  specialist_type_iterate_end;

  return nullptr;
}

/**
   Return the specialist type with the given (translated, plural) name.
   Returns nullptr if none match.
 */
struct specialist *specialist_by_translated_name(const char *name)
{
  specialist_type_iterate(i)
  {
    struct specialist *sp = specialist_by_number(i);
    if (0 == strcmp(specialist_plural_translation(sp), name)) {
      return sp;
    }
  }
  specialist_type_iterate_end;

  return nullptr;
}

/**
   Return the (untranslated) rule name of the specialist type.
   You don't have to free the return pointer.
 */
const char *specialist_rule_name(const struct specialist *sp)
{
  return rule_name_get(&sp->name);
}

/**
   Return the (translated, plural) name of the specialist type.
   You don't have to free the return pointer.
 */
const char *specialist_plural_translation(const struct specialist *sp)
{
  return name_translation_get(&sp->name);
}

/**
   Return the (translated) abbreviation of the specialist type.
   You don't have to free the return pointer.
 */
const char *specialist_abbreviation_translation(const struct specialist *sp)
{
  return name_translation_get(&sp->abbreviation);
}

/**
   Return a string containing all the specialist abbreviations, for instance
   "E/S/T".
   You don't have to free the return pointer.
 */
const char *specialists_abbreviation_string()
{
  static char buf[5 * SP_MAX];

  buf[0] = '\0';

  specialist_type_iterate(sp)
  {
    const char *separator = (buf[0] == '\0') ? "" : "/";

    cat_snprintf(
        buf, sizeof(buf), "%s%s", separator,
        specialist_abbreviation_translation(specialist_by_number(sp)));
  }
  specialist_type_iterate_end;

  return buf;
}

/**
   Return a string showing the number of specialists in the array.

   For instance with a city with (0,3,1) specialists call

     specialists_string(pcity->specialists);

   and you'll get "0/3/1".
 */
const char *specialists_string(const citizens *specialist_list)
{
  static char buf[5 * SP_MAX];

  buf[0] = '\0';

  specialist_type_iterate(sp)
  {
    const char *separator = (buf[0] == '\0') ? "" : "/";

    cat_snprintf(buf, sizeof(buf), "%s%d", separator, specialist_list[sp]);
  }
  specialist_type_iterate_end;

  return buf;
}

/**
   Return the output for the specialist type with this output type.
 */
int get_specialist_output(const struct city *pcity, Specialist_type_id sp,
                          Output_type_id otype)
{
  struct specialist *pspecialist = &specialists[sp];
  struct output_type *poutput = get_output_type(otype);

  return get_city_specialist_output_bonus(pcity, pspecialist, poutput,
                                          EFT_SPECIALIST_OUTPUT);
}
