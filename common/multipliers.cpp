/***********************************************************************
 Freeciv - Copyright (C) 2014 Lach Sławomir
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "fcintl.h"

/* common */
#include "game.h"

#include "multipliers.h"

static struct multiplier multipliers[MAX_NUM_MULTIPLIERS];

/************************************************************************/ /**
   Initialize all multipliers
 ****************************************************************************/
void multipliers_init()
{
  int i;

  for (i = 0; i < ARRAY_SIZE(multipliers); i++) {
    name_init(&multipliers[i].name);
    requirement_vector_init(&multipliers[i].reqs);
    multipliers[i].ruledit_disabled = false;
    multipliers[i].helptext = NULL;
  }
}

/************************************************************************/ /**
   Free all multipliers
 ****************************************************************************/
void multipliers_free()
{
  multipliers_iterate(pmul)
  {
    requirement_vector_free(&(pmul->reqs));
    NFCN_FREE(pmul->helptext);
  }
  multipliers_iterate_end;
}

/************************************************************************/ /**
   Returns multiplier associated to given number
 ****************************************************************************/
struct multiplier *multiplier_by_number(Multiplier_type_id id)
{
  fc_assert_ret_val(id >= 0 && id < game.control.num_multipliers, NULL);

  return &multipliers[id];
}

/************************************************************************/ /**
   Returns multiplier number.
 ****************************************************************************/
Multiplier_type_id multiplier_number(const struct multiplier *pmul)
{
  fc_assert_ret_val(NULL != pmul, 0);

  return pmul - multipliers;
}

/************************************************************************/ /**
   Returns multiplier index.

   Currently same as multiplier_number(), paired with multiplier_count()
   indicates use as an array index.
 ****************************************************************************/
Multiplier_type_id multiplier_index(const struct multiplier *pmul)
{
  return multiplier_number(pmul);
}

/************************************************************************/ /**
   Return number of loaded multipliers in the ruleset.
 ****************************************************************************/
Multiplier_type_id multiplier_count()
{
  return game.control.num_multipliers;
}

/************************************************************************/ /**
   Return the (translated) name of the multiplier.
   You don't have to free the return pointer.
 ****************************************************************************/
const char *multiplier_name_translation(const struct multiplier *pmul)
{
  return name_translation_get(&pmul->name);
}

/************************************************************************/ /**
   Return the (untranslated) rule name of the multiplier.
   You don't have to free the return pointer.
 ****************************************************************************/
const char *multiplier_rule_name(const struct multiplier *pmul)
{
  return rule_name_get(&pmul->name);
}

/************************************************************************/ /**
   Returns multiplier matching rule name, or NULL if there is no multiplier
   with such a name.
 ****************************************************************************/
struct multiplier *multiplier_by_rule_name(const char *name)
{
  const char *qs;

  if (name == NULL) {
    return NULL;
  }

  qs = Qn_(name);

  multipliers_iterate(pmul)
  {
    if (!fc_strcasecmp(multiplier_rule_name(pmul), qs)) {
      return pmul;
    }
  }
  multipliers_iterate_end;

  return NULL;
}

/************************************************************************/ /**
   Can player change multiplier value
 ****************************************************************************/
bool multiplier_can_be_changed(struct multiplier *pmul,
                               struct player *pplayer)
{
  return are_reqs_active(pplayer, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, &pmul->reqs, RPT_CERTAIN);
}
