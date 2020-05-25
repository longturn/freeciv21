
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

#include "hand_gen.h"
    
bool server_handle_packet(enum packet_type type, const void *packet,
                          struct player *pplayer, struct connection *pconn)
{
  switch (type) {
  case PACKET_NATION_SELECT_REQ:
    handle_nation_select_req(pconn,
      ((const struct packet_nation_select_req *)packet)->player_no,
      ((const struct packet_nation_select_req *)packet)->nation_no,
      ((const struct packet_nation_select_req *)packet)->is_male,
      ((const struct packet_nation_select_req *)packet)->name,
      ((const struct packet_nation_select_req *)packet)->style);
    return TRUE;

  case PACKET_PLAYER_READY:
    handle_player_ready(pplayer,
      ((const struct packet_player_ready *)packet)->player_no,
      ((const struct packet_player_ready *)packet)->is_ready);
    return TRUE;

  case PACKET_CHAT_MSG_REQ:
    handle_chat_msg_req(pconn,
      ((const struct packet_chat_msg_req *)packet)->message);
    return TRUE;

  case PACKET_CITY_SELL:
    handle_city_sell(pplayer,
      ((const struct packet_city_sell *)packet)->city_id,
      ((const struct packet_city_sell *)packet)->build_id);
    return TRUE;

  case PACKET_CITY_BUY:
    handle_city_buy(pplayer,
      ((const struct packet_city_buy *)packet)->city_id);
    return TRUE;

  case PACKET_CITY_CHANGE:
    handle_city_change(pplayer,
      ((const struct packet_city_change *)packet)->city_id,
      ((const struct packet_city_change *)packet)->production_kind,
      ((const struct packet_city_change *)packet)->production_value);
    return TRUE;

  case PACKET_CITY_WORKLIST:
    handle_city_worklist(pplayer,
      ((const struct packet_city_worklist *)packet)->city_id,
      &((const struct packet_city_worklist *)packet)->worklist);
    return TRUE;

  case PACKET_CITY_MAKE_SPECIALIST:
    handle_city_make_specialist(pplayer,
      ((const struct packet_city_make_specialist *)packet)->city_id,
      ((const struct packet_city_make_specialist *)packet)->worker_x,
      ((const struct packet_city_make_specialist *)packet)->worker_y);
    return TRUE;

  case PACKET_CITY_MAKE_WORKER:
    handle_city_make_worker(pplayer,
      ((const struct packet_city_make_worker *)packet)->city_id,
      ((const struct packet_city_make_worker *)packet)->worker_x,
      ((const struct packet_city_make_worker *)packet)->worker_y);
    return TRUE;

  case PACKET_CITY_CHANGE_SPECIALIST:
    handle_city_change_specialist(pplayer,
      ((const struct packet_city_change_specialist *)packet)->city_id,
      ((const struct packet_city_change_specialist *)packet)->from,
      ((const struct packet_city_change_specialist *)packet)->to);
    return TRUE;

  case PACKET_CITY_RENAME:
    handle_city_rename(pplayer,
      ((const struct packet_city_rename *)packet)->city_id,
      ((const struct packet_city_rename *)packet)->name);
    return TRUE;

  case PACKET_CITY_OPTIONS_REQ:
    handle_city_options_req(pplayer,
      ((const struct packet_city_options_req *)packet)->city_id,
      ((const struct packet_city_options_req *)packet)->options);
    return TRUE;

  case PACKET_CITY_REFRESH:
    handle_city_refresh(pplayer,
      ((const struct packet_city_refresh *)packet)->city_id);
    return TRUE;

  case PACKET_CITY_NAME_SUGGESTION_REQ:
    handle_city_name_suggestion_req(pplayer,
      ((const struct packet_city_name_suggestion_req *)packet)->unit_id);
    return TRUE;

  case PACKET_WORKER_TASK:
    handle_worker_task(pplayer, packet);
    return TRUE;

  case PACKET_PLAYER_PHASE_DONE:
    handle_player_phase_done(pplayer,
      ((const struct packet_player_phase_done *)packet)->turn);
    return TRUE;

  case PACKET_PLAYER_RATES:
    handle_player_rates(pplayer,
      ((const struct packet_player_rates *)packet)->tax,
      ((const struct packet_player_rates *)packet)->luxury,
      ((const struct packet_player_rates *)packet)->science);
    return TRUE;

  case PACKET_PLAYER_CHANGE_GOVERNMENT:
    handle_player_change_government(pplayer,
      ((const struct packet_player_change_government *)packet)->government);
    return TRUE;

  case PACKET_PLAYER_ATTRIBUTE_BLOCK:
    handle_player_attribute_block(pplayer);
    return TRUE;

  case PACKET_PLAYER_ATTRIBUTE_CHUNK:
    handle_player_attribute_chunk(pplayer, packet);
    return TRUE;

  case PACKET_PLAYER_MULTIPLIER:
    handle_player_multiplier(pplayer,
      ((const struct packet_player_multiplier *)packet)->count,
      ((const struct packet_player_multiplier *)packet)->multipliers);
    return TRUE;

  case PACKET_PLAYER_RESEARCH:
    handle_player_research(pplayer,
      ((const struct packet_player_research *)packet)->tech);
    return TRUE;

  case PACKET_PLAYER_TECH_GOAL:
    handle_player_tech_goal(pplayer,
      ((const struct packet_player_tech_goal *)packet)->tech);
    return TRUE;

  case PACKET_UNIT_BUILD_CITY:
    handle_unit_build_city(pplayer,
      ((const struct packet_unit_build_city *)packet)->unit_id,
      ((const struct packet_unit_build_city *)packet)->name);
    return TRUE;

  case PACKET_UNIT_DISBAND:
    handle_unit_disband(pplayer,
      ((const struct packet_unit_disband *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_CHANGE_HOMECITY:
    handle_unit_change_homecity(pplayer,
      ((const struct packet_unit_change_homecity *)packet)->unit_id,
      ((const struct packet_unit_change_homecity *)packet)->city_id);
    return TRUE;

  case PACKET_UNIT_BATTLEGROUP:
    handle_unit_battlegroup(pplayer,
      ((const struct packet_unit_battlegroup *)packet)->unit_id,
      ((const struct packet_unit_battlegroup *)packet)->battlegroup);
    return TRUE;

  case PACKET_UNIT_ORDERS:
    handle_unit_orders(pplayer, packet);
    return TRUE;

  case PACKET_UNIT_AUTOSETTLERS:
    handle_unit_autosettlers(pplayer,
      ((const struct packet_unit_autosettlers *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_LOAD:
    handle_unit_load(pplayer,
      ((const struct packet_unit_load *)packet)->cargo_id,
      ((const struct packet_unit_load *)packet)->transporter_id,
      ((const struct packet_unit_load *)packet)->transporter_tile);
    return TRUE;

  case PACKET_UNIT_UNLOAD:
    handle_unit_unload(pplayer,
      ((const struct packet_unit_unload *)packet)->cargo_id,
      ((const struct packet_unit_unload *)packet)->transporter_id);
    return TRUE;

  case PACKET_UNIT_UPGRADE:
    handle_unit_upgrade(pplayer,
      ((const struct packet_unit_upgrade *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_NUKE:
    handle_unit_nuke(pplayer,
      ((const struct packet_unit_nuke *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_PARADROP_TO:
    handle_unit_paradrop_to(pplayer,
      ((const struct packet_unit_paradrop_to *)packet)->unit_id,
      ((const struct packet_unit_paradrop_to *)packet)->tile);
    return TRUE;

  case PACKET_UNIT_AIRLIFT:
    handle_unit_airlift(pplayer,
      ((const struct packet_unit_airlift *)packet)->unit_id,
      ((const struct packet_unit_airlift *)packet)->city_id);
    return TRUE;

  case PACKET_UNIT_ACTION_QUERY:
    handle_unit_action_query(pconn,
      ((const struct packet_unit_action_query *)packet)->diplomat_id,
      ((const struct packet_unit_action_query *)packet)->target_id,
      ((const struct packet_unit_action_query *)packet)->action_type);
    return TRUE;

  case PACKET_UNIT_TYPE_UPGRADE:
    handle_unit_type_upgrade(pplayer,
      ((const struct packet_unit_type_upgrade *)packet)->type);
    return TRUE;

  case PACKET_UNIT_DO_ACTION:
    handle_unit_do_action(pplayer,
      ((const struct packet_unit_do_action *)packet)->actor_id,
      ((const struct packet_unit_do_action *)packet)->target_id,
      ((const struct packet_unit_do_action *)packet)->value,
      ((const struct packet_unit_do_action *)packet)->action_type);
    return TRUE;

  case PACKET_UNIT_GET_ACTIONS:
    handle_unit_get_actions(pconn,
      ((const struct packet_unit_get_actions *)packet)->actor_unit_id,
      ((const struct packet_unit_get_actions *)packet)->target_unit_id,
      ((const struct packet_unit_get_actions *)packet)->target_city_id,
      ((const struct packet_unit_get_actions *)packet)->target_tile_id,
      ((const struct packet_unit_get_actions *)packet)->disturb_player);
    return TRUE;

  case PACKET_UNIT_CHANGE_ACTIVITY:
    handle_unit_change_activity(pplayer,
      ((const struct packet_unit_change_activity *)packet)->unit_id,
      ((const struct packet_unit_change_activity *)packet)->activity,
      ((const struct packet_unit_change_activity *)packet)->target);
    return TRUE;

  case PACKET_DIPLOMACY_INIT_MEETING_REQ:
    handle_diplomacy_init_meeting_req(pplayer,
      ((const struct packet_diplomacy_init_meeting_req *)packet)->counterpart);
    return TRUE;

  case PACKET_DIPLOMACY_CANCEL_MEETING_REQ:
    handle_diplomacy_cancel_meeting_req(pplayer,
      ((const struct packet_diplomacy_cancel_meeting_req *)packet)->counterpart);
    return TRUE;

  case PACKET_DIPLOMACY_CREATE_CLAUSE_REQ:
    handle_diplomacy_create_clause_req(pplayer,
      ((const struct packet_diplomacy_create_clause_req *)packet)->counterpart,
      ((const struct packet_diplomacy_create_clause_req *)packet)->giver,
      ((const struct packet_diplomacy_create_clause_req *)packet)->type,
      ((const struct packet_diplomacy_create_clause_req *)packet)->value);
    return TRUE;

  case PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ:
    handle_diplomacy_remove_clause_req(pplayer,
      ((const struct packet_diplomacy_remove_clause_req *)packet)->counterpart,
      ((const struct packet_diplomacy_remove_clause_req *)packet)->giver,
      ((const struct packet_diplomacy_remove_clause_req *)packet)->type,
      ((const struct packet_diplomacy_remove_clause_req *)packet)->value);
    return TRUE;

  case PACKET_DIPLOMACY_ACCEPT_TREATY_REQ:
    handle_diplomacy_accept_treaty_req(pplayer,
      ((const struct packet_diplomacy_accept_treaty_req *)packet)->counterpart);
    return TRUE;

  case PACKET_DIPLOMACY_CANCEL_PACT:
    handle_diplomacy_cancel_pact(pplayer,
      ((const struct packet_diplomacy_cancel_pact *)packet)->other_player_id,
      ((const struct packet_diplomacy_cancel_pact *)packet)->clause);
    return TRUE;

  case PACKET_REPORT_REQ:
    handle_report_req(pconn,
      ((const struct packet_report_req *)packet)->type);
    return TRUE;

  case PACKET_CONN_PONG:
    handle_conn_pong(pconn);
    return TRUE;

  case PACKET_CLIENT_HEARTBEAT:
    handle_client_heartbeat(pconn);
    return TRUE;

  case PACKET_CLIENT_INFO:
    handle_client_info(pconn,
      ((const struct packet_client_info *)packet)->gui,
      ((const struct packet_client_info *)packet)->distribution);
    return TRUE;

  case PACKET_SPACESHIP_LAUNCH:
    handle_spaceship_launch(pplayer);
    return TRUE;

  case PACKET_SPACESHIP_PLACE:
    handle_spaceship_place(pplayer,
      ((const struct packet_spaceship_place *)packet)->type,
      ((const struct packet_spaceship_place *)packet)->num);
    return TRUE;

  case PACKET_SINGLE_WANT_HACK_REQ:
    handle_single_want_hack_req(pconn, packet);
    return TRUE;

  case PACKET_SAVE_SCENARIO:
    handle_save_scenario(pconn,
      ((const struct packet_save_scenario *)packet)->name);
    return TRUE;

  case PACKET_VOTE_SUBMIT:
    handle_vote_submit(pconn,
      ((const struct packet_vote_submit *)packet)->vote_no,
      ((const struct packet_vote_submit *)packet)->value);
    return TRUE;

  case PACKET_EDIT_MODE:
    handle_edit_mode(pconn,
      ((const struct packet_edit_mode *)packet)->state);
    return TRUE;

  case PACKET_EDIT_RECALCULATE_BORDERS:
    handle_edit_recalculate_borders(pconn);
    return TRUE;

  case PACKET_EDIT_CHECK_TILES:
    handle_edit_check_tiles(pconn);
    return TRUE;

  case PACKET_EDIT_TOGGLE_FOGOFWAR:
    handle_edit_toggle_fogofwar(pconn,
      ((const struct packet_edit_toggle_fogofwar *)packet)->player);
    return TRUE;

  case PACKET_EDIT_TILE_TERRAIN:
    handle_edit_tile_terrain(pconn,
      ((const struct packet_edit_tile_terrain *)packet)->tile,
      ((const struct packet_edit_tile_terrain *)packet)->terrain,
      ((const struct packet_edit_tile_terrain *)packet)->size);
    return TRUE;

  case PACKET_EDIT_TILE_RESOURCE:
    handle_edit_tile_resource(pconn,
      ((const struct packet_edit_tile_resource *)packet)->tile,
      ((const struct packet_edit_tile_resource *)packet)->resource,
      ((const struct packet_edit_tile_resource *)packet)->size);
    return TRUE;

  case PACKET_EDIT_TILE_EXTRA:
    handle_edit_tile_extra(pconn,
      ((const struct packet_edit_tile_extra *)packet)->tile,
      ((const struct packet_edit_tile_extra *)packet)->extra_type_id,
      ((const struct packet_edit_tile_extra *)packet)->removal,
      ((const struct packet_edit_tile_extra *)packet)->size);
    return TRUE;

  case PACKET_EDIT_STARTPOS:
    handle_edit_startpos(pconn, packet);
    return TRUE;

  case PACKET_EDIT_STARTPOS_FULL:
    handle_edit_startpos_full(pconn, packet);
    return TRUE;

  case PACKET_EDIT_TILE:
    handle_edit_tile(pconn, packet);
    return TRUE;

  case PACKET_EDIT_UNIT_CREATE:
    handle_edit_unit_create(pconn,
      ((const struct packet_edit_unit_create *)packet)->owner,
      ((const struct packet_edit_unit_create *)packet)->tile,
      ((const struct packet_edit_unit_create *)packet)->type,
      ((const struct packet_edit_unit_create *)packet)->count,
      ((const struct packet_edit_unit_create *)packet)->tag);
    return TRUE;

  case PACKET_EDIT_UNIT_REMOVE:
    handle_edit_unit_remove(pconn,
      ((const struct packet_edit_unit_remove *)packet)->owner,
      ((const struct packet_edit_unit_remove *)packet)->tile,
      ((const struct packet_edit_unit_remove *)packet)->type,
      ((const struct packet_edit_unit_remove *)packet)->count);
    return TRUE;

  case PACKET_EDIT_UNIT_REMOVE_BY_ID:
    handle_edit_unit_remove_by_id(pconn,
      ((const struct packet_edit_unit_remove_by_id *)packet)->id);
    return TRUE;

  case PACKET_EDIT_UNIT:
    handle_edit_unit(pconn, packet);
    return TRUE;

  case PACKET_EDIT_CITY_CREATE:
    handle_edit_city_create(pconn,
      ((const struct packet_edit_city_create *)packet)->owner,
      ((const struct packet_edit_city_create *)packet)->tile,
      ((const struct packet_edit_city_create *)packet)->size,
      ((const struct packet_edit_city_create *)packet)->tag);
    return TRUE;

  case PACKET_EDIT_CITY_REMOVE:
    handle_edit_city_remove(pconn,
      ((const struct packet_edit_city_remove *)packet)->id);
    return TRUE;

  case PACKET_EDIT_CITY:
    handle_edit_city(pconn, packet);
    return TRUE;

  case PACKET_EDIT_PLAYER_CREATE:
    handle_edit_player_create(pconn,
      ((const struct packet_edit_player_create *)packet)->tag);
    return TRUE;

  case PACKET_EDIT_PLAYER_REMOVE:
    handle_edit_player_remove(pconn,
      ((const struct packet_edit_player_remove *)packet)->id);
    return TRUE;

  case PACKET_EDIT_PLAYER:
    handle_edit_player(pconn, packet);
    return TRUE;

  case PACKET_EDIT_PLAYER_VISION:
    handle_edit_player_vision(pconn,
      ((const struct packet_edit_player_vision *)packet)->player,
      ((const struct packet_edit_player_vision *)packet)->tile,
      ((const struct packet_edit_player_vision *)packet)->known,
      ((const struct packet_edit_player_vision *)packet)->size);
    return TRUE;

  case PACKET_EDIT_GAME:
    handle_edit_game(pconn, packet);
    return TRUE;

  case PACKET_EDIT_SCENARIO_DESC:
    handle_edit_scenario_desc(pconn,
      ((const struct packet_edit_scenario_desc *)packet)->scenario_desc);
    return TRUE;

  default:
    return FALSE;
  }
}
