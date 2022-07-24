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
#include "disaster.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "requirements.h"
#include "specialist.h"
#include "style.h"
#include "tech.h"

#include "validity.h"

struct effect_list_cb_data {
  bool needed;
  struct universal *uni;
  requirers_cb cb;
  void *requirers_data;
};

/**
   Callback to check if effect needs universal.
 */
static bool effect_list_universal_needed_cb(struct effect *peffect,
                                            void *data)
{
  struct effect_list_cb_data *cbdata = (struct effect_list_cb_data *) data;

  if (universal_is_mentioned_by_requirements(&peffect->reqs, cbdata->uni)) {
    cbdata->cb(R__("Effect"), cbdata->requirers_data);
    cbdata->needed = true;
  }

  // Always continue to next until all effects checked
  return true;
}

/**
   Check if anything in ruleset needs universal
 */
static bool is_universal_needed(struct universal *uni, requirers_cb cb,
                                void *data)
{
  bool needed = false;
  bool needed_by_music_style = false;
  int i;
  struct effect_list_cb_data cb_data;

  disaster_type_iterate(pdis)
  {
    if (universal_is_mentioned_by_requirements(&pdis->reqs, uni)) {
      cb(disaster_rule_name(pdis), data);
      needed = true;
    }
  }
  disaster_type_iterate_end;

  improvement_iterate(pimprove)
  {
    if (universal_is_mentioned_by_requirements(&pimprove->reqs, uni)
        || universal_is_mentioned_by_requirements(&pimprove->obsolete_by,
                                                  uni)) {
      cb(improvement_rule_name(pimprove), data);
      needed = true;
    }
  }
  improvement_iterate_end;

  for (auto &pgov : governments) {
    if (universal_is_mentioned_by_requirements(&pgov.reqs, uni)) {
      cb(government_rule_name(&pgov), data);
      needed = true;
    }
  }

  specialist_type_iterate(sp)
  {
    struct specialist *psp = specialist_by_number(sp);

    if (universal_is_mentioned_by_requirements(&psp->reqs, uni)) {
      cb(specialist_rule_name(psp), data);
      needed = true;
    }
  }
  specialist_type_iterate_end;

  extra_type_iterate(pextra)
  {
    if (universal_is_mentioned_by_requirements(&pextra->reqs, uni)
        || universal_is_mentioned_by_requirements(&pextra->rmreqs, uni)) {
      cb(extra_rule_name(pextra), data);
      needed = true;
    }
  }
  extra_type_iterate_end;

  goods_type_iterate(pgood)
  {
    if (universal_is_mentioned_by_requirements(&pgood->reqs, uni)) {
      cb(goods_rule_name(pgood), data);
      needed = true;
    }
  }
  goods_type_iterate_end;

  action_iterate(act)
  {
    action_enabler_list_iterate(action_enablers_for_action(act), enabler)
    {
      if (universal_is_mentioned_by_requirements(&(enabler->actor_reqs), uni)
          || universal_is_mentioned_by_requirements(&(enabler->target_reqs),
                                                    uni)) {
        cb(R__("Action Enabler"), data);
        needed = true;
      }
    }
    action_enabler_list_iterate_end;
  }
  action_iterate_end;

  for (i = 0; i < game.control.styles_count; i++) {
    if (universal_is_mentioned_by_requirements(&city_styles[i].reqs, uni)) {
      cb(city_style_rule_name(i), data);
      needed = true;
    }
  }

  music_styles_iterate(pmus)
  {
    if (universal_is_mentioned_by_requirements(&pmus->reqs, uni)) {
      needed_by_music_style = true;
    }
  }
  music_styles_iterate_end;

  if (needed_by_music_style) {
    cb(R__("Music Style"), data);
    needed = true;
  }

  cb_data.needed = false;
  cb_data.uni = uni;
  cb_data.cb = cb;
  cb_data.requirers_data = data;

  iterate_effect_cache(effect_list_universal_needed_cb, &cb_data);
  needed |= cb_data.needed;

  return needed;
}

/**
   Check if anything in ruleset needs tech
 */
bool is_tech_needed(struct advance *padv, requirers_cb cb, void *data)
{
  struct universal uni = {.value = {.advance = padv}, .kind = VUT_ADVANCE};
  bool needed = false;

  advance_iterate(A_FIRST, pdependant)
  {
    if (pdependant->require[AR_ONE] == padv
        || pdependant->require[AR_TWO] == padv
        || pdependant->require[AR_ROOT] == padv) {
      cb(advance_rule_name(pdependant), data);
      needed = true;
    }
  }
  advance_iterate_end;

  unit_type_iterate(ptype)
  {
    if (ptype->require_advance == padv) {
      cb(utype_rule_name(ptype), data);
      needed = true;
    }
  }
  unit_type_iterate_end;

  extra_type_iterate(pextra)
  {
    if (pextra->visibility_req == advance_number(padv)) {
      char buf[512];

      fc_snprintf(buf, sizeof(buf), "%s visibility",
                  extra_rule_name(pextra));
      cb(buf, data);
    }
  }
  extra_type_iterate_end;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

/**
   Check if anything in ruleset needs building
 */
bool is_building_needed(struct impr_type *pimpr, requirers_cb cb, void *data)
{
  struct universal uni = {.value = {.building = pimpr},
                          .kind = VUT_IMPROVEMENT};
  bool needed = false;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

/**
   Check if anything in ruleset needs unit type
 */
bool is_utype_needed(struct unit_type *ptype, requirers_cb cb, void *data)
{
  struct universal uni = {.value = {.utype = ptype}, .kind = VUT_UTYPE};
  bool needed = false;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

/**
   Check if anything in ruleset needs goods type
 */
bool is_good_needed(struct goods_type *pgood, requirers_cb cb, void *data)
{
  struct universal uni = {.value = {.good = pgood}, .kind = VUT_GOOD};
  bool needed = false;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

/**
   Check if anything in ruleset needs extra type
 */
bool is_extra_needed(struct extra_type *pextra, requirers_cb cb, void *data)
{
  struct universal uni = {.value = {.extra = pextra}, .kind = VUT_EXTRA};
  bool needed = false;
  bool conflicts = false;
  bool hides = false;
  int id = extra_index(pextra);

  extra_type_iterate(requirer)
  {
    conflicts |= BV_ISSET(requirer->conflicts, id);
    hides |= BV_ISSET(requirer->hidden_by, id);
  }
  extra_type_iterate_end;

  if (conflicts) {
    cb(R__("Conflicting extra"), data);
  }
  if (hides) {
    cb(R__("Hidden extra"), data);
  }
  needed |= conflicts | hides;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

/**
   Check if anything in ruleset needs terrain type
 */
bool is_terrain_needed(struct terrain *pterr, requirers_cb cb, void *data)
{
  struct universal uni = {.value = {.terrain = pterr}, .kind = VUT_TERRAIN};
  bool needed = false;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

/**
   Check if anything in ruleset needs government
 */
bool is_government_needed(struct government *pgov, requirers_cb cb,
                          void *data)
{
  struct universal uni = {.value = {.govern = pgov}, .kind = VUT_GOVERNMENT};
  bool needed = false;

  needed |= is_universal_needed(&uni, cb, data);

  return needed;
}

struct effect_list_multiplier_data {
  bool needed;
  struct multiplier *pmul;
  requirers_cb cb;
  void *requirers_data;
};

/**
   Callback to check if effect needs universal.
 */
static bool effect_list_multiplier_cb(struct effect *peffect, void *data)
{
  struct effect_list_multiplier_data *cbdata =
      (struct effect_list_multiplier_data *) data;

  if (peffect->multiplier == cbdata->pmul) {
    cbdata->cb(R__("Effect"), cbdata->requirers_data);
    cbdata->needed = true;
  }

  // Always continue to next until all effects checked
  return true;
}

/**
   Check if anything in ruleset needs multiplier
 */
bool is_multiplier_needed(struct multiplier *pmul, requirers_cb cb,
                          void *data)
{
  struct effect_list_multiplier_data cb_data;
  bool needed = false;

  cb_data.needed = false;
  cb_data.pmul = pmul;
  cb_data.cb = cb;
  cb_data.requirers_data = data;

  iterate_effect_cache(effect_list_multiplier_cb, &cb_data);
  needed |= cb_data.needed;

  return needed;
}
