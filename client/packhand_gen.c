
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/



#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "packets.h"

#include "packhand_gen.h"
    
bool client_handle_packet(enum packet_type type, const void *packet)
{
  switch (type) {
  case PACKET_PROCESSING_STARTED:
    handle_processing_started();
    return TRUE;

  case PACKET_PROCESSING_FINISHED:
    handle_processing_finished();
    return TRUE;

  case PACKET_SERVER_JOIN_REPLY:
    handle_server_join_reply(
      ((const struct packet_server_join_reply *)packet)->you_can_join,
      ((const struct packet_server_join_reply *)packet)->message,
      ((const struct packet_server_join_reply *)packet)->capability,
      ((const struct packet_server_join_reply *)packet)->challenge_file,
      ((const struct packet_server_join_reply *)packet)->conn_id);
    return TRUE;

  case PACKET_AUTHENTICATION_REQ:
    handle_authentication_req(
      ((const struct packet_authentication_req *)packet)->type,
      ((const struct packet_authentication_req *)packet)->message);
    return TRUE;

  case PACKET_SERVER_SHUTDOWN:
    handle_server_shutdown();
    return TRUE;

  case PACKET_ENDGAME_REPORT:
    handle_endgame_report(packet);
    return TRUE;

  case PACKET_ENDGAME_PLAYER:
    handle_endgame_player(packet);
    return TRUE;

  case PACKET_TILE_INFO:
    handle_tile_info(packet);
    return TRUE;

  case PACKET_GAME_INFO:
    handle_game_info(packet);
    return TRUE;

  case PACKET_TIMEOUT_INFO:
    handle_timeout_info(
      ((const struct packet_timeout_info *)packet)->seconds_to_phasedone,
      ((const struct packet_timeout_info *)packet)->last_turn_change_time);
    return TRUE;

  case PACKET_MAP_INFO:
    handle_map_info(
      ((const struct packet_map_info *)packet)->xsize,
      ((const struct packet_map_info *)packet)->ysize,
      ((const struct packet_map_info *)packet)->topology_id);
    return TRUE;

  case PACKET_NUKE_TILE_INFO:
    handle_nuke_tile_info(
      ((const struct packet_nuke_tile_info *)packet)->tile);
    return TRUE;

  case PACKET_TEAM_NAME_INFO:
    handle_team_name_info(
      ((const struct packet_team_name_info *)packet)->team_id,
      ((const struct packet_team_name_info *)packet)->team_name);
    return TRUE;

  case PACKET_ACHIEVEMENT_INFO:
    handle_achievement_info(
      ((const struct packet_achievement_info *)packet)->id,
      ((const struct packet_achievement_info *)packet)->gained,
      ((const struct packet_achievement_info *)packet)->first);
    return TRUE;

  case PACKET_CHAT_MSG:
    handle_chat_msg(packet);
    return TRUE;

  case PACKET_EARLY_CHAT_MSG:
    handle_early_chat_msg(packet);
    return TRUE;

  case PACKET_CONNECT_MSG:
    handle_connect_msg(
      ((const struct packet_connect_msg *)packet)->message);
    return TRUE;

  case PACKET_CITY_REMOVE:
    handle_city_remove(
      ((const struct packet_city_remove *)packet)->city_id);
    return TRUE;

  case PACKET_CITY_INFO:
    handle_city_info(packet);
    return TRUE;

  case PACKET_CITY_SHORT_INFO:
    handle_city_short_info(packet);
    return TRUE;

  case PACKET_CITY_NAME_SUGGESTION_INFO:
    handle_city_name_suggestion_info(
      ((const struct packet_city_name_suggestion_info *)packet)->unit_id,
      ((const struct packet_city_name_suggestion_info *)packet)->name);
    return TRUE;

  case PACKET_CITY_SABOTAGE_LIST:
    handle_city_sabotage_list(
      ((const struct packet_city_sabotage_list *)packet)->diplomat_id,
      ((const struct packet_city_sabotage_list *)packet)->city_id,
      ((const struct packet_city_sabotage_list *)packet)->improvements);
    return TRUE;

  case PACKET_WORKER_TASK:
    handle_worker_task(packet);
    return TRUE;

  case PACKET_PLAYER_REMOVE:
    handle_player_remove(
      ((const struct packet_player_remove *)packet)->playerno);
    return TRUE;

  case PACKET_PLAYER_INFO:
    handle_player_info(packet);
    return TRUE;

  case PACKET_PLAYER_ATTRIBUTE_CHUNK:
    handle_player_attribute_chunk(packet);
    return TRUE;

  case PACKET_PLAYER_DIPLSTATE:
    handle_player_diplstate(packet);
    return TRUE;

  case PACKET_RESEARCH_INFO:
    handle_research_info(packet);
    return TRUE;

  case PACKET_UNIT_REMOVE:
    handle_unit_remove(
      ((const struct packet_unit_remove *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_INFO:
    handle_unit_info(packet);
    return TRUE;

  case PACKET_UNIT_SHORT_INFO:
    handle_unit_short_info(packet);
    return TRUE;

  case PACKET_UNIT_COMBAT_INFO:
    handle_unit_combat_info(
      ((const struct packet_unit_combat_info *)packet)->attacker_unit_id,
      ((const struct packet_unit_combat_info *)packet)->defender_unit_id,
      ((const struct packet_unit_combat_info *)packet)->attacker_hp,
      ((const struct packet_unit_combat_info *)packet)->defender_hp,
      ((const struct packet_unit_combat_info *)packet)->make_winner_veteran);
    return TRUE;

  case PACKET_UNIT_ACTION_ANSWER:
    handle_unit_action_answer(
      ((const struct packet_unit_action_answer *)packet)->diplomat_id,
      ((const struct packet_unit_action_answer *)packet)->target_id,
      ((const struct packet_unit_action_answer *)packet)->cost,
      ((const struct packet_unit_action_answer *)packet)->action_type);
    return TRUE;

  case PACKET_UNIT_ACTIONS:
    handle_unit_actions(packet);
    return TRUE;

  case PACKET_DIPLOMACY_INIT_MEETING:
    handle_diplomacy_init_meeting(
      ((const struct packet_diplomacy_init_meeting *)packet)->counterpart,
      ((const struct packet_diplomacy_init_meeting *)packet)->initiated_from);
    return TRUE;

  case PACKET_DIPLOMACY_CANCEL_MEETING:
    handle_diplomacy_cancel_meeting(
      ((const struct packet_diplomacy_cancel_meeting *)packet)->counterpart,
      ((const struct packet_diplomacy_cancel_meeting *)packet)->initiated_from);
    return TRUE;

  case PACKET_DIPLOMACY_CREATE_CLAUSE:
    handle_diplomacy_create_clause(
      ((const struct packet_diplomacy_create_clause *)packet)->counterpart,
      ((const struct packet_diplomacy_create_clause *)packet)->giver,
      ((const struct packet_diplomacy_create_clause *)packet)->type,
      ((const struct packet_diplomacy_create_clause *)packet)->value);
    return TRUE;

  case PACKET_DIPLOMACY_REMOVE_CLAUSE:
    handle_diplomacy_remove_clause(
      ((const struct packet_diplomacy_remove_clause *)packet)->counterpart,
      ((const struct packet_diplomacy_remove_clause *)packet)->giver,
      ((const struct packet_diplomacy_remove_clause *)packet)->type,
      ((const struct packet_diplomacy_remove_clause *)packet)->value);
    return TRUE;

  case PACKET_DIPLOMACY_ACCEPT_TREATY:
    handle_diplomacy_accept_treaty(
      ((const struct packet_diplomacy_accept_treaty *)packet)->counterpart,
      ((const struct packet_diplomacy_accept_treaty *)packet)->I_accepted,
      ((const struct packet_diplomacy_accept_treaty *)packet)->other_accepted);
    return TRUE;

  case PACKET_PAGE_MSG:
    handle_page_msg(
      ((const struct packet_page_msg *)packet)->caption,
      ((const struct packet_page_msg *)packet)->headline,
      ((const struct packet_page_msg *)packet)->event,
      ((const struct packet_page_msg *)packet)->len,
      ((const struct packet_page_msg *)packet)->parts);
    return TRUE;

  case PACKET_PAGE_MSG_PART:
    handle_page_msg_part(
      ((const struct packet_page_msg_part *)packet)->lines);
    return TRUE;

  case PACKET_CONN_INFO:
    handle_conn_info(packet);
    return TRUE;

  case PACKET_CONN_PING_INFO:
    handle_conn_ping_info(
      ((const struct packet_conn_ping_info *)packet)->connections,
      ((const struct packet_conn_ping_info *)packet)->conn_id,
      ((const struct packet_conn_ping_info *)packet)->ping_time);
    return TRUE;

  case PACKET_CONN_PING:
    handle_conn_ping();
    return TRUE;

  case PACKET_END_PHASE:
    handle_end_phase();
    return TRUE;

  case PACKET_START_PHASE:
    handle_start_phase(
      ((const struct packet_start_phase *)packet)->phase);
    return TRUE;

  case PACKET_NEW_YEAR:
    handle_new_year(
      ((const struct packet_new_year *)packet)->year,
      ((const struct packet_new_year *)packet)->fragments,
      ((const struct packet_new_year *)packet)->turn);
    return TRUE;

  case PACKET_BEGIN_TURN:
    handle_begin_turn();
    return TRUE;

  case PACKET_END_TURN:
    handle_end_turn();
    return TRUE;

  case PACKET_FREEZE_CLIENT:
    handle_freeze_client();
    return TRUE;

  case PACKET_THAW_CLIENT:
    handle_thaw_client();
    return TRUE;

  case PACKET_SPACESHIP_INFO:
    handle_spaceship_info(packet);
    return TRUE;

  case PACKET_RULESET_UNIT:
    handle_ruleset_unit(packet);
    return TRUE;

  case PACKET_RULESET_UNIT_BONUS:
    handle_ruleset_unit_bonus(packet);
    return TRUE;

  case PACKET_RULESET_UNIT_FLAG:
    handle_ruleset_unit_flag(packet);
    return TRUE;

  case PACKET_RULESET_GAME:
    handle_ruleset_game(packet);
    return TRUE;

  case PACKET_RULESET_SPECIALIST:
    handle_ruleset_specialist(packet);
    return TRUE;

  case PACKET_RULESET_GOVERNMENT_RULER_TITLE:
    handle_ruleset_government_ruler_title(packet);
    return TRUE;

  case PACKET_RULESET_TECH:
    handle_ruleset_tech(packet);
    return TRUE;

  case PACKET_RULESET_TECH_FLAG:
    handle_ruleset_tech_flag(packet);
    return TRUE;

  case PACKET_RULESET_GOVERNMENT:
    handle_ruleset_government(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN_CONTROL:
    handle_ruleset_terrain_control(packet);
    return TRUE;

  case PACKET_RULESETS_READY:
    handle_rulesets_ready();
    return TRUE;

  case PACKET_RULESET_NATION_SETS:
    handle_ruleset_nation_sets(packet);
    return TRUE;

  case PACKET_RULESET_NATION_GROUPS:
    handle_ruleset_nation_groups(packet);
    return TRUE;

  case PACKET_RULESET_NATION:
    handle_ruleset_nation(packet);
    return TRUE;

  case PACKET_NATION_AVAILABILITY:
    handle_nation_availability(
      ((const struct packet_nation_availability *)packet)->ncount,
      ((const struct packet_nation_availability *)packet)->is_pickable,
      ((const struct packet_nation_availability *)packet)->nationset_change);
    return TRUE;

  case PACKET_RULESET_STYLE:
    handle_ruleset_style(packet);
    return TRUE;

  case PACKET_RULESET_CITY:
    handle_ruleset_city(packet);
    return TRUE;

  case PACKET_RULESET_BUILDING:
    handle_ruleset_building(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN:
    handle_ruleset_terrain(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN_FLAG:
    handle_ruleset_terrain_flag(packet);
    return TRUE;

  case PACKET_RULESET_UNIT_CLASS:
    handle_ruleset_unit_class(packet);
    return TRUE;

  case PACKET_RULESET_EXTRA:
    handle_ruleset_extra(packet);
    return TRUE;

  case PACKET_RULESET_BASE:
    handle_ruleset_base(packet);
    return TRUE;

  case PACKET_RULESET_ROAD:
    handle_ruleset_road(packet);
    return TRUE;

  case PACKET_RULESET_DISASTER:
    handle_ruleset_disaster(packet);
    return TRUE;

  case PACKET_RULESET_ACHIEVEMENT:
    handle_ruleset_achievement(packet);
    return TRUE;

  case PACKET_RULESET_TRADE:
    handle_ruleset_trade(packet);
    return TRUE;

  case PACKET_RULESET_ACTION:
    handle_ruleset_action(packet);
    return TRUE;

  case PACKET_RULESET_ACTION_ENABLER:
    handle_ruleset_action_enabler(packet);
    return TRUE;

  case PACKET_RULESET_MUSIC:
    handle_ruleset_music(packet);
    return TRUE;

  case PACKET_RULESET_MULTIPLIER:
    handle_ruleset_multiplier(packet);
    return TRUE;

  case PACKET_RULESET_CONTROL:
    handle_ruleset_control(packet);
    return TRUE;

  case PACKET_RULESET_SUMMARY:
    handle_ruleset_summary(packet);
    return TRUE;

  case PACKET_RULESET_DESCRIPTION_PART:
    handle_ruleset_description_part(packet);
    return TRUE;

  case PACKET_SINGLE_WANT_HACK_REPLY:
    handle_single_want_hack_reply(
      ((const struct packet_single_want_hack_reply *)packet)->you_have_hack);
    return TRUE;

  case PACKET_RULESET_CHOICES:
    handle_ruleset_choices(packet);
    return TRUE;

  case PACKET_GAME_LOAD:
    handle_game_load(
      ((const struct packet_game_load *)packet)->load_successful,
      ((const struct packet_game_load *)packet)->load_filename);
    return TRUE;

  case PACKET_SERVER_SETTING_CONTROL:
    handle_server_setting_control(packet);
    return TRUE;

  case PACKET_SERVER_SETTING_CONST:
    handle_server_setting_const(packet);
    return TRUE;

  case PACKET_SERVER_SETTING_BOOL:
    handle_server_setting_bool(packet);
    return TRUE;

  case PACKET_SERVER_SETTING_INT:
    handle_server_setting_int(packet);
    return TRUE;

  case PACKET_SERVER_SETTING_STR:
    handle_server_setting_str(packet);
    return TRUE;

  case PACKET_SERVER_SETTING_ENUM:
    handle_server_setting_enum(packet);
    return TRUE;

  case PACKET_SERVER_SETTING_BITWISE:
    handle_server_setting_bitwise(packet);
    return TRUE;

  case PACKET_SET_TOPOLOGY:
    handle_set_topology(
      ((const struct packet_set_topology *)packet)->topology_id);
    return TRUE;

  case PACKET_RULESET_EFFECT:
    handle_ruleset_effect(packet);
    return TRUE;

  case PACKET_RULESET_RESOURCE:
    handle_ruleset_resource(packet);
    return TRUE;

  case PACKET_SCENARIO_INFO:
    handle_scenario_info(packet);
    return TRUE;

  case PACKET_SCENARIO_DESCRIPTION:
    handle_scenario_description(
      ((const struct packet_scenario_description *)packet)->description);
    return TRUE;

  case PACKET_VOTE_NEW:
    handle_vote_new(packet);
    return TRUE;

  case PACKET_VOTE_UPDATE:
    handle_vote_update(
      ((const struct packet_vote_update *)packet)->vote_no,
      ((const struct packet_vote_update *)packet)->yes,
      ((const struct packet_vote_update *)packet)->no,
      ((const struct packet_vote_update *)packet)->abstain,
      ((const struct packet_vote_update *)packet)->num_voters);
    return TRUE;

  case PACKET_VOTE_REMOVE:
    handle_vote_remove(
      ((const struct packet_vote_remove *)packet)->vote_no);
    return TRUE;

  case PACKET_VOTE_RESOLVE:
    handle_vote_resolve(
      ((const struct packet_vote_resolve *)packet)->vote_no,
      ((const struct packet_vote_resolve *)packet)->passed);
    return TRUE;

  case PACKET_EDIT_STARTPOS:
    handle_edit_startpos(packet);
    return TRUE;

  case PACKET_EDIT_STARTPOS_FULL:
    handle_edit_startpos_full(packet);
    return TRUE;

  case PACKET_EDIT_OBJECT_CREATED:
    handle_edit_object_created(
      ((const struct packet_edit_object_created *)packet)->tag,
      ((const struct packet_edit_object_created *)packet)->id);
    return TRUE;

  case PACKET_PLAY_MUSIC:
    handle_play_music(
      ((const struct packet_play_music *)packet)->tag);
    return TRUE;

  default:
    return FALSE;
  }
}
