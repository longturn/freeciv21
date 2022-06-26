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

// utility
#include "support.h"

// common
#include "improvement.h"
#include "requirements.h"
#include "unittype.h"

#include "advchoice.h"

/**
   Sets the values of the choice to initial values.
 */
void adv_init_choice(struct adv_choice *choice)
{
  choice->value.utype = nullptr;
  choice->want = 0;
  choice->type = CT_NONE;
  choice->need_boat = false;
#ifdef ADV_CHOICE_TRACK
  choice->use = nullptr;
  choice->log_if_chosen = FALSE;
#endif // ADV_CHOICE_TRACK
}

/**
   Clear choice without freeing it itself
 */
void adv_deinit_choice(struct adv_choice *choice)
{
#ifdef ADV_CHOICE_TRACK
  if (choice->use != nullptr) {
    free(choice->use);
    choice->use = nullptr;
  }
#endif // ADV_CHOICE_TRACK
}

/**
   Dynamically allocate a new choice.
 */
struct adv_choice *adv_new_choice()
{
  struct adv_choice *choice = new adv_choice();

  adv_init_choice(choice);

  return choice;
}

/**
   Free dynamically allocated choice.
 */
void adv_free_choice(struct adv_choice *choice)
{
#ifdef ADV_CHOICE_TRACK
  if (choice->use != nullptr) {
    free(choice->use);
  }
#endif // ADV_CHOICE_TRACK
  delete choice;
}

/**
   Return better one of the choices given. In case of a draw, first one
   is preferred.
 */
struct adv_choice *adv_better_choice(struct adv_choice *first,
                                     struct adv_choice *second)
{
  if (second->want > first->want) {
    return second;
  } else {
    return first;
  }
}

/**
   Return better one of the choices given, and free the other.
 */
struct adv_choice *adv_better_choice_free(struct adv_choice *first,
                                          struct adv_choice *second)
{
  if (second->want > first->want) {
    adv_free_choice(first);

    return second;
  } else {
    adv_free_choice(second);

    return first;
  }
}

/**
   Does choice type refer to unit
 */
bool is_unit_choice_type(enum choice_type type)
{
  return type == CT_CIVILIAN || type == CT_ATTACKER || type == CT_DEFENDER;
}

#ifdef ADV_CHOICE_TRACK
/**
   Copy contents of one choice structure to the other
 */
void adv_choice_copy(struct adv_choice *dest, struct adv_choice *src)
{
  if (dest != src) {
    dest->type = src->type;
    dest->value = src->value;
    dest->want = src->want;
    dest->need_boat = src->need_boat;
    if (dest->use != nullptr) {
      free(dest->use);
    }
    if (src->use != nullptr) {
      dest->use = fc_strdup(src->use);
    } else {
      dest->use = nullptr;
    }
    dest->log_if_chosen = src->log_if_chosen;
  }
}

/**
   Set the use the choice is meant for.
 */
void adv_choice_set_use(struct adv_choice *choice, const char *use)
{
  if (choice->use != nullptr) {
    free(choice->use);
  }
  choice->use = fc_strdup(use);
}

/**
   Log the choice information.
 */
void adv_choice_log_info(struct adv_choice *choice, const char *loc1,
                         const char *loc2)
{
  const char *use;
  const char *name;

  if (choice->use != nullptr) {
    use = choice->use;
  } else {
    use = "<unknown>";
  }

  if (choice->type == CT_BUILDING) {
    name = improvement_rule_name(choice->value.building);
  } else if (choice->type == CT_NONE) {
    name = "None";
  } else {
    name = utype_rule_name(choice->value.utype);
  }

  if (loc2 != nullptr) {
    log_base(ADV_CHOICE_QtMsgType,
             "Choice at \"%s:%s\": %s, "
             "want " ADV_WANT_PRINTF " as %s (%d)",
             loc1, loc2, name, choice->want, use, choice->type);
  } else {
    log_base(ADV_CHOICE_QtMsgType,
             "Choice at \"%s\": %s, "
             "want " ADV_WANT_PRINTF " as %s (%d)",
             loc1, name, choice->want, use, choice->type);
  }
}

/**
   Return choice use string
 */
const char *adv_choice_get_use(const struct adv_choice *choice)
{
  if (choice->use == nullptr) {
    return "(unset)";
  }

  return choice->use;
}

#endif // ADV_CHOICE_TRACK
