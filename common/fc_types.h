// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// utility
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"

/**
 * This file serves to reduce the cross-inclusion of header files which
 * occurs when a type which is defined in one file is needed for a function
 * definition in another file.
 *
 * Nothing in this file should require anything else from the common/
 * directory!
 *
 * NOTE: Most of the macros and enums types in this file are used by the
 * network protocol. Be careful of making any changes here.
 *
 * The types are arranged here in this order:
 * - common macros
 * - general
 * - network / packet
 * - network / server
 * - server
 * - advance / technology
 * - building / improvement
 * - unit
 * - diplomacy
 * - tile / terrain
 * - player
 * - city
 * - game
 * - traderoutes
 * - actions
 * - spaceship
 * - events
 * - effects
 * - requirements
 * - city prod worklist
 */

// Line breaks after this number of characters; be carefull and use only 70
#define LINE_BREAK 70

// Symbol to flag missing numbers for better debugging
#define IDENTITY_NUMBER_ZERO (0)

// Changing these will break network compatibility.
#define SP_MAX 20
#define RULESET_SUFFIX ".serv"

/**
 * MAX_LEN_* macros
 */
#define MAX_LEN_PACKET 4096
#define MAX_LEN_CONTENT (MAX_LEN_PACKET - 20)
#define MAX_LEN_BUFFER (MAX_LEN_PACKET * 128)
#define MAX_LEN_ROUTE 2000 // MAX_LEN_PACKET / 2 - header
#define MAX_LEN_CAPSTR 512
#define MAX_LEN_PASSWORD 512 // DO NOT change this under any circumstances
#define MAX_LEN_VET_SHORT_NAME 8
#define MAX_LEN_NAME 48
#define MAX_LEN_CITYNAME 80
#define MAX_LEN_MAP_LABEL 64
#define MAX_LEN_DEMOGRAPHY 32
#define MAX_LEN_ALLOW_TAKE 16
#define MAX_LEN_GAME_IDENTIFIER 33
#define MAX_LEN_STARTUNIT (20 + 1)
#define MAX_LEN_ENUM 64
#define MAX_LEN_MSG 1536

/**
 * MAX_NUM_* macros
 *
 * MAX_NUM_PLAYER_SLOTS must be divisable by 32 or iterations
 * in savegame2.cpp needs to be changed
 *
 * MAX_NUM_NATION_SETS will impact the RULESET_NATION_SETS packet and may
 * become too big if increased
 */
#define MAX_NUM_PLAYER_SLOTS 512 // Used in the network protocol.
#define MAX_NUM_TEAM_SLOTS MAX_NUM_PLAYER_SLOTS
#define MAX_NUM_BARBARIANS 12
#define MAX_NUM_PLAYERS MAX_NUM_PLAYER_SLOTS - MAX_NUM_BARBARIANS
#define MAX_NUM_CONNECTIONS (2 * (MAX_NUM_PLAYER_SLOTS))
#define MAX_NUM_ITEMS 200
#define MAX_NUM_TECH_LIST 10
#define MAX_NUM_TECH_CLASSES 16 // Used in the network protocol.
#define MAX_NUM_UNIT_LIST 10
#define MAX_NUM_BUILDING_LIST 10
#define MAX_NUM_ACTION_AUTO_PERFORMERS 4
#define MAX_NUM_MULTIPLIERS 15
#define MAX_NUM_LEADERS MAX_NUM_ITEMS // Used in the network protocol.
#define MAX_NUM_NATION_SETS 32        // Used in the network protocol.
#define MAX_NUM_NATION_GROUPS 128     // Used in the network protocol.
#define MAX_NUM_NATIONS MAX_UINT16
#define MAX_NUM_STARTPOS_NATIONS 1024 // Used in the network protocol.
#define MAX_NUM_REQS 20
#define MAX_NUM_RULESETS 63 // Used in the network protocol.

/**
 * Other MAX_* macros
 */
#define MAX_VET_LEVELS 20              // Used in the network protocol.
#define MAX_EXTRA_TYPES 128            // Used in the network protocol.
#define MAX_BASE_TYPES MAX_EXTRA_TYPES // Used in the network protocol.
#define MAX_ROAD_TYPES MAX_EXTRA_TYPES // Used in the network protocol.
#define MAX_GOODS_TYPES 25
#define MAX_DISASTER_TYPES 10
#define MAX_ACHIEVEMENT_TYPES 40
// nation count is a UINT16
#define MAX_CALENDAR_FRAGMENTS 52 // Used in the network protocol.
#define MAX_GRANARY_INIS 24
#define MAX_RULESET_NAME_LENGTH 64 // Used in the network protocol.

/**
 * General - Tile and City output types
 */
enum output_type_id {
  O_FOOD,
  O_SHIELD,
  O_TRADE,
  O_GOLD,
  O_LUXURY,
  O_SCIENCE,
  O_LAST
};

/**
 * General - common types
 */
enum setting_default_level {
  SETDEF_INTERNAL,
  SETDEF_RULESET,
  SETDEF_CHANGED
};

enum override_bool { OVERRIDE_TRUE, OVERRIDE_FALSE, NO_OVERRIDE };
enum trait_dist_mode { TDM_FIXED = 0, TDM_EVEN };
enum test_result { TR_SUCCESS, TR_OTHER_FAILURE, TR_ALREADY_SOLD };

typedef signed short Continent_id;
typedef int Terrain_type_id;
typedef int Resource_type_id;
typedef int Specialist_type_id;
typedef int Impr_type_id;
typedef int Tech_type_id;
typedef enum output_type_id Output_type_id;
typedef int Nation_type_id;
typedef int Government_type_id;
typedef int Unit_type_id;
typedef int Base_type_id;
typedef int Road_type_id;
typedef int Disaster_type_id;
typedef int Multiplier_type_id;
typedef int Goods_type_id;
typedef unsigned char citizens;
typedef int action_id;
// A server setting + its value.
typedef int ssetv;

typedef float adv_want;
#define ADV_WANT_PRINTF "%f"

struct advance;
struct city;
struct connection;
struct government;
struct impr_type;
struct nation_type;
struct output_type;
struct player;
struct specialist;
struct terrain;
struct tile;
struct unit;
struct achievement;
struct action;
struct resource_type;

BV_DEFINE(bv_extras, MAX_EXTRA_TYPES);
BV_DEFINE(bv_special, MAX_EXTRA_TYPES);
BV_DEFINE(bv_bases, MAX_BASE_TYPES);
BV_DEFINE(bv_roads, MAX_ROAD_TYPES);
BV_DEFINE(bv_startpos_nations, MAX_NUM_STARTPOS_NATIONS);

/**
 * Network / Packet
 *
 * The size of opaque (void *) data sent in the network packet. To avoid
 * fragmentation issues, this SHOULD NOT be larger than the standard
 * ethernet or PPP 1500 byte frame size (with room for headers).
 *
 * Do not spend much time optimizing, you have no idea of the actual dynamic
 * path characteristics between systems, such as VPNs and tunnels.
 */
#define ATTRIBUTE_CHUNK_SIZE (1400)

/**
 * Network / Packet - types
 */
enum authentication_type {
  AUTH_LOGIN_FIRST,   // request a password for a returning user
  AUTH_NEWUSER_FIRST, // request a password for a new user
  AUTH_LOGIN_RETRY,   // inform the client to try a different password
  AUTH_NEWUSER_RETRY  // inform the client to try a different [new] password
};

enum report_type {
  REPORT_WONDERS_OF_THE_WORLD,
  REPORT_TOP_5_CITIES,
  REPORT_DEMOGRAPHIC,
  REPORT_ACHIEVEMENTS
};

/**
 * Network / server - cmdlevel types
 *
 * Command access levels for client-side use; at present, they are only
 * used to control access to server commands typed at the client chatline.
 */
#define SPECENUM_NAME cmdlevel
// User may issue no commands at all.
#define SPECENUM_VALUE0 ALLOW_NONE
#define SPECENUM_VALUE0NAME "none"
// Informational or observer commands only.
#define SPECENUM_VALUE1 ALLOW_INFO
#define SPECENUM_VALUE1NAME "info"
// User may issue basic player commands.
#define SPECENUM_VALUE2 ALLOW_BASIC
#define SPECENUM_VALUE2NAME "basic"
/* User may issue commands that affect game & users
 * (starts a vote if the user's level is 'basic'). */
#define SPECENUM_VALUE3 ALLOW_CTRL
#define SPECENUM_VALUE3NAME "ctrl"
// User may issue commands that affect the server.
#define SPECENUM_VALUE4 ALLOW_ADMIN
#define SPECENUM_VALUE4NAME "admin"
// User may issue *all* commands - dangerous!
#define SPECENUM_VALUE5 ALLOW_HACK
#define SPECENUM_VALUE5NAME "hack"
#define SPECENUM_COUNT CMDLEVEL_COUNT
#include "specenum_gen.h"

/**
 * Network / server - Connection list

 * get 'struct conn_list' and related functions:
 * do this with forward definition of struct connection, so that
 * connection struct can contain a struct conn_list
 */
struct connection;
#define SPECLIST_TAG conn
#define SPECLIST_TYPE struct connection
#include "speclist.h"

#define conn_list_iterate(connlist, pconn)                                  \
  TYPED_LIST_ITERATE(struct connection, connlist, pconn)
#define conn_list_iterate_end LIST_ITERATE_END

/**
 * Network / server - connection types
 */
#define SPECENUM_NAME persistent_ready
#define SPECENUM_VALUE0 PERSISTENTR_DISABLED
#define SPECENUM_VALUE0NAME "Disabled"
#define SPECENUM_VALUE1 PERSISTENTR_CONNECTED
#define SPECENUM_VALUE1NAME "Connected"
#include "specenum_gen.h"

/**
 * Server - setting types
 */
#define SPECENUM_NAME sset_type
#define SPECENUM_VALUE0 SST_BOOL
#define SPECENUM_VALUE1 SST_INT
#define SPECENUM_VALUE2 SST_STRING
#define SPECENUM_VALUE3 SST_ENUM
#define SPECENUM_VALUE4 SST_BITWISE
#define SPECENUM_COUNT SST_COUNT
#include "specenum_gen.h"

// Mark server setting id's.
typedef int server_setting_id;

/**
 * Advance / Technology - macros
 *
 * A_NONE is the root tech. All players always know this tech. It is
 * used as a flag in various cases where there is no tech-requirement.
 *
 * A_FIRST is the first real tech id value
 *
 * A_UNSET indicates that no tech is selected (for research).
 *
 * A_FUTURE indicates that the player is researching a future tech.
 *
 * A_UNKNOWN may be passed to other players instead of the actual value.
 *
 * A_LAST is a value that is guaranteed to be larger than all
 * actual Tech_type_id values. It is used as a flag value; it can
 * also be used for fixed allocations to ensure ability to hold the
 * full number of techs.
 *
 * A_NEVER is the pointer equivalent replacement for A_LAST flag value.
 */
#define MAX_NUM_ADVANCES 250
#define A_NONE 0
#define A_FIRST 1
#define A_LAST (MAX_NUM_ADVANCES + 1) // Used in the network protocol.
#define A_FUTURE (A_LAST + 1)
#define A_ARRAY_SIZE (A_FUTURE + 1)
#define A_UNSET (A_LAST + 2)
#define A_UNKNOWN (A_LAST + 3)
#define A_NEVER (nullptr)

/**
 * Advance / Technology - tech flag types
 *
 * If a new flag is added techtools.cpp:research_tech_lost() should be
 * checked
 */
#define SPECENUM_NAME tech_flag_id
// player gets extra tech if rearched first
#define SPECENUM_VALUE0 TF_BONUS_TECH
/* TRANS: this and following strings are 'tech flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("Bonus_Tech")
// "Settler" unit types can build bridges over rivers
#define SPECENUM_VALUE1 TF_BRIDGE
#define SPECENUM_VALUE1NAME N_("Bridge")
// Player can build air units
#define SPECENUM_VALUE2 TF_BUILD_AIRBORNE
#define SPECENUM_VALUE2NAME N_("Build_Airborne")
// Player can claim ocean tiles non-adjacent to border source
#define SPECENUM_VALUE3 TF_CLAIM_OCEAN
#define SPECENUM_VALUE3NAME N_("Claim_Ocean")
/* Player can claim ocean tiles non-adjacent to border source as long
 * as source is ocean tile */
#define SPECENUM_VALUE4 TF_CLAIM_OCEAN_LIMITED
#define SPECENUM_VALUE4NAME N_("Claim_Ocean_Limited")
#define SPECENUM_VALUE5 TECH_USER_1
#define SPECENUM_VALUE6 TECH_USER_2
#define SPECENUM_VALUE7 TECH_USER_3
#define SPECENUM_VALUE8 TECH_USER_4
#define SPECENUM_VALUE9 TECH_USER_5
#define SPECENUM_VALUE10 TECH_USER_6
#define SPECENUM_VALUE11 TECH_USER_7
#define SPECENUM_VALUE12 TECH_USER_LAST
// Keep this last.
#define SPECENUM_COUNT TF_COUNT
#define SPECENUM_BITVECTOR bv_tech_flags
#define SPECENUM_NAMEOVERRIDE
#include "specenum_gen.h"

/**
 * Advance / Technology - free tech method types
 */
#define SPECENUM_NAME free_tech_method
#define SPECENUM_VALUE0 FTM_GOAL
#define SPECENUM_VALUE0NAME "Goal"
#define SPECENUM_VALUE1 FTM_RANDOM
#define SPECENUM_VALUE1NAME "Random"
#define SPECENUM_VALUE2 FTM_CHEAPEST
#define SPECENUM_VALUE2NAME "Cheapest"
#include "specenum_gen.h"

/**
 * Advance / Technology - tech upkeep types
 */
#define SPECENUM_NAME tech_upkeep_style
// No upkeep
#define SPECENUM_VALUE0 TECH_UPKEEP_NONE
#define SPECENUM_VALUE0NAME "None"
// Normal tech upkeep
#define SPECENUM_VALUE1 TECH_UPKEEP_BASIC
#define SPECENUM_VALUE1NAME "Basic"
// Tech upkeep multiplied by number of cities
#define SPECENUM_VALUE2 TECH_UPKEEP_PER_CITY
#define SPECENUM_VALUE2NAME "Cities"
#include "specenum_gen.h"

/**
 * Advance / Technology - tech cost style types
 */
#define SPECENUM_NAME tech_cost_style
#define SPECENUM_VALUE0 TECH_COST_CIV1CIV2
#define SPECENUM_VALUE0NAME "Civ I|II"
#define SPECENUM_VALUE1 TECH_COST_CLASSIC
#define SPECENUM_VALUE1NAME "Classic"
#define SPECENUM_VALUE2 TECH_COST_CLASSIC_PRESET
#define SPECENUM_VALUE2NAME "Classic+"
#define SPECENUM_VALUE3 TECH_COST_EXPERIMENTAL
#define SPECENUM_VALUE3NAME "Experimental"
#define SPECENUM_VALUE4 TECH_COST_EXPERIMENTAL_PRESET
#define SPECENUM_VALUE4NAME "Experimental+"
#define SPECENUM_VALUE5 TECH_COST_LINEAR
#define SPECENUM_VALUE5NAME "Linear"
#include "specenum_gen.h"

/**
 * Advance / Technology - tech leak style types
 */
#define SPECENUM_NAME tech_leakage_style
#define SPECENUM_VALUE0 TECH_LEAKAGE_NONE
#define SPECENUM_VALUE0NAME "None"
#define SPECENUM_VALUE1 TECH_LEAKAGE_EMBASSIES
#define SPECENUM_VALUE1NAME "Embassies"
#define SPECENUM_VALUE2 TECH_LEAKAGE_PLAYERS
#define SPECENUM_VALUE2NAME "All Players"
#define SPECENUM_VALUE3 TECH_LEAKAGE_NO_BARBS
#define SPECENUM_VALUE3NAME "Normal Players"
#include "specenum_gen.h"

/**
 * Building / Improvement - macros
 *
 * B_LAST is a value that is guaranteed to be larger than all
 * actual Impr_type_id values. It is used as a flag value; it can
 * also be used for fixed allocations to ensure ability to hold the
 * full number of improvement types.
 *
 * B_NEVER is the pointer equivalent replacement for B_LAST flag value.
 */
#define MAX_NUM_BUILDINGS 200
#define B_LAST MAX_NUM_BUILDINGS
#define B_NEVER (nullptr)

/**
 * Building / Improvement - flag types
 */
#define SPECENUM_NAME impr_flag_id
// improvement should be visible to others without spying
#define SPECENUM_VALUE0 IF_VISIBLE_BY_OTHERS
#define SPECENUM_VALUE0NAME "VisibleByOthers"
// this small wonder is moved to another city if game.savepalace is on.
#define SPECENUM_VALUE1 IF_SAVE_SMALL_WONDER
#define SPECENUM_VALUE1NAME "SaveSmallWonder"
// when built, gives gold
#define SPECENUM_VALUE2 IF_GOLD
#define SPECENUM_VALUE2NAME "Gold"
// Never destroyed by disasters
#define SPECENUM_VALUE3 IF_DISASTER_PROOF
#define SPECENUM_VALUE3NAME "DisasterProof"
#define SPECENUM_COUNT IF_COUNT
#define SPECENUM_BITVECTOR bv_impr_flags
#include "specenum_gen.h"

/**
 * Building / Improvement - types
 */
#define SPECENUM_NAME impr_genus_id
#define SPECENUM_VALUE0 IG_GREAT_WONDER
#define SPECENUM_VALUE0NAME "GreatWonder"
#define SPECENUM_VALUE1 IG_SMALL_WONDER
#define SPECENUM_VALUE1NAME "SmallWonder"
#define SPECENUM_VALUE2 IG_IMPROVEMENT
#define SPECENUM_VALUE2NAME "Improvement"
#define SPECENUM_VALUE3 IG_SPECIAL
#define SPECENUM_VALUE3NAME "Special"
#define SPECENUM_COUNT IG_COUNT
#include "specenum_gen.h"

/**
 * A bitvector for building / improvements
 */
BV_DEFINE(bv_imprs, B_LAST);

/**
 * Unit - macros
 *
 * Symbol used to flag no (sub) target of an action or for an activity.
 * IDENTITY_NUMBER_ZERO can't be used since 0 is a valid identity for
 * certain (sub) targets.
 *
 * U_LAST is a value which is guaranteed to be larger than all
 * actual Unit_type_id values. It is used as a flag value;
 * it can also be used for fixed allocations to ensure able
 * to hold full number of unit types.
 */
#define NO_TARGET (-1)
#define MAX_NUM_UNIT_TYPES 250
#define U_LAST MAX_NUM_UNIT_TYPES
#define L_MAX 64 // Used in the network protocol.

// Happens at once, not during turn change.
#define ACT_TIME_INSTANTANEOUS (-1)

/**
 * Unit - roles
 *
 * These are similar to unit flags but differ in that
 * they don't represent intrinsic propert*ies or abilities of units,
 * but determine which units are used (mainly by the server or AI)
 * in various circumstances, or "roles".
 * Note that in some cases flags can act as roles, eg, we don't need
 * a role for "settlers", because we can just use UTYF_SETTLERS.
 * (Now have to consider ACTION_FOUND_CITY too)
 * So we make sure flag values and role values are distinct,
 * so some functions can use them interchangably.
 * See data/classic/units.ruleset for documentation of their effects.
 */
#define L_FIRST (UTYF_LAST_USER_FLAG + 1)

// Used in network protocol.
enum unit_info_use {
  UNIT_INFO_IDENTITY,
  UNIT_INFO_CITY_SUPPORTED,
  UNIT_INFO_CITY_PRESENT
};

enum adv_unit_task { AUT_NONE, AUT_AUTO_SETTLER, AUT_BUILD_CITY };

/**
 * Unit - activity types
 *
 * Changing this enum will break savegame and network compatability.
 */
#define SPECENUM_NAME unit_activity
#define SPECENUM_VALUE0 ACTIVITY_IDLE
#define SPECENUM_VALUE0NAME N_("Idle")
#define SPECENUM_VALUE1 ACTIVITY_POLLUTION
#define SPECENUM_VALUE1NAME N_("Pollution")
#define SPECENUM_VALUE2 ACTIVITY_OLD_ROAD
#define SPECENUM_VALUE2NAME "Unused Road"
#define SPECENUM_VALUE3 ACTIVITY_MINE
#define SPECENUM_VALUE3NAME N_("Mine")
#define SPECENUM_VALUE4 ACTIVITY_IRRIGATE
#define SPECENUM_VALUE4NAME N_("Irrigate")
#define SPECENUM_VALUE5 ACTIVITY_FORTIFIED
#define SPECENUM_VALUE5NAME N_("Fortified")
#define SPECENUM_VALUE6 ACTIVITY_FORTRESS
#define SPECENUM_VALUE6NAME N_("Fortress")
#define SPECENUM_VALUE7 ACTIVITY_SENTRY
#define SPECENUM_VALUE7NAME N_("Sentry")
#define SPECENUM_VALUE8 ACTIVITY_OLD_RAILROAD
#define SPECENUM_VALUE8NAME "Unused Railroad"
#define SPECENUM_VALUE9 ACTIVITY_PILLAGE
#define SPECENUM_VALUE9NAME N_("Pillage")
#define SPECENUM_VALUE10 ACTIVITY_GOTO
#define SPECENUM_VALUE10NAME N_("Goto")
#define SPECENUM_VALUE11 ACTIVITY_EXPLORE
#define SPECENUM_VALUE11NAME N_("Explore")
#define SPECENUM_VALUE12 ACTIVITY_TRANSFORM
#define SPECENUM_VALUE12NAME N_("Transform")
#define SPECENUM_VALUE13 ACTIVITY_UNKNOWN
#define SPECENUM_VALUE13NAME "Unused"
#define SPECENUM_VALUE14 ACTIVITY_AIRBASE
#define SPECENUM_VALUE14NAME "Unused Airbase"
#define SPECENUM_VALUE15 ACTIVITY_FORTIFYING
#define SPECENUM_VALUE15NAME N_("Fortifying")
#define SPECENUM_VALUE16 ACTIVITY_FALLOUT
#define SPECENUM_VALUE16NAME N_("Fallout")
#define SPECENUM_VALUE17 ACTIVITY_PATROL_UNUSED
#define SPECENUM_VALUE17NAME "Unused Patrol"
#define SPECENUM_VALUE18 ACTIVITY_BASE
#define SPECENUM_VALUE18NAME N_("Base")
#define SPECENUM_VALUE19 ACTIVITY_GEN_ROAD
#define SPECENUM_VALUE19NAME N_("Road")
#define SPECENUM_VALUE20 ACTIVITY_CONVERT
#define SPECENUM_VALUE20NAME N_("Convert")
#define SPECENUM_VALUE21 ACTIVITY_CULTIVATE
#define SPECENUM_VALUE21NAME N_("Cultivate")
#define SPECENUM_VALUE22 ACTIVITY_PLANT
#define SPECENUM_VALUE22NAME N_("Plant")
#define SPECENUM_COUNT ACTIVITY_LAST
#include "specenum_gen.h"

typedef enum unit_activity Activity_type_id;

/**
 * Unit - action results
 */
#define SPECENUM_NAME action_result
#define SPECENUM_VALUE0 ACTRES_ESTABLISH_EMBASSY
#define SPECENUM_VALUE0NAME "Establish Embassy"
#define SPECENUM_VALUE1 ACTRES_SPY_INVESTIGATE_CITY
#define SPECENUM_VALUE1NAME "Investigate City"
#define SPECENUM_VALUE2 ACTRES_SPY_POISON
#define SPECENUM_VALUE2NAME "Poison City"
#define SPECENUM_VALUE3 ACTRES_SPY_STEAL_GOLD
#define SPECENUM_VALUE3NAME "Steal Gold"
#define SPECENUM_VALUE4 ACTRES_SPY_SABOTAGE_CITY
#define SPECENUM_VALUE4NAME "Sabotage City"
#define SPECENUM_VALUE5 ACTRES_SPY_TARGETED_SABOTAGE_CITY
#define SPECENUM_VALUE5NAME "Targeted Sabotage City"
#define SPECENUM_VALUE6 ACTRES_SPY_SABOTAGE_CITY_PRODUCTION
#define SPECENUM_VALUE6NAME "Sabotage City Production"
#define SPECENUM_VALUE7 ACTRES_SPY_STEAL_TECH
#define SPECENUM_VALUE7NAME "Steal Tech"
#define SPECENUM_VALUE8 ACTRES_SPY_TARGETED_STEAL_TECH
#define SPECENUM_VALUE8NAME "Targeted Steal Tech"
#define SPECENUM_VALUE9 ACTRES_SPY_INCITE_CITY
#define SPECENUM_VALUE9NAME "Incite City"
#define SPECENUM_VALUE10 ACTRES_TRADE_ROUTE
#define SPECENUM_VALUE10NAME "Establish Trade Route"
#define SPECENUM_VALUE11 ACTRES_MARKETPLACE
#define SPECENUM_VALUE11NAME "Enter Marketplace"
#define SPECENUM_VALUE12 ACTRES_HELP_WONDER
#define SPECENUM_VALUE12NAME "Help Wonder"
#define SPECENUM_VALUE13 ACTRES_SPY_BRIBE_UNIT
#define SPECENUM_VALUE13NAME "Bribe Unit"
#define SPECENUM_VALUE14 ACTRES_SPY_SABOTAGE_UNIT
#define SPECENUM_VALUE14NAME "Sabotage Unit"
#define SPECENUM_VALUE15 ACTRES_CAPTURE_UNITS
#define SPECENUM_VALUE15NAME "Capture Units"
#define SPECENUM_VALUE16 ACTRES_FOUND_CITY
#define SPECENUM_VALUE16NAME "Found City"
#define SPECENUM_VALUE17 ACTRES_JOIN_CITY
#define SPECENUM_VALUE17NAME "Join City"
#define SPECENUM_VALUE18 ACTRES_STEAL_MAPS
#define SPECENUM_VALUE18NAME "Steal Maps"
#define SPECENUM_VALUE19 ACTRES_BOMBARD
#define SPECENUM_VALUE19NAME "Bombard"
#define SPECENUM_VALUE20 ACTRES_SPY_NUKE
#define SPECENUM_VALUE20NAME "Suitcase Nuke"
#define SPECENUM_VALUE21 ACTRES_NUKE
#define SPECENUM_VALUE21NAME "Explode Nuclear"
#define SPECENUM_VALUE22 ACTRES_NUKE_CITY
#define SPECENUM_VALUE22NAME "Nuke City"
#define SPECENUM_VALUE23 ACTRES_NUKE_UNITS
#define SPECENUM_VALUE23NAME "Nuke Units"
#define SPECENUM_VALUE24 ACTRES_DESTROY_CITY
#define SPECENUM_VALUE24NAME "Destroy City"
#define SPECENUM_VALUE25 ACTRES_EXPEL_UNIT
#define SPECENUM_VALUE25NAME "Expel Unit"
#define SPECENUM_VALUE26 ACTRES_RECYCLE_UNIT
#define SPECENUM_VALUE26NAME "Recycle Unit"
#define SPECENUM_VALUE27 ACTRES_DISBAND_UNIT
#define SPECENUM_VALUE27NAME "Disband Unit"
#define SPECENUM_VALUE28 ACTRES_HOME_CITY
#define SPECENUM_VALUE28NAME "Home City"
#define SPECENUM_VALUE29 ACTRES_UPGRADE_UNIT
#define SPECENUM_VALUE29NAME "Upgrade Unit"
#define SPECENUM_VALUE30 ACTRES_PARADROP
#define SPECENUM_VALUE30NAME "Paradrop Unit"
#define SPECENUM_VALUE31 ACTRES_AIRLIFT
#define SPECENUM_VALUE31NAME "Airlift Unit"
#define SPECENUM_VALUE32 ACTRES_ATTACK
#define SPECENUM_VALUE32NAME "Attack"
#define SPECENUM_VALUE33 ACTRES_STRIKE_BUILDING
#define SPECENUM_VALUE33NAME "Surgical Strike Building"
#define SPECENUM_VALUE34 ACTRES_STRIKE_PRODUCTION
#define SPECENUM_VALUE34NAME "Surgical Strike Production"
#define SPECENUM_VALUE35 ACTRES_CONQUER_CITY
#define SPECENUM_VALUE35NAME "Conquer City"
#define SPECENUM_VALUE36 ACTRES_HEAL_UNIT
#define SPECENUM_VALUE36NAME "Heal Unit"
#define SPECENUM_VALUE37 ACTRES_TRANSFORM_TERRAIN
#define SPECENUM_VALUE37NAME "Transform Terrain"
#define SPECENUM_VALUE38 ACTRES_CULTIVATE
#define SPECENUM_VALUE38NAME "Cultivate"
#define SPECENUM_VALUE39 ACTRES_PLANT
#define SPECENUM_VALUE39NAME "Plant"
#define SPECENUM_VALUE40 ACTRES_PILLAGE
#define SPECENUM_VALUE40NAME "Pillage"
#define SPECENUM_VALUE41 ACTRES_FORTIFY
#define SPECENUM_VALUE41NAME "Fortify"
#define SPECENUM_VALUE42 ACTRES_ROAD
#define SPECENUM_VALUE42NAME "Build Road"
#define SPECENUM_VALUE43 ACTRES_CONVERT
#define SPECENUM_VALUE43NAME "Convert Unit"
#define SPECENUM_VALUE44 ACTRES_BASE
#define SPECENUM_VALUE44NAME "Build Base"
#define SPECENUM_VALUE45 ACTRES_MINE
#define SPECENUM_VALUE45NAME "Build Mine"
#define SPECENUM_VALUE46 ACTRES_IRRIGATE
#define SPECENUM_VALUE46NAME "Build Irrigation"
#define SPECENUM_VALUE47 ACTRES_CLEAN_POLLUTION
#define SPECENUM_VALUE47NAME "Clean Pollution"
#define SPECENUM_VALUE48 ACTRES_CLEAN_FALLOUT
#define SPECENUM_VALUE48NAME "Clean Fallout"
#define SPECENUM_VALUE49 ACTRES_TRANSPORT_ALIGHT
#define SPECENUM_VALUE49NAME "Transport Alight"
#define SPECENUM_VALUE50 ACTRES_TRANSPORT_UNLOAD
#define SPECENUM_VALUE50NAME "Transport Unload"
#define SPECENUM_VALUE51 ACTRES_TRANSPORT_DISEMBARK
#define SPECENUM_VALUE51NAME "Transport Disembark"
#define SPECENUM_VALUE52 ACTRES_TRANSPORT_BOARD
#define SPECENUM_VALUE52NAME "Transport Board"
#define SPECENUM_VALUE53 ACTRES_TRANSPORT_EMBARK
#define SPECENUM_VALUE53NAME "Transport Embark"
#define SPECENUM_VALUE54 ACTRES_SPY_SPREAD_PLAGUE
#define SPECENUM_VALUE54NAME "Spread Plague"
#define SPECENUM_VALUE55 ACTRES_SPY_ATTACK
#define SPECENUM_VALUE55NAME "Spy Attack"
// All consequences are handled as (ruleset) action data.
#define SPECENUM_COUNT ACTRES_NONE
#include "specenum_gen.h"

/**
 * Unit - class flag types
 */
#define SPECENUM_NAME unit_class_flag_id
#define SPECENUM_VALUE0 UCF_TERRAIN_SPEED
#define SPECENUM_VALUE0NAME N_("?uclassflag:TerrainSpeed")
#define SPECENUM_VALUE1 UCF_TERRAIN_DEFENSE
#define SPECENUM_VALUE1NAME N_("?uclassflag:TerrainDefense")
#define SPECENUM_VALUE2 UCF_DAMAGE_SLOWS
#define SPECENUM_VALUE2NAME N_("?uclassflag:DamageSlows")
// Can occupy enemy cities
#define SPECENUM_VALUE3 UCF_CAN_OCCUPY_CITY
#define SPECENUM_VALUE3NAME N_("?uclassflag:CanOccupyCity")
#define SPECENUM_VALUE4 UCF_BUILD_ANYWHERE
#define SPECENUM_VALUE4NAME N_("?uclassflag:BuildAnywhere")
#define SPECENUM_VALUE5 UCF_UNREACHABLE
#define SPECENUM_VALUE5NAME N_("?uclassflag:Unreachable")
// Can collect ransom from barbarian leader
#define SPECENUM_VALUE6 UCF_COLLECT_RANSOM
#define SPECENUM_VALUE6NAME N_("?uclassflag:CollectRansom")
// Is subject to ZOC
#define SPECENUM_VALUE7 UCF_ZOC
#define SPECENUM_VALUE7NAME N_("?uclassflag:ZOC")
// Cities can still work tile when enemy unit on it
#define SPECENUM_VALUE8 UCF_DOESNT_OCCUPY_TILE
#define SPECENUM_VALUE8NAME N_("?uclassflag:DoesntOccupyTile")
// Can attack against units on non-native tiles
#define SPECENUM_VALUE9 UCF_ATTACK_NON_NATIVE
#define SPECENUM_VALUE9NAME N_("?uclassflag:AttackNonNative")
// Kills citizens upon successful attack against a city
#define SPECENUM_VALUE10 UCF_KILLCITIZEN
#define SPECENUM_VALUE10NAME N_("?uclassflag:KillCitizen")

#define SPECENUM_VALUE11 UCF_USER_FLAG_1
#define SPECENUM_VALUE12 UCF_USER_FLAG_2
#define SPECENUM_VALUE13 UCF_USER_FLAG_3
#define SPECENUM_VALUE14 UCF_USER_FLAG_4
#define SPECENUM_VALUE15 UCF_USER_FLAG_5
#define SPECENUM_VALUE16 UCF_USER_FLAG_6
#define SPECENUM_VALUE17 UCF_USER_FLAG_7
#define SPECENUM_VALUE18 UCF_USER_FLAG_8
#define SPECENUM_VALUE19 UCF_USER_FLAG_9
#define SPECENUM_VALUE20 UCF_USER_FLAG_10
#define SPECENUM_VALUE21 UCF_USER_FLAG_11
#define SPECENUM_VALUE22 UCF_USER_FLAG_12

// keep this last
#define SPECENUM_COUNT UCF_COUNT
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_unit_class_flags
#include "specenum_gen.h"

// Unit Class List, also 32-bit vector?
#define UCL_LAST 32 // Used in the network protocol.
typedef int Unit_Class_id;

/**
 * A bitvector for unit classes
 */
BV_DEFINE(bv_unit_classes, UCL_LAST);

/**
 * Unit - "special effects" flags
 *
 * NOTE: this is now an enumerated type, and not power-of-two integers
 * for bits, though unit_type.flags is still a bitfield, and code
 * which uses unit_has_type_flag() without twiddling bits is unchanged.
 * (It is easier to go from i to (1<<i) than the reverse.)
 * See data/classic/units.ruleset for documentation of their effects.
 * Change the array *flag_names[] in unittype.c accordingly.
 */
#define SPECENUM_NAME unit_type_flag_id
// Feel free to take this.
#define SPECENUM_VALUE0 UTYF_RESERVED_2
#define SPECENUM_VALUE0NAME N_("Reserved 2")
// Unit has no ZOC
#define SPECENUM_VALUE1 UTYF_NOZOC
#define SPECENUM_VALUE1NAME N_("?unitflag:HasNoZOC")
#define SPECENUM_VALUE2 UTYF_IGZOC
// TRANS: unit type flag (rarely shown): "ignore zones of control"
#define SPECENUM_VALUE2NAME N_("?unitflag:IgZOC")
#define SPECENUM_VALUE3 UTYF_CIVILIAN
#define SPECENUM_VALUE3NAME N_("?unitflag:NonMil")
#define SPECENUM_VALUE4 UTYF_IGTER
// TRANS: unit type flag (rarely shown): "ignore terrain"
#define SPECENUM_VALUE4NAME N_("?unitflag:IgTer")
#define SPECENUM_VALUE5 UTYF_ONEATTACK
#define SPECENUM_VALUE5NAME N_("?unitflag:OneAttack")
#define SPECENUM_VALUE6 UTYF_FIELDUNIT
#define SPECENUM_VALUE6NAME N_("?unitflag:FieldUnit")
/* autoattack: a unit will choose to attack this unit even if defending
 * against it has better odds. */
#define SPECENUM_VALUE7 UTYF_PROVOKING
#define SPECENUM_VALUE7NAME N_("?unitflag:Provoking")
// Overrides unreachable_protects server setting
#define SPECENUM_VALUE8 UTYF_NEVER_PROTECTS
#define SPECENUM_VALUE8NAME N_("?unitflag:NeverProtects")
// Does not include ability to found cities
#define SPECENUM_VALUE9 UTYF_SETTLERS
#define SPECENUM_VALUE9NAME N_("?unitflag:Settlers")
#define SPECENUM_VALUE10 UTYF_DIPLOMAT
#define SPECENUM_VALUE10NAME N_("?unitflag:Diplomat")
// Can't leave the coast
#define SPECENUM_VALUE11 UTYF_COAST_STRICT
#define SPECENUM_VALUE11NAME N_("?unitflag:CoastStrict")
// Can 'refuel' at coast - meaningless if fuel value not set
#define SPECENUM_VALUE12 UTYF_COAST
#define SPECENUM_VALUE12NAME N_("?unitflag:Coast")
// upkeep can switch from shield to gold
#define SPECENUM_VALUE13 UTYF_SHIELD2GOLD
#define SPECENUM_VALUE13NAME N_("?unitflag:Shield2Gold")
// Strong in diplomatic battles.
#define SPECENUM_VALUE14 UTYF_SPY
#define SPECENUM_VALUE14NAME N_("?unitflag:Spy")
// Cannot attack vs non-native tiles even if class can
#define SPECENUM_VALUE15 UTYF_ONLY_NATIVE_ATTACK
#define SPECENUM_VALUE15NAME N_("?unitflag:Only_Native_Attack")
// Only Fundamentalist government can build these units
#define SPECENUM_VALUE16 UTYF_FANATIC
#define SPECENUM_VALUE16NAME N_("?unitflag:Fanatic")
// Losing this unit means losing the game
#define SPECENUM_VALUE17 UTYF_GAMELOSS
#define SPECENUM_VALUE17NAME N_("?unitflag:GameLoss")
// A player can only have one unit of this type
#define SPECENUM_VALUE18 UTYF_UNIQUE
#define SPECENUM_VALUE18NAME N_("?unitflag:Unique")
/* When a transport containing this unit disappears the game will try to
 * rescue units with this flag before it tries to rescue units without
 * it. */
#define SPECENUM_VALUE19 UTYF_EVAC_FIRST
#define SPECENUM_VALUE19NAME N_("?unitflag:EvacuateFirst")
// Always wins diplomatic contests
#define SPECENUM_VALUE20 UTYF_SUPERSPY
#define SPECENUM_VALUE20NAME N_("?unitflag:SuperSpy")
// Has no homecity
#define SPECENUM_VALUE21 UTYF_NOHOME
#define SPECENUM_VALUE21NAME N_("?unitflag:NoHome")
// Cannot increase veteran level
#define SPECENUM_VALUE22 UTYF_NO_VETERAN
#define SPECENUM_VALUE22NAME N_("?unitflag:NoVeteran")
// Gets double firepower against cities
#define SPECENUM_VALUE23 UTYF_CITYBUSTER
#define SPECENUM_VALUE23NAME N_("?unitflag:CityBuster")
// Unit cannot be built (barb leader etc)
#define SPECENUM_VALUE24 UTYF_NOBUILD
#define SPECENUM_VALUE24NAME N_("?unitflag:NoBuild")
/* Firepower set to 1 when EFT_DEFEND_BONUS applies
 * (for example, land unit attacking city with walls) */
#define SPECENUM_VALUE25 UTYF_BADWALLATTACKER
#define SPECENUM_VALUE25NAME N_("?unitflag:BadWallAttacker")
// Firepower set to 1 and attackers x2 when in city
#define SPECENUM_VALUE26 UTYF_BADCITYDEFENDER
#define SPECENUM_VALUE26NAME N_("?unitflag:BadCityDefender")
// Only barbarians can build this unit
#define SPECENUM_VALUE27 UTYF_BARBARIAN_ONLY
#define SPECENUM_VALUE27NAME N_("?unitflag:BarbarianOnly")
// Feel free to take this.
#define SPECENUM_VALUE28 UTYF_RESERVED_1
#define SPECENUM_VALUE28NAME N_("Reserved 1")
// Unit can't be built in scenarios where founding new cities is prevented.
#define SPECENUM_VALUE29 UTYF_NEWCITY_GAMES_ONLY
#define SPECENUM_VALUE29NAME N_("?unitflag:NewCityGamesOnly")
// Can escape when killstack occours
#define SPECENUM_VALUE30 UTYF_CANESCAPE
#define SPECENUM_VALUE30NAME N_("?unitflag:CanEscape")
// Can kill escaping units
#define SPECENUM_VALUE31 UTYF_CANKILLESCAPING
#define SPECENUM_VALUE31NAME N_("?unitflag:CanKillEscaping")

#define SPECENUM_VALUE32 UTYF_USER_FLAG_1
#define SPECENUM_VALUE33 UTYF_USER_FLAG_2
#define SPECENUM_VALUE34 UTYF_USER_FLAG_3
#define SPECENUM_VALUE35 UTYF_USER_FLAG_4
#define SPECENUM_VALUE36 UTYF_USER_FLAG_5
#define SPECENUM_VALUE37 UTYF_USER_FLAG_6
#define SPECENUM_VALUE38 UTYF_USER_FLAG_7
#define SPECENUM_VALUE39 UTYF_USER_FLAG_8
#define SPECENUM_VALUE40 UTYF_USER_FLAG_9
#define SPECENUM_VALUE41 UTYF_USER_FLAG_10
#define SPECENUM_VALUE42 UTYF_USER_FLAG_11
#define SPECENUM_VALUE43 UTYF_USER_FLAG_12
#define SPECENUM_VALUE44 UTYF_USER_FLAG_13
#define SPECENUM_VALUE45 UTYF_USER_FLAG_14
#define SPECENUM_VALUE46 UTYF_USER_FLAG_15
#define SPECENUM_VALUE47 UTYF_USER_FLAG_16
#define SPECENUM_VALUE48 UTYF_USER_FLAG_17
#define SPECENUM_VALUE49 UTYF_USER_FLAG_18
#define SPECENUM_VALUE50 UTYF_USER_FLAG_19
#define SPECENUM_VALUE51 UTYF_USER_FLAG_20
#define SPECENUM_VALUE52 UTYF_USER_FLAG_21
#define SPECENUM_VALUE53 UTYF_USER_FLAG_22
#define SPECENUM_VALUE54 UTYF_USER_FLAG_23
#define SPECENUM_VALUE55 UTYF_USER_FLAG_24
#define SPECENUM_VALUE56 UTYF_USER_FLAG_25
#define SPECENUM_VALUE57 UTYF_USER_FLAG_26
#define SPECENUM_VALUE58 UTYF_USER_FLAG_27
#define SPECENUM_VALUE59 UTYF_USER_FLAG_28
#define SPECENUM_VALUE60 UTYF_USER_FLAG_29
#define SPECENUM_VALUE61 UTYF_USER_FLAG_30
#define SPECENUM_VALUE62 UTYF_USER_FLAG_31
#define SPECENUM_VALUE63 UTYF_USER_FLAG_32
#define SPECENUM_VALUE64 UTYF_USER_FLAG_33
#define SPECENUM_VALUE65 UTYF_USER_FLAG_34
#define SPECENUM_VALUE66 UTYF_USER_FLAG_35
#define SPECENUM_VALUE67 UTYF_USER_FLAG_36
#define SPECENUM_VALUE68 UTYF_USER_FLAG_37
#define SPECENUM_VALUE69 UTYF_USER_FLAG_38
#define SPECENUM_VALUE70 UTYF_USER_FLAG_39
#define SPECENUM_VALUE71 UTYF_USER_FLAG_40
#define SPECENUM_VALUE72 UTYF_USER_FLAG_41
#define SPECENUM_VALUE73 UTYF_USER_FLAG_42
#define SPECENUM_VALUE74 UTYF_USER_FLAG_43
#define SPECENUM_VALUE75 UTYF_USER_FLAG_44
#define SPECENUM_VALUE76 UTYF_USER_FLAG_45
// Note that first role must have value next to last flag

#define UTYF_LAST_USER_FLAG UTYF_USER_FLAG_45
#define MAX_NUM_USER_UNIT_FLAGS (UTYF_LAST_USER_FLAG - UTYF_USER_FLAG_1 + 1)
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_unit_type_flags
#include "specenum_gen.h"

/**
 * Unit - role types
 */
#define SPECENUM_NAME unit_role_id
// is built first when city established
#define SPECENUM_VALUE77 L_FIRSTBUILD
#define SPECENUM_VALUE77NAME N_("?unitflag:FirstBuild")
// initial explorer unit
#define SPECENUM_VALUE78 L_EXPLORER
#define SPECENUM_VALUE78NAME N_("?unitflag:Explorer")
// can be found in hut
#define SPECENUM_VALUE79 L_HUT
#define SPECENUM_VALUE79NAME N_("?unitflag:Hut")
// can be found in hut, tech required
#define SPECENUM_VALUE80 L_HUT_TECH
#define SPECENUM_VALUE80NAME N_("?unitflag:HutTech")
// is created in Partisan circumstances
#define SPECENUM_VALUE81 L_PARTISAN
#define SPECENUM_VALUE81NAME N_("?unitflag:Partisan")
// ok on defense (AI)
#define SPECENUM_VALUE82 L_DEFEND_OK
#define SPECENUM_VALUE82NAME N_("?unitflag:DefendOk")
// primary purpose is defense (AI)
#define SPECENUM_VALUE83 L_DEFEND_GOOD
#define SPECENUM_VALUE83NAME N_("?unitflag:DefendGood")
// is useful for ferrying (AI)
#define SPECENUM_VALUE84 L_FERRYBOAT
#define SPECENUM_VALUE84NAME N_("?unitflag:FerryBoat")
// barbarians unit, land only
#define SPECENUM_VALUE85 L_BARBARIAN
#define SPECENUM_VALUE85NAME N_("?unitflag:Barbarian")
// barbarians unit, global tech required
#define SPECENUM_VALUE86 L_BARBARIAN_TECH
#define SPECENUM_VALUE86NAME N_("?unitflag:BarbarianTech")
// barbarian boat
#define SPECENUM_VALUE87 L_BARBARIAN_BOAT
#define SPECENUM_VALUE87NAME N_("?unitflag:BarbarianBoat")
// what barbarians should build
#define SPECENUM_VALUE88 L_BARBARIAN_BUILD
#define SPECENUM_VALUE88NAME N_("BarbarianBuild")
// barbarians build when global tech
#define SPECENUM_VALUE89 L_BARBARIAN_BUILD_TECH
#define SPECENUM_VALUE89NAME N_("?unitflag:BarbarianBuildTech")
// barbarian leader
#define SPECENUM_VALUE90 L_BARBARIAN_LEADER
#define SPECENUM_VALUE90NAME N_("?unitflag:BarbarianLeader")
// sea raider unit
#define SPECENUM_VALUE91 L_BARBARIAN_SEA
#define SPECENUM_VALUE91NAME N_("?unitflag:BarbarianSea")
// sea raider unit, global tech required
#define SPECENUM_VALUE92 L_BARBARIAN_SEA_TECH
#define SPECENUM_VALUE92NAME N_("?unitflag:BarbarianSeaTech")
// Startunit: Cities
#define SPECENUM_VALUE93 L_START_CITIES
#define SPECENUM_VALUE93NAME N_("?unitflag:CitiesStartunit")
// Startunit: Worker
#define SPECENUM_VALUE94 L_START_WORKER
#define SPECENUM_VALUE94NAME N_("?unitflag:WorkerStartunit")
// Startunit: Explorer
#define SPECENUM_VALUE95 L_START_EXPLORER
#define SPECENUM_VALUE95NAME N_("?unitflag:ExplorerStartunit")
// Startunit: King
#define SPECENUM_VALUE96 L_START_KING
#define SPECENUM_VALUE96NAME N_("?unitflag:KingStartunit")
// Startunit: Diplomat
#define SPECENUM_VALUE97 L_START_DIPLOMAT
#define SPECENUM_VALUE97NAME N_("?unitflag:DiplomatStartunit")
// Startunit: Ferryboat
#define SPECENUM_VALUE98 L_START_FERRY
#define SPECENUM_VALUE98NAME N_("?unitflag:FerryStartunit")
// Startunit: DefendOk
#define SPECENUM_VALUE99 L_START_DEFEND_OK
#define SPECENUM_VALUE99NAME N_("?unitflag:DefendOkStartunit")
// Startunit: DefendGood
#define SPECENUM_VALUE100 L_START_DEFEND_GOOD
#define SPECENUM_VALUE100NAME N_("?unitflag:DefendGoodStartunit")
// Startunit: AttackFast
#define SPECENUM_VALUE101 L_START_ATTACK_FAST
#define SPECENUM_VALUE101NAME N_("?unitflag:AttackFastStartunit")
// Startunit: AttackStrong
#define SPECENUM_VALUE102 L_START_ATTACK_STRONG
#define SPECENUM_VALUE102NAME N_("?unitflag:AttackStrongStartunit")
// AI hunter type unit
#define SPECENUM_VALUE103 L_HUNTER
#define SPECENUM_VALUE103NAME N_("?unitflag:Hunter")
// can improve terrain
#define SPECENUM_VALUE104 L_SETTLERS
#define SPECENUM_VALUE104NAME N_("?unitflag:Settlers")
#define L_LAST (L_SETTLERS + 1)
#include "specenum_gen.h"

FC_STATIC_ASSERT(L_LAST - L_FIRST <= L_MAX, too_many_unit_roles);

/**
 * Unit - A bitvector of unit type roles
 */
BV_DEFINE(bv_unit_type_roles, L_MAX);

/**
 * Unit - State requirement property types.
 */
#define SPECENUM_NAME ustate_prop
#define SPECENUM_VALUE0 USP_TRANSPORTED
#define SPECENUM_VALUE0NAME "Transported"
#define SPECENUM_VALUE1 USP_LIVABLE_TILE
#define SPECENUM_VALUE1NAME "OnLivableTile"
#define SPECENUM_VALUE2 USP_DOMESTIC_TILE
#define SPECENUM_VALUE2NAME "OnDomesticTile"
#define SPECENUM_VALUE3 USP_TRANSPORTING
#define SPECENUM_VALUE3NAME "Transporting"
#define SPECENUM_VALUE4 USP_HAS_HOME_CITY
#define SPECENUM_VALUE4NAME "HasHomeCity"
#define SPECENUM_VALUE5 USP_NATIVE_TILE
#define SPECENUM_VALUE5NAME "OnNativeTile"
#define SPECENUM_VALUE6 USP_NATIVE_EXTRA
#define SPECENUM_VALUE6NAME "InNativeExtra"
#define SPECENUM_VALUE7 USP_MOVED_THIS_TURN
#define SPECENUM_VALUE7NAME "MovedThisTurn"
#define SPECENUM_COUNT USP_COUNT
#include "specenum_gen.h"

/**
 * Unit - airlifting style types
 */
#define SPECENUM_NAME airlifting_style
#define SPECENUM_BITWISE
// Like classical Freeciv.  One unit per turn.
#define SPECENUM_ZERO AIRLIFTING_CLASSICAL
// Allow airlifting from allied cities.
#define SPECENUM_VALUE0 AIRLIFTING_ALLIED_SRC
// Allow airlifting to allied cities.
#define SPECENUM_VALUE1 AIRLIFTING_ALLIED_DEST
/* Unlimited units to airlift from the source (but always needs an Airport
 * or equivalent). */
#define SPECENUM_VALUE2 AIRLIFTING_UNLIMITED_SRC
/* Unlimited units to airlift to the destination (doesn't require any
 * Airport or equivalent). */
#define SPECENUM_VALUE3 AIRLIFTING_UNLIMITED_DEST
#include "specenum_gen.h"

/**
 * Unit - caravan bonus style types
 */
#define SPECENUM_NAME caravan_bonus_style
#define SPECENUM_VALUE0 CBS_CLASSIC
#define SPECENUM_VALUE0NAME "Classic"
#define SPECENUM_VALUE1 CBS_LOGARITHMIC
#define SPECENUM_VALUE1NAME "Logarithmic"
#define SPECENUM_VALUE2 CBS_LINEAR
#define SPECENUM_VALUE2NAME "Linear"
#define SPECENUM_VALUE3 CBS_DISTANCE
#define SPECENUM_VALUE3NAME "Distance"
#include "specenum_gen.h"

/**
 * Unit - unit wait time types
 */
#define SPECENUM_NAME unitwaittime_style
#define SPECENUM_BITWISE
// Unit wait time only prevents moving.
#define SPECENUM_ZERO UWT_CLASSICAL
#define SPECENUM_ZERONAME "Classical"
// Wait time applies to activities such as building roads and fortresses.
#define SPECENUM_VALUE0 UWT_ACTIVITIES
#define SPECENUM_VALUE0NAME "Activities"
#include "specenum_gen.h"

/**
 * Unit - Action Types
 */
#define SPECENUM_NAME action_actor_kind
#define SPECENUM_VALUE0 AAK_UNIT
#define SPECENUM_VALUE0NAME N_("a unit")
#define SPECENUM_COUNT AAK_COUNT
#include "specenum_gen.h"

#define SPECENUM_NAME action_target_kind
#define SPECENUM_VALUE0 ATK_CITY
#define SPECENUM_VALUE0NAME N_("individual cities")
#define SPECENUM_VALUE1 ATK_UNIT
#define SPECENUM_VALUE1NAME N_("individual units")
#define SPECENUM_VALUE2 ATK_UNITS
#define SPECENUM_VALUE2NAME N_("unit stacks")
#define SPECENUM_VALUE3 ATK_TILE
#define SPECENUM_VALUE3NAME N_("tiles")
// No target except the actor itself.
#define SPECENUM_VALUE4 ATK_SELF
#define SPECENUM_VALUE4NAME N_("itself")
#define SPECENUM_COUNT ATK_COUNT
#include "specenum_gen.h"

// Values used in the network protocol.
#define SPECENUM_NAME action_sub_target_kind
#define SPECENUM_VALUE0 ASTK_NONE
#define SPECENUM_VALUE0NAME N_("nothing")
#define SPECENUM_VALUE1 ASTK_BUILDING
#define SPECENUM_VALUE1NAME N_("buildings in")
#define SPECENUM_VALUE2 ASTK_TECH
#define SPECENUM_VALUE2NAME N_("techs from")
#define SPECENUM_VALUE3 ASTK_EXTRA
#define SPECENUM_VALUE3NAME N_("extras on")
#define SPECENUM_VALUE4 ASTK_EXTRA_NOT_THERE
#define SPECENUM_VALUE4NAME N_("create extras on")
#define SPECENUM_COUNT ASTK_COUNT
#include "specenum_gen.h"

// Values used in the network protocol.
// Names used in file formats but not normally shown to users.
#define SPECENUM_NAME gen_action
#define SPECENUM_VALUE0 ACTION_ESTABLISH_EMBASSY
#define SPECENUM_VALUE0NAME "Establish Embassy"
#define SPECENUM_VALUE1 ACTION_ESTABLISH_EMBASSY_STAY
#define SPECENUM_VALUE1NAME "Establish Embassy Stay"
#define SPECENUM_VALUE2 ACTION_SPY_INVESTIGATE_CITY
#define SPECENUM_VALUE2NAME "Investigate City"
#define SPECENUM_VALUE3 ACTION_INV_CITY_SPEND
#define SPECENUM_VALUE3NAME "Investigate City Spend Unit"
#define SPECENUM_VALUE4 ACTION_SPY_POISON
#define SPECENUM_VALUE4NAME "Poison City"
#define SPECENUM_VALUE5 ACTION_SPY_POISON_ESC
#define SPECENUM_VALUE5NAME "Poison City Escape"
#define SPECENUM_VALUE6 ACTION_SPY_STEAL_GOLD
#define SPECENUM_VALUE6NAME "Steal Gold"
#define SPECENUM_VALUE7 ACTION_SPY_STEAL_GOLD_ESC
#define SPECENUM_VALUE7NAME "Steal Gold Escape"
#define SPECENUM_VALUE8 ACTION_SPY_SABOTAGE_CITY
#define SPECENUM_VALUE8NAME "Sabotage City"
#define SPECENUM_VALUE9 ACTION_SPY_SABOTAGE_CITY_ESC
#define SPECENUM_VALUE9NAME "Sabotage City Escape"
#define SPECENUM_VALUE10 ACTION_SPY_TARGETED_SABOTAGE_CITY
#define SPECENUM_VALUE10NAME "Targeted Sabotage City"
#define SPECENUM_VALUE11 ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC
#define SPECENUM_VALUE11NAME "Targeted Sabotage City Escape"
#define SPECENUM_VALUE12 ACTION_SPY_SABOTAGE_CITY_PRODUCTION
#define SPECENUM_VALUE12NAME "Sabotage City Production"
#define SPECENUM_VALUE13 ACTION_SPY_SABOTAGE_CITY_PRODUCTION_ESC
#define SPECENUM_VALUE13NAME "Sabotage City Production Escape"
#define SPECENUM_VALUE14 ACTION_SPY_STEAL_TECH
#define SPECENUM_VALUE14NAME "Steal Tech"
#define SPECENUM_VALUE15 ACTION_SPY_STEAL_TECH_ESC
#define SPECENUM_VALUE15NAME "Steal Tech Escape Expected"
#define SPECENUM_VALUE16 ACTION_SPY_TARGETED_STEAL_TECH
#define SPECENUM_VALUE16NAME "Targeted Steal Tech"
#define SPECENUM_VALUE17 ACTION_SPY_TARGETED_STEAL_TECH_ESC
#define SPECENUM_VALUE17NAME "Targeted Steal Tech Escape Expected"
#define SPECENUM_VALUE18 ACTION_SPY_INCITE_CITY
#define SPECENUM_VALUE18NAME "Incite City"
#define SPECENUM_VALUE19 ACTION_SPY_INCITE_CITY_ESC
#define SPECENUM_VALUE19NAME "Incite City Escape"
#define SPECENUM_VALUE20 ACTION_TRADE_ROUTE
#define SPECENUM_VALUE20NAME "Establish Trade Route"
#define SPECENUM_VALUE21 ACTION_MARKETPLACE
#define SPECENUM_VALUE21NAME "Enter Marketplace"
#define SPECENUM_VALUE22 ACTION_HELP_WONDER
#define SPECENUM_VALUE22NAME "Help Wonder"
#define SPECENUM_VALUE23 ACTION_SPY_BRIBE_UNIT
#define SPECENUM_VALUE23NAME "Bribe Unit"
#define SPECENUM_VALUE24 ACTION_SPY_SABOTAGE_UNIT
#define SPECENUM_VALUE24NAME "Sabotage Unit"
#define SPECENUM_VALUE25 ACTION_SPY_SABOTAGE_UNIT_ESC
#define SPECENUM_VALUE25NAME "Sabotage Unit Escape"
#define SPECENUM_VALUE26 ACTION_CAPTURE_UNITS
#define SPECENUM_VALUE26NAME "Capture Units"
#define SPECENUM_VALUE27 ACTION_FOUND_CITY
#define SPECENUM_VALUE27NAME "Found City"
#define SPECENUM_VALUE28 ACTION_JOIN_CITY
#define SPECENUM_VALUE28NAME "Join City"
#define SPECENUM_VALUE29 ACTION_STEAL_MAPS
#define SPECENUM_VALUE29NAME "Steal Maps"
#define SPECENUM_VALUE30 ACTION_STEAL_MAPS_ESC
#define SPECENUM_VALUE30NAME "Steal Maps Escape"
#define SPECENUM_VALUE31 ACTION_BOMBARD
#define SPECENUM_VALUE31NAME "Bombard"
#define SPECENUM_VALUE32 ACTION_BOMBARD2
#define SPECENUM_VALUE32NAME "Bombard 2"
#define SPECENUM_VALUE33 ACTION_BOMBARD3
#define SPECENUM_VALUE33NAME "Bombard 3"
#define SPECENUM_VALUE34 ACTION_SPY_NUKE
#define SPECENUM_VALUE34NAME "Suitcase Nuke"
#define SPECENUM_VALUE35 ACTION_SPY_NUKE_ESC
#define SPECENUM_VALUE35NAME "Suitcase Nuke Escape"
#define SPECENUM_VALUE36 ACTION_NUKE
#define SPECENUM_VALUE36NAME "Explode Nuclear"
#define SPECENUM_VALUE37 ACTION_NUKE_CITY
#define SPECENUM_VALUE37NAME "Nuke City"
#define SPECENUM_VALUE38 ACTION_NUKE_UNITS
#define SPECENUM_VALUE38NAME "Nuke Units"
#define SPECENUM_VALUE39 ACTION_DESTROY_CITY
#define SPECENUM_VALUE39NAME "Destroy City"
#define SPECENUM_VALUE40 ACTION_EXPEL_UNIT
#define SPECENUM_VALUE40NAME "Expel Unit"
#define SPECENUM_VALUE41 ACTION_RECYCLE_UNIT
#define SPECENUM_VALUE41NAME "Recycle Unit"
#define SPECENUM_VALUE42 ACTION_DISBAND_UNIT
#define SPECENUM_VALUE42NAME "Disband Unit"
#define SPECENUM_VALUE43 ACTION_HOME_CITY
#define SPECENUM_VALUE43NAME "Home City"
#define SPECENUM_VALUE44 ACTION_UPGRADE_UNIT
#define SPECENUM_VALUE44NAME "Upgrade Unit"
#define SPECENUM_VALUE45 ACTION_PARADROP
#define SPECENUM_VALUE45NAME "Paradrop Unit"
#define SPECENUM_VALUE46 ACTION_AIRLIFT
#define SPECENUM_VALUE46NAME "Airlift Unit"
#define SPECENUM_VALUE47 ACTION_ATTACK
#define SPECENUM_VALUE47NAME "Attack"
#define SPECENUM_VALUE48 ACTION_SUICIDE_ATTACK
#define SPECENUM_VALUE48NAME "Suicide Attack"
#define SPECENUM_VALUE49 ACTION_STRIKE_BUILDING
#define SPECENUM_VALUE49NAME "Surgical Strike Building"
#define SPECENUM_VALUE50 ACTION_STRIKE_PRODUCTION
#define SPECENUM_VALUE50NAME "Surgical Strike Production"
#define SPECENUM_VALUE51 ACTION_CONQUER_CITY
#define SPECENUM_VALUE51NAME "Conquer City"
#define SPECENUM_VALUE52 ACTION_CONQUER_CITY2
#define SPECENUM_VALUE52NAME "Conquer City 2"
#define SPECENUM_VALUE53 ACTION_HEAL_UNIT
#define SPECENUM_VALUE53NAME "Heal Unit"
#define SPECENUM_VALUE54 ACTION_TRANSFORM_TERRAIN
#define SPECENUM_VALUE54NAME "Transform Terrain"
#define SPECENUM_VALUE55 ACTION_CULTIVATE
#define SPECENUM_VALUE55NAME "Cultivate"
#define SPECENUM_VALUE56 ACTION_PLANT
#define SPECENUM_VALUE56NAME "Plant"
#define SPECENUM_VALUE57 ACTION_PILLAGE
#define SPECENUM_VALUE57NAME "Pillage"
#define SPECENUM_VALUE58 ACTION_FORTIFY
#define SPECENUM_VALUE58NAME "Fortify"
#define SPECENUM_VALUE59 ACTION_ROAD
#define SPECENUM_VALUE59NAME "Build Road"
#define SPECENUM_VALUE60 ACTION_CONVERT
#define SPECENUM_VALUE60NAME "Convert Unit"
#define SPECENUM_VALUE61 ACTION_BASE
#define SPECENUM_VALUE61NAME "Build Base"
#define SPECENUM_VALUE62 ACTION_MINE
#define SPECENUM_VALUE62NAME "Build Mine"
#define SPECENUM_VALUE63 ACTION_IRRIGATE
#define SPECENUM_VALUE63NAME "Build Irrigation"
#define SPECENUM_VALUE64 ACTION_CLEAN_POLLUTION
#define SPECENUM_VALUE64NAME "Clean Pollution"
#define SPECENUM_VALUE65 ACTION_CLEAN_FALLOUT
#define SPECENUM_VALUE65NAME "Clean Fallout"
#define SPECENUM_VALUE66 ACTION_TRANSPORT_ALIGHT
#define SPECENUM_VALUE66NAME "Transport Alight"
#define SPECENUM_VALUE67 ACTION_TRANSPORT_UNLOAD
#define SPECENUM_VALUE67NAME "Transport Unload"
#define SPECENUM_VALUE68 ACTION_TRANSPORT_DISEMBARK1
#define SPECENUM_VALUE68NAME "Transport Disembark"
#define SPECENUM_VALUE69 ACTION_TRANSPORT_DISEMBARK2
#define SPECENUM_VALUE69NAME "Transport Disembark 2"
#define SPECENUM_VALUE70 ACTION_TRANSPORT_BOARD
#define SPECENUM_VALUE70NAME "Transport Board"
#define SPECENUM_VALUE71 ACTION_TRANSPORT_EMBARK
#define SPECENUM_VALUE71NAME "Transport Embark"
#define SPECENUM_VALUE72 ACTION_SPY_SPREAD_PLAGUE
#define SPECENUM_VALUE72NAME "Spread Plague"
#define SPECENUM_VALUE73 ACTION_SPY_ATTACK
#define SPECENUM_VALUE73NAME "Spy Attack"
#define SPECENUM_VALUE74 ACTION_USER_ACTION1
#define SPECENUM_VALUE74NAME "User Action 1"
#define SPECENUM_VALUE75 ACTION_USER_ACTION2
#define SPECENUM_VALUE75NAME "User Action 2"
#define SPECENUM_VALUE76 ACTION_USER_ACTION3
#define SPECENUM_VALUE76NAME "User Action 3"
#define SPECENUM_BITVECTOR bv_actions
#define SPECENUM_COUNT ACTION_COUNT
#include "specenum_gen.h"

// Fake action id used in searches to signal "any action at all".
#define ACTION_ANY ACTION_COUNT

// Fake action id used to signal the absence of any actions.
#define ACTION_NONE ACTION_COUNT

// Used in the network protocol.
#define MAX_NUM_ACTIONS ACTION_COUNT
#define NUM_ACTIONS MAX_NUM_ACTIONS

// The reason why an action should be auto performed.
#define SPECENUM_NAME action_auto_perf_cause
// Can't pay the unit's upkeep.
// (Can be triggered by food, shield or gold upkeep)
#define SPECENUM_VALUE0 AAPC_UNIT_UPKEEP
#define SPECENUM_VALUE0NAME N_("Unit Upkeep")
// A unit moved to an adjacent tile (auto attack).
#define SPECENUM_VALUE1 AAPC_UNIT_MOVED_ADJ
#define SPECENUM_VALUE1NAME N_("Moved Adjacent")
// Number of forced action auto performer causes.
#define SPECENUM_COUNT AAPC_COUNT
#include "specenum_gen.h"

/**
 * Unit - combat bonus types
 */
#define SPECENUM_NAME combat_bonus_type
#define SPECENUM_VALUE0 CBONUS_DEFENSE_MULTIPLIER
#define SPECENUM_VALUE0NAME "DefenseMultiplier"
#define SPECENUM_VALUE1 CBONUS_DEFENSE_DIVIDER
#define SPECENUM_VALUE1NAME "DefenseDivider"
#define SPECENUM_VALUE2 CBONUS_FIREPOWER1
#define SPECENUM_VALUE2NAME "Firepower1"
#define SPECENUM_VALUE3 CBONUS_DEFENSE_MULTIPLIER_PCT
#define SPECENUM_VALUE3NAME "DefenseMultiplierPct"
#define SPECENUM_VALUE4 CBONUS_DEFENSE_DIVIDER_PCT
#define SPECENUM_VALUE4NAME "DefenseDividerPct"
#include "specenum_gen.h"

/**
 * Unit - server side data types
 */
#define SPECENUM_NAME unit_ss_data_type
/* The player wants to be reminded to ask what actions the unit can perform
 * to a certain target tile. */
#define SPECENUM_VALUE0 USSDT_QUEUE
/* The player no longer wants the reminder to ask what actions the unit can
 * perform to a certain target tile. */
#define SPECENUM_VALUE1 USSDT_UNQUEUE
/* The player wants to record that the unit now belongs to the specified
 * battle group. */
#define SPECENUM_VALUE2 USSDT_BATTLE_GROUP
#include "specenum_gen.h"

/**
 * Unit - server side agent
 */
#define SPECENUM_NAME server_side_agent
#define SPECENUM_VALUE0 SSA_NONE
#define SPECENUM_VALUE0NAME N_("None")
#define SPECENUM_VALUE1 SSA_AUTOSETTLER
#define SPECENUM_VALUE1NAME N_("Autosettlers")
#define SPECENUM_VALUE2 SSA_AUTOEXPLORE
#define SPECENUM_VALUE2NAME N_("Autoexplore")
#define SPECENUM_COUNT SSA_COUNT
#include "specenum_gen.h"

/**
 * Diplomacy - macros
 *
 * The victim gets a casus belli if EFT_CASUS_BELLI_* is equal to or above
 * CASUS_BELLI_VICTIM. To change this value you must update the
 * documentation of each Casus_Belli_* effect, update existing rulesets and
 * add ruleset compatibility code to server/rscompat.cpp.
 *
 * International outrage: Everyone gets a casus belli if EFT_CASUS_BELLI_*
 * is equal to or above CASUS_BELLI_OUTRAGE. To change this value you must
 * update the documentation of each Casus_Belli_* effect, update existing
 * rulesets and add ruleset compatibility code to server/rscompat.cpp.
 *
 */
#define CASUS_BELLI_VICTIM 1
#define CASUS_BELLI_OUTRAGE 1000

/**
 * Diplomacy - casus belli types
 */

// How "large" a Casus Belli is.
#define SPECENUM_NAME casus_belli_range
// No one gets a Casus Belli.
#define SPECENUM_VALUE0 CBR_NONE
#define SPECENUM_VALUE0NAME N_("No Casus Belli")
// Only the victim player gets a Casus Belli.
#define SPECENUM_VALUE1 CBR_VICTIM_ONLY
#define SPECENUM_VALUE1NAME N_("Victim Casus Belli")
// Every other player, including the victim, gets a Casus Belli.
#define SPECENUM_VALUE2 CBR_INTERNATIONAL_OUTRAGE
#define SPECENUM_VALUE2NAME N_("International Outrage")
#define SPECENUM_COUNT CBR_LAST
#include "specenum_gen.h"

/**
 * Diplomacy - national intelligence types
 */
#define SPECENUM_NAME national_intelligence
#define SPECENUM_VALUE0 NI_MULTIPLIERS
#define SPECENUM_VALUE0NAME N_("Multipliers")
#define SPECENUM_VALUE1 NI_WONDERS
#define SPECENUM_VALUE1NAME N_("Wonders")
#define SPECENUM_VALUE2 NI_SCORE
#define SPECENUM_VALUE2NAME N_("Score")
#define SPECENUM_VALUE3 NI_GOLD
#define SPECENUM_VALUE3NAME N_("Gold")
#define SPECENUM_VALUE4 NI_GOVERNMENT
#define SPECENUM_VALUE4NAME N_("Government")
#define SPECENUM_VALUE5 NI_DIPLOMACY
#define SPECENUM_VALUE5NAME N_("Diplomacy")
#define SPECENUM_VALUE6 NI_TECHS
#define SPECENUM_VALUE6NAME N_("Techs")
#define SPECENUM_VALUE7 NI_TAX_RATES
#define SPECENUM_VALUE7NAME N_("Tax Rates")
#define SPECENUM_VALUE8 NI_CULTURE
#define SPECENUM_VALUE8NAME N_("Culture")
#define SPECENUM_VALUE9 NI_MOOD
#define SPECENUM_VALUE9NAME N_("Mood")
#define SPECENUM_VALUE10 NI_HISTORY
#define SPECENUM_VALUE10NAME N_("History")
#define SPECENUM_COUNT NI_COUNT
#include "specenum_gen.h"

BV_DEFINE(bv_intel_visible, NI_COUNT);

/**
 * Diplomacy - state types (how one player views another).
 * (Some diplomatic states are "pacts" (mutual agreements), others aren't.)
 *
 * Adding to or reordering this array will break many things.
 */
#define SPECENUM_NAME diplstate_type
#define SPECENUM_VALUE0 DS_ARMISTICE
#define SPECENUM_VALUE0NAME N_("?diplomatic_state:Armistice")
#define SPECENUM_VALUE1 DS_WAR
#define SPECENUM_VALUE1NAME N_("?diplomatic_state:War")
#define SPECENUM_VALUE2 DS_CEASEFIRE
#define SPECENUM_VALUE2NAME N_("?diplomatic_state:Cease-fire")
#define SPECENUM_VALUE3 DS_PEACE
#define SPECENUM_VALUE3NAME N_("?diplomatic_state:Peace")
#define SPECENUM_VALUE4 DS_ALLIANCE
#define SPECENUM_VALUE4NAME N_("?diplomatic_state:Alliance")
#define SPECENUM_VALUE5 DS_NO_CONTACT
#define SPECENUM_VALUE5NAME N_("?diplomatic_state:Never met")
#define SPECENUM_VALUE6 DS_TEAM
#define SPECENUM_VALUE6NAME N_("?diplomatic_state:Team")
/* When adding or removing entries, note that first value
 * of diplrel_other should be next to last diplstate_type */
#define SPECENUM_COUNT DS_LAST // leave this last
#include "specenum_gen.h"

/**
 * Diplomacy - treaty clause types
 */
#define SPECENUM_NAME clause_type
#define SPECENUM_VALUE0 CLAUSE_ADVANCE
#define SPECENUM_VALUE0NAME "Advance"
#define SPECENUM_VALUE1 CLAUSE_GOLD
#define SPECENUM_VALUE1NAME "Gold"
#define SPECENUM_VALUE2 CLAUSE_MAP
#define SPECENUM_VALUE2NAME "Map"
#define SPECENUM_VALUE3 CLAUSE_SEAMAP
#define SPECENUM_VALUE3NAME "Seamap"
#define SPECENUM_VALUE4 CLAUSE_CITY
#define SPECENUM_VALUE4NAME "City"
#define SPECENUM_VALUE5 CLAUSE_CEASEFIRE
#define SPECENUM_VALUE5NAME "Ceasefire"
#define SPECENUM_VALUE6 CLAUSE_PEACE
#define SPECENUM_VALUE6NAME "Peace"
#define SPECENUM_VALUE7 CLAUSE_ALLIANCE
#define SPECENUM_VALUE7NAME "Alliance"
#define SPECENUM_VALUE8 CLAUSE_VISION
#define SPECENUM_VALUE8NAME "Vision"
#define SPECENUM_VALUE9 CLAUSE_EMBASSY
#define SPECENUM_VALUE9NAME "Embassy"
#define SPECENUM_COUNT CLAUSE_COUNT
#include "specenum_gen.h"

enum diplomacy_mode {
  DIPLO_FOR_ALL,
  DIPLO_FOR_HUMANS,
  DIPLO_FOR_AIS,
  DIPLO_NO_AIS,
  DIPLO_NO_MIXED,
  DIPLO_FOR_TEAMS,
  DIPLO_DISABLED,
};

/**
 * Player - AI level types
 *
 * NOTE: server/commands.cpp must match these
 */
#define SPECENUM_NAME ai_level
#define SPECENUM_VALUE0 AI_LEVEL_AWAY
#define SPECENUM_VALUE0NAME N_("Away")
#define SPECENUM_VALUE1 AI_LEVEL_HANDICAPPED
#define SPECENUM_VALUE1NAME N_("Handicapped")
#define SPECENUM_VALUE2 AI_LEVEL_NOVICE
#define SPECENUM_VALUE2NAME N_("Novice")
#define SPECENUM_VALUE3 AI_LEVEL_EASY
#define SPECENUM_VALUE3NAME N_("Easy")
#define SPECENUM_VALUE4 AI_LEVEL_NORMAL
#define SPECENUM_VALUE4NAME N_("Normal")
#define SPECENUM_VALUE5 AI_LEVEL_HARD
#define SPECENUM_VALUE5NAME N_("Hard")
#define SPECENUM_VALUE6 AI_LEVEL_CHEATING
#define SPECENUM_VALUE6NAME N_("Cheating")

#ifdef FREECIV_DEBUG
#define SPECENUM_VALUE7 AI_LEVEL_EXPERIMENTAL
#define SPECENUM_VALUE7NAME N_("Experimental")
#endif // FREECIV_DEBUG

#define SPECENUM_COUNT AI_LEVEL_COUNT
#include "specenum_gen.h"

/**
 * Player - AI trait types and macros
 */
#define SPECENUM_NAME trait
#define SPECENUM_VALUE0 TRAIT_EXPANSIONIST
#define SPECENUM_VALUE0NAME "Expansionist"
#define SPECENUM_VALUE1 TRAIT_TRADER
#define SPECENUM_VALUE1NAME "Trader"
#define SPECENUM_VALUE2 TRAIT_AGGRESSIVE
#define SPECENUM_VALUE2NAME "Aggressive"
#define SPECENUM_VALUE3 TRAIT_BUILDER
#define SPECENUM_VALUE3NAME "Builder"
#define SPECENUM_COUNT TRAIT_COUNT
#include "specenum_gen.h"

#define TRAIT_DEFAULT_VALUE 50
#define TRAIT_MAX_VALUE (TRAIT_DEFAULT_VALUE * TRAIT_DEFAULT_VALUE)
#define TRAIT_MAX_VALUE_SR (TRAIT_DEFAULT_VALUE)

/**
 * Player - flag types
 */
#define SPECENUM_NAME plr_flag_id
#define SPECENUM_VALUE0 PLRF_AI
#define SPECENUM_VALUE0NAME "ai"
#define SPECENUM_VALUE1 PLRF_SCENARIO_RESERVED
#define SPECENUM_VALUE1NAME "ScenarioReserved"
#define SPECENUM_COUNT PLRF_COUNT
#define SPECENUM_BITVECTOR bv_plr_flags
#include "specenum_gen.h"

/**
 * Player - barbarian types
 *
 * pplayer->ai.barbarian_type and nations use this enum.
 */
#define SPECENUM_NAME barbarian_type
#define SPECENUM_VALUE0 NOT_A_BARBARIAN
#define SPECENUM_VALUE0NAME "None"
#define SPECENUM_VALUE1 LAND_BARBARIAN
#define SPECENUM_VALUE1NAME "Land"
#define SPECENUM_VALUE2 SEA_BARBARIAN
#define SPECENUM_VALUE2NAME "Sea"
#define SPECENUM_VALUE3 ANIMAL_BARBARIAN
#define SPECENUM_VALUE3NAME "Animal"
#define SPECENUM_VALUE4 LAND_AND_SEA_BARBARIAN
#define SPECENUM_VALUE4NAME "LandAndSea"
#include "specenum_gen.h"

/**
 * Player - mood types
 */
#define SPECENUM_NAME mood_type
#define SPECENUM_VALUE0 MOOD_PEACEFUL
#define SPECENUM_VALUE0NAME "Peaceful"
#define SPECENUM_VALUE1 MOOD_COMBAT
#define SPECENUM_VALUE1NAME "Combat"
#define SPECENUM_COUNT MOOD_COUNT
#include "specenum_gen.h"

/**
 * A bitvector for all player slots.
 */
BV_DEFINE(bv_player, MAX_NUM_PLAYER_SLOTS);

/**
 * City - options types
 *
 * Various city options. These are stored by the server and can be
 * toggled by the user. Each one defaults to off. Adding new ones
 * will break network compatibility. If you want to reorder or remove
 * an option remember to load the city option order from the savegame.
 * It is stored in savefile.city_options_vector
 */
#define SPECENUM_NAME city_options
// If unit production (e.g. settler) is allowed to disband a small city
#define SPECENUM_VALUE0 CITYO_DISBAND
#define SPECENUM_VALUE0NAME "Disband"
// If new citizens are science specialists
#define SPECENUM_VALUE1 CITYO_SCIENCE_SPECIALISTS
#define SPECENUM_VALUE1NAME "Sci_Specialists"
// If new citizens are gold specialists
#define SPECENUM_VALUE2 CITYO_GOLD_SPECIALISTS
#define SPECENUM_VALUE2NAME "Tax_Specialists"
#define SPECENUM_COUNT CITYO_LAST
#define SPECENUM_BITVECTOR bv_city_options
#include "specenum_gen.h"

/**
 * City - Tile requirement types
 */
#define SPECENUM_NAME citytile_type
#define SPECENUM_VALUE0 CITYT_CENTER
#define SPECENUM_VALUE0NAME "Center"
#define SPECENUM_VALUE1 CITYT_CLAIMED
#define SPECENUM_VALUE1NAME "Claimed"
#define SPECENUM_COUNT CITYT_LAST
#include "specenum_gen.h"

/**
 * City - Status requirement types.
 */
#define SPECENUM_NAME citystatus_type
#define SPECENUM_VALUE0 CITYS_OWNED_BY_ORIGINAL
#define SPECENUM_VALUE0NAME "OwnedByOriginal"
#define SPECENUM_COUNT CITYS_LAST
#include "specenum_gen.h"

/**
 * City - capital types
 */
#define SPECENUM_NAME capital_type
#define SPECENUM_VALUE0 CAPITAL_NOT
#define SPECENUM_VALUE0NAME "Not"
#define SPECENUM_VALUE1 CAPITAL_SECONDARY
#define SPECENUM_VALUE1NAME "Secondary"
#define SPECENUM_VALUE2 CAPITAL_PRIMARY
#define SPECENUM_VALUE2NAME "Primary"
#include "specenum_gen.h"

/**
 * City - citizen mood
 *
 * Changing this order will break network compatibility,
 * and clients that don't use the symbols.
 *
 * Used in the network protocol
 */
enum citizen_feeling {
  FEELING_BASE,        // before any of the modifiers below
  FEELING_LUXURY,      // after luxury
  FEELING_EFFECT,      // after building effects
  FEELING_NATIONALITY, // after citizen nationality effects
  FEELING_MARTIAL,     // after units enforce martial order
  FEELING_FINAL,       // after wonders (final result)
  FEELING_LAST
};

/**
 * Tile / Terrain - macros
 *
 * No direction. Understood as the origin tile that a direction would have
 * been relative to.
 */
#define DIR8_ORIGIN direction8_invalid()

/**
 * Some code requires compile time value for number of directions, and
 * cannot use specenum function call direction8_max().
 */
#define DIR8_MAGIC_MAX 8

/**
 * A hard limit on the number of terrains; useful for static arrays.
 */
#define MAX_NUM_TERRAINS (96)

/**
 * Reflect reality; but theoretically could be larger than terrains!
 */
#define MAX_RESOURCE_TYPES (MAX_NUM_TERRAINS / 2)

#define MAX_LEN_MAPDEF 256

/**
 * Tile / Terrain - types
 *
 * The direction8 gives the 8 possible directions. These may be used in
 * a number of ways, for instance as an index into the DIR_DX/DIR_DY
 * arrays. Not all directions may be valid; see is_valid_dir and
 * is_cardinal_dir.
 *
 * The DIR8/direction8 naming system is used to avoid conflict with
 * DIR4/direction4 in client/tilespec.h
 *
 * NOTE: Changing the order of the directions will break network
 * compatability.
 *
 * Some code assumes that the first 4 directions are the reverses of the
 * last 4 (in no particular order). See client/goto.cpp and
 * common/map.cpp:opposite_direction().
 */
#define SPECENUM_NAME direction8
#define SPECENUM_VALUE0 DIR8_NORTHWEST
#define SPECENUM_VALUE0NAME "Northwest"
#define SPECENUM_VALUE1 DIR8_NORTH
#define SPECENUM_VALUE1NAME "North"
#define SPECENUM_VALUE2 DIR8_NORTHEAST
#define SPECENUM_VALUE2NAME "Northeast"
#define SPECENUM_VALUE3 DIR8_WEST
#define SPECENUM_VALUE3NAME "West"
#define SPECENUM_VALUE4 DIR8_EAST
#define SPECENUM_VALUE4NAME "East"
#define SPECENUM_VALUE5 DIR8_SOUTHWEST
#define SPECENUM_VALUE5NAME "Southwest"
#define SPECENUM_VALUE6 DIR8_SOUTH
#define SPECENUM_VALUE6NAME "South"
#define SPECENUM_VALUE7 DIR8_SOUTHEAST
#define SPECENUM_VALUE7NAME "Southeast"
#define SPECENUM_INVALID ((enum direction8)(DIR8_SOUTHEAST + 1))
#include "specenum_gen.h"

/**
 * Tile / Terrain - map image layers
 */
#define SPECENUM_NAME mapimg_layer
#define SPECENUM_VALUE0 MAPIMG_LAYER_AREA
#define SPECENUM_VALUE0NAME "a"
#define SPECENUM_VALUE1 MAPIMG_LAYER_BORDERS
#define SPECENUM_VALUE1NAME "b"
#define SPECENUM_VALUE2 MAPIMG_LAYER_CITIES
#define SPECENUM_VALUE2NAME "c"
#define SPECENUM_VALUE3 MAPIMG_LAYER_FOGOFWAR
#define SPECENUM_VALUE3NAME "f"
#define SPECENUM_VALUE4 MAPIMG_LAYER_KNOWLEDGE
#define SPECENUM_VALUE4NAME "k"
#define SPECENUM_VALUE5 MAPIMG_LAYER_TERRAIN
#define SPECENUM_VALUE5NAME "t"
#define SPECENUM_VALUE6 MAPIMG_LAYER_UNITS
#define SPECENUM_VALUE6NAME "u"
// used a possible dummy value
#define SPECENUM_COUNT MAPIMG_LAYER_COUNT
#define SPECENUM_COUNTNAME "-"
#include "specenum_gen.h"

/**
 * Tile / Terrain - reveal map
 */
#define SPECENUM_NAME reveal_map
#define SPECENUM_BITWISE
// Reveal only the area around the first units at the beginning.
#define SPECENUM_ZERO REVEAL_MAP_NONE
// Reveal the (fogged) map at the beginning of the game.
#define SPECENUM_VALUE0 REVEAL_MAP_START
// Reveal (and unfog) the map for dead players.
#define SPECENUM_VALUE1 REVEAL_MAP_DEAD
#include "specenum_gen.h"

/**
 * Tile / Terrain - fog of war types
 */
enum known_type {
  TILE_UNKNOWN = 0,
  TILE_KNOWN_UNSEEN = 1,
  TILE_KNOWN_SEEN = 2,
};

/**
 * Tile / Terrain - extra types
 */
#define SPECENUM_NAME extra_category
#define SPECENUM_VALUE0 ECAT_INFRA
#define SPECENUM_VALUE0NAME "Infra"
#define SPECENUM_VALUE1 ECAT_NATURAL
#define SPECENUM_VALUE1NAME "Natural"
#define SPECENUM_VALUE2 ECAT_NUISANCE
#define SPECENUM_VALUE2NAME "Nuisance"
#define SPECENUM_VALUE3 ECAT_BONUS
#define SPECENUM_VALUE3NAME "Bonus"
#define SPECENUM_VALUE4 ECAT_RESOURCE
#define SPECENUM_VALUE4NAME "Resource"
#define SPECENUM_COUNT ECAT_COUNT
#include "specenum_gen.h"
#define ECAT_NONE ECAT_COUNT

/**
 * Tile / Terrain - tile extra cause types
 */
#define SPECENUM_NAME extra_cause
#define SPECENUM_VALUE0 EC_IRRIGATION
#define SPECENUM_VALUE0NAME "Irrigation"
#define SPECENUM_VALUE1 EC_MINE
#define SPECENUM_VALUE1NAME "Mine"
#define SPECENUM_VALUE2 EC_ROAD
#define SPECENUM_VALUE2NAME "Road"
#define SPECENUM_VALUE3 EC_BASE
#define SPECENUM_VALUE3NAME "Base"
#define SPECENUM_VALUE4 EC_POLLUTION
#define SPECENUM_VALUE4NAME "Pollution"
#define SPECENUM_VALUE5 EC_FALLOUT
#define SPECENUM_VALUE5NAME "Fallout"
#define SPECENUM_VALUE6 EC_HUT
#define SPECENUM_VALUE6NAME "Hut"
#define SPECENUM_VALUE7 EC_APPEARANCE
#define SPECENUM_VALUE7NAME "Appear"
#define SPECENUM_VALUE8 EC_RESOURCE
#define SPECENUM_VALUE8NAME "Resource"
#define SPECENUM_COUNT EC_COUNT
#define SPECENUM_BITVECTOR bv_causes
#include "specenum_gen.h"
#define EC_NONE EC_COUNT
#define EC_SPECIAL ((enum extra_cause)(EC_NONE + 1))
#define EC_DEFENSIVE ((enum extra_cause)(EC_NONE + 2))
#define EC_NATURAL_DEFENSIVE ((enum extra_cause)(EC_NONE + 3))
#define EC_LAST ((enum extra_cause)(EC_NONE + 4))

// struct extra_type reserve 16 bits (0-15) for these.
FC_STATIC_ASSERT(EC_COUNT < 16, extra_causes_over_limit);

/**
 * Tile / Terrain - tile extra remove cause types
 */
#define SPECENUM_NAME extra_rmcause
#define SPECENUM_VALUE0 ERM_PILLAGE
#define SPECENUM_VALUE0NAME "Pillage"
#define SPECENUM_VALUE1 ERM_CLEANPOLLUTION
#define SPECENUM_VALUE1NAME "CleanPollution"
#define SPECENUM_VALUE2 ERM_CLEANFALLOUT
#define SPECENUM_VALUE2NAME "CleanFallout"
#define SPECENUM_VALUE3 ERM_DISAPPEARANCE
#define SPECENUM_VALUE3NAME "Disappear"
#define SPECENUM_VALUE4 ERM_ENTER
#define SPECENUM_VALUE4NAME "Enter"
#define SPECENUM_COUNT ERM_COUNT
#define SPECENUM_BITVECTOR bv_rmcauses
#include "specenum_gen.h"
#define ERM_NONE ERM_COUNT

// struct extra_type reserve 8 bits (0-7) for these.
FC_STATIC_ASSERT(ERM_COUNT < 8, extra_rmcauses_over_limit);

/**
 * Tile / Terrain - tile unit seen types
 */
#define SPECENUM_NAME extra_unit_seen_type
#define SPECENUM_VALUE0 EUS_NORMAL
#define SPECENUM_VALUE0NAME "Normal"
#define SPECENUM_VALUE1 EUS_HIDDEN
#define SPECENUM_VALUE1NAME "Hidden"
#include "specenum_gen.h"

/**
 * Tile / Terrain - flags
 */
#define SPECENUM_NAME terrain_flag_id
// No barbarians summoned on this terrain.
#define SPECENUM_VALUE0 TER_NO_BARBS
/* TRANS: this and following strings are 'terrain flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("NoBarbs")
// No cities on this terrain.
#define SPECENUM_VALUE1 TER_NO_CITIES
#define SPECENUM_VALUE1NAME N_("NoCities")
// Players will start on this terrain type.
#define SPECENUM_VALUE2 TER_STARTER
#define SPECENUM_VALUE2NAME N_("Starter")
// Terrains with this type can have road with "River" flag on them.
#define SPECENUM_VALUE3 TER_CAN_HAVE_RIVER
#define SPECENUM_VALUE3NAME N_("CanHaveRiver")
/*this tile is not safe as coast, (all ocean / ice) */
#define SPECENUM_VALUE4 TER_UNSAFE_COAST
#define SPECENUM_VALUE4NAME N_("UnsafeCoast")
// Fresh water terrain
#define SPECENUM_VALUE5 TER_FRESHWATER
#define SPECENUM_VALUE5NAME N_("FreshWater")
// Map generator does not place this terrain
#define SPECENUM_VALUE6 TER_NOT_GENERATED
#define SPECENUM_VALUE6NAME N_("NotGenerated")
// Units on this terrain are not generating or subject to zoc
#define SPECENUM_VALUE7 TER_NO_ZOC
#define SPECENUM_VALUE7NAME N_("NoZoc")
// Ice-covered terrain (affects minimap)
#define SPECENUM_VALUE8 TER_FROZEN
#define SPECENUM_VALUE8NAME N_("Frozen")
#define SPECENUM_VALUE9 TER_USER_1
#define SPECENUM_VALUE10 TER_USER_2
#define SPECENUM_VALUE11 TER_USER_3
#define SPECENUM_VALUE12 TER_USER_4
#define SPECENUM_VALUE13 TER_USER_5
#define SPECENUM_VALUE14 TER_USER_6
#define SPECENUM_VALUE15 TER_USER_7
#define SPECENUM_VALUE16 TER_USER_8
#define SPECENUM_VALUE17 TER_USER_9
#define SPECENUM_VALUE18 TER_USER_LAST
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_terrain_flags
#include "specenum_gen.h"

/**
 * Tile / Terrain - extra flags
 */
#define SPECENUM_NAME extra_flag_id
// Tile with this extra is considered native for units in tile.
#define SPECENUM_VALUE0 EF_NATIVE_TILE
#define SPECENUM_VALUE0NAME N_("?extraflag:NativeTile")
// Refuel native units
#define SPECENUM_VALUE1 EF_REFUEL
#define SPECENUM_VALUE1NAME N_("?extraflag:Refuel")
#define SPECENUM_VALUE2 EF_TERR_CHANGE_REMOVES
#define SPECENUM_VALUE2NAME N_("?extraflag:TerrChangeRemoves")
// Extra will be built in cities automatically
#define SPECENUM_VALUE3 EF_AUTO_ON_CITY_CENTER
#define SPECENUM_VALUE3NAME N_("?extraflag:AutoOnCityCenter")
// Extra is always present in cities
#define SPECENUM_VALUE4 EF_ALWAYS_ON_CITY_CENTER
#define SPECENUM_VALUE4NAME N_("?extraflag:AlwaysOnCityCenter")
// Road style gfx from ocean extra connects to nearby land
#define SPECENUM_VALUE5 EF_CONNECT_LAND
#define SPECENUM_VALUE5NAME N_("?extraflag:ConnectLand")
// Counts towards Global Warming
#define SPECENUM_VALUE6 EF_GLOBAL_WARMING
#define SPECENUM_VALUE6NAME N_("?extraflag:GlobalWarming")
// Counts towards Nuclear Winter
#define SPECENUM_VALUE7 EF_NUCLEAR_WINTER
#define SPECENUM_VALUE7NAME N_("?extraflag:NuclearWinter")
// Owner's flag will be shown on the tile
#define SPECENUM_VALUE8 EF_SHOW_FLAG
#define SPECENUM_VALUE8NAME N_("?extraflag:ShowFlag")
/* Extra's defense bonus will be counted to
 * separate "Natural" defense layer. */
#define SPECENUM_VALUE9 EF_NATURAL_DEFENSE
#define SPECENUM_VALUE9NAME N_("?extraflag:NaturalDefense")
// Units inside will not die all at once
#define SPECENUM_VALUE10 EF_NO_STACK_DEATH
#define SPECENUM_VALUE10NAME N_("NoStackDeath")

#define SPECENUM_VALUE11 EF_USER_FLAG_1
#define SPECENUM_VALUE12 EF_USER_FLAG_2
#define SPECENUM_VALUE13 EF_USER_FLAG_3
#define SPECENUM_VALUE14 EF_USER_FLAG_4
#define SPECENUM_VALUE15 EF_USER_FLAG_5
#define SPECENUM_VALUE16 EF_USER_FLAG_6
#define SPECENUM_VALUE17 EF_USER_FLAG_7
#define SPECENUM_VALUE18 EF_USER_FLAG_8

#define SPECENUM_COUNT EF_COUNT
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_extra_flags
#include "specenum_gen.h"

/**
 * Tile / Terrain - vision types
 */
#define SPECENUM_NAME vision_layer
#define SPECENUM_VALUE0 V_MAIN
#define SPECENUM_VALUE0NAME N_("Main")
#define SPECENUM_VALUE1 V_INVIS
#define SPECENUM_VALUE1NAME N_("Stealth")
#define SPECENUM_VALUE2 V_SUBSURFACE
#define SPECENUM_VALUE2NAME N_("Subsurface")
#define SPECENUM_COUNT V_COUNT
#include "specenum_gen.h"

/**
 * Tile / Terrain - topography types
 *
 * Changing these values will break map_init_topology.
 * Changing the names will break file format compatibility.
 */
#define SPECENUM_NAME topo_flag
#define SPECENUM_BITWISE
#define SPECENUM_VALUE0 TF_WRAPX
#define SPECENUM_VALUE0NAME N_("WrapX")
#define SPECENUM_VALUE1 TF_WRAPY
#define SPECENUM_VALUE1NAME N_("WrapY")
#define SPECENUM_VALUE2 TF_ISO
#define SPECENUM_VALUE2NAME N_("ISO")
#define SPECENUM_VALUE3 TF_HEX
#define SPECENUM_VALUE3NAME N_("Hex")
#define TOPO_FLAG_BITS 4
#include "specenum_gen.h"

/**
 * Tile / Terrain - base types
 */
#define SPECENUM_NAME base_gui_type
#define SPECENUM_VALUE0 BASE_GUI_FORTRESS
#define SPECENUM_VALUE0NAME "Fortress"
#define SPECENUM_VALUE1 BASE_GUI_AIRBASE
#define SPECENUM_VALUE1NAME "Airbase"
#define SPECENUM_VALUE2 BASE_GUI_OTHER
#define SPECENUM_VALUE2NAME "Other"
#include "specenum_gen.h"

// Used in the network protocol.
#define SPECENUM_NAME base_flag_id
// Unit inside are not considered aggressive if base is close to city
#define SPECENUM_VALUE0 BF_NOT_AGGRESSIVE
/* TRANS: this and following strings are 'base flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("NoAggressive")

#define SPECENUM_COUNT BF_COUNT
#define SPECENUM_BITVECTOR bv_base_flags
#include "specenum_gen.h"

/**
 * Tile / Terrain - road move mode Flag
 */
#define SPECENUM_NAME road_move_mode
#define SPECENUM_VALUE0 RMM_CARDINAL
#define SPECENUM_VALUE0NAME "Cardinal"
#define SPECENUM_VALUE1 RMM_RELAXED
#define SPECENUM_VALUE1NAME "Relaxed"
#define SPECENUM_VALUE2 RMM_FAST_ALWAYS
#define SPECENUM_VALUE2NAME "FastAlways"
#include "specenum_gen.h"

// Used in the network protocol.
#define SPECENUM_NAME road_flag_id
#define SPECENUM_VALUE0 RF_RIVER
/* TRANS: this and following strings are 'road flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("River")
#define SPECENUM_VALUE1 RF_UNRESTRICTED_INFRA
#define SPECENUM_VALUE1NAME N_("UnrestrictedInfra")
#define SPECENUM_VALUE2 RF_JUMP_FROM
#define SPECENUM_VALUE2NAME N_("JumpFrom")
#define SPECENUM_VALUE3 RF_JUMP_TO
#define SPECENUM_VALUE3NAME N_("JumpTo")
#define SPECENUM_COUNT RF_COUNT
#define SPECENUM_BITVECTOR bv_road_flags
#include "specenum_gen.h"

/**
 * Tile / Terrain - Road type compatibility with old specials based roads.
 */
enum road_compat { ROCO_ROAD, ROCO_RAILROAD, ROCO_RIVER, ROCO_NONE };

/**
 * Tile / Terrain - border mode types
 */
enum borders_mode {
  BORDERS_DISABLED = 0,
  BORDERS_ENABLED,
  BORDERS_SEE_INSIDE,
  BORDERS_EXPAND
};

/**
 * Tile / Terrain - global warming / nuclear winter types
 */
enum environment_upset_type { EUT_GLOBAL_WARMING = 0, EUT_NUCLEAR_WINTER };

/**
 * Tile / Terrain - happy border types
 */
enum happyborders_type { HB_DISABLED = 0, HB_NATIONAL, HB_ALLIANCE };

/**
 * Game - macros
 */

// Phase mode change has changed meaning of the phase numbers
#define PHASE_INVALIDATED -1
// Phase was never known
#define PHASE_UNKNOWN -2

/**
 * Game - loss style types
 */
#define SPECENUM_NAME gameloss_style
#define SPECENUM_BITWISE
// Like classical Freeciv. No special effects.
#define SPECENUM_ZERO GAMELOSS_STYLE_CLASSICAL
// Remaining cities are taken by barbarians.
#define SPECENUM_VALUE0 GAMELOSS_STYLE_BARB
#define SPECENUM_VALUE0NAME "Barbarians"
// Try civil war.
#define SPECENUM_VALUE1 GAMELOSS_STYLE_CWAR
#define SPECENUM_VALUE1NAME "CivilWar"
// Do some looting
#define SPECENUM_VALUE2 GAMELOSS_STYLE_LOOT
#define SPECENUM_VALUE2NAME "Loot"
#include "specenum_gen.h"

/**
 * Game - Numerical values used in savegames
 */
#define SPECENUM_NAME phase_mode_type
#define SPECENUM_VALUE0 PMT_CONCURRENT
#define SPECENUM_VALUE0NAME "Concurrent"
#define SPECENUM_VALUE1 PMT_PLAYERS_ALTERNATE
#define SPECENUM_VALUE1NAME "Players Alternate"
#define SPECENUM_VALUE2 PMT_TEAMS_ALTERNATE
#define SPECENUM_VALUE2NAME "Teams Alternate"
#include "specenum_gen.h"

/**
 * Game - achievement types
 */
#define SPECENUM_NAME achievement_type
#define SPECENUM_VALUE0 ACHIEVEMENT_SPACESHIP
#define SPECENUM_VALUE0NAME "Spaceship"
#define SPECENUM_VALUE1 ACHIEVEMENT_MAP
#define SPECENUM_VALUE1NAME "Map_Known"
#define SPECENUM_VALUE2 ACHIEVEMENT_MULTICULTURAL
#define SPECENUM_VALUE2NAME "Multicultural"
#define SPECENUM_VALUE3 ACHIEVEMENT_CULTURED_CITY
#define SPECENUM_VALUE3NAME "Cultured_City"
#define SPECENUM_VALUE4 ACHIEVEMENT_CULTURED_NATION
#define SPECENUM_VALUE4NAME "Cultured_Nation"
#define SPECENUM_VALUE5 ACHIEVEMENT_LUCKY
#define SPECENUM_VALUE5NAME "Lucky"
#define SPECENUM_VALUE6 ACHIEVEMENT_HUTS
#define SPECENUM_VALUE6NAME "Huts"
#define SPECENUM_VALUE7 ACHIEVEMENT_METROPOLIS
#define SPECENUM_VALUE7NAME "Metropolis"
#define SPECENUM_VALUE8 ACHIEVEMENT_LITERATE
#define SPECENUM_VALUE8NAME "Literate"
#define SPECENUM_VALUE9 ACHIEVEMENT_LAND_AHOY
#define SPECENUM_VALUE9NAME "Land_Ahoy"
#define SPECENUM_COUNT ACHIEVEMENT_COUNT
#include "specenum_gen.h"

/**
 * Game - gold upkeep style types
 */
#define SPECENUM_NAME gold_upkeep_style
#define SPECENUM_VALUE0 GOLD_UPKEEP_CITY
#define SPECENUM_VALUE0NAME "City"
#define SPECENUM_VALUE1 GOLD_UPKEEP_MIXED
#define SPECENUM_VALUE1NAME "Mixed"
#define SPECENUM_VALUE2 GOLD_UPKEEP_NATION
#define SPECENUM_VALUE2NAME "Nation"
#include "specenum_gen.h"

/**
 * Game - victory conditions
 */
enum victory_condition_type { VC_SPACERACE = 0, VC_ALLIED, VC_CULTURE };

/**
 * Game - revolution types
 */
enum revolen_type {
  REVOLEN_FIXED = 0,
  REVOLEN_RANDOM,
  REVOLEN_QUICKENING,
  REVOLEN_RANDQUICK
};

/**
 * Traderoutes - macros
 */

// Maximum number of trade routes a city can have in any situation.
#define MAX_TRADE_ROUTES 5

/**
 * Traderoutes - revenue stye types
 */
#define SPECENUM_NAME trade_revenue_style
#define SPECENUM_VALUE0 TRS_CLASSIC
#define SPECENUM_VALUE0NAME "Classic"
#define SPECENUM_VALUE1 TRS_SIMPLE
#define SPECENUM_VALUE1NAME "Simple"
#include "specenum_gen.h"

/**
 * Traderoutes - direction types
 */
#define SPECENUM_NAME route_direction
#define SPECENUM_VALUE0 RDIR_FROM
#define SPECENUM_VALUE0NAME N_("?routedir:From")
#define SPECENUM_VALUE1 RDIR_TO
#define SPECENUM_VALUE1NAME N_("?routedir:To")
#define SPECENUM_VALUE2 RDIR_BIDIRECTIONAL
#define SPECENUM_VALUE2NAME N_("?routedir:Bidirectional")
#include "specenum_gen.h"

/**
 * Traderoutes - good selection method types
 */
#define SPECENUM_NAME goods_selection_method
#define SPECENUM_VALUE0 GSM_LEAVING
#define SPECENUM_VALUE0NAME "Leaving"
#define SPECENUM_VALUE1 GSM_ARRIVAL
#define SPECENUM_VALUE1NAME "Arrival"
#include "specenum_gen.h"

/**
 * Traderoutes - good flags
 */
#define SPECENUM_NAME goods_flag_id
#define SPECENUM_VALUE0 GF_BIDIRECTIONAL
#define SPECENUM_VALUE0NAME "Bidirectional"
#define SPECENUM_VALUE1 GF_DEPLETES
#define SPECENUM_VALUE1NAME "Depletes"
#define SPECENUM_COUNT GF_COUNT
#define SPECENUM_BITVECTOR bv_goods_flags
#include "specenum_gen.h"

/**
 * Traderoutes - bonus type flags
 */
#define SPECENUM_NAME traderoute_bonus_type
#define SPECENUM_VALUE0 TBONUS_NONE
#define SPECENUM_VALUE0NAME "None"
#define SPECENUM_VALUE1 TBONUS_GOLD
#define SPECENUM_VALUE1NAME "Gold"
#define SPECENUM_VALUE2 TBONUS_SCIENCE
#define SPECENUM_VALUE2NAME "Science"
#define SPECENUM_VALUE3 TBONUS_BOTH
#define SPECENUM_VALUE3NAME "Both"
#include "specenum_gen.h"

/**
 * Traderoutes - What to do with previously established traderoutes that are
 * now illegal.
 */
enum traderoute_illegal_cancelling {
  TRI_ACTIVE = 0,   // Keep them active
  TRI_INACTIVE = 1, // They are inactive
  TRI_CANCEL = 2,   // Completely cancel them
  TRI_LAST = 3
};

/**
 * Action - probability
 *
 * An action probability is the probability that an action will be
 * successful under the given circumstances. It is an interval that
 * includes the end points. An end point goes from 0% to 100%.
 * Alternatively it can signal a special case.
 *
 * End point values from 0 up to and including 200 should be understood as
 * the chance of success measured in half percentage points. In other words:
 * The value 3 indicates that the chance is 1.5%. The value 10 indicates
 * that the chance is 5%. The probability of a minimum may be rounded down
 * to the nearest half percentage point. The probability of a maximum may
 * be rounded up to the nearest half percentage point.
 *
 * Values with a higher minimum than maximum are special case values. All
 * special cases should be declared and documented below. An undocumented
 * value in this range should be considered a bug. If a special value for
 * internal use is needed please avoid the range from and including 0 up
 * to and including 255.
 *
 * [0, 0]     ACTPROB_IMPOSSIBLE is another way of saying that the
 *            probability is 0%. It isn't really a special value since it
 *            is in range.
 *
 * [200, 200] ACTPROB_CERTAIN is another way of saying that the probability
 *            is 100%. It isn't really a special value since it is in range.
 *
 * [253, 0]   ACTPROB_NA indicates that no probability should exist.
 *
 * [254, 0]   ACTPROB_NOT_IMPLEMENTED indicates that support for finding
 *            this probability currently is missing.
 *
 * [0, 200]   ACTPROB_NOT_KNOWN indicates that the player don't know enough
 *            to find out. It is caused by the probability depending on a
 *            rule that depends on game state the player don't have access
 *            to. It may be possible for the player to later gain access to
 *            this game state. It isn't really a special value since it is
 *            in range.
 */
struct act_prob {
  int min;
  int max;
};

/**
 * Action - decision types
 */
#define SPECENUM_NAME action_decision
// Doesn't need the player to decide what action to take.
#define SPECENUM_VALUE0 ACT_DEC_NOTHING
#define SPECENUM_VALUE0NAME N_("nothing")
// Wants a decision because of something done to the actor.
#define SPECENUM_VALUE1 ACT_DEC_PASSIVE
#define SPECENUM_VALUE1NAME N_("passive")
// Wants a decision because of something the actor did.
#define SPECENUM_VALUE2 ACT_DEC_ACTIVE
#define SPECENUM_VALUE2NAME N_("active")
#define SPECENUM_COUNT ACT_DEC_COUNT
#include "specenum_gen.h"

// Spaceship - macro
#define NUM_SS_STRUCTURALS 32

// Spaceship - component types
enum spaceship_place_type {
  SSHIP_PLACE_STRUCTURAL,
  SSHIP_PLACE_FUEL,
  SSHIP_PLACE_PROPULSION,
  SSHIP_PLACE_HABITATION,
  SSHIP_PLACE_LIFE_SUPPORT,
  SSHIP_PLACE_SOLAR_PANELS
};

// A bitvector of spaceship structural components
BV_DEFINE(bv_spaceship_structure, NUM_SS_STRUCTURALS);

/**
 * Events - event types
 */
#define SPECENUM_NAME event_type
#define SPECENUM_VALUE0 E_CITY_CANTBUILD
#define SPECENUM_VALUE1 E_CITY_LOST
#define SPECENUM_VALUE2 E_CITY_LOVE
#define SPECENUM_VALUE3 E_CITY_DISORDER
#define SPECENUM_VALUE4 E_CITY_FAMINE
#define SPECENUM_VALUE5 E_CITY_FAMINE_FEARED
#define SPECENUM_VALUE6 E_CITY_GROWTH
#define SPECENUM_VALUE7 E_CITY_MAY_SOON_GROW
#define SPECENUM_VALUE8 E_CITY_IMPROVEMENT
#define SPECENUM_VALUE9 E_CITY_IMPROVEMENT_BLDG
#define SPECENUM_VALUE10 E_CITY_NORMAL
#define SPECENUM_VALUE11 E_CITY_NUKED
#define SPECENUM_VALUE12 E_CITY_CMA_RELEASE
#define SPECENUM_VALUE13 E_CITY_GRAN_THROTTLE
#define SPECENUM_VALUE14 E_CITY_TRANSFER
#define SPECENUM_VALUE15 E_CITY_BUILD
#define SPECENUM_VALUE16 E_CITY_PRODUCTION_CHANGED
#define SPECENUM_VALUE17 E_WORKLIST
#define SPECENUM_VALUE18 E_UPRISING
#define SPECENUM_VALUE19 E_CIVIL_WAR
#define SPECENUM_VALUE20 E_ANARCHY
#define SPECENUM_VALUE21 E_FIRST_CONTACT
#define SPECENUM_VALUE22 E_NEW_GOVERNMENT
#define SPECENUM_VALUE23 E_LOW_ON_FUNDS
#define SPECENUM_VALUE24 E_POLLUTION
#define SPECENUM_VALUE25 E_REVOLT_DONE
#define SPECENUM_VALUE26 E_REVOLT_START
#define SPECENUM_VALUE27 E_SPACESHIP
#define SPECENUM_VALUE28 E_MY_DIPLOMAT_BRIBE
#define SPECENUM_VALUE29 E_DIPLOMATIC_INCIDENT
#define SPECENUM_VALUE30 E_MY_DIPLOMAT_ESCAPE
#define SPECENUM_VALUE31 E_MY_DIPLOMAT_EMBASSY
#define SPECENUM_VALUE32 E_MY_DIPLOMAT_FAILED
#define SPECENUM_VALUE33 E_MY_DIPLOMAT_INCITE
#define SPECENUM_VALUE34 E_MY_DIPLOMAT_POISON
#define SPECENUM_VALUE35 E_MY_DIPLOMAT_SABOTAGE
#define SPECENUM_VALUE36 E_MY_DIPLOMAT_THEFT
#define SPECENUM_VALUE37 E_ENEMY_DIPLOMAT_BRIBE
#define SPECENUM_VALUE38 E_ENEMY_DIPLOMAT_EMBASSY
#define SPECENUM_VALUE39 E_ENEMY_DIPLOMAT_FAILED
#define SPECENUM_VALUE40 E_ENEMY_DIPLOMAT_INCITE
#define SPECENUM_VALUE41 E_ENEMY_DIPLOMAT_POISON
#define SPECENUM_VALUE42 E_ENEMY_DIPLOMAT_SABOTAGE
#define SPECENUM_VALUE43 E_ENEMY_DIPLOMAT_THEFT
#define SPECENUM_VALUE44 E_CARAVAN_ACTION
#define SPECENUM_VALUE45 E_SCRIPT
#define SPECENUM_VALUE46 E_BROADCAST_REPORT
#define SPECENUM_VALUE47 E_GAME_END
#define SPECENUM_VALUE48 E_GAME_START
#define SPECENUM_VALUE49 E_NATION_SELECTED
#define SPECENUM_VALUE50 E_DESTROYED
#define SPECENUM_VALUE51 E_REPORT
#define SPECENUM_VALUE52 E_TURN_BELL
#define SPECENUM_VALUE53 E_NEXT_YEAR
#define SPECENUM_VALUE54 E_GLOBAL_ECO
#define SPECENUM_VALUE55 E_NUKE
#define SPECENUM_VALUE56 E_HUT_BARB
#define SPECENUM_VALUE57 E_HUT_CITY
#define SPECENUM_VALUE58 E_HUT_GOLD
#define SPECENUM_VALUE59 E_HUT_BARB_KILLED
#define SPECENUM_VALUE60 E_HUT_MERC
#define SPECENUM_VALUE61 E_HUT_SETTLER
#define SPECENUM_VALUE62 E_HUT_TECH
#define SPECENUM_VALUE63 E_HUT_BARB_CITY_NEAR
#define SPECENUM_VALUE64 E_IMP_BUY
#define SPECENUM_VALUE65 E_IMP_BUILD
#define SPECENUM_VALUE66 E_IMP_AUCTIONED
#define SPECENUM_VALUE67 E_IMP_AUTO
#define SPECENUM_VALUE68 E_IMP_SOLD
#define SPECENUM_VALUE69 E_TECH_GAIN
#define SPECENUM_VALUE70 E_TECH_LEARNED
#define SPECENUM_VALUE71 E_TREATY_ALLIANCE
#define SPECENUM_VALUE72 E_TREATY_BROKEN
#define SPECENUM_VALUE73 E_TREATY_CEASEFIRE
#define SPECENUM_VALUE74 E_TREATY_PEACE
#define SPECENUM_VALUE75 E_TREATY_SHARED_VISION
#define SPECENUM_VALUE76 E_UNIT_LOST_ATT
#define SPECENUM_VALUE77 E_UNIT_WIN_ATT
#define SPECENUM_VALUE78 E_UNIT_BUY
#define SPECENUM_VALUE79 E_UNIT_BUILT
#define SPECENUM_VALUE80 E_UNIT_LOST_DEF
#define SPECENUM_VALUE81 E_UNIT_WIN_DEF
#define SPECENUM_VALUE82 E_UNIT_BECAME_VET
#define SPECENUM_VALUE83 E_UNIT_UPGRADED
#define SPECENUM_VALUE84 E_UNIT_RELOCATED
#define SPECENUM_VALUE85 E_UNIT_ORDERS
#define SPECENUM_VALUE86 E_WONDER_BUILD
#define SPECENUM_VALUE87 E_WONDER_OBSOLETE
#define SPECENUM_VALUE88 E_WONDER_STARTED
#define SPECENUM_VALUE89 E_WONDER_STOPPED
#define SPECENUM_VALUE90 E_WONDER_WILL_BE_BUILT
#define SPECENUM_VALUE91 E_DIPLOMACY
#define SPECENUM_VALUE92 E_TREATY_EMBASSY
// Illegal command sent from client.
#define SPECENUM_VALUE93 E_BAD_COMMAND
// Messages for changed server settings
#define SPECENUM_VALUE94 E_SETTING
// Chatline messages
#define SPECENUM_VALUE95 E_CHAT_MSG
// Message from server operator
#define SPECENUM_VALUE96 E_MESSAGE_WALL
// Chatline errors (bad syntax, etc.)
#define SPECENUM_VALUE97 E_CHAT_ERROR
// Messages about acquired or lost connections
#define SPECENUM_VALUE98 E_CONNECTION
// AI debugging messages
#define SPECENUM_VALUE99 E_AI_DEBUG
// Warning messages
#define SPECENUM_VALUE100 E_LOG_ERROR
#define SPECENUM_VALUE101 E_LOG_FATAL
// Changed tech goal
#define SPECENUM_VALUE102 E_TECH_GOAL
// Non-battle unit deaths
#define SPECENUM_VALUE103 E_UNIT_LOST_MISC
// Plague within a city
#define SPECENUM_VALUE104 E_CITY_PLAGUE
#define SPECENUM_VALUE105 E_VOTE_NEW
#define SPECENUM_VALUE106 E_VOTE_RESOLVED
#define SPECENUM_VALUE107 E_VOTE_ABORTED
// Change of the city radius
#define SPECENUM_VALUE108 E_CITY_RADIUS_SQ
// A unit with population cost was built; the city shrinks.
#define SPECENUM_VALUE109 E_UNIT_BUILT_POP_COST
#define SPECENUM_VALUE110 E_DISASTER
#define SPECENUM_VALUE111 E_ACHIEVEMENT
#define SPECENUM_VALUE112 E_TECH_LOST
// Used to notify tech events for foreigner players.
#define SPECENUM_VALUE113 E_TECH_EMBASSY
#define SPECENUM_VALUE114 E_MY_SPY_STEAL_GOLD
#define SPECENUM_VALUE115 E_ENEMY_SPY_STEAL_GOLD
#define SPECENUM_VALUE116 E_SPONTANEOUS_EXTRA
#define SPECENUM_VALUE117 E_UNIT_ILLEGAL_ACTION
#define SPECENUM_VALUE118 E_MY_SPY_STEAL_MAP
#define SPECENUM_VALUE119 E_ENEMY_SPY_STEAL_MAP
#define SPECENUM_VALUE120 E_MY_SPY_NUKE
#define SPECENUM_VALUE121 E_ENEMY_SPY_NUKE
#define SPECENUM_VALUE122 E_UNIT_WAS_EXPELLED
#define SPECENUM_VALUE123 E_UNIT_DID_EXPEL
#define SPECENUM_VALUE124 E_UNIT_ACTION_FAILED
#define SPECENUM_VALUE125 E_UNIT_ESCAPED
#define SPECENUM_VALUE126 E_DEPRECATION_WARNING
// Used for messages about things experienced players already know.
#define SPECENUM_VALUE127 E_BEGINNER_HELP
#define SPECENUM_VALUE128 E_MY_UNIT_DID_HEAL
#define SPECENUM_VALUE129 E_MY_UNIT_WAS_HEALED
#define SPECENUM_VALUE130 E_MULTIPLIER
#define SPECENUM_VALUE131 E_UNIT_ACTION_ACTOR_SUCCESS
#define SPECENUM_VALUE132 E_UNIT_ACTION_ACTOR_FAILURE
#define SPECENUM_VALUE133 E_UNIT_ACTION_TARGET_OTHER
#define SPECENUM_VALUE134 E_UNIT_ACTION_TARGET_HOSTILE
#define SPECENUM_VALUE135 E_UNIT_WAKE
// Combat without winner (Combat_Rounds)
#define SPECENUM_VALUE136 E_UNIT_TIE_ATT
#define SPECENUM_VALUE137 E_UNIT_TIE_DEF
// bombarding
#define SPECENUM_VALUE138 E_UNIT_BOMB_ATT
#define SPECENUM_VALUE139 E_UNIT_BOMB_DEF

/**
 * Events - macro (after the specenum since its derived)
 *
 * NOTE: If you add a new event, make sure you make a similar change
 * to the events array in "common/events.cpp" using GEN_EV, to
 * "data/stdsounds.soundspec", which serves as the documentation to
 * soundset authors, and to "data/misc/events.spec".
 */
#define SPECENUM_COUNT E_COUNT
/**
 * The sound system also generates "e_game_quit", although there's no
 * corresponding identifier E_GAME_QUIT.
 */
#include "specenum_gen.h"

/**
 * Effects - types
 *
 * Add new values via SPECENUM_VALUE%d and
 * SPECENUM_VALUE%dNAME at the end of the list.
 */
#define SPECENUM_NAME effect_type
#define SPECENUM_VALUE0 EFT_TECH_PARASITE
#define SPECENUM_VALUE0NAME "Tech_Parasite"
#define SPECENUM_VALUE1 EFT_AIRLIFT
#define SPECENUM_VALUE1NAME "Airlift"
#define SPECENUM_VALUE2 EFT_ANY_GOVERNMENT
#define SPECENUM_VALUE2NAME "Any_Government"
#define SPECENUM_VALUE3 EFT_CAPITAL_CITY
#define SPECENUM_VALUE3NAME "Capital_City"
#define SPECENUM_VALUE4 EFT_ENABLE_NUKE
#define SPECENUM_VALUE4NAME "Enable_Nuke"
#define SPECENUM_VALUE5 EFT_ENABLE_SPACE
#define SPECENUM_VALUE5NAME "Enable_Space"
#define SPECENUM_VALUE6 EFT_SPECIALIST_OUTPUT
#define SPECENUM_VALUE6NAME "Specialist_Output"
#define SPECENUM_VALUE7 EFT_OUTPUT_BONUS
#define SPECENUM_VALUE7NAME "Output_Bonus"
#define SPECENUM_VALUE8 EFT_OUTPUT_BONUS_2
#define SPECENUM_VALUE8NAME "Output_Bonus_2"
// add to each worked tile
#define SPECENUM_VALUE9 EFT_OUTPUT_ADD_TILE
#define SPECENUM_VALUE9NAME "Output_Add_Tile"
// add to each worked tile that already has output
#define SPECENUM_VALUE10 EFT_OUTPUT_INC_TILE
#define SPECENUM_VALUE10NAME "Output_Inc_Tile"
// increase tile output by given %
#define SPECENUM_VALUE11 EFT_OUTPUT_PER_TILE
#define SPECENUM_VALUE11NAME "Output_Per_Tile"
#define SPECENUM_VALUE12 EFT_OUTPUT_WASTE_PCT
#define SPECENUM_VALUE12NAME "Output_Waste_Pct"
#define SPECENUM_VALUE13 EFT_FORCE_CONTENT
#define SPECENUM_VALUE13NAME "Force_Content"
// TODO: EFT_FORCE_CONTENT_PCT
#define SPECENUM_VALUE14 EFT_GIVE_IMM_TECH
#define SPECENUM_VALUE14NAME "Give_Imm_Tech"
#define SPECENUM_VALUE15 EFT_GROWTH_FOOD
#define SPECENUM_VALUE15NAME "Growth_Food"
// reduced illness due to buildings ...
#define SPECENUM_VALUE16 EFT_HEALTH_PCT
#define SPECENUM_VALUE16NAME "Health_Pct"
#define SPECENUM_VALUE17 EFT_HAVE_EMBASSIES
#define SPECENUM_VALUE17NAME "Have_Embassies"
#define SPECENUM_VALUE18 EFT_MAKE_CONTENT
#define SPECENUM_VALUE18NAME "Make_Content"
#define SPECENUM_VALUE19 EFT_MAKE_CONTENT_MIL
#define SPECENUM_VALUE19NAME "Make_Content_Mil"
#define SPECENUM_VALUE20 EFT_MAKE_CONTENT_MIL_PER
#define SPECENUM_VALUE20NAME "Make_Content_Mil_Per"
// TODO: EFT_MAKE_CONTENT_PCT
#define SPECENUM_VALUE21 EFT_MAKE_HAPPY
#define SPECENUM_VALUE21NAME "Make_Happy"
#define SPECENUM_VALUE22 EFT_NO_ANARCHY
#define SPECENUM_VALUE22NAME "No_Anarchy"
#define SPECENUM_VALUE23 EFT_NUKE_PROOF
#define SPECENUM_VALUE23NAME "Nuke_Proof"
// TODO: EFT_POLLU_ADJ
// TODO: EFT_POLLU_PCT
// TODO: EFT_POLLU_POP_ADJ
#define SPECENUM_VALUE24 EFT_POLLU_POP_PCT
#define SPECENUM_VALUE24NAME "Pollu_Pop_Pct"
#define SPECENUM_VALUE25 EFT_POLLU_POP_PCT_2
#define SPECENUM_VALUE25NAME "Pollu_Pop_Pct_2"
// TODO: EFT_POLLU_PROD_ADJ
#define SPECENUM_VALUE26 EFT_POLLU_PROD_PCT
#define SPECENUM_VALUE26NAME "Pollu_Prod_Pct"
// TODO: EFT_PROD_PCT
#define SPECENUM_VALUE27 EFT_REVEAL_CITIES
#define SPECENUM_VALUE27NAME "Reveal_Cities"
#define SPECENUM_VALUE28 EFT_REVEAL_MAP
#define SPECENUM_VALUE28NAME "Reveal_Map"
// TODO: EFT_INCITE_DIST_ADJ
#define SPECENUM_VALUE29 EFT_INCITE_COST_PCT
#define SPECENUM_VALUE29NAME "Incite_Cost_Pct"
#define SPECENUM_VALUE30 EFT_SIZE_ADJ
#define SPECENUM_VALUE30NAME "Size_Adj"
#define SPECENUM_VALUE31 EFT_SIZE_UNLIMIT
#define SPECENUM_VALUE31NAME "Size_Unlimit"
#define SPECENUM_VALUE32 EFT_SS_STRUCTURAL
#define SPECENUM_VALUE32NAME "SS_Structural"
#define SPECENUM_VALUE33 EFT_SS_COMPONENT
#define SPECENUM_VALUE33NAME "SS_Component"
#define SPECENUM_VALUE34 EFT_SS_MODULE
#define SPECENUM_VALUE34NAME "SS_Module"
#define SPECENUM_VALUE35 EFT_SPY_RESISTANT
#define SPECENUM_VALUE35NAME "Spy_Resistant"
#define SPECENUM_VALUE36 EFT_MOVE_BONUS
#define SPECENUM_VALUE36NAME "Move_Bonus"
#define SPECENUM_VALUE37 EFT_UNIT_NO_LOSE_POP
#define SPECENUM_VALUE37NAME "Unit_No_Lose_Pop"
#define SPECENUM_VALUE38 EFT_UNIT_RECOVER
#define SPECENUM_VALUE38NAME "Unit_Recover"
#define SPECENUM_VALUE39 EFT_UPGRADE_UNIT
#define SPECENUM_VALUE39NAME "Upgrade_Unit"
#define SPECENUM_VALUE40 EFT_UPKEEP_FREE
#define SPECENUM_VALUE40NAME "Upkeep_Free"
#define SPECENUM_VALUE41 EFT_TECH_UPKEEP_FREE
#define SPECENUM_VALUE41NAME "Tech_Upkeep_Free"
#define SPECENUM_VALUE42 EFT_NO_UNHAPPY
#define SPECENUM_VALUE42NAME "No_Unhappy"
#define SPECENUM_VALUE43 EFT_VETERAN_BUILD
#define SPECENUM_VALUE43NAME "Veteran_Build"
#define SPECENUM_VALUE44 EFT_VETERAN_COMBAT
#define SPECENUM_VALUE44NAME "Veteran_Combat"
#define SPECENUM_VALUE45 EFT_HP_REGEN
#define SPECENUM_VALUE45NAME "HP_Regen"
#define SPECENUM_VALUE46 EFT_CITY_VISION_RADIUS_SQ
#define SPECENUM_VALUE46NAME "City_Vision_Radius_Sq"
#define SPECENUM_VALUE47 EFT_UNIT_VISION_RADIUS_SQ
#define SPECENUM_VALUE47NAME "Unit_Vision_Radius_Sq"
// Interacts with UTYF_BADWALLATTACKER
#define SPECENUM_VALUE48 EFT_DEFEND_BONUS
#define SPECENUM_VALUE48NAME "Defend_Bonus"
#define SPECENUM_VALUE49 EFT_TRADEROUTE_PCT
#define SPECENUM_VALUE49NAME "Traderoute_Pct"
#define SPECENUM_VALUE50 EFT_GAIN_AI_LOVE
#define SPECENUM_VALUE50NAME "Gain_AI_Love"
#define SPECENUM_VALUE51 EFT_TURN_YEARS
#define SPECENUM_VALUE51NAME "Turn_Years"
#define SPECENUM_VALUE52 EFT_SLOW_DOWN_TIMELINE
#define SPECENUM_VALUE52NAME "Slow_Down_Timeline"
#define SPECENUM_VALUE53 EFT_CIVIL_WAR_CHANCE
#define SPECENUM_VALUE53NAME "Civil_War_Chance"
// change of the migration score
#define SPECENUM_VALUE54 EFT_MIGRATION_PCT
#define SPECENUM_VALUE54NAME "Migration_Pct"
// +1 unhappy when more than this cities
#define SPECENUM_VALUE55 EFT_EMPIRE_SIZE_BASE
#define SPECENUM_VALUE55NAME "Empire_Size_Base"
// adds additional +1 unhappy steps to above
#define SPECENUM_VALUE56 EFT_EMPIRE_SIZE_STEP
#define SPECENUM_VALUE56NAME "Empire_Size_Step"
#define SPECENUM_VALUE57 EFT_MAX_RATES
#define SPECENUM_VALUE57NAME "Max_Rates"
#define SPECENUM_VALUE58 EFT_MARTIAL_LAW_EACH
#define SPECENUM_VALUE58NAME "Martial_Law_Each"
#define SPECENUM_VALUE59 EFT_MARTIAL_LAW_MAX
#define SPECENUM_VALUE59NAME "Martial_Law_Max"
#define SPECENUM_VALUE60 EFT_RAPTURE_GROW
#define SPECENUM_VALUE60NAME "Rapture_Grow"
#define SPECENUM_VALUE61 EFT_REVOLUTION_UNHAPPINESS
#define SPECENUM_VALUE61NAME "Revolution_Unhappiness"
#define SPECENUM_VALUE62 EFT_HAS_SENATE
#define SPECENUM_VALUE62NAME "Has_Senate"
#define SPECENUM_VALUE63 EFT_INSPIRE_PARTISANS
#define SPECENUM_VALUE63NAME "Inspire_Partisans"
#define SPECENUM_VALUE64 EFT_HAPPINESS_TO_GOLD
#define SPECENUM_VALUE64NAME "Happiness_To_Gold"
// stupid special case; we hate it
#define SPECENUM_VALUE65 EFT_FANATICS
#define SPECENUM_VALUE65NAME "Fanatics"
#define SPECENUM_VALUE66 EFT_NO_DIPLOMACY
#define SPECENUM_VALUE66NAME "No_Diplomacy"
#define SPECENUM_VALUE67 EFT_TRADE_REVENUE_BONUS
#define SPECENUM_VALUE67NAME "Trade_Revenue_Bonus"
// multiply unhappy upkeep by this effect
#define SPECENUM_VALUE68 EFT_UNHAPPY_FACTOR
#define SPECENUM_VALUE68NAME "Unhappy_Factor"
// multiply upkeep by this effect
#define SPECENUM_VALUE69 EFT_UPKEEP_FACTOR
#define SPECENUM_VALUE69NAME "Upkeep_Factor"
// this many units are free from upkeep
#define SPECENUM_VALUE70 EFT_UNIT_UPKEEP_FREE_PER_CITY
#define SPECENUM_VALUE70NAME "Unit_Upkeep_Free_Per_City"
#define SPECENUM_VALUE71 EFT_OUTPUT_WASTE
#define SPECENUM_VALUE71NAME "Output_Waste"
#define SPECENUM_VALUE72 EFT_OUTPUT_WASTE_BY_DISTANCE
#define SPECENUM_VALUE72NAME "Output_Waste_By_Distance"
// -1 penalty to tiles producing more than this
#define SPECENUM_VALUE73 EFT_OUTPUT_PENALTY_TILE
#define SPECENUM_VALUE73NAME "Output_Penalty_Tile"
#define SPECENUM_VALUE74 EFT_OUTPUT_INC_TILE_CELEBRATE
#define SPECENUM_VALUE74NAME "Output_Inc_Tile_Celebrate"
// all citizens after this are unhappy
#define SPECENUM_VALUE75 EFT_CITY_UNHAPPY_SIZE
#define SPECENUM_VALUE75NAME "City_Unhappy_Size"
// add to default squared city radius
#define SPECENUM_VALUE76 EFT_CITY_RADIUS_SQ
#define SPECENUM_VALUE76NAME "City_Radius_Sq"
// number of build slots for units
#define SPECENUM_VALUE77 EFT_CITY_BUILD_SLOTS
#define SPECENUM_VALUE77NAME "City_Build_Slots"
#define SPECENUM_VALUE78 EFT_UPGRADE_PRICE_PCT
#define SPECENUM_VALUE78NAME "Upgrade_Price_Pct"
// City should use walls gfx
#define SPECENUM_VALUE79 EFT_VISIBLE_WALLS
#define SPECENUM_VALUE79NAME "Visible_Walls"
#define SPECENUM_VALUE80 EFT_TECH_COST_FACTOR
#define SPECENUM_VALUE80NAME "Tech_Cost_Factor"
// [x%] gold upkeep instead of [1] shield upkeep for units
#define SPECENUM_VALUE81 EFT_SHIELD2GOLD_FACTOR
#define SPECENUM_VALUE81NAME "Shield2Gold_Factor"
#define SPECENUM_VALUE82 EFT_TILE_WORKABLE
#define SPECENUM_VALUE82NAME "Tile_Workable"
// The index for the city image of the given city style.
#define SPECENUM_VALUE83 EFT_CITY_IMAGE
#define SPECENUM_VALUE83NAME "City_Image"
#define SPECENUM_VALUE84 EFT_IMPR_BUILD_COST_PCT
#define SPECENUM_VALUE84NAME "Building_Build_Cost_Pct"
#define SPECENUM_VALUE85 EFT_MAX_TRADE_ROUTES
#define SPECENUM_VALUE85NAME "Max_Trade_Routes"
#define SPECENUM_VALUE86 EFT_GOV_CENTER
#define SPECENUM_VALUE86NAME "Gov_Center"
#define SPECENUM_VALUE87 EFT_COMBAT_ROUNDS
#define SPECENUM_VALUE87NAME "Combat_Rounds"
#define SPECENUM_VALUE88 EFT_IMPR_BUY_COST_PCT
#define SPECENUM_VALUE88NAME "Building_Buy_Cost_Pct"
#define SPECENUM_VALUE89 EFT_UNIT_BUILD_COST_PCT
#define SPECENUM_VALUE89NAME "Unit_Build_Cost_Pct"
#define SPECENUM_VALUE90 EFT_UNIT_BUY_COST_PCT
#define SPECENUM_VALUE90NAME "Unit_Buy_Cost_Pct"
#define SPECENUM_VALUE91 EFT_NOT_TECH_SOURCE
#define SPECENUM_VALUE91NAME "Not_Tech_Source"
#define SPECENUM_VALUE92 EFT_ENEMY_CITIZEN_UNHAPPY_PCT
#define SPECENUM_VALUE92NAME "Enemy_Citizen_Unhappy_Pct"
#define SPECENUM_VALUE93 EFT_IRRIGATION_PCT
#define SPECENUM_VALUE93NAME "Irrigation_Pct"
#define SPECENUM_VALUE94 EFT_MINING_PCT
#define SPECENUM_VALUE94NAME "Mining_Pct"
#define SPECENUM_VALUE95 EFT_OUTPUT_TILE_PUNISH_PCT
#define SPECENUM_VALUE95NAME "Output_Tile_Punish_Pct"
#define SPECENUM_VALUE96 EFT_UNIT_BRIBE_COST_PCT
#define SPECENUM_VALUE96NAME "Unit_Bribe_Cost_Pct"
#define SPECENUM_VALUE97 EFT_VICTORY
#define SPECENUM_VALUE97NAME "Victory"
#define SPECENUM_VALUE98 EFT_PERFORMANCE
#define SPECENUM_VALUE98NAME "Performance"
#define SPECENUM_VALUE99 EFT_HISTORY
#define SPECENUM_VALUE99NAME "History"
#define SPECENUM_VALUE100 EFT_NATION_PERFORMANCE
#define SPECENUM_VALUE100NAME "National_Performance"
#define SPECENUM_VALUE101 EFT_NATION_HISTORY
#define SPECENUM_VALUE101NAME "National_History"
#define SPECENUM_VALUE102 EFT_TURN_FRAGMENTS
#define SPECENUM_VALUE102NAME "Turn_Fragments"
#define SPECENUM_VALUE103 EFT_MAX_STOLEN_GOLD_PM
#define SPECENUM_VALUE103NAME "Max_Stolen_Gold_Pm"
#define SPECENUM_VALUE104 EFT_THIEFS_SHARE_PM
#define SPECENUM_VALUE104NAME "Thiefs_Share_Pm"
#define SPECENUM_VALUE105 EFT_RETIRE_PCT
#define SPECENUM_VALUE105NAME "Retire_Pct"
#define SPECENUM_VALUE106 EFT_ILLEGAL_ACTION_MOVE_COST
#define SPECENUM_VALUE106NAME "Illegal_Action_Move_Cost"
#define SPECENUM_VALUE107 EFT_HAVE_CONTACTS
#define SPECENUM_VALUE107NAME "Have_Contacts"
#define SPECENUM_VALUE108 EFT_CASUS_BELLI_CAUGHT
#define SPECENUM_VALUE108NAME "Casus_Belli_Caught"
#define SPECENUM_VALUE109 EFT_CASUS_BELLI_SUCCESS
#define SPECENUM_VALUE109NAME "Casus_Belli_Success"
#define SPECENUM_VALUE110 EFT_ACTION_ODDS_PCT
#define SPECENUM_VALUE110NAME "Action_Odds_Pct"
#define SPECENUM_VALUE111 EFT_BORDER_VISION
#define SPECENUM_VALUE111NAME "Border_Vision"
#define SPECENUM_VALUE112 EFT_STEALINGS_IGNORE
#define SPECENUM_VALUE112NAME "Stealings_Ignore"
#define SPECENUM_VALUE113 EFT_OUTPUT_WASTE_BY_REL_DISTANCE
#define SPECENUM_VALUE113NAME "Output_Waste_By_Rel_Distance"
#define SPECENUM_VALUE114 EFT_SABOTEUR_RESISTANT
#define SPECENUM_VALUE114NAME "Building_Saboteur_Resistant"
#define SPECENUM_VALUE115 EFT_UNIT_SLOTS
#define SPECENUM_VALUE115NAME "Unit_Slots"
#define SPECENUM_VALUE116 EFT_ATTACK_BONUS
#define SPECENUM_VALUE116NAME "Attack_Bonus"
#define SPECENUM_VALUE117 EFT_CONQUEST_TECH_PCT
#define SPECENUM_VALUE117NAME "Conquest_Tech_Pct"
#define SPECENUM_VALUE118 EFT_ACTION_SUCCESS_MOVE_COST
#define SPECENUM_VALUE118NAME "Action_Success_Actor_Move_Cost"
#define SPECENUM_VALUE119 EFT_ACTION_SUCCESS_TARGET_MOVE_COST
#define SPECENUM_VALUE119NAME "Action_Success_Target_Move_Cost"
#define SPECENUM_VALUE120 EFT_INFRA_POINTS
#define SPECENUM_VALUE120NAME "Infra_Points"
#define SPECENUM_VALUE121 EFT_FORTIFY_DEFENSE_BONUS
#define SPECENUM_VALUE121NAME "Fortify_Defense_Bonus"
#define SPECENUM_VALUE122 EFT_MAPS_STOLEN_PCT
#define SPECENUM_VALUE122NAME "Maps_Stolen_Pct"
#define SPECENUM_VALUE123 EFT_UNIT_SHIELD_VALUE_PCT
#define SPECENUM_VALUE123NAME "Unit_Shield_Value_Pct"
#define SPECENUM_VALUE124 EFT_CASUS_BELLI_COMPLETE
#define SPECENUM_VALUE124NAME "Casus_Belli_Complete"
#define SPECENUM_VALUE125 EFT_ILLEGAL_ACTION_HP_COST
#define SPECENUM_VALUE125NAME "Illegal_Action_HP_Cost"
#define SPECENUM_VALUE128 EFT_NUKE_IMPROVEMENT_PCT
#define SPECENUM_VALUE128NAME "Nuke_Improvement_Pct"
#define SPECENUM_VALUE129 EFT_HP_REGEN_MIN
#define SPECENUM_VALUE129NAME "HP_Regen_Min"
#define SPECENUM_VALUE130 EFT_BOMBARD_LIMIT_PCT
#define SPECENUM_VALUE130NAME "Bombard_Limit_Pct"
#define SPECENUM_VALUE131 EFT_TRADE_REVENUE_EXPONENT
#define SPECENUM_VALUE131NAME "Trade_Revenue_Exponent"
#define SPECENUM_VALUE132 EFT_WONDER_VISIBLE
#define SPECENUM_VALUE132NAME "Wonder_Visible"
#define SPECENUM_VALUE133 EFT_NATION_INTELLIGENCE
#define SPECENUM_VALUE133NAME "Nation_Intelligence"
#define SPECENUM_VALUE134 EFT_NUKE_INFRASTRUCTURE_PCT
#define SPECENUM_VALUE134NAME "Nuke_Infrastructure_Pct"
#define SPECENUM_VALUE135 EFT_GROWTH_SURPLUS_PCT
#define SPECENUM_VALUE135NAME "Growth_Surplus_Pct"
#define SPECENUM_VALUE136 EFT_POLLU_TRADE_PCT
#define SPECENUM_VALUE136NAME "Pollu_Trade_Pct"
// keep this last
#define SPECENUM_COUNT EFT_COUNT
#include "specenum_gen.h"

/**
 * Effects - Disaster macros
 */
#define DISASTER_BASE_RARITY 1000000

/**
 * Effects - Disaster effects
 */
#define SPECENUM_NAME disaster_effect_id
#define SPECENUM_VALUE0 DE_DESTROY_BUILDING
#define SPECENUM_VALUE0NAME "DestroyBuilding"
#define SPECENUM_VALUE1 DE_REDUCE_POP
#define SPECENUM_VALUE1NAME "ReducePopulation"
#define SPECENUM_VALUE2 DE_EMPTY_FOODSTOCK
#define SPECENUM_VALUE2NAME "EmptyFoodStock"
#define SPECENUM_VALUE3 DE_EMPTY_PRODSTOCK
#define SPECENUM_VALUE3NAME "EmptyProdStock"
#define SPECENUM_VALUE4 DE_POLLUTION
#define SPECENUM_VALUE4NAME "Pollution"
#define SPECENUM_VALUE5 DE_FALLOUT
#define SPECENUM_VALUE5NAME "Fallout"
#define SPECENUM_VALUE6 DE_REDUCE_DESTROY
#define SPECENUM_VALUE6NAME "ReducePopDestroy"
#define SPECENUM_COUNT DE_COUNT
#define SPECENUM_BITVECTOR bv_disaster_effects
#include "specenum_gen.h"

/**
 * Requirement - types
 *
 * Originally in requirements.h, bumped up and revised to unify with
 * city_production and worklists. Functions remain in requirements.cpp
 */
typedef union {
  struct advance *advance;
  struct government *govern;
  const struct impr_type *building;
  struct nation_type *nation;
  struct nation_type *nationality;
  struct specialist *specialist;
  struct terrain *terrain;
  struct unit_class *uclass;
  const struct unit_type *utype;
  struct extra_type *extra;
  struct achievement *achievement;
  struct nation_group *nationgroup;
  struct nation_style *style;
  struct action *action;
  struct goods_type *good;

  enum ai_level ai_level;
  enum citytile_type citytile;
  enum citystatus_type citystatus;
  enum vision_layer vlayer;
  enum national_intelligence nintel;
  int minsize;
  int minculture;
  int minforeignpct;
  int minyear;
  int mincalfrag;
  Output_type_id outputtype;
  int terrainclass;  // enum terrain_class
  int terrainalter;  // enum terrain_alteration
  int unitclassflag; // enum unit_class_flag_id
  int unitflag;      // enum unit_flag_id
  int terrainflag;   // enum terrain_flag_id
  int techflag;      // enum tech_flag_id
  int baseflag;      // enum base_flag_id
  int roadflag;      // enum road_flag_id
  int extraflag;
  int diplrel; /* enum diplstate_type or
  enum diplrel_other */
  enum ustate_prop unit_state;
  enum unit_activity activity;
  enum impr_genus_id impr_genus;
  int minmoves;
  int max_tile_units;
  int minveteran;
  int min_hit_points;
  int age;
  int min_techs;

  enum topo_flag topo_property;
  ssetv ssetval;
} universals_u;

/**
 * Requirement - universal types
 *
 * The kind of universals_u (value_union_type was req_source_type).
 */
#define SPECENUM_NAME universals_n
#define SPECENUM_VALUE0 VUT_NONE
#define SPECENUM_VALUE0NAME "None"
#define SPECENUM_VALUE1 VUT_ADVANCE
#define SPECENUM_VALUE1NAME "Tech"
#define SPECENUM_VALUE2 VUT_GOVERNMENT
#define SPECENUM_VALUE2NAME "Gov"
#define SPECENUM_VALUE3 VUT_IMPROVEMENT
#define SPECENUM_VALUE3NAME "Building"
#define SPECENUM_VALUE4 VUT_TERRAIN
#define SPECENUM_VALUE4NAME "Terrain"
#define SPECENUM_VALUE5 VUT_NATION
#define SPECENUM_VALUE5NAME "Nation"
#define SPECENUM_VALUE6 VUT_UTYPE
#define SPECENUM_VALUE6NAME "UnitType"
#define SPECENUM_VALUE7 VUT_UTFLAG
#define SPECENUM_VALUE7NAME "UnitFlag"
#define SPECENUM_VALUE8 VUT_UCLASS
#define SPECENUM_VALUE8NAME "UnitClass"
#define SPECENUM_VALUE9 VUT_UCFLAG
#define SPECENUM_VALUE9NAME "UnitClassFlag"
#define SPECENUM_VALUE10 VUT_OTYPE
#define SPECENUM_VALUE10NAME "OutputType"
#define SPECENUM_VALUE11 VUT_SPECIALIST
#define SPECENUM_VALUE11NAME "Specialist"
// Minimum size: at city range means city size
#define SPECENUM_VALUE12 VUT_MINSIZE
#define SPECENUM_VALUE12NAME "MinSize"
// AI level of the player
#define SPECENUM_VALUE13 VUT_AI_LEVEL
#define SPECENUM_VALUE13NAME "AI"
// More generic terrain type currently "Land" or "Ocean"
#define SPECENUM_VALUE14 VUT_TERRAINCLASS
#define SPECENUM_VALUE14NAME "TerrainClass"
#define SPECENUM_VALUE15 VUT_MINYEAR
#define SPECENUM_VALUE15NAME "MinYear"
// Terrain alterations that are possible
#define SPECENUM_VALUE16 VUT_TERRAINALTER
#define SPECENUM_VALUE16NAME "TerrainAlter"
// Target tile is used by city.
#define SPECENUM_VALUE17 VUT_CITYTILE
#define SPECENUM_VALUE17NAME "CityTile"
#define SPECENUM_VALUE18 VUT_GOOD
#define SPECENUM_VALUE18NAME "Good"
#define SPECENUM_VALUE19 VUT_TERRFLAG
#define SPECENUM_VALUE19NAME "TerrainFlag"
#define SPECENUM_VALUE20 VUT_NATIONALITY
#define SPECENUM_VALUE20NAME "Nationality"
#define SPECENUM_VALUE21 VUT_BASEFLAG
#define SPECENUM_VALUE21NAME "BaseFlag"
#define SPECENUM_VALUE22 VUT_ROADFLAG
#define SPECENUM_VALUE22NAME "RoadFlag"
#define SPECENUM_VALUE23 VUT_EXTRA
#define SPECENUM_VALUE23NAME "Extra"
#define SPECENUM_VALUE24 VUT_TECHFLAG
#define SPECENUM_VALUE24NAME "TechFlag"
#define SPECENUM_VALUE25 VUT_ACHIEVEMENT
#define SPECENUM_VALUE25NAME "Achievement"
#define SPECENUM_VALUE26 VUT_DIPLREL
#define SPECENUM_VALUE26NAME "DiplRel"
#define SPECENUM_VALUE27 VUT_MAXTILEUNITS
#define SPECENUM_VALUE27NAME "MaxUnitsOnTile"
#define SPECENUM_VALUE28 VUT_STYLE
#define SPECENUM_VALUE28NAME "Style"
#define SPECENUM_VALUE29 VUT_MINCULTURE
#define SPECENUM_VALUE29NAME "MinCulture"
#define SPECENUM_VALUE30 VUT_UNITSTATE
#define SPECENUM_VALUE30NAME "UnitState"
#define SPECENUM_VALUE31 VUT_MINMOVES
#define SPECENUM_VALUE31NAME "MinMoveFrags"
#define SPECENUM_VALUE32 VUT_MINVETERAN
#define SPECENUM_VALUE32NAME "MinVeteran"
#define SPECENUM_VALUE33 VUT_MINHP
#define SPECENUM_VALUE33NAME "MinHitPoints"
#define SPECENUM_VALUE34 VUT_AGE
#define SPECENUM_VALUE34NAME "Age"
#define SPECENUM_VALUE35 VUT_NATIONGROUP
#define SPECENUM_VALUE35NAME "NationGroup"
#define SPECENUM_VALUE36 VUT_TOPO
#define SPECENUM_VALUE36NAME "Topology"
#define SPECENUM_VALUE37 VUT_IMPR_GENUS
#define SPECENUM_VALUE37NAME "BuildingGenus"
#define SPECENUM_VALUE38 VUT_ACTION
#define SPECENUM_VALUE38NAME "Action"
#define SPECENUM_VALUE39 VUT_MINTECHS
#define SPECENUM_VALUE39NAME "MinTechs"
#define SPECENUM_VALUE40 VUT_EXTRAFLAG
#define SPECENUM_VALUE40NAME "ExtraFlag"
#define SPECENUM_VALUE41 VUT_MINCALFRAG
#define SPECENUM_VALUE41NAME "MinCalFrag"
#define SPECENUM_VALUE42 VUT_SERVERSETTING
#define SPECENUM_VALUE42NAME "ServerSetting"
#define SPECENUM_VALUE43 VUT_CITYSTATUS
#define SPECENUM_VALUE43NAME "CityStatus"
#define SPECENUM_VALUE44 VUT_MINFOREIGNPCT
#define SPECENUM_VALUE44NAME "MinForeignPct"
#define SPECENUM_VALUE45 VUT_ACTIVITY
#define SPECENUM_VALUE45NAME "Activity"
#define SPECENUM_VALUE46 VUT_VISIONLAYER
#define SPECENUM_VALUE46NAME "VisionLayer"
#define SPECENUM_VALUE47 VUT_NINTEL
#define SPECENUM_VALUE47NAME "NationalIntelligence"
// Keep this last.
#define SPECENUM_COUNT VUT_COUNT
#include "specenum_gen.h"

// Used in the network protocol.
struct universal {
  universals_u value;
  enum universals_n kind; // formerly .type and .is_unit
};

/**
 * Requirement - Range
 *
 * Order is important -- wider ranges should come later -- some code
 * assumes a total order, or tests for e.g. >= REQ_RANGE_PLAYER.
 * Ranges of similar types should be supersets, for example:
 *  - the set of Adjacent tiles contains the set of CAdjacent tiles,
 *    and both contain the center Local tile (a requirement on the local
 *    tile is also within Adjacent range);
 *  - World contains Alliance contains Player (a requirement we ourselves
 *    have is also within Alliance range).
 */
#define SPECENUM_NAME req_range
#define SPECENUM_VALUE0 REQ_RANGE_LOCAL
#define SPECENUM_VALUE0NAME "Local"
#define SPECENUM_VALUE1 REQ_RANGE_CADJACENT
#define SPECENUM_VALUE1NAME "CAdjacent"
#define SPECENUM_VALUE2 REQ_RANGE_ADJACENT
#define SPECENUM_VALUE2NAME "Adjacent"
#define SPECENUM_VALUE3 REQ_RANGE_CITY
#define SPECENUM_VALUE3NAME "City"
#define SPECENUM_VALUE4 REQ_RANGE_TRADEROUTE
#define SPECENUM_VALUE4NAME "Traderoute"
#define SPECENUM_VALUE5 REQ_RANGE_CONTINENT
#define SPECENUM_VALUE5NAME "Continent"
#define SPECENUM_VALUE6 REQ_RANGE_PLAYER
#define SPECENUM_VALUE6NAME "Player"
#define SPECENUM_VALUE7 REQ_RANGE_TEAM
#define SPECENUM_VALUE7NAME "Team"
#define SPECENUM_VALUE8 REQ_RANGE_ALLIANCE
#define SPECENUM_VALUE8NAME "Alliance"
#define SPECENUM_VALUE9 REQ_RANGE_WORLD
#define SPECENUM_VALUE9NAME "World"
#define SPECENUM_COUNT REQ_RANGE_COUNT // keep this last
#include "specenum_gen.h"

/**
 * Requirement - structure
 *
 * This requirement is basically a conditional; it may or
 * may not be active on a target. If it is active then something happens.
 * For instance units and buildings have requirements to be built, techs
 * have requirements to be researched, and effects have requirements to be
 * active.
 */
struct requirement {
  struct universal source; // requirement source
  enum req_range range;    // requirement range
  bool survives;           /* set if destroyed sources satisfy the req*/
  bool present;            // set if the requirement is to be present
  bool quiet;              // do not list this in helptext
};

/**
 * Requirement - vector
 *
 * Sometimes we don't know (or don't care) if some requirements for effect
 * are currently fulfilled or not. This enum tells lower level functions
 * how to handle uncertain requirements.
 */
enum req_problem_type {
  RPT_POSSIBLE, // We want to know if it is possible that effect is active
  RPT_CERTAIN   // We want to know if it is certain that effect is active
};

#define REVERSED_RPT(x) (x == RPT_CERTAIN ? RPT_POSSIBLE : RPT_CERTAIN)

/**
 * City - Production worklist
 */
#define MAX_LEN_WORKLIST 64
#define MAX_NUM_WORKLISTS 16

struct worklist {
  int length;
  struct universal entries[MAX_LEN_WORKLIST];
};
