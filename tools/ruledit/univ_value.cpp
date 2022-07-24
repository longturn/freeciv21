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

// common
#include "achievements.h"
#include "game.h"
#include "government.h"
#include "nation.h"
#include "server_settings.h"
#include "specialist.h"
#include "style.h"
#include "tech.h"
#include "traderoutes.h"
// server
#include "rssanity.h"
#include "settings.h"

#include "univ_value.h"

/**
   Initialize universal value with a value suitable for the kind.

   Returns TRUE iff there's any values universal kind can have with
   current ruleset.
 */
bool universal_value_initial(struct universal *src)
{
  switch (src->kind) {
  case VUT_NONE:
    // Value of None should never be used
    return true;
  case VUT_ADVANCE:
    if (game.control.num_tech_types <= 0) {
      return false;
    }
    src->value.advance = advance_by_number(A_NONE);
    return true;
  case VUT_GOVERNMENT:
    src->value.govern = game.government_during_revolution;
    return true;
  case VUT_IMPROVEMENT:
    if (game.control.num_impr_types <= 0) {
      return false;
    }
    src->value.building = improvement_by_number(0);
    return true;
  case VUT_TERRAIN:
    src->value.terrain = terrain_by_number(0);
    return true;
  case VUT_NATION:
    if (game.control.nation_count <= 0) {
      return false;
    }
    src->value.nation = nation_by_number(0);
    return true;
  case VUT_UTYPE:
    if (game.control.num_unit_types <= 0) {
      return false;
    }
    src->value.utype = utype_by_number(0);
    return true;
  case VUT_UTFLAG:
    src->value.unitflag = (enum unit_type_flag_id) 0;
    return true;
  case VUT_UCLASS:
    if (game.control.num_unit_classes <= 0) {
      return false;
    }
    src->value.uclass = uclass_by_number(0);
    return true;
  case VUT_UCFLAG:
    src->value.unitclassflag = (enum unit_class_flag_id) 0;
    return true;
  case VUT_OTYPE:
    src->value.outputtype = (enum output_type_id) 0;
    return true;
  case VUT_SPECIALIST:
    if (game.control.num_specialist_types <= 0) {
      return false;
    }
    src->value.specialist = specialist_by_number(0);
    return true;
  case VUT_MINSIZE:
    src->value.minsize = 0;
    return true;
  case VUT_AI_LEVEL:
    src->value.ai_level = AI_LEVEL_CHEATING;
    return true;
  case VUT_TERRAINCLASS:
    src->value.terrainclass = TC_LAND;
    return true;
  case VUT_MINYEAR:
    src->value.minyear = 0;
    return true;
  case VUT_MINCALFRAG:
    src->value.mincalfrag = 0;
    return true;
  case VUT_TERRAINALTER:
    src->value.terrainalter = TA_CAN_IRRIGATE;
    return true;
  case VUT_CITYTILE:
    src->value.citytile = CITYT_CENTER;
    return true;
  case VUT_CITYSTATUS:
    src->value.citystatus = CITYS_OWNED_BY_ORIGINAL;
    return true;
  case VUT_GOOD:
    if (game.control.num_goods_types <= 0) {
      return false;
    }
    src->value.good = goods_by_number(0);
    return true;
  case VUT_TERRFLAG:
    src->value.terrainflag = TER_NO_BARBS;
    return true;
  case VUT_NATIONALITY:
    if (game.control.nation_count <= 0) {
      return false;
    }
    src->value.nationality = nation_by_number(0);
    return true;
  case VUT_BASEFLAG:
    src->value.baseflag = BF_NOT_AGGRESSIVE;
    return true;
  case VUT_ROADFLAG:
    src->value.roadflag = RF_RIVER;
    return true;
  case VUT_EXTRA:
    if (game.control.num_extra_types <= 0) {
      return false;
    }
    src->value.extra = extra_by_number(0);
    return true;
  case VUT_TECHFLAG:
    src->value.techflag = TF_BONUS_TECH;
    return true;
  case VUT_ACHIEVEMENT:
    if (game.control.num_achievement_types <= 0) {
      return false;
    }
    src->value.achievement = achievement_by_number(0);
    return true;
  case VUT_DIPLREL:
    src->value.diplrel = DS_WAR;
    return true;
  case VUT_MAXTILEUNITS:
    src->value.max_tile_units = 0;
    return true;
  case VUT_STYLE:
    if (game.control.num_styles <= 0) {
      return false;
    }
    src->value.style = style_by_number(0);
    return true;
  case VUT_MINCULTURE:
    src->value.minculture = 0;
    return true;
  case VUT_MINFOREIGNPCT:
    src->value.minforeignpct = 0;
    return true;
  case VUT_UNITSTATE:
    src->value.unit_state = USP_TRANSPORTED;
    return true;
  case VUT_ACTIVITY:
    src->value.activity = ACTIVITY_IDLE;
    return true;
  case VUT_MINMOVES:
    src->value.minmoves = 0;
    return true;
  case VUT_MINVETERAN:
    src->value.minveteran = 0;
    return true;
  case VUT_MINHP:
    src->value.min_hit_points = 0;
    return true;
  case VUT_AGE:
    src->value.age = 0;
    return true;
  case VUT_NATIONGROUP:
    if (nation_group_count() <= 0) {
      return false;
    }
    src->value.nationgroup = nation_group_by_number(0);
    return true;
  case VUT_TOPO:
    src->value.topo_property = TF_ISO;
    return true;
  case VUT_SERVERSETTING:
    src->value.ssetval =
        ssetv_from_values(server_setting_by_name("killstack"), true);
    return true;
  case VUT_IMPR_GENUS:
    src->value.impr_genus = IG_IMPROVEMENT;
    return true;
  case VUT_ACTION:
    src->value.action = action_by_number(0);
    return true;
  case VUT_MINTECHS:
    src->value.min_techs = 0;
    return true;
  case VUT_EXTRAFLAG:
    src->value.extraflag = EF_NATIVE_TILE;
    return true;
  case VUT_VISIONLAYER:
    src->value.vlayer = V_MAIN;
    return true;
  case VUT_COUNT:
    fc_assert(src->kind != VUT_COUNT);
    return false;
  }

  return false;
}

/**
   Call cb for each value possible for the universal kind.
 */
void universal_kind_values(struct universal *univ, univ_kind_values_cb cb,
                           void *data)
{
  int i;

  switch (univ->kind) {
  case VUT_NONE:
    break;
  case VUT_ADVANCE:
    advance_re_active_iterate(padv)
    {
      cb(advance_rule_name(padv), univ->value.advance == padv, data);
    }
    advance_re_active_iterate_end;
    break;
  case VUT_GOVERNMENT:
    for (auto &pgov : governments) {
      if (!pgov.ruledit_disabled) {
        {
          cb(government_rule_name(&pgov), univ->value.govern == &pgov, data);
        }
      }
    }
    break;
  case VUT_IMPROVEMENT:
    improvement_re_active_iterate(pimpr)
    {
      cb(improvement_rule_name(pimpr), univ->value.building == pimpr, data);
    }
    improvement_re_active_iterate_end;
    break;
  case VUT_TERRAIN:
    terrain_re_active_iterate(pterr)
    {
      cb(terrain_rule_name(pterr), univ->value.terrain == pterr, data);
    }
    terrain_re_active_iterate_end;
    break;
  case VUT_NATION:
    for (auto &pnat : nations) {
      cb(nation_rule_name(&pnat), univ->value.nation == &pnat, data);
    }
    break;
  case VUT_UTYPE:
    unit_type_re_active_iterate(putype)
    {
      cb(utype_rule_name(putype), univ->value.utype == putype, data);
    }
    unit_type_re_active_iterate_end;
    break;
  case VUT_UCLASS:
    unit_class_re_active_iterate(pclass)
    {
      cb(uclass_rule_name(pclass), univ->value.uclass == pclass, data);
    }
    unit_class_re_active_iterate_end;
    break;
  case VUT_OTYPE:
    output_type_iterate(otype)
    {
      cb(get_output_name(otype), univ->value.outputtype == otype, data);
    }
    output_type_iterate_end;
    break;
  case VUT_GOOD:
    goods_type_re_active_iterate(pgood)
    {
      cb(goods_rule_name(pgood), univ->value.good == pgood, data);
    }
    goods_type_re_active_iterate_end;
    break;
  case VUT_NATIONALITY:
    for (auto &pnat : nations) {
      cb(nation_rule_name(&pnat), univ->value.nationality == &pnat, data);
    }
    break;
  case VUT_EXTRA:
    extra_type_re_active_iterate(pextra)
    {
      cb(extra_rule_name(pextra), univ->value.extra == pextra, data);
    }
    extra_type_re_active_iterate_end;
    break;
  case VUT_STYLE:
    styles_re_active_iterate(pstyle)
    {
      cb(style_rule_name(pstyle), univ->value.style == pstyle, data);
    }
    styles_re_active_iterate_end;
    break;
  case VUT_AI_LEVEL:
    for (i = 0; i < AI_LEVEL_COUNT; i++) {
      cb(ai_level_name(ai_level(i)), univ->value.ai_level == i, data);
    }
    break;
  case VUT_SPECIALIST:
    specialist_type_re_active_iterate(pspe)
    {
      cb(specialist_rule_name(pspe), univ->value.specialist == pspe, data);
    }
    specialist_type_re_active_iterate_end;
    break;
  case VUT_TERRAINCLASS:
    for (i = 0; i < TC_COUNT; i++) {
      cb(terrain_class_name(terrain_class(i)), univ->value.terrainclass == i,
         data);
    }
    break;
  case VUT_UTFLAG:
    for (i = 0; i < UTYF_LAST_USER_FLAG; i++) {
      cb(unit_type_flag_id_name(unit_type_flag_id(i)),
         univ->value.unitflag == i, data);
    }
    break;
  case VUT_UCFLAG:
    for (i = 0; i < UCF_COUNT; i++) {
      cb(unit_class_flag_id_name(unit_class_flag_id(i)),
         univ->value.unitclassflag == i, data);
    }
    break;
  case VUT_TERRFLAG:
    for (i = 0; i < TER_USER_LAST; i++) {
      cb(terrain_flag_id_name(terrain_flag_id(i)),
         univ->value.terrainflag == i, data);
    }
    break;
  case VUT_BASEFLAG:
    for (i = 0; i < BF_COUNT; i++) {
      cb(base_flag_id_name(base_flag_id(i)), univ->value.baseflag == i,
         data);
    }
    break;
  case VUT_ROADFLAG:
    for (i = 0; i < RF_COUNT; i++) {
      cb(road_flag_id_name(road_flag_id(i)), univ->value.roadflag == i,
         data);
    }
    break;
  case VUT_TECHFLAG:
    for (i = 0; i < TF_COUNT; i++) {
      cb(tech_flag_id_name(tech_flag_id(i)), univ->value.techflag == i,
         data);
    }
    break;
  case VUT_EXTRAFLAG:
    for (i = 0; i < EF_COUNT; i++) {
      cb(extra_flag_id_name(extra_flag_id(i)), univ->value.extraflag == i,
         data);
    }
    break;
  case VUT_TERRAINALTER:
    for (i = 0; i < TA_COUNT; i++) {
      cb(terrain_alteration_name(terrain_alteration(i)),
         univ->value.terrainalter == i, data);
    }
    break;
  case VUT_CITYTILE:
    for (i = 0; i < CITYT_LAST; i++) {
      cb(citytile_type_name(citytile_type(i)), univ->value.citytile == i,
         data);
    }
    break;
  case VUT_CITYSTATUS:
    for (i = 0; i < CITYS_LAST; i++) {
      cb(citystatus_type_name(citystatus_type(i)),
         univ->value.citystatus == i, data);
    }
    break;
  case VUT_ACHIEVEMENT:
    achievements_re_active_iterate(pach)
    {
      cb(achievement_rule_name(pach), univ->value.achievement == pach, data);
    }
    achievements_re_active_iterate_end;
    break;
  case VUT_DIPLREL:
    for (i = 0; i < DS_LAST; i++) {
      cb(diplstate_type_name(diplstate_type(i)), univ->value.diplrel == i,
         data);
    }
    for (; i < DRO_LAST; i++) {
      cb(diplrel_other_name(diplrel_other(i)), univ->value.diplrel == i,
         data);
    }
    break;
  case VUT_UNITSTATE:
    for (i = 0; i < USP_COUNT; i++) {
      cb(ustate_prop_name(ustate_prop(i)), univ->value.unit_state == i,
         data);
    }
    break;
  case VUT_ACTIVITY:
    activity_type_iterate(act)
    {
      cb(unit_activity_name(act), univ->value.activity == act, data);
    }
    activity_type_iterate_end;
    break;
  case VUT_NATIONGROUP:
    nation_groups_iterate(pgroup)
    {
      cb(nation_group_rule_name(pgroup), univ->value.nationgroup == pgroup,
         data);
    }
    nation_groups_iterate_end;
    break;
  case VUT_TOPO:
    for (i = 0; i < TOPO_FLAG_BITS; i++) {
      cb(topo_flag_name(topo_flag(1 << i)),
         univ->value.topo_property == 1 << i, data);
    }
    break;
  case VUT_SERVERSETTING:
    for (i = 0;
         /* Only binary settings with the value TRUE are currently
          * supported. */
         i < settings_number(); i++) {
      if (sanity_check_server_setting_value_in_req(i)) {
        cb(ssetv_rule_name(i),
           univ->value.ssetval == ssetv_from_values(i, true), data);
      }
    }
    break;
  case VUT_IMPR_GENUS:
    for (i = 0; i < IG_COUNT; i++) {
      cb(impr_genus_id_name(impr_genus_id(i)), univ->value.impr_genus == i,
         data);
    }
    break;
  case VUT_ACTION:
    action_iterate(act)
    {
      struct action *pact = action_by_number(act);

      cb(action_rule_name(pact), univ->value.action == pact, data);
    }
    action_iterate_end;
    break;
  case VUT_VISIONLAYER:
    for (i = 0; i < V_COUNT; i++) {
      cb(vision_layer_name(vision_layer(i)), univ->value.vlayer == i, data);
    }
    break;
  case VUT_MINSIZE:
  case VUT_MINYEAR:
  case VUT_MINCALFRAG:
  case VUT_MAXTILEUNITS:
  case VUT_MINCULTURE:
  case VUT_MINFOREIGNPCT:
  case VUT_MINMOVES:
  case VUT_MINVETERAN:
  case VUT_MINHP:
  case VUT_AGE:
  case VUT_MINTECHS:
    // Requirement types having numerical value
    cb(nullptr, false, data);
    break;
  case VUT_COUNT:
    fc_assert(univ->kind != VUT_COUNT);
    break;
  }
}
