
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "actions.h"
#include "disaster.h"

struct packet_processing_started {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_processing_finished {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_server_join_req {
  char username[48];
  char capability[512];
  char version_label[48];
  int major_version;
  int minor_version;
  int patch_version;
};

struct packet_server_join_reply {
  bool you_can_join;
  char message[1536];
  char capability[512];
  char challenge_file[4095];
  int conn_id;
};

struct packet_authentication_req {
  enum authentication_type type;
  char message[MAX_LEN_MSG];
};

struct packet_authentication_reply {
  char password[MAX_LEN_PASSWORD];
};

struct packet_server_shutdown {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_nation_select_req {
  int player_no;
  Nation_type_id nation_no;
  bool is_male;
  char name[MAX_LEN_NAME];
  int style;
};

struct packet_player_ready {
  int player_no;
  bool is_ready;
};

struct packet_endgame_report {
  int category_num;
  char category_name[32][MAX_LEN_NAME];
  int player_num;
};

struct packet_endgame_player {
  int category_num;
  int player_id;
  int score;
  int category_score[32];
  bool winner;
};

struct packet_tile_info {
  int tile;
  Continent_id continent;
  enum known_type known;
  int owner;
  int extras_owner;
  int worked;
  Terrain_type_id terrain;
  Resource_type_id resource;
  bv_extras extras;
  char spec_sprite[MAX_LEN_NAME];
  char label[MAX_LEN_NAME];
};

struct packet_game_info {
  int add_to_size_limit;
  int aifill;
  enum persistent_ready persistent_ready;
  enum airlifting_style airlifting_style;
  int angrycitizen;
  int base_pollution;
  int base_tech_cost;
  int border_city_radius_sq;
  int border_size_effect;
  int border_city_permanent_radius_sq;
  enum borders_mode borders;
  int base_bribe_cost;
  int culture_vic_points;
  int culture_vic_lead;
  int culture_migration_pml;
  bool calendar_skip_0;
  int celebratesize;
  bool changable_tax;
  int pop_report_zeroes;
  bool citizen_nationality;
  int citizen_convert_speed;
  int citizen_partisans_pct;
  int citymindist;
  int cooling;
  int coolinglevel;
  enum diplomacy_mode diplomacy;
  bool fogofwar;
  int food_cost;
  int foodbox;
  int forced_gold;
  int forced_luxury;
  int forced_science;
  int fulltradesize;
  bool global_advances[A_LAST];
  bool global_warming;
  int globalwarming;
  int gold;
  enum gold_upkeep_style gold_upkeep_style;
  enum revolen_type revolentype;
  Government_type_id default_government_id;
  Government_type_id government_during_revolution_id;
  int granary_food_inc;
  int granary_food_ini[MAX_GRANARY_INIS];
  int granary_num_inis;
  int great_wonder_owners[B_LAST];
  int happy_cost;
  enum happyborders_type happyborders;
  int heating;
  int illness_base_factor;
  int illness_min_size;
  bool illness_on;
  int illness_pollution_factor;
  int illness_trade_infection;
  int init_city_radius_sq;
  bool is_edit_mode;
  bool is_new_game;
  bool killcitizen;
  bool killstack;
  int min_city_center_output[O_LAST];
  char negative_year_label[MAX_LEN_NAME];
  int notradesize;
  bool nuclear_winter;
  int nuclearwinter;
  int phase;
  enum phase_mode_types phase_mode;
  bool pillage_select;
  bool tech_steal_allow_holes;
  bool tech_trade_allow_holes;
  bool tech_trade_loss_allow_holes;
  bool tech_parasite_allow_holes;
  bool tech_loss_allow_holes;
  char positive_year_label[MAX_LEN_NAME];
  int rapturedelay;
  int disasters;
  bool restrictinfra;
  bool unreachable_protects;
  int sciencebox;
  int shieldbox;
  int skill_level;
  bool slow_invasions;
  enum victory_condition_type victory_conditions;
  bool team_pooled_research;
  int tech;
  enum tech_cost_style tech_cost_style;
  enum tech_leakage_style tech_leakage;
  int tech_upkeep_divider;
  enum tech_upkeep_style tech_upkeep_style;
  int techloss_forgiveness;
  enum free_tech_method free_tech_method;
  enum gameloss_style gameloss_style;
  int timeout;
  int first_timeout;
  bool tired_attack;
  int trademindist;
  bool force_trade_route;
  bool trading_city;
  bool trading_gold;
  bool trading_tech;
  int turn;
  int warminglevel;
  int year;
  bool year_0_hack;
  int calendar_fragments;
  int fragment_count;
  char calendar_fragment_name[MAX_CALENDAR_FRAGMENTS][MAX_LEN_NAME];
  bool civil_war_enabled;
  bool paradrop_to_transport;
};

struct packet_timeout_info {
  float seconds_to_phasedone;
  float last_turn_change_time;
};

struct packet_map_info {
  int xsize;
  int ysize;
  int topology_id;
};

struct packet_nuke_tile_info {
  int tile;
};

struct packet_team_name_info {
  int team_id;
  char team_name[MAX_LEN_NAME];
};

struct packet_achievement_info {
  int id;
  bool gained;
  bool first;
};

struct packet_chat_msg {
  char message[MAX_LEN_MSG];
  int tile;
  enum event_type event;
  int turn;
  int phase;
  int conn_id;
};

struct packet_early_chat_msg {
  char message[MAX_LEN_MSG];
  int tile;
  enum event_type event;
  int turn;
  int phase;
  int conn_id;
};

struct packet_chat_msg_req {
  char message[MAX_LEN_MSG];
};

struct packet_connect_msg {
  char message[MAX_LEN_MSG];
};

struct packet_city_remove {
  int city_id;
};

struct packet_city_info {
  int id;
  int tile;
  int owner;
  int size;
  int city_radius_sq;
  int style;
  int ppl_happy[FEELING_LAST];
  int ppl_content[FEELING_LAST];
  int ppl_unhappy[FEELING_LAST];
  int ppl_angry[FEELING_LAST];
  int specialists_size;
  int specialists[SP_MAX];
  int nationalities_count;
  int nation_id[MAX_NUM_PLAYER_SLOTS];
  int nation_citizens[MAX_NUM_PLAYER_SLOTS];
  int history;
  int culture;
  int surplus[O_LAST];
  int waste[O_LAST];
  int unhappy_penalty[O_LAST];
  int prod[O_LAST];
  int citizen_base[O_LAST];
  int usage[O_LAST];
  int food_stock;
  int shield_stock;
  int trade[MAX_TRADE_ROUTES];
  int trade_value[MAX_TRADE_ROUTES];
  int pollution;
  int illness_trade;
  int production_kind;
  int production_value;
  int turn_founded;
  int turn_last_built;
  int changed_from_kind;
  int changed_from_value;
  int before_change_shields;
  int disbanded_shields;
  int caravan_shields;
  int last_turns_shield_surplus;
  int airlift;
  bool did_buy;
  bool did_sell;
  bool was_happy;
  bool diplomat_investigate;
  int walls;
  int city_image;
  struct worklist worklist;
  bv_imprs improvements;
  bv_city_options city_options;
  char name[MAX_LEN_NAME];
};

struct packet_city_short_info {
  int id;
  int tile;
  int owner;
  int size;
  int style;
  bool occupied;
  int walls;
  bool happy;
  bool unhappy;
  int city_image;
  bv_imprs improvements;
  char name[MAX_LEN_NAME];
};

struct packet_city_sell {
  int city_id;
  int build_id;
};

struct packet_city_buy {
  int city_id;
};

struct packet_city_change {
  int city_id;
  int production_kind;
  int production_value;
};

struct packet_city_worklist {
  int city_id;
  struct worklist worklist;
};

struct packet_city_make_specialist {
  int city_id;
  int worker_x;
  int worker_y;
};

struct packet_city_make_worker {
  int city_id;
  int worker_x;
  int worker_y;
};

struct packet_city_change_specialist {
  int city_id;
  Specialist_type_id from;
  Specialist_type_id to;
};

struct packet_city_rename {
  int city_id;
  char name[MAX_LEN_NAME];
};

struct packet_city_options_req {
  int city_id;
  bv_city_options options;
};

struct packet_city_refresh {
  int city_id;
};

struct packet_city_name_suggestion_req {
  int unit_id;
};

struct packet_city_name_suggestion_info {
  int unit_id;
  char name[MAX_LEN_NAME];
};

struct packet_city_sabotage_list {
  int diplomat_id;
  int city_id;
  bv_imprs improvements;
};

struct packet_worker_task {
  int city_id;
  int tile_id;
  enum unit_activity activity;
  int tgt;
  int want;
};

struct packet_player_remove {
  int playerno;
};

struct packet_player_info {
  int playerno;
  char name[MAX_LEN_NAME];
  char username[MAX_LEN_NAME];
  bool unassigned_user;
  int score;
  bool is_male;
  bool was_created;
  Government_type_id government;
  Government_type_id target_government;
  bool real_embassy[MAX_NUM_PLAYER_SLOTS];
  enum mood_type mood;
  int style;
  int music_style;
  Nation_type_id nation;
  int team;
  bool is_ready;
  bool phase_done;
  int nturns_idle;
  int turns_alive;
  bool is_alive;
  int gold;
  int tax;
  int science;
  int luxury;
  int tech_upkeep;
  int science_cost;
  bool is_connected;
  int revolution_finishes;
  bool ai;
  int ai_skill_level;
  enum barbarian_type barbarian_type;
  bv_player gives_shared_vision;
  int history;
  int love[MAX_NUM_PLAYER_SLOTS];
  bool color_valid;
  bool color_changeable;
  int color_red;
  int color_green;
  int color_blue;
  int wonders[B_LAST];
  int multip_count;
  int multiplier[MAX_NUM_MULTIPLIERS];
  int multiplier_target[MAX_NUM_MULTIPLIERS];
};

struct packet_player_phase_done {
  int turn;
};

struct packet_player_rates {
  int tax;
  int luxury;
  int science;
};

struct packet_player_change_government {
  Government_type_id government;
};

struct packet_player_attribute_block {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_player_attribute_chunk {
  int offset;
  int total_length;
  int chunk_length;
  unsigned char data[ATTRIBUTE_CHUNK_SIZE];
};

struct packet_player_diplstate {
  int diplstate_id;
  int plr1;
  int plr2;
  enum diplstate_type type;
  int turns_left;
  int has_reason_to_cancel;
  int contact_turns_left;
};

struct packet_player_multiplier {
  int count;
  int multipliers[MAX_NUM_MULTIPLIERS];
};

struct packet_research_info {
  int id;
  int techs_researched;
  int future_tech;
  int researching;
  int researching_cost;
  int bulbs_researched;
  int tech_goal;
  int total_bulbs_prod;
  char inventions[A_LAST + 1];
};

struct packet_player_research {
  int tech;
};

struct packet_player_tech_goal {
  int tech;
};

struct packet_unit_remove {
  int unit_id;
};

struct packet_unit_info {
  int id;
  int owner;
  int nationality;
  int tile;
  enum direction8 facing;
  int homecity;
  int upkeep[O_LAST];
  int veteran;
  bool ai;
  bool paradropped;
  bool occupied;
  bool transported;
  bool done_moving;
  Unit_type_id type;
  int transported_by;
  int movesleft;
  int hp;
  int fuel;
  int activity_count;
  int changed_from_count;
  int goto_tile;
  enum unit_activity activity;
  int activity_tgt;
  enum unit_activity changed_from;
  int changed_from_tgt;
  int battlegroup;
  bool has_orders;
  int orders_length;
  int orders_index;
  bool orders_repeat;
  bool orders_vigilant;
  enum unit_orders orders[MAX_LEN_ROUTE];
  enum direction8 orders_dirs[MAX_LEN_ROUTE];
  enum unit_activity orders_activities[MAX_LEN_ROUTE];
  int orders_targets[MAX_LEN_ROUTE];
  enum action_decision action_decision_want;
  int action_decision_tile;
};

struct packet_unit_short_info {
  int id;
  int owner;
  int tile;
  enum direction8 facing;
  Unit_type_id type;
  int veteran;
  bool occupied;
  bool transported;
  int hp;
  int activity;
  int activity_tgt;
  int transported_by;
  int packet_use;
  int info_city_id;
};

struct packet_unit_combat_info {
  int attacker_unit_id;
  int defender_unit_id;
  int attacker_hp;
  int defender_hp;
  bool make_winner_veteran;
};

struct packet_unit_build_city {
  int unit_id;
  char name[MAX_LEN_NAME];
};

struct packet_unit_disband {
  int unit_id;
};

struct packet_unit_change_homecity {
  int unit_id;
  int city_id;
};

struct packet_unit_battlegroup {
  int unit_id;
  int battlegroup;
};

struct packet_unit_orders {
  int unit_id;
  int src_tile;
  int length;
  bool repeat;
  bool vigilant;
  enum unit_orders orders[MAX_LEN_ROUTE];
  enum direction8 dir[MAX_LEN_ROUTE];
  enum unit_activity activity[MAX_LEN_ROUTE];
  int target[MAX_LEN_ROUTE];
  int dest_tile;
};

struct packet_unit_autosettlers {
  int unit_id;
};

struct packet_unit_load {
  int cargo_id;
  int transporter_id;
  int transporter_tile;
};

struct packet_unit_unload {
  int cargo_id;
  int transporter_id;
};

struct packet_unit_upgrade {
  int unit_id;
};

struct packet_unit_nuke {
  int unit_id;
};

struct packet_unit_paradrop_to {
  int unit_id;
  int tile;
};

struct packet_unit_airlift {
  int unit_id;
  int city_id;
};

struct packet_unit_action_query {
  int diplomat_id;
  int target_id;
  enum gen_action action_type;
};

struct packet_unit_type_upgrade {
  Unit_type_id type;
};

struct packet_unit_do_action {
  int actor_id;
  int target_id;
  int value;
  enum gen_action action_type;
};

struct packet_unit_action_answer {
  int diplomat_id;
  int target_id;
  int cost;
  enum gen_action action_type;
};

struct packet_unit_get_actions {
  int actor_unit_id;
  int target_unit_id;
  int target_city_id;
  int target_tile_id;
  bool disturb_player;
};

struct packet_unit_actions {
  int actor_unit_id;
  int target_unit_id;
  int target_city_id;
  int target_tile_id;
  bool disturb_player;
  struct act_prob action_probabilities[ACTION_COUNT];
};

struct packet_unit_change_activity {
  int unit_id;
  enum unit_activity activity;
  int target;
};

struct packet_diplomacy_init_meeting_req {
  int counterpart;
};

struct packet_diplomacy_init_meeting {
  int counterpart;
  int initiated_from;
};

struct packet_diplomacy_cancel_meeting_req {
  int counterpart;
};

struct packet_diplomacy_cancel_meeting {
  int counterpart;
  int initiated_from;
};

struct packet_diplomacy_create_clause_req {
  int counterpart;
  int giver;
  enum clause_type type;
  int value;
};

struct packet_diplomacy_create_clause {
  int counterpart;
  int giver;
  enum clause_type type;
  int value;
};

struct packet_diplomacy_remove_clause_req {
  int counterpart;
  int giver;
  enum clause_type type;
  int value;
};

struct packet_diplomacy_remove_clause {
  int counterpart;
  int giver;
  enum clause_type type;
  int value;
};

struct packet_diplomacy_accept_treaty_req {
  int counterpart;
};

struct packet_diplomacy_accept_treaty {
  int counterpart;
  bool I_accepted;
  bool other_accepted;
};

struct packet_diplomacy_cancel_pact {
  int other_player_id;
  enum clause_type clause;
};

struct packet_page_msg {
  char caption[MAX_LEN_MSG];
  char headline[MAX_LEN_MSG];
  enum event_type event;
  int len;
  int parts;
};

struct packet_page_msg_part {
  char lines[MAX_LEN_CONTENT];
};

struct packet_report_req {
  enum report_type type;
};

struct packet_conn_info {
  int id;
  bool used;
  bool established;
  bool observer;
  int player_num;
  enum cmdlevel access_level;
  char username[MAX_LEN_NAME];
  char addr[MAX_LEN_ADDR];
  char capability[MAX_LEN_CAPSTR];
};

struct packet_conn_ping_info {
  int connections;
  int conn_id[MAX_NUM_CONNECTIONS];
  float ping_time[MAX_NUM_CONNECTIONS];
};

struct packet_conn_ping {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_conn_pong {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_client_heartbeat {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_client_info {
  enum gui_type gui;
  char distribution[MAX_LEN_NAME];
};

struct packet_end_phase {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_start_phase {
  int phase;
};

struct packet_new_year {
  int year;
  int fragments;
  int turn;
};

struct packet_begin_turn {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_end_turn {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_freeze_client {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_thaw_client {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_spaceship_launch {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_spaceship_place {
  enum spaceship_place_type type;
  int num;
};

struct packet_spaceship_info {
  int player_num;
  int sship_state;
  int structurals;
  int components;
  int modules;
  int fuel;
  int propulsion;
  int habitation;
  int life_support;
  int solar_panels;
  int launch_year;
  int population;
  int mass;
  bv_spaceship_structure structure;
  float support_rate;
  float energy_rate;
  float success_rate;
  float travel_time;
};

struct packet_ruleset_unit {
  Unit_type_id id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char sound_move[MAX_LEN_NAME];
  char sound_move_alt[MAX_LEN_NAME];
  char sound_fight[MAX_LEN_NAME];
  char sound_fight_alt[MAX_LEN_NAME];
  int unit_class_id;
  int build_cost;
  int pop_cost;
  int attack_strength;
  int defense_strength;
  int move_rate;
  int tech_requirement;
  int impr_requirement;
  Government_type_id gov_requirement;
  int vision_radius_sq;
  int transport_capacity;
  int hp;
  int firepower;
  int obsoleted_by;
  int converted_to;
  int convert_time;
  int fuel;
  int happy_cost;
  int upkeep[O_LAST];
  int paratroopers_range;
  int paratroopers_mr_req;
  int paratroopers_mr_sub;
  int veteran_levels;
  char veteran_name[MAX_VET_LEVELS][MAX_LEN_NAME];
  int power_fact[MAX_VET_LEVELS];
  int move_bonus[MAX_VET_LEVELS];
  int bombard_rate;
  int city_size;
  bv_unit_classes cargo;
  bv_unit_classes targets;
  bv_unit_classes embarks;
  bv_unit_classes disembarks;
  char helptext[MAX_LEN_PACKET];
  bv_unit_type_flags flags;
  bv_unit_type_roles roles;
};

struct packet_ruleset_unit_bonus {
  Unit_type_id unit;
  enum unit_type_flag_id flag;
  enum combat_bonus_type type;
  int value;
  bool quiet;
};

struct packet_ruleset_unit_flag {
  int id;
  char name[MAX_LEN_NAME];
  char helptxt[MAX_LEN_PACKET];
};

struct packet_ruleset_game {
  int default_specialist;
  int global_init_techs[MAX_NUM_TECH_LIST];
  int global_init_buildings[MAX_NUM_BUILDING_LIST];
  int veteran_levels;
  char veteran_name[MAX_VET_LEVELS][MAX_LEN_NAME];
  int power_fact[MAX_VET_LEVELS];
  int move_bonus[MAX_VET_LEVELS];
  int background_red;
  int background_green;
  int background_blue;
};

struct packet_ruleset_specialist {
  Specialist_type_id id;
  char plural_name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char short_name[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_government_ruler_title {
  Government_type_id gov;
  Nation_type_id nation;
  char male_title[MAX_LEN_NAME];
  char female_title[MAX_LEN_NAME];
};

struct packet_ruleset_tech {
  int id;
  int req[2];
  int root_req;
  bv_tech_flags flags;
  float cost;
  int num_reqs;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char helptext[MAX_LEN_PACKET];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
};

struct packet_ruleset_tech_flag {
  int id;
  char name[MAX_LEN_NAME];
  char helptxt[MAX_LEN_PACKET];
};

struct packet_ruleset_government {
  Government_type_id id;
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_terrain_control {
  int ocean_reclaim_requirement_pct;
  int land_channel_requirement_pct;
  int terrain_thaw_requirement_pct;
  int terrain_freeze_requirement_pct;
  int lake_max_size;
  int min_start_native_area;
  int move_fragments;
  int igter_cost;
  bool pythagorean_diagonal;
  char gui_type_base0[MAX_LEN_NAME];
  char gui_type_base1[MAX_LEN_NAME];
};

struct packet_rulesets_ready {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_ruleset_nation_sets {
  int nsets;
  char names[MAX_NUM_NATION_SETS][MAX_LEN_NAME];
  char rule_names[MAX_NUM_NATION_SETS][MAX_LEN_NAME];
  char descriptions[MAX_NUM_NATION_SETS][MAX_LEN_MSG];
};

struct packet_ruleset_nation_groups {
  int ngroups;
  char groups[MAX_NUM_NATION_GROUPS][MAX_LEN_NAME];
  bool hidden[MAX_NUM_NATION_GROUPS];
};

struct packet_ruleset_nation {
  Nation_type_id id;
  char translation_domain[MAX_LEN_NAME];
  char adjective[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char noun_plural[MAX_LEN_NAME];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char legend[MAX_LEN_MSG];
  int style;
  int leader_count;
  char leader_name[MAX_NUM_LEADERS][MAX_LEN_NAME];
  bool leader_is_male[MAX_NUM_LEADERS];
  bool is_playable;
  enum barbarian_type barbarian_type;
  int nsets;
  int sets[MAX_NUM_NATION_SETS];
  int ngroups;
  int groups[MAX_NUM_NATION_GROUPS];
  Government_type_id init_government_id;
  int init_techs[MAX_NUM_TECH_LIST];
  int init_units[MAX_NUM_UNIT_LIST];
  int init_buildings[MAX_NUM_BUILDING_LIST];
};

struct packet_nation_availability {
  int ncount;
  bool is_pickable[MAX_NUM_NATIONS];
  bool nationset_change;
};

struct packet_ruleset_style {
  int id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
};

struct packet_ruleset_city {
  int style_id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char citizens_graphic[MAX_LEN_NAME];
  char citizens_graphic_alt[MAX_LEN_NAME];
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
  char graphic[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
};

struct packet_ruleset_building {
  Impr_type_id id;
  enum impr_genus_id genus;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
  int obs_count;
  struct requirement obs_reqs[MAX_NUM_REQS];
  int build_cost;
  int upkeep;
  int sabotage;
  bv_impr_flags flags;
  char soundtag[MAX_LEN_NAME];
  char soundtag_alt[MAX_LEN_NAME];
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_terrain {
  Terrain_type_id id;
  int tclass;
  bv_terrain_flags flags;
  bv_unit_classes native_to;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  int movement_cost;
  int defense_bonus;
  int output[O_LAST];
  int num_resources;
  Resource_type_id resources[MAX_NUM_RESOURCES];
  int road_output_incr_pct[O_LAST];
  int base_time;
  int road_time;
  Terrain_type_id irrigation_result;
  int irrigation_food_incr;
  int irrigation_time;
  Terrain_type_id mining_result;
  int mining_shield_incr;
  int mining_time;
  int animal;
  Terrain_type_id transform_result;
  int transform_time;
  int clean_pollution_time;
  int clean_fallout_time;
  int pillage_time;
  int color_red;
  int color_green;
  int color_blue;
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_terrain_flag {
  int id;
  char name[MAX_LEN_NAME];
  char helptxt[MAX_LEN_PACKET];
};

struct packet_ruleset_unit_class {
  int id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  int min_speed;
  int hp_loss_pct;
  int hut_behavior;
  int non_native_def_pct;
  bv_unit_class_flags flags;
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_extra {
  int id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  int category;
  int causes;
  int rmcauses;
  char activity_gfx[MAX_LEN_NAME];
  char act_gfx_alt[MAX_LEN_NAME];
  char act_gfx_alt2[MAX_LEN_NAME];
  char rmact_gfx[MAX_LEN_NAME];
  char rmact_gfx_alt[MAX_LEN_NAME];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
  int rmreqs_count;
  struct requirement rmreqs[MAX_NUM_REQS];
  bool buildable;
  int build_time;
  int build_time_factor;
  int removal_time;
  int removal_time_factor;
  int defense_bonus;
  bv_unit_classes native_to;
  bv_extra_flags flags;
  bv_extras hidden_by;
  bv_extras conflicts;
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_base {
  int id;
  enum base_gui_type gui_type;
  int border_sq;
  int vision_main_sq;
  int vision_invis_sq;
  bv_base_flags flags;
};

struct packet_ruleset_road {
  int id;
  int first_reqs_count;
  struct requirement first_reqs[MAX_NUM_REQS];
  int move_cost;
  enum road_move_mode move_mode;
  int tile_incr_const[O_LAST];
  int tile_incr[O_LAST];
  int tile_bonus[O_LAST];
  enum road_compat compat;
  bv_roads integrates;
  bv_road_flags flags;
};

struct packet_ruleset_disaster {
  int id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
  int frequency;
  bv_disaster_effects effects;
};

struct packet_ruleset_achievement {
  int id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  enum achievement_type type;
  bool unique;
  int value;
};

struct packet_ruleset_trade {
  int id;
  int trade_pct;
  enum traderoute_illegal_cancelling cancelling;
  enum traderoute_bonus_type bonus_type;
};

struct packet_ruleset_action {
  enum gen_action id;
  char ui_name[MAX_LEN_NAME];
  bool quiet;
};

struct packet_ruleset_action_enabler {
  enum gen_action enabled_action;
  int actor_reqs_count;
  struct requirement actor_reqs[MAX_NUM_REQS];
  int target_reqs_count;
  struct requirement target_reqs[MAX_NUM_REQS];
};

struct packet_ruleset_music {
  int id;
  char music_peaceful[MAX_LEN_NAME];
  char music_combat[MAX_LEN_NAME];
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
};

struct packet_ruleset_multiplier {
  Multiplier_type_id id;
  int start;
  int stop;
  int step;
  int def;
  int offset;
  int factor;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  char helptext[MAX_LEN_PACKET];
};

struct packet_ruleset_control {
  int num_unit_classes;
  int num_unit_types;
  int num_impr_types;
  int num_tech_types;
  int num_extra_types;
  int num_base_types;
  int num_road_types;
  int num_disaster_types;
  int num_achievement_types;
  int num_multipliers;
  int num_styles;
  int num_music_styles;
  int government_count;
  int nation_count;
  int styles_count;
  int terrain_count;
  int resource_count;
  int num_specialist_types;
  char preferred_tileset[MAX_LEN_NAME];
  char preferred_soundset[MAX_LEN_NAME];
  char preferred_musicset[MAX_LEN_NAME];
  bool popup_tech_help;
  char name[MAX_LEN_NAME];
  char version[MAX_LEN_NAME];
  int desc_length;
};

struct packet_ruleset_summary {
  char text[MAX_LEN_CONTENT];
};

struct packet_ruleset_description_part {
  char text[MAX_LEN_CONTENT];
};

struct packet_single_want_hack_req {
  char token[MAX_LEN_NAME];
};

struct packet_single_want_hack_reply {
  bool you_have_hack;
};

struct packet_ruleset_choices {
  int ruleset_count;
  char rulesets[MAX_NUM_RULESETS][MAX_RULESET_NAME_LENGTH];
};

struct packet_game_load {
  bool load_successful;
  char load_filename[MAX_LEN_PACKET];
};

struct packet_server_setting_control {
  int settings_num;
  int categories_num;
  char category_names[256][MAX_LEN_NAME];
};

struct packet_server_setting_const {
  int id;
  char name[MAX_LEN_NAME];
  char short_help[MAX_LEN_PACKET];
  char extra_help[MAX_LEN_PACKET];
  int category;
};

struct packet_server_setting_bool {
  int id;
  bool is_visible;
  bool is_changeable;
  bool initial_setting;
  bool val;
  bool default_val;
};

struct packet_server_setting_int {
  int id;
  bool is_visible;
  bool is_changeable;
  bool initial_setting;
  int val;
  int default_val;
  int min_val;
  int max_val;
};

struct packet_server_setting_str {
  int id;
  bool is_visible;
  bool is_changeable;
  bool initial_setting;
  char val[MAX_LEN_PACKET];
  char default_val[MAX_LEN_PACKET];
};

struct packet_server_setting_enum {
  int id;
  bool is_visible;
  bool is_changeable;
  bool initial_setting;
  int val;
  int default_val;
  int values_num;
  char support_names[64][MAX_LEN_NAME];
  char pretty_names[64][MAX_LEN_ENUM];
};

struct packet_server_setting_bitwise {
  int id;
  bool is_visible;
  bool is_changeable;
  bool initial_setting;
  int val;
  int default_val;
  int bits_num;
  char support_names[64][MAX_LEN_NAME];
  char pretty_names[64][MAX_LEN_ENUM];
};

struct packet_set_topology {
  int topology_id;
};

struct packet_ruleset_effect {
  enum effect_type effect_type;
  int effect_value;
  bool has_multiplier;
  Multiplier_type_id multiplier;
  int reqs_count;
  struct requirement reqs[MAX_NUM_REQS];
};

struct packet_ruleset_resource {
  Resource_type_id id;
  char name[MAX_LEN_NAME];
  char rule_name[MAX_LEN_NAME];
  int output[O_LAST];
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
};

struct packet_scenario_info {
  bool is_scenario;
  char name[256];
  char authors[MAX_LEN_PACKET / 3];
  bool players;
  bool startpos_nations;
  bool save_random;
  bool prevent_new_cities;
  bool lake_flooding;
  bool handmade;
  bool allow_ai_type_fallback;
  bool have_resources;
};

struct packet_scenario_description {
  char description[MAX_LEN_CONTENT];
};

struct packet_save_scenario {
  char name[MAX_LEN_NAME];
};

struct packet_vote_new {
  int vote_no;
  char user[MAX_LEN_NAME];
  char desc[512];
  int percent_required;
  int flags;
};

struct packet_vote_update {
  int vote_no;
  int yes;
  int no;
  int abstain;
  int num_voters;
};

struct packet_vote_remove {
  int vote_no;
};

struct packet_vote_resolve {
  int vote_no;
  bool passed;
};

struct packet_vote_submit {
  int vote_no;
  int value;
};

struct packet_edit_mode {
  bool state;
};

struct packet_edit_recalculate_borders {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_edit_check_tiles {
  char __dummy;			/* to avoid malloc(0); */
};

struct packet_edit_toggle_fogofwar {
  int player;
};

struct packet_edit_tile_terrain {
  int tile;
  Terrain_type_id terrain;
  int size;
};

struct packet_edit_tile_resource {
  int tile;
  Resource_type_id resource;
  int size;
};

struct packet_edit_tile_extra {
  int tile;
  int extra_type_id;
  bool removal;
  int size;
};

struct packet_edit_startpos {
  int id;
  bool removal;
  int tag;
};

struct packet_edit_startpos_full {
  int id;
  bool exclude;
  bv_startpos_nations nations;
};

struct packet_edit_tile {
  int tile;
  bv_extras extras;
  Resource_type_id resource;
  Terrain_type_id terrain;
  Nation_type_id startpos_nation;
  char label[MAX_LEN_NAME];
};

struct packet_edit_unit_create {
  int owner;
  int tile;
  Unit_type_id type;
  int count;
  int tag;
};

struct packet_edit_unit_remove {
  int owner;
  int tile;
  Unit_type_id type;
  int count;
};

struct packet_edit_unit_remove_by_id {
  int id;
};

struct packet_edit_unit {
  int id;
  Unit_type_id utype;
  int owner;
  int homecity;
  int moves_left;
  int hp;
  int veteran;
  int fuel;
  enum unit_activity activity;
  int activity_count;
  Base_type_id activity_base;
  bool debug;
  bool moved;
  bool paradropped;
  bool done_moving;
  int transported_by;
};

struct packet_edit_city_create {
  int owner;
  int tile;
  int size;
  int tag;
};

struct packet_edit_city_remove {
  int id;
};

struct packet_edit_city {
  int id;
  char name[MAX_LEN_NAME];
  int owner;
  int original;
  int size;
  int history;
  int ppl_happy[5];
  int ppl_content[5];
  int ppl_unhappy[5];
  int ppl_angry[5];
  int specialists_size;
  int specialists[SP_MAX];
  int trade[MAX_TRADE_ROUTES];
  int food_stock;
  int shield_stock;
  bool airlift;
  bool debug;
  bool did_buy;
  bool did_sell;
  bool was_happy;
  int anarchy;
  int rapture;
  int steal;
  int turn_founded;
  int turn_last_built;
  int built[B_LAST];
  int production_kind;
  int production_value;
  int last_turns_shield_surplus;
  bv_city_options city_options;
};

struct packet_edit_player_create {
  int tag;
};

struct packet_edit_player_remove {
  int id;
};

struct packet_edit_player {
  int id;
  char name[MAX_LEN_NAME];
  char username[MAX_LEN_NAME];
  char ranked_username[MAX_LEN_NAME];
  int user_turns;
  bool is_male;
  Government_type_id government;
  Government_type_id target_government;
  Nation_type_id nation;
  int team;
  bool phase_done;
  int nturns_idle;
  bool is_alive;
  int revolution_finishes;
  bool capital;
  bv_player embassy;
  int gold;
  int tax;
  int science;
  int luxury;
  int future_tech;
  int researching;
  int bulbs_researched;
  bool inventions[A_LAST+1];
  bool ai;
};

struct packet_edit_player_vision {
  int player;
  int tile;
  bool known;
  int size;
};

struct packet_edit_game {
  int year;
  bool scenario;
  char scenario_name[256];
  char scenario_authors[MAX_LEN_PACKET / 3];
  bool scenario_random;
  bool scenario_players;
  bool startpos_nations;
  bool prevent_new_cities;
  bool lake_flooding;
};

struct packet_edit_scenario_desc {
  char scenario_desc[MAX_LEN_CONTENT];
};

struct packet_edit_object_created {
  int tag;
  int id;
};

struct packet_play_music {
  char tag[MAX_LEN_NAME];
};

enum packet_type {
  PACKET_PROCESSING_STARTED,             /* 0 */
  PACKET_PROCESSING_FINISHED,
  PACKET_SERVER_JOIN_REQ = 4,
  PACKET_SERVER_JOIN_REPLY,
  PACKET_AUTHENTICATION_REQ,
  PACKET_AUTHENTICATION_REPLY,
  PACKET_SERVER_SHUTDOWN,
  PACKET_NATION_SELECT_REQ = 10,         /* 10 */
  PACKET_PLAYER_READY,
  PACKET_ENDGAME_REPORT,
  PACKET_TILE_INFO = 15,
  PACKET_GAME_INFO,
  PACKET_MAP_INFO,
  PACKET_NUKE_TILE_INFO,
  PACKET_TEAM_NAME_INFO,
  PACKET_CHAT_MSG = 25,
  PACKET_CHAT_MSG_REQ,
  PACKET_CONNECT_MSG,
  PACKET_EARLY_CHAT_MSG,
  PACKET_CITY_REMOVE = 30,               /* 30 */
  PACKET_CITY_INFO,
  PACKET_CITY_SHORT_INFO,
  PACKET_CITY_SELL,
  PACKET_CITY_BUY,
  PACKET_CITY_CHANGE,
  PACKET_CITY_WORKLIST,
  PACKET_CITY_MAKE_SPECIALIST,
  PACKET_CITY_MAKE_WORKER,
  PACKET_CITY_CHANGE_SPECIALIST,
  PACKET_CITY_RENAME,                    /* 40 */
  PACKET_CITY_OPTIONS_REQ,
  PACKET_CITY_REFRESH,
  PACKET_CITY_NAME_SUGGESTION_REQ,
  PACKET_CITY_NAME_SUGGESTION_INFO,
  PACKET_CITY_SABOTAGE_LIST,
  PACKET_PLAYER_REMOVE = 50,             /* 50 */
  PACKET_PLAYER_INFO,
  PACKET_PLAYER_PHASE_DONE,
  PACKET_PLAYER_RATES,
  PACKET_PLAYER_CHANGE_GOVERNMENT,
  PACKET_PLAYER_RESEARCH,
  PACKET_PLAYER_TECH_GOAL,
  PACKET_PLAYER_ATTRIBUTE_BLOCK,
  PACKET_PLAYER_ATTRIBUTE_CHUNK,
  PACKET_PLAYER_DIPLSTATE,
  PACKET_RESEARCH_INFO,                  /* 60 */
  PACKET_UNIT_REMOVE = 62,
  PACKET_UNIT_INFO,
  PACKET_UNIT_SHORT_INFO,
  PACKET_UNIT_COMBAT_INFO,
  PACKET_UNIT_BUILD_CITY = 67,
  PACKET_UNIT_DISBAND,
  PACKET_UNIT_CHANGE_HOMECITY,
  PACKET_UNIT_BATTLEGROUP = 71,
  PACKET_UNIT_ORDERS = 73,
  PACKET_UNIT_AUTOSETTLERS,
  PACKET_UNIT_LOAD,
  PACKET_UNIT_UNLOAD,
  PACKET_UNIT_UPGRADE,
  PACKET_UNIT_NUKE = 79,
  PACKET_UNIT_PARADROP_TO,               /* 80 */
  PACKET_UNIT_AIRLIFT,
  PACKET_UNIT_ACTION_QUERY,
  PACKET_UNIT_TYPE_UPGRADE,
  PACKET_UNIT_DO_ACTION,
  PACKET_UNIT_ACTION_ANSWER,
  PACKET_UNIT_GET_ACTIONS = 87,
  PACKET_CONN_PING,
  PACKET_CONN_PONG,
  PACKET_UNIT_ACTIONS,                   /* 90 */
  PACKET_DIPLOMACY_INIT_MEETING_REQ = 95,
  PACKET_DIPLOMACY_INIT_MEETING,
  PACKET_DIPLOMACY_CANCEL_MEETING_REQ,
  PACKET_DIPLOMACY_CANCEL_MEETING,
  PACKET_DIPLOMACY_CREATE_CLAUSE_REQ,
  PACKET_DIPLOMACY_CREATE_CLAUSE,        /* 100 */
  PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ,
  PACKET_DIPLOMACY_REMOVE_CLAUSE,
  PACKET_DIPLOMACY_ACCEPT_TREATY_REQ,
  PACKET_DIPLOMACY_ACCEPT_TREATY,
  PACKET_DIPLOMACY_CANCEL_PACT,
  PACKET_PAGE_MSG = 110,                 /* 110 */
  PACKET_REPORT_REQ,
  PACKET_CONN_INFO = 115,
  PACKET_CONN_PING_INFO,
  PACKET_CLIENT_INFO = 119,
  PACKET_END_PHASE = 125,
  PACKET_START_PHASE,
  PACKET_NEW_YEAR,
  PACKET_BEGIN_TURN,
  PACKET_END_TURN,
  PACKET_FREEZE_CLIENT,                  /* 130 */
  PACKET_THAW_CLIENT,
  PACKET_SPACESHIP_LAUNCH = 135,
  PACKET_SPACESHIP_PLACE,
  PACKET_SPACESHIP_INFO,
  PACKET_RULESET_UNIT = 140,             /* 140 */
  PACKET_RULESET_GAME,
  PACKET_RULESET_SPECIALIST,
  PACKET_RULESET_GOVERNMENT_RULER_TITLE,
  PACKET_RULESET_TECH,
  PACKET_RULESET_GOVERNMENT,
  PACKET_RULESET_TERRAIN_CONTROL,
  PACKET_RULESET_NATION_GROUPS,
  PACKET_RULESET_NATION,
  PACKET_RULESET_CITY,
  PACKET_RULESET_BUILDING,               /* 150 */
  PACKET_RULESET_TERRAIN,
  PACKET_RULESET_UNIT_CLASS,
  PACKET_RULESET_BASE,
  PACKET_RULESET_CONTROL = 155,
  PACKET_SINGLE_WANT_HACK_REQ = 160,     /* 160 */
  PACKET_SINGLE_WANT_HACK_REPLY,
  PACKET_RULESET_CHOICES,
  PACKET_GAME_LOAD,
  PACKET_SERVER_SETTING_CONTROL,
  PACKET_SERVER_SETTING_CONST,
  PACKET_SERVER_SETTING_BOOL,
  PACKET_SERVER_SETTING_INT,
  PACKET_SERVER_SETTING_STR,
  PACKET_SERVER_SETTING_ENUM,
  PACKET_SERVER_SETTING_BITWISE,         /* 170 */
  PACKET_RULESET_EFFECT = 175,
  PACKET_RULESET_RESOURCE = 177,
  PACKET_SCENARIO_INFO = 180,            /* 180 */
  PACKET_SAVE_SCENARIO,
  PACKET_VOTE_NEW = 185,
  PACKET_VOTE_UPDATE,
  PACKET_VOTE_REMOVE,
  PACKET_VOTE_RESOLVE,
  PACKET_VOTE_SUBMIT,
  PACKET_EDIT_MODE,                      /* 190 */
  PACKET_EDIT_RECALCULATE_BORDERS = 197,
  PACKET_EDIT_CHECK_TILES,
  PACKET_EDIT_TOGGLE_FOGOFWAR,
  PACKET_EDIT_TILE_TERRAIN,              /* 200 */
  PACKET_EDIT_TILE_RESOURCE,
  PACKET_EDIT_TILE_EXTRA,
  PACKET_EDIT_STARTPOS = 204,
  PACKET_EDIT_STARTPOS_FULL,
  PACKET_EDIT_TILE,
  PACKET_EDIT_UNIT_CREATE,
  PACKET_EDIT_UNIT_REMOVE,
  PACKET_EDIT_UNIT_REMOVE_BY_ID,
  PACKET_EDIT_UNIT,                      /* 210 */
  PACKET_EDIT_CITY_CREATE,
  PACKET_EDIT_CITY_REMOVE,
  PACKET_EDIT_CITY,
  PACKET_EDIT_PLAYER_CREATE,
  PACKET_EDIT_PLAYER_REMOVE,
  PACKET_EDIT_PLAYER,
  PACKET_EDIT_PLAYER_VISION,
  PACKET_EDIT_GAME,
  PACKET_EDIT_OBJECT_CREATED,
  PACKET_RULESET_ROAD,                   /* 220 */
  PACKET_UNIT_CHANGE_ACTIVITY = 222,
  PACKET_ENDGAME_PLAYER,
  PACKET_RULESET_DISASTER,
  PACKET_RULESETS_READY,
  PACKET_RULESET_TRADE = 227,
  PACKET_RULESET_UNIT_BONUS,
  PACKET_RULESET_UNIT_FLAG,
  PACKET_RULESET_TERRAIN_FLAG = 231,
  PACKET_RULESET_EXTRA,
  PACKET_RULESET_ACHIEVEMENT,
  PACKET_RULESET_TECH_FLAG,
  PACKET_RULESET_ACTION_ENABLER,
  PACKET_RULESET_NATION_SETS,
  PACKET_NATION_AVAILABILITY,
  PACKET_ACHIEVEMENT_INFO,
  PACKET_RULESET_STYLE,
  PACKET_RULESET_MUSIC,                  /* 240 */
  PACKET_WORKER_TASK,
  PACKET_PLAYER_MULTIPLIER,
  PACKET_RULESET_MULTIPLIER,
  PACKET_TIMEOUT_INFO,
  PACKET_PLAY_MUSIC,
  PACKET_RULESET_ACTION,
  PACKET_RULESET_DESCRIPTION_PART,
  PACKET_PAGE_MSG_PART,
  PACKET_RULESET_SUMMARY,
  PACKET_SET_TOPOLOGY,                   /* 250 */
  PACKET_CLIENT_HEARTBEAT,
  PACKET_SCENARIO_DESCRIPTION,
  PACKET_EDIT_SCENARIO_DESC,

  PACKET_LAST  /* leave this last */
};

int send_packet_processing_started(struct connection *pc);

int send_packet_processing_finished(struct connection *pc);

int send_packet_server_join_req(struct connection *pc, const struct packet_server_join_req *packet);
int dsend_packet_server_join_req(struct connection *pc, const char *username, const char *capability, const char *version_label, int major_version, int minor_version, int patch_version);

int send_packet_server_join_reply(struct connection *pc, const struct packet_server_join_reply *packet);

int send_packet_authentication_req(struct connection *pc, const struct packet_authentication_req *packet);
int dsend_packet_authentication_req(struct connection *pc, enum authentication_type type, const char *message);

int send_packet_authentication_reply(struct connection *pc, const struct packet_authentication_reply *packet);

int send_packet_server_shutdown(struct connection *pc);
void lsend_packet_server_shutdown(struct conn_list *dest);

int send_packet_nation_select_req(struct connection *pc, const struct packet_nation_select_req *packet);
int dsend_packet_nation_select_req(struct connection *pc, int player_no, Nation_type_id nation_no, bool is_male, const char *name, int style);

int send_packet_player_ready(struct connection *pc, const struct packet_player_ready *packet);
int dsend_packet_player_ready(struct connection *pc, int player_no, bool is_ready);

int send_packet_endgame_report(struct connection *pc, const struct packet_endgame_report *packet);
void lsend_packet_endgame_report(struct conn_list *dest, const struct packet_endgame_report *packet);

int send_packet_endgame_player(struct connection *pc, const struct packet_endgame_player *packet);
void lsend_packet_endgame_player(struct conn_list *dest, const struct packet_endgame_player *packet);

int send_packet_tile_info(struct connection *pc, const struct packet_tile_info *packet);
void lsend_packet_tile_info(struct conn_list *dest, const struct packet_tile_info *packet);

int send_packet_game_info(struct connection *pc, const struct packet_game_info *packet);

int send_packet_timeout_info(struct connection *pc, const struct packet_timeout_info *packet);

int send_packet_map_info(struct connection *pc, const struct packet_map_info *packet);
void lsend_packet_map_info(struct conn_list *dest, const struct packet_map_info *packet);

int send_packet_nuke_tile_info(struct connection *pc, const struct packet_nuke_tile_info *packet);
void lsend_packet_nuke_tile_info(struct conn_list *dest, const struct packet_nuke_tile_info *packet);
int dsend_packet_nuke_tile_info(struct connection *pc, int tile);
void dlsend_packet_nuke_tile_info(struct conn_list *dest, int tile);

int send_packet_team_name_info(struct connection *pc, const struct packet_team_name_info *packet);
void lsend_packet_team_name_info(struct conn_list *dest, const struct packet_team_name_info *packet);

int send_packet_achievement_info(struct connection *pc, const struct packet_achievement_info *packet);
void lsend_packet_achievement_info(struct conn_list *dest, const struct packet_achievement_info *packet);

int send_packet_chat_msg(struct connection *pc, const struct packet_chat_msg *packet);
void lsend_packet_chat_msg(struct conn_list *dest, const struct packet_chat_msg *packet);

int send_packet_early_chat_msg(struct connection *pc, const struct packet_early_chat_msg *packet);
void lsend_packet_early_chat_msg(struct conn_list *dest, const struct packet_early_chat_msg *packet);

int send_packet_chat_msg_req(struct connection *pc, const struct packet_chat_msg_req *packet);
int dsend_packet_chat_msg_req(struct connection *pc, const char *message);

int send_packet_connect_msg(struct connection *pc, const struct packet_connect_msg *packet);
int dsend_packet_connect_msg(struct connection *pc, const char *message);

int send_packet_city_remove(struct connection *pc, const struct packet_city_remove *packet);
void lsend_packet_city_remove(struct conn_list *dest, const struct packet_city_remove *packet);
int dsend_packet_city_remove(struct connection *pc, int city_id);
void dlsend_packet_city_remove(struct conn_list *dest, int city_id);

int send_packet_city_info(struct connection *pc, const struct packet_city_info *packet, bool force_to_send);
void lsend_packet_city_info(struct conn_list *dest, const struct packet_city_info *packet, bool force_to_send);

int send_packet_city_short_info(struct connection *pc, const struct packet_city_short_info *packet);
void lsend_packet_city_short_info(struct conn_list *dest, const struct packet_city_short_info *packet);

int send_packet_city_sell(struct connection *pc, const struct packet_city_sell *packet);
int dsend_packet_city_sell(struct connection *pc, int city_id, int build_id);

int send_packet_city_buy(struct connection *pc, const struct packet_city_buy *packet);
int dsend_packet_city_buy(struct connection *pc, int city_id);

int send_packet_city_change(struct connection *pc, const struct packet_city_change *packet);
int dsend_packet_city_change(struct connection *pc, int city_id, int production_kind, int production_value);

int send_packet_city_worklist(struct connection *pc, const struct packet_city_worklist *packet);
int dsend_packet_city_worklist(struct connection *pc, int city_id, const struct worklist *worklist);

int send_packet_city_make_specialist(struct connection *pc, const struct packet_city_make_specialist *packet);
int dsend_packet_city_make_specialist(struct connection *pc, int city_id, int worker_x, int worker_y);

int send_packet_city_make_worker(struct connection *pc, const struct packet_city_make_worker *packet);
int dsend_packet_city_make_worker(struct connection *pc, int city_id, int worker_x, int worker_y);

int send_packet_city_change_specialist(struct connection *pc, const struct packet_city_change_specialist *packet);
int dsend_packet_city_change_specialist(struct connection *pc, int city_id, Specialist_type_id from, Specialist_type_id to);

int send_packet_city_rename(struct connection *pc, const struct packet_city_rename *packet);
int dsend_packet_city_rename(struct connection *pc, int city_id, const char *name);

int send_packet_city_options_req(struct connection *pc, const struct packet_city_options_req *packet);
int dsend_packet_city_options_req(struct connection *pc, int city_id, bv_city_options options);

int send_packet_city_refresh(struct connection *pc, const struct packet_city_refresh *packet);
int dsend_packet_city_refresh(struct connection *pc, int city_id);

int send_packet_city_name_suggestion_req(struct connection *pc, const struct packet_city_name_suggestion_req *packet);
int dsend_packet_city_name_suggestion_req(struct connection *pc, int unit_id);

int send_packet_city_name_suggestion_info(struct connection *pc, const struct packet_city_name_suggestion_info *packet);
void lsend_packet_city_name_suggestion_info(struct conn_list *dest, const struct packet_city_name_suggestion_info *packet);
int dsend_packet_city_name_suggestion_info(struct connection *pc, int unit_id, const char *name);
void dlsend_packet_city_name_suggestion_info(struct conn_list *dest, int unit_id, const char *name);

int send_packet_city_sabotage_list(struct connection *pc, const struct packet_city_sabotage_list *packet);
void lsend_packet_city_sabotage_list(struct conn_list *dest, const struct packet_city_sabotage_list *packet);

int send_packet_worker_task(struct connection *pc, const struct packet_worker_task *packet);
void lsend_packet_worker_task(struct conn_list *dest, const struct packet_worker_task *packet);

int send_packet_player_remove(struct connection *pc, const struct packet_player_remove *packet);
int dsend_packet_player_remove(struct connection *pc, int playerno);

int send_packet_player_info(struct connection *pc, const struct packet_player_info *packet);

int send_packet_player_phase_done(struct connection *pc, const struct packet_player_phase_done *packet);
int dsend_packet_player_phase_done(struct connection *pc, int turn);

int send_packet_player_rates(struct connection *pc, const struct packet_player_rates *packet);
int dsend_packet_player_rates(struct connection *pc, int tax, int luxury, int science);

int send_packet_player_change_government(struct connection *pc, const struct packet_player_change_government *packet);
int dsend_packet_player_change_government(struct connection *pc, Government_type_id government);

int send_packet_player_attribute_block(struct connection *pc);

int send_packet_player_attribute_chunk(struct connection *pc, const struct packet_player_attribute_chunk *packet);

int send_packet_player_diplstate(struct connection *pc, const struct packet_player_diplstate *packet);

int send_packet_player_multiplier(struct connection *pc, const struct packet_player_multiplier *packet);

int send_packet_research_info(struct connection *pc, const struct packet_research_info *packet);
void lsend_packet_research_info(struct conn_list *dest, const struct packet_research_info *packet);

int send_packet_player_research(struct connection *pc, const struct packet_player_research *packet);
int dsend_packet_player_research(struct connection *pc, int tech);

int send_packet_player_tech_goal(struct connection *pc, const struct packet_player_tech_goal *packet);
int dsend_packet_player_tech_goal(struct connection *pc, int tech);

int send_packet_unit_remove(struct connection *pc, const struct packet_unit_remove *packet);
void lsend_packet_unit_remove(struct conn_list *dest, const struct packet_unit_remove *packet);
int dsend_packet_unit_remove(struct connection *pc, int unit_id);
void dlsend_packet_unit_remove(struct conn_list *dest, int unit_id);

int send_packet_unit_info(struct connection *pc, const struct packet_unit_info *packet);
void lsend_packet_unit_info(struct conn_list *dest, const struct packet_unit_info *packet);

int send_packet_unit_short_info(struct connection *pc, const struct packet_unit_short_info *packet, bool force_to_send);
void lsend_packet_unit_short_info(struct conn_list *dest, const struct packet_unit_short_info *packet, bool force_to_send);

int send_packet_unit_combat_info(struct connection *pc, const struct packet_unit_combat_info *packet);
void lsend_packet_unit_combat_info(struct conn_list *dest, const struct packet_unit_combat_info *packet);

int send_packet_unit_build_city(struct connection *pc, const struct packet_unit_build_city *packet);
int dsend_packet_unit_build_city(struct connection *pc, int unit_id, const char *name);

int send_packet_unit_disband(struct connection *pc, const struct packet_unit_disband *packet);
int dsend_packet_unit_disband(struct connection *pc, int unit_id);

int send_packet_unit_change_homecity(struct connection *pc, const struct packet_unit_change_homecity *packet);
int dsend_packet_unit_change_homecity(struct connection *pc, int unit_id, int city_id);

int send_packet_unit_battlegroup(struct connection *pc, const struct packet_unit_battlegroup *packet);
int dsend_packet_unit_battlegroup(struct connection *pc, int unit_id, int battlegroup);

int send_packet_unit_orders(struct connection *pc, const struct packet_unit_orders *packet);

int send_packet_unit_autosettlers(struct connection *pc, const struct packet_unit_autosettlers *packet);
int dsend_packet_unit_autosettlers(struct connection *pc, int unit_id);

int send_packet_unit_load(struct connection *pc, const struct packet_unit_load *packet);
int dsend_packet_unit_load(struct connection *pc, int cargo_id, int transporter_id, int transporter_tile);

int send_packet_unit_unload(struct connection *pc, const struct packet_unit_unload *packet);
int dsend_packet_unit_unload(struct connection *pc, int cargo_id, int transporter_id);

int send_packet_unit_upgrade(struct connection *pc, const struct packet_unit_upgrade *packet);
int dsend_packet_unit_upgrade(struct connection *pc, int unit_id);

int send_packet_unit_nuke(struct connection *pc, const struct packet_unit_nuke *packet);
int dsend_packet_unit_nuke(struct connection *pc, int unit_id);

int send_packet_unit_paradrop_to(struct connection *pc, const struct packet_unit_paradrop_to *packet);
int dsend_packet_unit_paradrop_to(struct connection *pc, int unit_id, int tile);

int send_packet_unit_airlift(struct connection *pc, const struct packet_unit_airlift *packet);
int dsend_packet_unit_airlift(struct connection *pc, int unit_id, int city_id);

int send_packet_unit_action_query(struct connection *pc, const struct packet_unit_action_query *packet);
int dsend_packet_unit_action_query(struct connection *pc, int diplomat_id, int target_id, enum gen_action action_type);

int send_packet_unit_type_upgrade(struct connection *pc, const struct packet_unit_type_upgrade *packet);
int dsend_packet_unit_type_upgrade(struct connection *pc, Unit_type_id type);

int send_packet_unit_do_action(struct connection *pc, const struct packet_unit_do_action *packet);
int dsend_packet_unit_do_action(struct connection *pc, int actor_id, int target_id, int value, enum gen_action action_type);

int send_packet_unit_action_answer(struct connection *pc, const struct packet_unit_action_answer *packet);
int dsend_packet_unit_action_answer(struct connection *pc, int diplomat_id, int target_id, int cost, enum gen_action action_type);

int send_packet_unit_get_actions(struct connection *pc, const struct packet_unit_get_actions *packet);
int dsend_packet_unit_get_actions(struct connection *pc, int actor_unit_id, int target_unit_id, int target_city_id, int target_tile_id, bool disturb_player);

int send_packet_unit_actions(struct connection *pc, const struct packet_unit_actions *packet);
int dsend_packet_unit_actions(struct connection *pc, int actor_unit_id, int target_unit_id, int target_city_id, int target_tile_id, bool disturb_player, const struct act_prob *action_probabilities);

int send_packet_unit_change_activity(struct connection *pc, const struct packet_unit_change_activity *packet);
int dsend_packet_unit_change_activity(struct connection *pc, int unit_id, enum unit_activity activity, int target);

int send_packet_diplomacy_init_meeting_req(struct connection *pc, const struct packet_diplomacy_init_meeting_req *packet);
int dsend_packet_diplomacy_init_meeting_req(struct connection *pc, int counterpart);

int send_packet_diplomacy_init_meeting(struct connection *pc, const struct packet_diplomacy_init_meeting *packet);
void lsend_packet_diplomacy_init_meeting(struct conn_list *dest, const struct packet_diplomacy_init_meeting *packet);
int dsend_packet_diplomacy_init_meeting(struct connection *pc, int counterpart, int initiated_from);
void dlsend_packet_diplomacy_init_meeting(struct conn_list *dest, int counterpart, int initiated_from);

int send_packet_diplomacy_cancel_meeting_req(struct connection *pc, const struct packet_diplomacy_cancel_meeting_req *packet);
int dsend_packet_diplomacy_cancel_meeting_req(struct connection *pc, int counterpart);

int send_packet_diplomacy_cancel_meeting(struct connection *pc, const struct packet_diplomacy_cancel_meeting *packet);
void lsend_packet_diplomacy_cancel_meeting(struct conn_list *dest, const struct packet_diplomacy_cancel_meeting *packet);
int dsend_packet_diplomacy_cancel_meeting(struct connection *pc, int counterpart, int initiated_from);
void dlsend_packet_diplomacy_cancel_meeting(struct conn_list *dest, int counterpart, int initiated_from);

int send_packet_diplomacy_create_clause_req(struct connection *pc, const struct packet_diplomacy_create_clause_req *packet);
int dsend_packet_diplomacy_create_clause_req(struct connection *pc, int counterpart, int giver, enum clause_type type, int value);

int send_packet_diplomacy_create_clause(struct connection *pc, const struct packet_diplomacy_create_clause *packet);
void lsend_packet_diplomacy_create_clause(struct conn_list *dest, const struct packet_diplomacy_create_clause *packet);
int dsend_packet_diplomacy_create_clause(struct connection *pc, int counterpart, int giver, enum clause_type type, int value);
void dlsend_packet_diplomacy_create_clause(struct conn_list *dest, int counterpart, int giver, enum clause_type type, int value);

int send_packet_diplomacy_remove_clause_req(struct connection *pc, const struct packet_diplomacy_remove_clause_req *packet);
int dsend_packet_diplomacy_remove_clause_req(struct connection *pc, int counterpart, int giver, enum clause_type type, int value);

int send_packet_diplomacy_remove_clause(struct connection *pc, const struct packet_diplomacy_remove_clause *packet);
void lsend_packet_diplomacy_remove_clause(struct conn_list *dest, const struct packet_diplomacy_remove_clause *packet);
int dsend_packet_diplomacy_remove_clause(struct connection *pc, int counterpart, int giver, enum clause_type type, int value);
void dlsend_packet_diplomacy_remove_clause(struct conn_list *dest, int counterpart, int giver, enum clause_type type, int value);

int send_packet_diplomacy_accept_treaty_req(struct connection *pc, const struct packet_diplomacy_accept_treaty_req *packet);
int dsend_packet_diplomacy_accept_treaty_req(struct connection *pc, int counterpart);

int send_packet_diplomacy_accept_treaty(struct connection *pc, const struct packet_diplomacy_accept_treaty *packet);
void lsend_packet_diplomacy_accept_treaty(struct conn_list *dest, const struct packet_diplomacy_accept_treaty *packet);
int dsend_packet_diplomacy_accept_treaty(struct connection *pc, int counterpart, bool I_accepted, bool other_accepted);
void dlsend_packet_diplomacy_accept_treaty(struct conn_list *dest, int counterpart, bool I_accepted, bool other_accepted);

int send_packet_diplomacy_cancel_pact(struct connection *pc, const struct packet_diplomacy_cancel_pact *packet);
int dsend_packet_diplomacy_cancel_pact(struct connection *pc, int other_player_id, enum clause_type clause);

int send_packet_page_msg(struct connection *pc, const struct packet_page_msg *packet);
void lsend_packet_page_msg(struct conn_list *dest, const struct packet_page_msg *packet);

int send_packet_page_msg_part(struct connection *pc, const struct packet_page_msg_part *packet);
void lsend_packet_page_msg_part(struct conn_list *dest, const struct packet_page_msg_part *packet);

int send_packet_report_req(struct connection *pc, const struct packet_report_req *packet);
int dsend_packet_report_req(struct connection *pc, enum report_type type);

int send_packet_conn_info(struct connection *pc, const struct packet_conn_info *packet);
void lsend_packet_conn_info(struct conn_list *dest, const struct packet_conn_info *packet);

int send_packet_conn_ping_info(struct connection *pc, const struct packet_conn_ping_info *packet);
void lsend_packet_conn_ping_info(struct conn_list *dest, const struct packet_conn_ping_info *packet);

int send_packet_conn_ping(struct connection *pc);

int send_packet_conn_pong(struct connection *pc);

int send_packet_client_heartbeat(struct connection *pc);

int send_packet_client_info(struct connection *pc, const struct packet_client_info *packet);

int send_packet_end_phase(struct connection *pc);
void lsend_packet_end_phase(struct conn_list *dest);

int send_packet_start_phase(struct connection *pc, const struct packet_start_phase *packet);
void lsend_packet_start_phase(struct conn_list *dest, const struct packet_start_phase *packet);
int dsend_packet_start_phase(struct connection *pc, int phase);
void dlsend_packet_start_phase(struct conn_list *dest, int phase);

int send_packet_new_year(struct connection *pc, const struct packet_new_year *packet);
void lsend_packet_new_year(struct conn_list *dest, const struct packet_new_year *packet);

int send_packet_begin_turn(struct connection *pc);
void lsend_packet_begin_turn(struct conn_list *dest);

int send_packet_end_turn(struct connection *pc);
void lsend_packet_end_turn(struct conn_list *dest);

int send_packet_freeze_client(struct connection *pc);
void lsend_packet_freeze_client(struct conn_list *dest);

int send_packet_thaw_client(struct connection *pc);
void lsend_packet_thaw_client(struct conn_list *dest);

int send_packet_spaceship_launch(struct connection *pc);

int send_packet_spaceship_place(struct connection *pc, const struct packet_spaceship_place *packet);
int dsend_packet_spaceship_place(struct connection *pc, enum spaceship_place_type type, int num);

int send_packet_spaceship_info(struct connection *pc, const struct packet_spaceship_info *packet);
void lsend_packet_spaceship_info(struct conn_list *dest, const struct packet_spaceship_info *packet);

int send_packet_ruleset_unit(struct connection *pc, const struct packet_ruleset_unit *packet);
void lsend_packet_ruleset_unit(struct conn_list *dest, const struct packet_ruleset_unit *packet);

int send_packet_ruleset_unit_bonus(struct connection *pc, const struct packet_ruleset_unit_bonus *packet);
void lsend_packet_ruleset_unit_bonus(struct conn_list *dest, const struct packet_ruleset_unit_bonus *packet);

int send_packet_ruleset_unit_flag(struct connection *pc, const struct packet_ruleset_unit_flag *packet);
void lsend_packet_ruleset_unit_flag(struct conn_list *dest, const struct packet_ruleset_unit_flag *packet);

int send_packet_ruleset_game(struct connection *pc, const struct packet_ruleset_game *packet);
void lsend_packet_ruleset_game(struct conn_list *dest, const struct packet_ruleset_game *packet);

int send_packet_ruleset_specialist(struct connection *pc, const struct packet_ruleset_specialist *packet);
void lsend_packet_ruleset_specialist(struct conn_list *dest, const struct packet_ruleset_specialist *packet);

int send_packet_ruleset_government_ruler_title(struct connection *pc, const struct packet_ruleset_government_ruler_title *packet);
void lsend_packet_ruleset_government_ruler_title(struct conn_list *dest, const struct packet_ruleset_government_ruler_title *packet);

int send_packet_ruleset_tech(struct connection *pc, const struct packet_ruleset_tech *packet);
void lsend_packet_ruleset_tech(struct conn_list *dest, const struct packet_ruleset_tech *packet);

int send_packet_ruleset_tech_flag(struct connection *pc, const struct packet_ruleset_tech_flag *packet);
void lsend_packet_ruleset_tech_flag(struct conn_list *dest, const struct packet_ruleset_tech_flag *packet);

int send_packet_ruleset_government(struct connection *pc, const struct packet_ruleset_government *packet);
void lsend_packet_ruleset_government(struct conn_list *dest, const struct packet_ruleset_government *packet);

int send_packet_ruleset_terrain_control(struct connection *pc, const struct packet_ruleset_terrain_control *packet);
void lsend_packet_ruleset_terrain_control(struct conn_list *dest, const struct packet_ruleset_terrain_control *packet);

int send_packet_rulesets_ready(struct connection *pc);
void lsend_packet_rulesets_ready(struct conn_list *dest);

int send_packet_ruleset_nation_sets(struct connection *pc, const struct packet_ruleset_nation_sets *packet);
void lsend_packet_ruleset_nation_sets(struct conn_list *dest, const struct packet_ruleset_nation_sets *packet);

int send_packet_ruleset_nation_groups(struct connection *pc, const struct packet_ruleset_nation_groups *packet);
void lsend_packet_ruleset_nation_groups(struct conn_list *dest, const struct packet_ruleset_nation_groups *packet);

int send_packet_ruleset_nation(struct connection *pc, const struct packet_ruleset_nation *packet);
void lsend_packet_ruleset_nation(struct conn_list *dest, const struct packet_ruleset_nation *packet);

int send_packet_nation_availability(struct connection *pc, const struct packet_nation_availability *packet);
void lsend_packet_nation_availability(struct conn_list *dest, const struct packet_nation_availability *packet);

int send_packet_ruleset_style(struct connection *pc, const struct packet_ruleset_style *packet);
void lsend_packet_ruleset_style(struct conn_list *dest, const struct packet_ruleset_style *packet);

int send_packet_ruleset_city(struct connection *pc, const struct packet_ruleset_city *packet);
void lsend_packet_ruleset_city(struct conn_list *dest, const struct packet_ruleset_city *packet);

int send_packet_ruleset_building(struct connection *pc, const struct packet_ruleset_building *packet);
void lsend_packet_ruleset_building(struct conn_list *dest, const struct packet_ruleset_building *packet);

int send_packet_ruleset_terrain(struct connection *pc, const struct packet_ruleset_terrain *packet);
void lsend_packet_ruleset_terrain(struct conn_list *dest, const struct packet_ruleset_terrain *packet);

int send_packet_ruleset_terrain_flag(struct connection *pc, const struct packet_ruleset_terrain_flag *packet);
void lsend_packet_ruleset_terrain_flag(struct conn_list *dest, const struct packet_ruleset_terrain_flag *packet);

int send_packet_ruleset_unit_class(struct connection *pc, const struct packet_ruleset_unit_class *packet);
void lsend_packet_ruleset_unit_class(struct conn_list *dest, const struct packet_ruleset_unit_class *packet);

int send_packet_ruleset_extra(struct connection *pc, const struct packet_ruleset_extra *packet);
void lsend_packet_ruleset_extra(struct conn_list *dest, const struct packet_ruleset_extra *packet);

int send_packet_ruleset_base(struct connection *pc, const struct packet_ruleset_base *packet);
void lsend_packet_ruleset_base(struct conn_list *dest, const struct packet_ruleset_base *packet);

int send_packet_ruleset_road(struct connection *pc, const struct packet_ruleset_road *packet);
void lsend_packet_ruleset_road(struct conn_list *dest, const struct packet_ruleset_road *packet);

int send_packet_ruleset_disaster(struct connection *pc, const struct packet_ruleset_disaster *packet);
void lsend_packet_ruleset_disaster(struct conn_list *dest, const struct packet_ruleset_disaster *packet);

int send_packet_ruleset_achievement(struct connection *pc, const struct packet_ruleset_achievement *packet);
void lsend_packet_ruleset_achievement(struct conn_list *dest, const struct packet_ruleset_achievement *packet);

int send_packet_ruleset_trade(struct connection *pc, const struct packet_ruleset_trade *packet);
void lsend_packet_ruleset_trade(struct conn_list *dest, const struct packet_ruleset_trade *packet);

int send_packet_ruleset_action(struct connection *pc, const struct packet_ruleset_action *packet);
void lsend_packet_ruleset_action(struct conn_list *dest, const struct packet_ruleset_action *packet);

int send_packet_ruleset_action_enabler(struct connection *pc, const struct packet_ruleset_action_enabler *packet);
void lsend_packet_ruleset_action_enabler(struct conn_list *dest, const struct packet_ruleset_action_enabler *packet);

int send_packet_ruleset_music(struct connection *pc, const struct packet_ruleset_music *packet);
void lsend_packet_ruleset_music(struct conn_list *dest, const struct packet_ruleset_music *packet);

int send_packet_ruleset_multiplier(struct connection *pc, const struct packet_ruleset_multiplier *packet);
void lsend_packet_ruleset_multiplier(struct conn_list *dest, const struct packet_ruleset_multiplier *packet);
int dsend_packet_ruleset_multiplier(struct connection *pc, Multiplier_type_id id, int start, int stop, int step, int def, int offset, int factor, const char *name, const char *rule_name, const char *helptext);
void dlsend_packet_ruleset_multiplier(struct conn_list *dest, Multiplier_type_id id, int start, int stop, int step, int def, int offset, int factor, const char *name, const char *rule_name, const char *helptext);

int send_packet_ruleset_control(struct connection *pc, const struct packet_ruleset_control *packet);
void lsend_packet_ruleset_control(struct conn_list *dest, const struct packet_ruleset_control *packet);

int send_packet_ruleset_summary(struct connection *pc, const struct packet_ruleset_summary *packet);
void lsend_packet_ruleset_summary(struct conn_list *dest, const struct packet_ruleset_summary *packet);

int send_packet_ruleset_description_part(struct connection *pc, const struct packet_ruleset_description_part *packet);
void lsend_packet_ruleset_description_part(struct conn_list *dest, const struct packet_ruleset_description_part *packet);

int send_packet_single_want_hack_req(struct connection *pc, const struct packet_single_want_hack_req *packet);

int send_packet_single_want_hack_reply(struct connection *pc, const struct packet_single_want_hack_reply *packet);
int dsend_packet_single_want_hack_reply(struct connection *pc, bool you_have_hack);

int send_packet_ruleset_choices(struct connection *pc, const struct packet_ruleset_choices *packet);

int send_packet_game_load(struct connection *pc, const struct packet_game_load *packet);
void lsend_packet_game_load(struct conn_list *dest, const struct packet_game_load *packet);
int dsend_packet_game_load(struct connection *pc, bool load_successful, const char *load_filename);
void dlsend_packet_game_load(struct conn_list *dest, bool load_successful, const char *load_filename);

int send_packet_server_setting_control(struct connection *pc, const struct packet_server_setting_control *packet);

int send_packet_server_setting_const(struct connection *pc, const struct packet_server_setting_const *packet);

int send_packet_server_setting_bool(struct connection *pc, const struct packet_server_setting_bool *packet);

int send_packet_server_setting_int(struct connection *pc, const struct packet_server_setting_int *packet);

int send_packet_server_setting_str(struct connection *pc, const struct packet_server_setting_str *packet);

int send_packet_server_setting_enum(struct connection *pc, const struct packet_server_setting_enum *packet);

int send_packet_server_setting_bitwise(struct connection *pc, const struct packet_server_setting_bitwise *packet);

int send_packet_set_topology(struct connection *pc, const struct packet_set_topology *packet);

int send_packet_ruleset_effect(struct connection *pc, const struct packet_ruleset_effect *packet);
void lsend_packet_ruleset_effect(struct conn_list *dest, const struct packet_ruleset_effect *packet);

int send_packet_ruleset_resource(struct connection *pc, const struct packet_ruleset_resource *packet);
void lsend_packet_ruleset_resource(struct conn_list *dest, const struct packet_ruleset_resource *packet);

int send_packet_scenario_info(struct connection *pc, const struct packet_scenario_info *packet);

int send_packet_scenario_description(struct connection *pc, const struct packet_scenario_description *packet);

int send_packet_save_scenario(struct connection *pc, const struct packet_save_scenario *packet);
int dsend_packet_save_scenario(struct connection *pc, const char *name);

int send_packet_vote_new(struct connection *pc, const struct packet_vote_new *packet);

int send_packet_vote_update(struct connection *pc, const struct packet_vote_update *packet);

int send_packet_vote_remove(struct connection *pc, const struct packet_vote_remove *packet);

int send_packet_vote_resolve(struct connection *pc, const struct packet_vote_resolve *packet);

int send_packet_vote_submit(struct connection *pc, const struct packet_vote_submit *packet);

int send_packet_edit_mode(struct connection *pc, const struct packet_edit_mode *packet);
int dsend_packet_edit_mode(struct connection *pc, bool state);

int send_packet_edit_recalculate_borders(struct connection *pc);

int send_packet_edit_check_tiles(struct connection *pc);

int send_packet_edit_toggle_fogofwar(struct connection *pc, const struct packet_edit_toggle_fogofwar *packet);
int dsend_packet_edit_toggle_fogofwar(struct connection *pc, int player);

int send_packet_edit_tile_terrain(struct connection *pc, const struct packet_edit_tile_terrain *packet);
int dsend_packet_edit_tile_terrain(struct connection *pc, int tile, Terrain_type_id terrain, int size);

int send_packet_edit_tile_resource(struct connection *pc, const struct packet_edit_tile_resource *packet);
int dsend_packet_edit_tile_resource(struct connection *pc, int tile, Resource_type_id resource, int size);

int send_packet_edit_tile_extra(struct connection *pc, const struct packet_edit_tile_extra *packet);
int dsend_packet_edit_tile_extra(struct connection *pc, int tile, int extra_type_id, bool removal, int size);

int send_packet_edit_startpos(struct connection *pc, const struct packet_edit_startpos *packet);
int dsend_packet_edit_startpos(struct connection *pc, int id, bool removal, int tag);

int send_packet_edit_startpos_full(struct connection *pc, const struct packet_edit_startpos_full *packet);

int send_packet_edit_tile(struct connection *pc, const struct packet_edit_tile *packet);

int send_packet_edit_unit_create(struct connection *pc, const struct packet_edit_unit_create *packet);
int dsend_packet_edit_unit_create(struct connection *pc, int owner, int tile, Unit_type_id type, int count, int tag);

int send_packet_edit_unit_remove(struct connection *pc, const struct packet_edit_unit_remove *packet);
int dsend_packet_edit_unit_remove(struct connection *pc, int owner, int tile, Unit_type_id type, int count);

int send_packet_edit_unit_remove_by_id(struct connection *pc, const struct packet_edit_unit_remove_by_id *packet);
int dsend_packet_edit_unit_remove_by_id(struct connection *pc, int id);

int send_packet_edit_unit(struct connection *pc, const struct packet_edit_unit *packet);

int send_packet_edit_city_create(struct connection *pc, const struct packet_edit_city_create *packet);
int dsend_packet_edit_city_create(struct connection *pc, int owner, int tile, int size, int tag);

int send_packet_edit_city_remove(struct connection *pc, const struct packet_edit_city_remove *packet);
int dsend_packet_edit_city_remove(struct connection *pc, int id);

int send_packet_edit_city(struct connection *pc, const struct packet_edit_city *packet);

int send_packet_edit_player_create(struct connection *pc, const struct packet_edit_player_create *packet);
int dsend_packet_edit_player_create(struct connection *pc, int tag);

int send_packet_edit_player_remove(struct connection *pc, const struct packet_edit_player_remove *packet);
int dsend_packet_edit_player_remove(struct connection *pc, int id);

int send_packet_edit_player(struct connection *pc, const struct packet_edit_player *packet);
void lsend_packet_edit_player(struct conn_list *dest, const struct packet_edit_player *packet);

int send_packet_edit_player_vision(struct connection *pc, const struct packet_edit_player_vision *packet);
int dsend_packet_edit_player_vision(struct connection *pc, int player, int tile, bool known, int size);

int send_packet_edit_game(struct connection *pc, const struct packet_edit_game *packet);

int send_packet_edit_scenario_desc(struct connection *pc, const struct packet_edit_scenario_desc *packet);

int send_packet_edit_object_created(struct connection *pc, const struct packet_edit_object_created *packet);
int dsend_packet_edit_object_created(struct connection *pc, int tag, int id);

int send_packet_play_music(struct connection *pc, const struct packet_play_music *packet);
void lsend_packet_play_music(struct conn_list *dest, const struct packet_play_music *packet);


void delta_stats_report(void);
void delta_stats_reset(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
