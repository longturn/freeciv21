// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "multipliers.h"

// utility
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "fc_types.h"
#include "game.h"
#include "name_translation.h"
#include "player.h"
#include "requirements.h"

static struct multiplier multipliers[MAX_NUM_MULTIPLIERS];

/**
   Initialize all multipliers
 */
void multipliers_init()
{
  int i;

  for (i = 0; i < ARRAY_SIZE(multipliers); i++) {
    name_init(&multipliers[i].name);
    requirement_vector_init(&multipliers[i].reqs);
    multipliers[i].ruledit_disabled = false;
    multipliers[i].helptext = nullptr;
  }
}

/**
   Free all multipliers
 */
void multipliers_free()
{
  multipliers_iterate(pmul)
  {
    requirement_vector_free(&(pmul->reqs));
    delete pmul->helptext;
    pmul->helptext = nullptr;
  }
  multipliers_iterate_end;
}

/**
   Returns multiplier associated to given number
 */
struct multiplier *multiplier_by_number(Multiplier_type_id id)
{
  fc_assert_ret_val(id >= 0 && id < game.control.num_multipliers, nullptr);

  return &multipliers[id];
}

/**
   Returns multiplier number.
 */
Multiplier_type_id multiplier_number(const struct multiplier *pmul)
{
  fc_assert_ret_val(nullptr != pmul, 0);

  return pmul - multipliers;
}

/**
   Returns multiplier index.

   Currently same as multiplier_number(), paired with multiplier_count()
   indicates use as an array index.
 */
Multiplier_type_id multiplier_index(const struct multiplier *pmul)
{
  return multiplier_number(pmul);
}

/**
   Return number of loaded multipliers in the ruleset.
 */
Multiplier_type_id multiplier_count()
{
  return game.control.num_multipliers;
}

/**
   Return the (translated) name of the multiplier.
   You don't have to free the return pointer.
 */
const char *multiplier_name_translation(const struct multiplier *pmul)
{
  return name_translation_get(&pmul->name);
}

/**
   Return the (untranslated) rule name of the multiplier.
   You don't have to free the return pointer.
 */
const char *multiplier_rule_name(const struct multiplier *pmul)
{
  return rule_name_get(&pmul->name);
}

/**
   Returns multiplier matching rule name, or nullptr if there is no
   multiplier with such a name.
 */
struct multiplier *multiplier_by_rule_name(const char *name)
{
  const char *qs;

  if (name == nullptr) {
    return nullptr;
  }

  qs = Qn_(name);

  multipliers_iterate(pmul)
  {
    if (!fc_strcasecmp(multiplier_rule_name(pmul), qs)) {
      return pmul;
    }
  }
  multipliers_iterate_end;

  return nullptr;
}

/**
   Can player change multiplier value
 */
bool multiplier_can_be_changed(struct multiplier *pmul,
                               struct player *pplayer)
{
  return are_reqs_active(pplayer, nullptr, nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr, nullptr, nullptr,
                         &pmul->reqs, RPT_CERTAIN);
}
