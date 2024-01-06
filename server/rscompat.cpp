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

#include <string.h>

// utility
#include "capability.h"
#include "fcintl.h"
#include "registry.h"
#include "section_file.h"

// common
#include "actions.h"
#include "effects.h"
#include "movement.h"
#include "requirements.h"
#include "unittype.h"

// server
#include "rssanity.h"
#include "ruleset.h"

#include "rscompat.h"

struct new_flags {
  const char *name;
  const char *helptxt;
};

#define UTYF_LAST_USER_FLAG_3_0 UTYF_USER_FLAG_40
#define UCF_LAST_USER_FLAG_3_0 UCF_USER_FLAG_8
#define TER_LAST_USER_FLAG_3_0 TER_USER_8

static void rscompat_optional_capabilities(rscompat_info *info);

/**
   Ruleset files should have a capabilities string datafile.options
   This checks the string and that the required capabilities are satisfied.
 */
int rscompat_check_capabilities(struct section_file *file,
                                const char *filename,
                                struct rscompat_info *info)
{
  const char *datafile_options =
      secfile_lookup_str(file, "datafile.options");
  bool ok = false;
  int format;

  if (!datafile_options) {
    qCCritical(ruleset_category,
               "\"%s\": ruleset capability problem:", filename);
    qCCritical(ruleset_category, "%s", secfile_error());

    return 0;
  }

  if (info->compat_mode) {
    /* Check alternative capstr first, so that when we do the main capstr
     * check, we already know that failures there are fatal (error message
     * correct, can return immediately) */

    if (has_capabilities(RULESET_COMPAT_CAP, datafile_options)
        && has_capabilities(datafile_options, RULESET_COMPAT_CAP)) {
      ok = true;
    }
  }

  if (!ok) {
    if (!has_capabilities(RULESET_CAPABILITIES, datafile_options)) {
      qCCritical(ruleset_category,
                 "\"%s\": ruleset datafile appears incompatible:", filename);
      qCCritical(ruleset_category, "  datafile options: %s",
                 datafile_options);
      qCCritical(ruleset_category, "  supported options: %s",
                 RULESET_CAPABILITIES);
      qCCritical(ruleset_category, "Capability problem");

      return 0;
    }
    if (!has_capabilities(datafile_options, RULESET_CAPABILITIES)) {
      qCCritical(ruleset_category,
                 "\"%s\": ruleset datafile claims required option(s)"
                 " that we don't support:",
                 filename);
      qCCritical(ruleset_category, "  datafile options: %s",
                 datafile_options);
      qCCritical(ruleset_category, "  supported options: %s",
                 RULESET_CAPABILITIES);
      qCCritical(ruleset_category, "Capability problem");

      return 0;
    }
  }

  if (!secfile_lookup_int(file, &format, "datafile.format_version")) {
    qCritical("\"%s\": lacking legal format_version field", filename);
    qCCritical(ruleset_category, "%s", secfile_error());

    return 0;
  } else if (format == 0) {
    qCritical("\"%s\": Illegal format_version value", filename);
    qCCritical(ruleset_category, "Format version error");
  }

  return format;
}

/**
   Add all hard obligatory requirements to an action enabler or disable it.
   @param ae the action enabler to add requirements to.
   @return TRUE iff adding obligatory hard reqs for the enabler's action
                needs to restart - say if an enabler was added or removed.
 */
static bool
rscompat_enabler_add_obligatory_hard_reqs(struct action_enabler *ae)
{
  struct req_vec_problem *problem;

  struct action *paction = action_by_number(ae->action);
  /* Some changes requires starting to process an action's enablers from
   * the beginning. */
  bool needs_restart = false;

  while ((problem = action_enabler_suggest_repair(ae)) != nullptr) {
    // A hard obligatory requirement is missing.

    int i;

    if (problem->num_suggested_solutions == 0) {
      // Didn't get any suggestions about how to solve this.

      qCritical("Dropping an action enabler for %s."
                " Don't know how to fix: %s.",
                action_rule_name(paction), problem->description);
      ae->disabled = true;

      req_vec_problem_free(problem);
      return true;
    }

    // Sanity check.
    fc_assert_ret_val(problem->num_suggested_solutions > 0, needs_restart);

    // Only append is supported for upgrade
    for (i = 0; i < problem->num_suggested_solutions; i++) {
      if (problem->suggested_solutions[i].operation != RVCO_APPEND) {
        /* A problem that isn't caused by missing obligatory hard
         * requirements has been detected. Probably an old requirement that
         * contradicted a hard requirement that wasn't documented by making
         * it obligatory. In that case the enabler was never in use. The
         * action it self would have blocked it. */

        qCritical("While adding hard obligatory reqs to action enabler"
                  " for %s: %s Dropping it.",
                  action_rule_name(paction), problem->description);
        ae->disabled = true;
        req_vec_problem_free(problem);
        return true;
      }
    }

    for (i = 0; i < problem->num_suggested_solutions; i++) {
      struct action_enabler *new_enabler;

      /* There can be more than one suggestion to apply. In that case both
       * are applied to their own copy. The original should therefore be
       * kept for now. */
      new_enabler = action_enabler_copy(ae);

      // Apply the solution.
      if (!req_vec_change_apply(&problem->suggested_solutions[i],
                                action_enabler_vector_by_number,
                                new_enabler)) {
        qCritical(
            "Failed to apply solution %s for %s to action enabler"
            " for %s. Dropping it.",
            req_vec_change_translation(&problem->suggested_solutions[i],
                                       action_enabler_vector_by_number_name),
            problem->description, action_rule_name(paction));
        new_enabler->disabled = true;
        req_vec_problem_free(problem);
        return true;
      }

      if (problem->num_suggested_solutions - 1 == i) {
        // The last modification is to the original enabler.
        ae->action = new_enabler->action;
        ae->disabled = new_enabler->disabled;
        requirement_vector_copy(&ae->actor_reqs, &new_enabler->actor_reqs);
        requirement_vector_copy(&ae->target_reqs, &new_enabler->target_reqs);
        delete new_enabler;
        new_enabler = nullptr;
      } else {
        // Register the new enabler
        action_enabler_add(new_enabler);

        // This changes the number of action enablers.
        needs_restart = true;
      }
    }

    req_vec_problem_free(problem);

    if (needs_restart) {
      // May need to apply future upgrades to the copies too.
      return true;
    }
  }

  return needs_restart;
}

/**
   Update existing action enablers for new hard obligatory requirements.
   Disable those that can't be upgraded.
 */
void rscompat_enablers_add_obligatory_hard_reqs()
{
  action_iterate(act_id)
  {
    bool restart_enablers_for_action;
    do {
      restart_enablers_for_action = false;
      action_enabler_list_iterate(action_enablers_for_action(act_id), ae)
      {
        if (ae->disabled) {
          // Ignore disabled enablers
          continue;
        }
        if (rscompat_enabler_add_obligatory_hard_reqs(ae)) {
          /* Something important, probably the number of action enablers
           * for this action, changed. Start over again on this action's
           * enablers. */
          restart_enablers_for_action = true;
          break;
        }
      }
      action_enabler_list_iterate_end;
    } while (restart_enablers_for_action);
  }
  action_iterate_end;
}

/**
   Find and return the first unused unit type user flag. If all unit type
   user flags are taken MAX_NUM_USER_UNIT_FLAGS is returned.
 */
static int first_free_unit_type_user_flag()
{
  int flag;

  // Find the first unused user defined unit type flag.
  for (flag = 0; flag < MAX_NUM_USER_UNIT_FLAGS; flag++) {
    if (unit_type_flag_id_name_cb(unit_type_flag_id(flag + UTYF_USER_FLAG_1))
        == nullptr) {
      return flag;
    }
  }

  // All unit type user flags are taken.
  return MAX_NUM_USER_UNIT_FLAGS;
}

/**
   Find and return the first unused unit class user flag. If all unit class
   user flags are taken MAX_NUM_USER_UCLASS_FLAGS is returned.
 */
static int first_free_unit_class_user_flag()
{
  int flag;

  // Find the first unused user defined unit class flag.
  for (flag = 0; flag < MAX_NUM_USER_UCLASS_FLAGS; flag++) {
    if (unit_class_flag_id_name_cb(
            unit_class_flag_id(flag + UCF_USER_FLAG_1))
        == nullptr) {
      return flag;
    }
  }

  // All unit class user flags are taken.
  return MAX_NUM_USER_UCLASS_FLAGS;
}

/**
   Find and return the first unused terrain user flag. If all terrain
   user flags are taken MAX_NUM_USER_TER_FLAGS is returned.
 */
static int first_free_terrain_user_flag()
{
  int flag;

  // Find the first unused user defined terrain flag.
  for (flag = 0; flag < MAX_NUM_USER_TER_FLAGS; flag++) {
    if (terrain_flag_id_name_cb(terrain_flag_id(flag + TER_USER_1))
        == nullptr) {
      return flag;
    }
  }

  // All terrain user flags are taken.
  return MAX_NUM_USER_TER_FLAGS;
}

/**
   Do compatibility things with names before they are referred to. Runs
   after names are loaded from the ruleset but before the ruleset objects
   that may refer to them are loaded.

   This is needed when previously hard coded items that are referred to in
   the ruleset them self becomes ruleset defined.

   Returns FALSE if an error occurs.
 */
bool rscompat_names(struct rscompat_info *info)
{
  if (info->ver_units < 20) {
    /* Some unit type flags moved to the ruleset between 3.0 and 3.1.
     * Add them back as user flags.
     * XXX: ruleset might not need all of these, and may have enough
     * flags of its own that these additional ones prevent conversion. */
    const std::vector<new_flags> new_flags_31 = {
        new_flags{N_("BeachLander"),
                  N_("Won't lose all movement when moving from"
                     " non-native terrain to native terrain.")},
        new_flags{N_("Cant_Fortify"), nullptr},
    };
    fc_assert_ret_val(new_flags_31.size()
                          <= UTYF_LAST_USER_FLAG - UTYF_LAST_USER_FLAG_3_0,
                      false);

    /* Some unit class flags moved to the ruleset between 3.0 and 3.1.
     * Add them back as user flags.
     * XXX: ruleset might not need all of these, and may have enough
     * flags of its own that these additional ones prevent conversion. */
    const std::vector<new_flags> new_class_flags_31 = {
        new_flags{N_("Missile"), N_("Unit is destroyed when it attacks")},
        new_flags{N_("CanPillage"), N_("Can pillage tile improvements.")},
        new_flags{N_("CanFortify"), N_("Gets a 50% defensive bonus while"
                                       " in cities.")},
    };
    fc_assert_ret_val(new_class_flags_31.size()
                          <= UCF_LAST_USER_FLAG - UCF_LAST_USER_FLAG_3_0,
                      false);

    int first_free;
    int i;

    // Unit type flags.
    first_free = first_free_unit_type_user_flag() + UTYF_USER_FLAG_1;

    for (i = 0; i < new_flags_31.size(); i++) {
      if (UTYF_USER_FLAG_1 + MAX_NUM_USER_UNIT_FLAGS <= first_free + i) {
        // Can't add the user unit type flags.
        qCCritical(ruleset_category,
                   "Can't upgrade the ruleset. Not enough free unit type "
                   "user flags to add user flags for the unit type flags "
                   "that used to be hardcoded.");
        return false;
      }
      /* Shouldn't be possible for valid old ruleset to have flag names that
       * clash with these ones */
      if (unit_type_flag_id_by_name(new_flags_31[i].name, fc_strcasecmp)
          != unit_type_flag_id_invalid()) {
        qCCritical(ruleset_category,
                   "Ruleset had illegal user unit type flag '%s'",
                   new_flags_31[i].name);
        return false;
      }
      set_user_unit_type_flag_name(unit_type_flag_id(first_free + i),
                                   new_flags_31[i].name,
                                   new_flags_31[i].helptxt);
    }

    // Unit type class flags.
    first_free = first_free_unit_class_user_flag() + UCF_USER_FLAG_1;

    for (i = 0; i < new_class_flags_31.size(); i++) {
      if (UCF_USER_FLAG_1 + MAX_NUM_USER_UCLASS_FLAGS <= first_free + i) {
        // Can't add the user unit type class flags.
        qCCritical(ruleset_category,
                   "Can't upgrade the ruleset. Not enough free unit "
                   "type class user flags to add user flags for the "
                   "unit type class flags that used to be hardcoded.");
        return false;
      }
      /* Shouldn't be possible for valid old ruleset to have flag names that
       * clash with these ones */
      if (unit_class_flag_id_by_name(new_class_flags_31[i].name,
                                     fc_strcasecmp)
          != unit_class_flag_id_invalid()) {
        qCCritical(ruleset_category,
                   "Ruleset had illegal user unit class flag '%s'",
                   new_class_flags_31[i].name);
        return false;
      }
      set_user_unit_class_flag_name(unit_class_flag_id(first_free + i),
                                    new_class_flags_31[i].name,
                                    new_class_flags_31[i].helptxt);
    }
  }

  if (info->ver_terrain < 20) {
    /* Some terrain flags moved to the ruleset between 3.0 and 3.1.
     * Add them back as user flags.
     * XXX: ruleset might not need all of these, and may have enough
     * flags of its own that these additional ones prevent conversion. */
    const std::vector<new_flags> new_flags_31 = {
        new_flags{N_("NoFortify"),
                  N_("No units can fortify on this terrain.")},
    };
    fc_assert_ret_val(new_flags_31.size()
                          <= TER_USER_LAST - TER_LAST_USER_FLAG_3_0,
                      false);

    int first_free;
    int i;

    // Terrain flags.
    first_free = first_free_terrain_user_flag() + TER_USER_1;

    for (i = 0; i < new_flags_31.size(); i++) {
      if (TER_USER_1 + MAX_NUM_USER_TER_FLAGS <= first_free + i) {
        // Can't add the user terrain flags.
        qCCritical(ruleset_category,
                   "Can't upgrade the ruleset. Not enough free terrain "
                   "user flags to add user flags for the terrain flags "
                   "that used to be hardcoded.");
        return false;
      }
      /* Shouldn't be possible for valid old ruleset to have flag names that
       * clash with these ones */
      if (terrain_flag_id_by_name(new_flags_31[i].name, fc_strcasecmp)
          != terrain_flag_id_invalid()) {
        qCCritical(ruleset_category,
                   "Ruleset had illegal user terrain flag '%s'",
                   new_flags_31[i].name);
        return false;
      }
      set_user_terrain_flag_name(terrain_flag_id(first_free + i),
                                 new_flags_31[i].name,
                                 new_flags_31[i].helptxt);
    }
  }

  // No errors encountered.
  return true;
}

/**
   Handle a universal being separated from an original universal.

   A universal may be split into two new universals. An effect may mention
   the universal that now has been split in its requirement list. In that
   case two effect - one for the original and one for the universal being
   separated from it - are needed.

   Check if the original universal is mentioned in the requirement list of
   peffect. Handle creating one effect for the original and one for the
   universal that has been separated out if it is.
 */
static bool effect_handle_split_universal(struct effect *peffect,
                                          struct universal original,
                                          struct universal separated)
{
  if (universal_is_mentioned_by_requirements(&peffect->reqs, &original)) {
    // Copy the old effect.
    struct effect *peffect_copy = effect_copy(peffect);

    // Replace the original requirement with the separated requirement.
    return universal_replace_in_req_vec(&peffect_copy->reqs, &original,
                                        &separated);
  }

  return false;
}

/**
   Adjust effects
 */
static bool effect_list_compat_cb(struct effect *peffect, void *data)
{
  struct rscompat_info *info = static_cast<struct rscompat_info *>(data);

  if (info->ver_effects < 20) {
    // Attack has been split in regular "Attack" and "Suicide Attack".
    effect_handle_split_universal(
        peffect, universal_by_number(VUT_ACTION, ACTION_ATTACK),
        universal_by_number(VUT_ACTION, ACTION_SUICIDE_ATTACK));

    /* "Nuke City" and "Nuke Units" has been split from "Explode Nuclear".
     * "Explode Nuclear" is now only about exploding at the current tile. */
    effect_handle_split_universal(
        peffect, universal_by_number(VUT_ACTION, ACTION_NUKE),
        universal_by_number(VUT_ACTION, ACTION_NUKE_CITY));
    effect_handle_split_universal(
        peffect, universal_by_number(VUT_ACTION, ACTION_NUKE),
        universal_by_number(VUT_ACTION, ACTION_NUKE_UNITS));

    /* Production or building targeted actions have been split in one action
     * for each target. */
    effect_handle_split_universal(
        peffect,
        universal_by_number(VUT_ACTION, ACTION_SPY_TARGETED_SABOTAGE_CITY),
        universal_by_number(VUT_ACTION,
                            ACTION_SPY_SABOTAGE_CITY_PRODUCTION));
    effect_handle_split_universal(
        peffect,
        universal_by_number(VUT_ACTION,
                            ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC),
        universal_by_number(VUT_ACTION,
                            ACTION_SPY_SABOTAGE_CITY_PRODUCTION_ESC));

    if (peffect->type == EFT_ILLEGAL_ACTION_MOVE_COST) {
      /* Boarding a transporter became action enabler controlled in
       * Freeciv 3.1. Old hard coded rules had no punishment for trying to
       * do this when it is illegal according to the rules. */
      effect_req_append(peffect,
                        req_from_str("Action", "Local", false, false, false,
                                     "Transport Board"));
      effect_req_append(peffect,
                        req_from_str("Action", "Local", false, false, false,
                                     "Transport Embark"));
      /* Disembarking became action enabler controlled in Freeciv 3.1. Old
       * hard coded rules had no punishment for trying to do those when it
       * is illegal according to the rules. */
      effect_req_append(peffect,
                        req_from_str("Action", "Local", false, false, false,
                                     "Transport Disembark"));
      effect_req_append(peffect,
                        req_from_str("Action", "Local", false, false, false,
                                     "Transport Disembark 2"));
    }
  }

  // Go to the next effect.
  return true;
}

/**
   Turn old effect to an action enabler.
 */
static void effect_to_enabler(action_id action, struct section_file *file,
                              const char *sec_name,
                              struct rscompat_info *compat, const char *type)
{
  int value = secfile_lookup_int_default(file, 1, "%s.value", sec_name);
  char buf[1024];

  if (value > 0) {
    // It was an enabling effect. Add enabler
    struct action_enabler *enabler;
    struct requirement_vector *reqs;
    struct requirement settler_req;

    enabler = action_enabler_new();
    enabler->action = action;

    reqs = lookup_req_list(file, compat, sec_name, "reqs", "old effect");

    /* TODO: Divide requirements to actor_reqs and target_reqs depending
     *       their type. */
    requirement_vector_copy(&enabler->actor_reqs, reqs);

    settler_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, false, true,
                                  false, UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, settler_req);

    // Add the enabler to the ruleset.
    action_enabler_add(enabler);

    if (compat->log_cb != nullptr) {
      fc_snprintf(buf, sizeof(buf),
                  "Converted effect %s in %s to an action enabler. Make "
                  "sure requirements "
                  "are correctly divided to actor and target requirements.",
                  type, sec_name);
      compat->log_cb(buf);
    }
  } else if (value < 0) {
    if (compat->log_cb != nullptr) {
      fc_snprintf(buf, sizeof(buf),
                  "%s effect with negative value in %s can't be "
                  "automatically converted "
                  "to an action enabler. Do that manually.",
                  type, sec_name);
      compat->log_cb(buf);
    }
  }
}

/**
   Check if effect name refers to one of the removed effects, and handle it
   if it does. Returns TRUE iff name was a valid old name.
 */
bool rscompat_old_effect_3_1(const char *type, struct section_file *file,
                             const char *sec_name,
                             struct rscompat_info *compat)
{
  if (compat->ver_effects < 20) {
    if (!fc_strcasecmp(type, "Transform_Possible")) {
      effect_to_enabler(ACTION_TRANSFORM_TERRAIN, file, sec_name, compat,
                        type);
      return true;
    }
    if (!fc_strcasecmp(type, "Irrig_TF_Possible")) {
      effect_to_enabler(ACTION_CULTIVATE, file, sec_name, compat, type);
      return true;
    }
    if (!fc_strcasecmp(type, "Mining_TF_Possible")) {
      effect_to_enabler(ACTION_PLANT, file, sec_name, compat, type);
      return true;
    }
    if (!fc_strcasecmp(type, "Mining_Possible")) {
      effect_to_enabler(ACTION_MINE, file, sec_name, compat, type);
      return true;
    }
    if (!fc_strcasecmp(type, "Irrig_Possible")) {
      effect_to_enabler(ACTION_IRRIGATE, file, sec_name, compat, type);
      return true;
    }
  }

  return false;
}

/**
   Do compatibility things after regular ruleset loading.
 */
void rscompat_postprocess(struct rscompat_info *info)
{
  if (info->compat_mode) {
    /* Upgrade existing effects. Done before new effects are added to
     * prevent the new effects from being upgraded by accident. */
    iterate_effect_cache(effect_list_compat_cb, info);
  }

  if (info->compat_mode && info->ver_effects < 20) {
    struct effect *peffect;

    /* Post successful action move fragment loss for "Bombard"
     * has moved to the ruleset. */
    peffect =
        effect_new(EFT_ACTION_SUCCESS_MOVE_COST, MAX_MOVE_FRAGS, nullptr);

    // The reduction only applies to "Bombard".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Bombard"));

    /* Post successful action move fragment loss for "Heal Unit"
     * has moved to the ruleset. */
    peffect =
        effect_new(EFT_ACTION_SUCCESS_MOVE_COST, MAX_MOVE_FRAGS, nullptr);

    // The reduction only applies to "Heal Unit".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Heal Unit"));

    /* Post successful action move fragment loss for "Expel Unit"
     * has moved to the ruleset. */
    peffect = effect_new(EFT_ACTION_SUCCESS_MOVE_COST, SINGLE_MOVE, nullptr);

    // The reduction only applies to "Expel Unit".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Expel Unit"));

    /* Post successful action move fragment loss for "Capture Units"
     * has moved to the ruleset. */
    peffect = effect_new(EFT_ACTION_SUCCESS_MOVE_COST, SINGLE_MOVE, nullptr);

    // The reduction only applies to "Capture Units".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Capture Units"));

    /* Post successful action move fragment loss for "Establish Embassy"
     * has moved to the ruleset. */
    peffect = effect_new(EFT_ACTION_SUCCESS_MOVE_COST, 1, nullptr);

    // The reduction only applies to "Establish Embassy".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Establish Embassy"));

    /* Post successful action move fragment loss for "Investigate City"
     * has moved to the ruleset. */
    peffect = effect_new(EFT_ACTION_SUCCESS_MOVE_COST, 1, nullptr);

    // The reduction only applies to "Investigate City".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Investigate City"));

    /* Post successful action move fragment loss for targets of "Expel Unit"
     * has moved to the ruleset. */
    peffect = effect_new(EFT_ACTION_SUCCESS_TARGET_MOVE_COST, MAX_MOVE_FRAGS,
                         nullptr);

    // The reduction only applies to "Expel Unit".
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            true, "Expel Unit"));

    /* Post successful action move fragment loss for targets of
     * "Paradrop Unit" has moved to the Action_Success_Actor_Move_Cost
     * effect. */
    unit_type_iterate(putype)
    {
      if (!utype_can_do_action(putype, ACTION_PARADROP)) {
        // Not relevant
        continue;
      }

      if (putype->rscompat_cache.paratroopers_mr_sub == 0) {
        // Not relevant
        continue;
      }

      // Subtract the value via the Action_Success_Actor_Move_Cost effect
      peffect =
          effect_new(EFT_ACTION_SUCCESS_MOVE_COST,
                     putype->rscompat_cache.paratroopers_mr_sub, nullptr);

      // The reduction only applies to "Paradrop Unit".
      effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                              false, "Paradrop Unit"));

      // The reduction only applies to this unit type.
      effect_req_append(peffect,
                        req_from_str("UnitType", "Local", false, true, false,
                                     utype_rule_name(putype)));
    }
    unit_type_iterate_end;

    // Fortifying rules have been unhardcoded to effects.
    peffect = effect_new(EFT_FORTIFY_DEFENSE_BONUS, 50, nullptr);

    /* Unit actually fortified. This does not need checks for unit class or
     * type flags for unit's ability to fortify as it would not be fortified
     * if it can't. */
    effect_req_append(peffect, req_from_str("Activity", "Local", false, true,
                                            false, "Fortified"));

    // Fortify bonus in cities
    peffect = effect_new(EFT_FORTIFY_DEFENSE_BONUS, 50, nullptr);

    // City center
    effect_req_append(peffect, req_from_str("CityTile", "Local", false, true,
                                            false, "Center"));
    // Not cumulative with regular fortified bonus
    effect_req_append(peffect, req_from_str("Activity", "Local", false,
                                            false, false, "Fortified"));
    // Unit flags
    effect_req_append(peffect, req_from_str("UnitClassFlag", "Local", false,
                                            true, false, "CanFortify"));
    effect_req_append(peffect, req_from_str("UnitFlag", "Local", false,
                                            false, false, "Cant_Fortify"));

    /* The probability that "Steal Maps" and "Steal Maps Escape" steals the
     * map of a tile has moved to the ruleset. */
    peffect = effect_new(EFT_MAPS_STOLEN_PCT, -50, nullptr);

    /* The rule that "Recycle Unit"'s unit shield value is 50% has moved to
     * the ruleset. */
    peffect = effect_new(EFT_UNIT_SHIELD_VALUE_PCT, -50, nullptr);
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            false, "Recycle Unit"));

    /* The rule that "Upgrade Unit"'s current unit shield value is 50% when
     * calculating unit upgrade price has moved to the ruleset. */
    peffect = effect_new(EFT_UNIT_SHIELD_VALUE_PCT, -50, nullptr);
    effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                            false, "Upgrade Unit"));
  }

  if (info->compat_mode && info->ver_game < 20) {
    // New enablers
    struct action_enabler *enabler;
    struct requirement e_req;

    enabler = action_enabler_new();
    enabler->action = ACTION_PILLAGE;
    e_req = req_from_str("UnitClassFlag", "Local", false, true, false,
                         "CanPillage");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_CLEAN_FALLOUT;
    e_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, false, true, false,
                            UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_CLEAN_POLLUTION;
    e_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, false, true, false,
                            UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_FORTIFY;
    e_req = req_from_str("UnitClassFlag", "Local", false, true, true,
                         "CanFortify");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    e_req = req_from_str("UnitFlag", "Local", false, false, true,
                         "Cant_Fortify");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    e_req = req_from_str("TerrainFlag", "Local", false, false, true,
                         "NoFortify");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_FORTIFY;
    e_req = req_from_str("UnitClassFlag", "Local", false, true, true,
                         "CanFortify");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    e_req = req_from_str("UnitFlag", "Local", false, false, true,
                         "Cant_Fortify");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    e_req = req_from_str("CityTile", "Local", false, true, true, "Center");
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_ROAD;
    e_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, false, true, false,
                            UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_CONVERT;
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_BASE;
    e_req = req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL, false, true, false,
                            UTYF_SETTLERS);
    requirement_vector_append(&enabler->actor_reqs, e_req);
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_TRANSPORT_ALIGHT;
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_TRANSPORT_BOARD;
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_TRANSPORT_EMBARK;
    action_enabler_add(enabler);

    enabler = action_enabler_new();
    enabler->action = ACTION_TRANSPORT_UNLOAD;
    action_enabler_add(enabler);

    // Update action enablers.
    rscompat_enablers_add_obligatory_hard_reqs();
    action_enablers_iterate(ae)
    {
      /* "Attack" is split in a unit consuming and a non unit consuming
       * version. */
      if (ae->action == ACTION_ATTACK) {
        // The old rule is represented with two action enablers.
        enabler = action_enabler_copy(ae);

        // One allows regular attacks.
        requirement_vector_append(
            &ae->actor_reqs, req_from_str("UnitClassFlag", "Local", false,
                                          false, true, "Missile"));

        // The other allows suicide attacks.
        enabler->action = ACTION_SUICIDE_ATTACK;
        requirement_vector_append(&enabler->actor_reqs,
                                  req_from_str("UnitClassFlag", "Local",
                                               false, true, true,
                                               "Missile"));

        // Add after the action was changed.
        action_enabler_add(enabler);
      }

      /* "Explode Nuclear"'s adjacent tile attack is split to "Nuke City"
       * and "Nuke Units". */
      if (ae->action == ACTION_NUKE) {
        /* The old rule is represented with three action enablers:
         * 1) "Explode Nuclear" against the actors own tile.
         * 2) "Nuke City" against adjacent enemy cities.
         * 3) "Nuke Units" against adjacent enemy unit stacks. */

        struct action_enabler *city;
        struct action_enabler *units;

        // Against city targets.
        city = action_enabler_copy(ae);
        city->action = ACTION_NUKE_CITY;

        // Against unit stack targets.
        units = action_enabler_copy(ae);
        units->action = ACTION_NUKE_UNITS;

        // "Explode Nuclear" required this to target an adjacent tile.
        /* While this isn't a real move (because of enemy city/units) at
         * target tile it pretends to be one. */
        requirement_vector_append(
            &city->actor_reqs, req_from_values(VUT_MINMOVES, REQ_RANGE_LOCAL,
                                               false, true, false, 1));
        requirement_vector_append(&units->actor_reqs,
                                  req_from_values(VUT_MINMOVES,
                                                  REQ_RANGE_LOCAL, false,
                                                  true, false, 1));

        /* Be slightly stricter about the relationship to target unit stacks
         * than "Explode Nuclear" was before it would target an adjacent
         * tile. I think the intention was that you shouldn't nuke your
         * friends and allies. */
        requirement_vector_append(
            &city->actor_reqs, req_from_values(VUT_DIPLREL, REQ_RANGE_LOCAL,
                                               false, true, false, DS_WAR));
        requirement_vector_append(
            &units->actor_reqs, req_from_values(VUT_DIPLREL, REQ_RANGE_LOCAL,
                                                false, true, false, DS_WAR));

        // Only display one nuke action at once.
        requirement_vector_append(
            &units->target_reqs,
            req_from_values(VUT_CITYTILE, REQ_RANGE_LOCAL, false, false,
                            false, CITYT_CENTER));

        // Add after the action was changed.
        action_enabler_add(city);
        action_enabler_add(units);
      }

      /* "Targeted Sabotage City" is split in a production targeted and a
       * building targeted version. */
      if (ae->action == ACTION_SPY_TARGETED_SABOTAGE_CITY) {
        // The old rule is represented with two action enablers.
        enabler = action_enabler_copy(ae);

        enabler->action = ACTION_SPY_SABOTAGE_CITY_PRODUCTION;

        // Add after the action was changed.
        action_enabler_add(enabler);
      }

      /* "Targeted Sabotage City Escape" is split in a production targeted
       * and a building targeted version. */
      if (ae->action == ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC) {
        // The old rule is represented with two action enablers.
        enabler = action_enabler_copy(ae);

        enabler->action = ACTION_SPY_SABOTAGE_CITY_PRODUCTION_ESC;

        // Add after the action was changed.
        action_enabler_add(enabler);
      }
    }
    action_enablers_iterate_end;

    /* The paratroopers_mr_req field has moved to the enabler for the
     * "Paradrop Unit" action. */
    {
      bool generic_in_use = false;
      struct action_enabler_list *ae_custom = action_enabler_list_new();

      action_enabler_list_iterate(
          action_enablers_for_action(ACTION_PARADROP), ae)
      {
        unit_type_iterate(putype)
        {
          if (!requirement_fulfilled_by_unit_type(putype,
                                                  &(ae->actor_reqs))) {
            // This action enabler isn't for this unit type at all.
            continue;
          }

          requirement_vector_iterate(&ae->actor_reqs, preq)
          {
            if (preq->source.kind == VUT_MINMOVES) {
              if (!preq->present) {
                /* A max move fragments req has been found. Is it too
                 * large? */
                if (preq->source.value.minmoves
                    < putype->rscompat_cache.paratroopers_mr_req) {
                  // Avoid self contradiciton
                  continue;
                }
              }
            }
          }
          requirement_vector_iterate_end;

          if (putype->rscompat_cache.paratroopers_mr_req > 0) {
            // This unit type needs a custom enabler

            enabler = action_enabler_copy(ae);

            // This enabler is specific to the unit type
            e_req = req_from_values(VUT_UTYPE, REQ_RANGE_LOCAL, false, true,
                                    false, utype_number(putype));
            requirement_vector_append(&enabler->actor_reqs, e_req);

            // Add the minimum amout of move fragments
            e_req = req_from_values(
                VUT_MINMOVES, REQ_RANGE_LOCAL, false, true, false,
                putype->rscompat_cache.paratroopers_mr_req);
            requirement_vector_append(&enabler->actor_reqs, e_req);

            action_enabler_list_append(ae_custom, enabler);

            log_debug("paratroopers_mr_req upgrade: %s uses custom enabler",
                      utype_rule_name(putype));
          } else {
            // The old one works just fine

            generic_in_use = true;

            log_debug("paratroopers_mr_req upgrade: %s uses generic enabler",
                      utype_rule_name(putype));
          }
        }
        unit_type_iterate_end;

        if (!generic_in_use) {
          // The generic enabler isn't in use any more
          action_enabler_remove(ae);
        }
      }
      action_enabler_list_iterate_end;

      action_enabler_list_iterate(ae_custom, ae)
      {
        // Append the custom enablers.
        action_enabler_add(ae);
      }
      action_enabler_list_iterate_end;

      action_enabler_list_destroy(ae_custom);
    }

    // Enable all clause types
    {
      int i;

      for (i = 0; i < CLAUSE_COUNT; i++) {
        struct clause_info *cinfo = clause_info_get(clause_type(i));

        cinfo->enabled = true;
      }
    }
  }

  rscompat_optional_capabilities(info);

  /* The ruleset may need adjustments it didn't need before compatibility
   * post processing.
   *
   * If this isn't done a user of ruleset compatibility that ends up using
   * the rules risks bad rules. A user that saves the ruleset rather than
   * using it risks an unexpected change on the next load and save. */
  autoadjust_ruleset_data();
}

/**
 * Adds <VisionLayer, Main, Local, True> req to all unit/city vision reqs,
 * as compat for missing CAP_VUT_VISIONLAYER
 *
 * @data is a struct rscompat_info *.
 */
static bool rscompat_vision_effect_cb(struct effect *peffect, void *data)
{
  if (peffect->type == EFT_UNIT_VISION_RADIUS_SQ
      || peffect->type == EFT_CITY_VISION_RADIUS_SQ) {
    effect_req_append(peffect, req_from_str("VisionLayer", "Local", false,
                                            true, false, "Main"));
  }

  return true;
}

/**
 * Adds effects to reproduce the traditional visibility of intelligence
 * depending on the diplomatic status (EFT_NATION_INTELLIGENCE).
 */
static void rscompat_migrate_eft_nation_intelligence()
{
  for (int i = 0; i < NI_COUNT; ++i) {
    switch (static_cast<national_intelligence>(i)) {
      // Used to require being on the same team, all info gets shared with
      // the team now.
    case NI_HISTORY:
    case NI_MOOD:
    case NI_COUNT: // Never happens
      break;
      // Used to be always visible
    case NI_WONDERS: {
      auto effect = effect_new(EFT_NATION_INTELLIGENCE, 1, nullptr);
      effect_req_append(effect, req_from_values(VUT_NINTEL, REQ_RANGE_PLAYER,
                                                false, true, false, i));
    } break;
      // Used to be visible with simple contact
    case NI_DIPLOMACY:
    case NI_GOLD:
    case NI_GOVERNMENT:
    case NI_SCORE: {
      auto effect = effect_new(EFT_NATION_INTELLIGENCE, 1, nullptr);
      effect_req_append(effect, req_from_values(VUT_NINTEL, REQ_RANGE_PLAYER,
                                                false, true, false, i));
      effect_req_append(effect,
                        req_from_values(VUT_DIPLREL, REQ_RANGE_LOCAL, false,
                                        true, false, DRO_HAS_CONTACT));
    }
      [[fallthrough]]; // Embassy gives contact
      // Used to require an embassy
    case NI_CULTURE:
    case NI_MULTIPLIERS:
    case NI_TAX_RATES:
    case NI_TECHS: {
      auto effect = effect_new(EFT_NATION_INTELLIGENCE, 1, nullptr);
      effect_req_append(effect, req_from_values(VUT_NINTEL, REQ_RANGE_PLAYER,
                                                false, true, false, i));
      effect_req_append(effect,
                        req_from_values(VUT_DIPLREL, REQ_RANGE_LOCAL, false,
                                        true, false, DRO_HAS_EMBASSY));
    } break;
    }
  }
}

/**
 * Handles compatibility with older versions when the new behavior is
 * tied to the presence of an optional ruleset capability.
 */
static void rscompat_optional_capabilities(rscompat_info *info)
{
  if (!has_capability(CAP_EFT_HP_REGEN_MIN, info->cap_effects.data())) {
    // Create new effects to replace the old hard-coded minimum HP
    // recovery.

    // 1- 10% base recovery for units without hp_loss_pct
    auto effect = effect_new(EFT_HP_REGEN, 10, nullptr);
    unit_class_iterate(uclass)
    {
      if (uclass->hp_loss_pct > 0) {
        effect_req_append(effect,
                          req_from_str("UnitClass", "Local", false, false,
                                       false, uclass_rule_name(uclass)));
      }
    }
    unit_class_iterate_end;

    // 2- 10% base recovery for fortified units
    effect = effect_new(EFT_HP_REGEN, 10, nullptr);
    effect_req_append(effect, req_from_str("Activity", "Local", false, true,
                                           false, "Fortified"));

    // 3- At least one third in cities
    effect = effect_new(EFT_HP_REGEN_MIN, 33, nullptr);
    effect_req_append(effect, req_from_str("CityTile", "Local", false, true,
                                           false, "Center"));

    // 4- 10% more for units in cities without hp_loss_pct
    effect = effect_new(EFT_HP_REGEN_MIN, 10, nullptr);
    effect_req_append(effect, req_from_str("CityTile", "Local", false, true,
                                           false, "Center"));
    unit_class_iterate(uclass)
    {
      if (uclass->hp_loss_pct > 0) {
        effect_req_append(effect,
                          req_from_str("UnitClass", "Local", false, false,
                                       false, uclass_rule_name(uclass)));
      }
    }
    unit_class_iterate_end;

    // 5- 10% more for units fortified in cities
    effect = effect_new(EFT_HP_REGEN_MIN, 10, nullptr);
    effect_req_append(effect, req_from_str("CityTile", "Local", false, true,
                                           false, "Center"));
    effect_req_append(effect, req_from_str("Activity", "Local", false, true,
                                           false, "Fortified"));
  }

  if (!has_capability(CAP_EFT_BOMBARD_LIMIT_PCT, info->cap_effects.data())) {
    // Create new effect to replace the old hard-coded bombard limit

    // 1% base bombard limit
    effect_new(EFT_BOMBARD_LIMIT_PCT, 1, nullptr);
    unit_type_iterate(putype)
    {
      if (putype->hp > 100) {
        qCWarning(ruleset_category,
                  "Ruleset has units (such as '%s"
                  "') for which bombard limit changed as hp > 100",
                  utype_name_translation(putype));
        break;
      }
    }
    unit_type_iterate_end;
  }

  if (!has_capability(CAP_EFT_WONDER_VISIBLE, info->cap_effects.data())) {
    // Make Great Wonders visible to everyone
    auto effect = effect_new(EFT_WONDER_VISIBLE, 1, nullptr);
    effect_req_append(effect, req_from_str("BuildingGenus", "Local", false,
                                           true, false, "GreatWonder"));
  }

  if (!has_capability(CAP_VUT_VISIONLAYER, info->cap_effects.data())) {
    // Add vlayer=Main to existing vision effects
    iterate_effect_cache(rscompat_vision_effect_cb, info);
    // Add effect to give cities radius 2 vision on other layers
    auto effect = effect_new(EFT_CITY_VISION_RADIUS_SQ, 2, nullptr);
    effect_req_append(effect, req_from_str("VisionLayer", "Local", false,
                                           false, false, "Main"));
  }

  if (!has_capability(CAP_EFT_NATION_INTELLIGENCE,
                      info->cap_effects.data())) {
    rscompat_migrate_eft_nation_intelligence();
  }
}

/**
   Replace deprecated auto_attack configuration.
 */
bool rscompat_auto_attack_3_1(struct rscompat_info *compat,
                              struct action_auto_perf *auto_perf,
                              size_t psize,
                              enum unit_type_flag_id *protecor_flag)
{
  int i;

  if (compat->ver_game < 20) {
    // Auto attack happens during war.
    requirement_vector_append(&auto_perf->reqs,
                              req_from_values(VUT_DIPLREL, REQ_RANGE_LOCAL,
                                              false, true, true, DS_WAR));

    // Needs a movement point to auto attack.
    requirement_vector_append(&auto_perf->reqs,
                              req_from_values(VUT_MINMOVES, REQ_RANGE_LOCAL,
                                              false, true, true, 1));

    for (i = 0; i < psize; i++) {
      // Add each protecor_flag as a !present requirement.
      requirement_vector_append(&auto_perf->reqs,
                                req_from_values(VUT_UTFLAG, REQ_RANGE_LOCAL,
                                                false, false, true,
                                                protecor_flag[i]));
    }

    auto_perf->alternatives[0] = ACTION_CAPTURE_UNITS;
    auto_perf->alternatives[1] = ACTION_BOMBARD;
    auto_perf->alternatives[2] = ACTION_ATTACK;
    auto_perf->alternatives[3] = ACTION_SUICIDE_ATTACK;
  }

  return true;
}

/**
   Replace slow_invasions and friends.
 */
bool rscompat_old_slow_invasions_3_1(struct rscompat_info *compat,
                                     bool slow_invasions)
{
  if (compat->ver_effects < 20 && compat->ver_game < 20) {
    /* BeachLander and slow_invasions has moved to the ruleset. Use a "fake
     * generalized" Transport Disembark and Conquer City to handle it. */

    struct action_enabler *enabler;
    struct requirement e_req;

    enabler = action_enabler_new();
    enabler->action = ACTION_TRANSPORT_DISEMBARK1;

    if (slow_invasions) {
      /* Use for disembarking from native terrain so disembarking from
       * non native terain is handled by "Transport Disembark 2". */
      e_req = req_from_values(VUT_UNITSTATE, REQ_RANGE_LOCAL, false, true,
                              true, USP_NATIVE_TILE);
      requirement_vector_append(&enabler->actor_reqs, e_req);
    }

    action_enabler_add(enabler);

    if (slow_invasions) {
      // Make disembarking from non native terrain a different action.

      struct effect *peffect;
      struct action *paction;

      struct action_enabler_list *to_upgrade;

      // Add the actions

      // Use "Transport Disembark 2" for disembarking from non native.
      paction = action_by_number(ACTION_TRANSPORT_DISEMBARK2);
      /* "Transport Disembark" and "Transport Disembark 2" won't appear in
       * the same action selection dialog given their opposite
       * requirements. */
      paction->quiet = true;
      // Make what is happening clear.
      // TRANS: _Disembark from non native (100% chance of success).
      sz_strlcpy(paction->ui_name, N_("%sDisembark from non native%s"));

      // Use "Conquer City 2" for conquring from non native.
      paction = action_by_number(ACTION_CONQUER_CITY2);
      /* "Conquer City" and "Conquer City 2" won't appear in
       * the same action selection dialog given their opposite
       * requirements. */
      paction->quiet = true;
      // Make what is happening clear.
      // TRANS: _Conquer City from non native (100% chance of success).
      sz_strlcpy(paction->ui_name, N_("%sConquer City from non native%s"));

      // Enablers for disembark

      // City center counts as native.
      enabler = action_enabler_new();
      enabler->action = ACTION_TRANSPORT_DISEMBARK1;
      e_req = req_from_values(VUT_CITYTILE, REQ_RANGE_LOCAL, false, true,
                              true, CITYT_CENTER);
      requirement_vector_append(&enabler->actor_reqs, e_req);
      action_enabler_add(enabler);

      // No TerrainSpeed sees everything as native.
      enabler = action_enabler_new();
      enabler->action = ACTION_TRANSPORT_DISEMBARK1;
      e_req = req_from_values(VUT_UCFLAG, REQ_RANGE_LOCAL, false, false,
                              true, UCF_TERRAIN_SPEED);
      requirement_vector_append(&enabler->actor_reqs, e_req);
      action_enabler_add(enabler);

      // "BeachLander" sees everything as native.
      enabler = action_enabler_new();
      enabler->action = ACTION_TRANSPORT_DISEMBARK1;
      e_req = req_from_str("UnitFlag", "Local", false, true, true,
                           "BeachLander");
      requirement_vector_append(&enabler->actor_reqs, e_req);
      action_enabler_add(enabler);

      // "Transport Disembark 2" enabler
      enabler = action_enabler_new();
      enabler->action = ACTION_TRANSPORT_DISEMBARK2;

      // Native terrain is native.
      e_req = req_from_values(VUT_UNITSTATE, REQ_RANGE_LOCAL, false, false,
                              true, USP_NATIVE_TILE);
      requirement_vector_append(&enabler->actor_reqs, e_req);

      // City is native.
      e_req = req_from_values(VUT_CITYTILE, REQ_RANGE_LOCAL, false, false,
                              true, CITYT_CENTER);
      requirement_vector_append(&enabler->actor_reqs, e_req);

      // "BeachLander" sees everything as native.
      e_req = req_from_str("UnitFlag", "Local", false, false, true,
                           "BeachLander");
      requirement_vector_append(&enabler->actor_reqs, e_req);

      // No TerrainSpeed sees everything as native.
      e_req = req_from_values(VUT_UCFLAG, REQ_RANGE_LOCAL, false, true, true,
                              UCF_TERRAIN_SPEED);
      requirement_vector_append(&enabler->actor_reqs, e_req);

      action_enabler_add(enabler);

      /* Take movement for disembarking and conquering native terrain from
       * non native terrain */

      // Take movement for disembarking from non native terrain
      peffect =
          effect_new(EFT_ACTION_SUCCESS_MOVE_COST, MAX_MOVE_FRAGS, nullptr);

      // The reduction only applies to "Transport Disembark 2".
      effect_req_append(peffect,
                        req_from_str("Action", "Local", false, true, true,
                                     "Transport Disembark 2"));

      // No reduction here unless disembarking to native terrain.
      effect_req_append(peffect,
                        req_from_values(VUT_UNITSTATE, REQ_RANGE_LOCAL,
                                        false, true, true, USP_NATIVE_TILE));

      // Take movement for conquering from non native terrain
      peffect =
          effect_new(EFT_ACTION_SUCCESS_MOVE_COST, MAX_MOVE_FRAGS, nullptr);

      // The reduction only applies to "Conquer City 2".
      effect_req_append(peffect, req_from_str("Action", "Local", false, true,
                                              true, "Conquer City 2"));

      // No reduction here unless disembarking to native terrain.
      effect_req_append(peffect,
                        req_from_values(VUT_UNITSTATE, REQ_RANGE_LOCAL,
                                        false, true, true, USP_NATIVE_TILE));

      // Upgrade exisiting Conquer City action enablers
      to_upgrade = action_enabler_list_copy(
          action_enablers_for_action(ACTION_CONQUER_CITY));

      action_enabler_list_iterate(to_upgrade, conquer_city_enabler)
      {
        // City center counts as native.
        enabler = action_enabler_copy(conquer_city_enabler);
        e_req = req_from_values(VUT_CITYTILE, REQ_RANGE_LOCAL, false, true,
                                true, CITYT_CENTER);
        requirement_vector_append(&enabler->actor_reqs, e_req);
        action_enabler_add(enabler);
      }
      action_enabler_list_iterate_end;

      action_enabler_list_iterate(to_upgrade, conquer_city_enabler)
      {
        // No TerrainSpeed sees everything as native.
        enabler = action_enabler_copy(conquer_city_enabler);
        e_req = req_from_values(VUT_UCFLAG, REQ_RANGE_LOCAL, false, false,
                                true, UCF_TERRAIN_SPEED);
        requirement_vector_append(&enabler->actor_reqs, e_req);
        action_enabler_add(enabler);
      }
      action_enabler_list_iterate_end;

      action_enabler_list_iterate(to_upgrade, conquer_city_enabler)
      {
        // "BeachLander" sees everything as native.
        enabler = action_enabler_copy(conquer_city_enabler);
        e_req = req_from_str("UnitFlag", "Local", false, true, true,
                             "BeachLander");
        requirement_vector_append(&enabler->actor_reqs, e_req);
        action_enabler_add(enabler);
      }
      action_enabler_list_iterate_end;

      action_enabler_list_iterate(to_upgrade, conquer_city_enabler)
      {
        // Use "Conquer City 2" for conquring from non native.
        enabler = action_enabler_copy(conquer_city_enabler);
        enabler->action = ACTION_CONQUER_CITY2;

        // Native terrain is native.
        e_req = req_from_values(VUT_UNITSTATE, REQ_RANGE_LOCAL, false, false,
                                true, USP_NATIVE_TILE);
        requirement_vector_append(&enabler->actor_reqs, e_req);

        // City is native.
        e_req = req_from_values(VUT_CITYTILE, REQ_RANGE_LOCAL, false, false,
                                true, CITYT_CENTER);
        requirement_vector_append(&enabler->actor_reqs, e_req);

        // No TerrainSpeed sees everything as native.
        e_req = req_from_values(VUT_UCFLAG, REQ_RANGE_LOCAL, false, true,
                                true, UCF_TERRAIN_SPEED);
        requirement_vector_append(&enabler->actor_reqs, e_req);

        // "BeachLander" sees everything as native.
        e_req = req_from_str("UnitFlag", "Local", false, false, true,
                             "BeachLander");
        requirement_vector_append(&enabler->actor_reqs, e_req);

        action_enabler_add(enabler);
      }
      action_enabler_list_iterate_end;

      action_enabler_list_iterate(to_upgrade, conquer_city_enabler)
      {
        /* Use for conquering from native terrain so conquest from
         * non native terain is handled by "Conquer City 2". */
        e_req = req_from_values(VUT_UNITSTATE, REQ_RANGE_LOCAL, false, true,
                                true, USP_NATIVE_TILE);
        requirement_vector_append(&conquer_city_enabler->actor_reqs, e_req);
      }
      action_enabler_list_iterate_end;

      action_enabler_list_destroy(to_upgrade);
    }
  }

  return true;
}

/**
   Replace deprecated requirement type names with currently valid ones.

   The extra arguments are for situation where some, but not all, instances
   of a requirement type should become something else.
 */
const char *rscompat_req_name_3_1(const char *type, const char *old_name)
{
  if (!fc_strcasecmp("DiplRel", type)
      && !fc_strcasecmp("Is foreign", old_name)) {
    return "Foreign";
  }

  return old_name;
}

/**
   Replace deprecated unit type flag names with currently valid ones.
 */
const char *rscompat_utype_flag_name_3_1(struct rscompat_info *compat,
                                         const char *old_type)
{
  return old_type;
}

/**
   Adjust freeciv-3.0 ruleset extra definitions to freeciv-3.1
 */
void rscompat_extra_adjust_3_1(struct rscompat_info *compat,
                               struct extra_type *pextra)
{
  if (compat->compat_mode && compat->ver_terrain < 20) {
    // Give remove cause ERM_ENTER for huts
    if (is_extra_caused_by(pextra, EC_HUT)) {
      pextra->rmcauses |= (1 << ERM_ENTER);
      extra_to_removed_by_list(pextra, ERM_ENTER);
    }
  }
}
