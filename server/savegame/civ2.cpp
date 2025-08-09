// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "civ2.h"

// utility
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

// common
#include "ai.h"
#include "base.h"
#include "city.h"
#include "extras.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "nation.h"
#include "player.h"
#include "rgbcolor.h"
#include "team.h"
#include "terrain.h"

// ai
#include "aitraits.h"
#include "difficulty.h"

// server
#include "citytools.h"
#include "infracache.h"
#include "mapgen_utils.h"
#include "maphand.h"
#include "plrhand.h"
#include "ruleset.h"
#include "srv_main.h"

#include <QFile>
#include <QString>
#include <QStringDecoder>
#include <QtDebug>
#include <QtEndian>

#include <cstdint>

/**
 * \file civ2.cpp Loads civ2 saves.
 *
 * Useful resources:
 * * https://apolyton.net/forum/civilization-series/civilization-i-and-civilization-ii/130935-civilization-ii-sav-scn-file-format
 * * http://civgaming.net/mercator/reference/mapstruct.html in the Wayback
 *   machine
 * * https://github.com/evyscr/civ2mp2fc/tree/master
 * * https://ia800304.us.archive.org/11/items/civ2-hex-editing/Hex%20Editing.pdf
 */

namespace civ2 {
/// Magic string found at the beginning of a save.
const static auto MAGIC = QByteArrayLiteral("CIVILIZE\0\x1A");

/// Number of techs for Conflicts in Civliization
const static auto NUM_TECHS_CIC = 93;
/// Number of techs for most versions
const static auto NUM_TECHS = 100;
// Freeciv21 identifiers of civ2 terrains (in order).
constexpr std::string_view TERRAIN_IDS = "dpgfhmtasj ";
/// Number of Great Wonder slots
const static auto NUM_WONDERS = 28;
/// Number of civilizations (including barbarians)
const static auto NUM_PLAYERS = 8;
/// Total number of playable civs.
const static auto NUM_CIVS = 21;
/// Constant for wonders that haven't been built.
const static auto CIV2_WONDER_NOT_BUILT = -1;
/// Constant for wonders that have been destroyed.
const static auto CIV2_WONDER_DESTROYED = -2;
/// Colors used by Civ 3
std::array<int, 8> CIV_COLORS = {
    0xff0000, // Red (barbarians)
    0xffffff, // White
    0x00ff00, // Green
    0x0000ff, // Blue
    0xffff00, // Yellow
    0x00ffff, // Cyan
    0xffa500, // Orange
    0x800080, // Purple
};

/**
 * Internal Civliization II version numbers.
 */
enum civ2_version {
  CIV2_CLASSIC = 39,         ///< Classic, Conflicts in Civilization
  CIV2_FANTASTIC = 40,       ///< Fantastic Worlds
  CIV2_MULTIPLAYER = 44,     ///< Multiplayer v1.3
  CIV2_TEST_OF_TIME_10 = 49, ///< Test of Time v1.0
  CIV2_TEST_OF_TIME_11 = 50, ///< Test of Time v1.1
};
/// The highest supported version (from above)
const static auto SUPPORTED_VERSION = CIV2_MULTIPLAYER;

/**
 * The civ2 file header.
 */
struct header {
  std::int16_t version;    ///< Save format version number
  std::int16_t turn;       ///< Current turn
  std::int16_t year;       ///< Current year
  std::uint8_t difficulty; ///< Difficulty (0 to 5).
  std::uint8_t barbarians; ///< Barbarian activity.
  std::uint8_t players_alive; ///< Bitfield: who is alive?
  std::uint8_t players_human; ///< Bitfield: which players are humans?
  int num_techs;           ///< Number of techs in the current version
  std::int16_t unit_count; ///< Number of unit slots in the current game.
  std::int16_t city_count; ///< Number of city slots in the current game.
  /// First nation that discovered each tech
  std::array<char, NUM_TECHS> first_to_discover;
  /// Which nations have discoved which techs (bitfield)
  std::array<char, NUM_TECHS> discovered_by;
  /// City ID for Great Wonders
  std::array<std::int16_t, NUM_WONDERS> wonder_locations;
};

/**
 * Reads in the game header.
 */
bool read_header(QDataStream &bytes, header &head)
{
  bytes.skipRawData(MAGIC.size()); // CIVILIZE 0x00 0x1A
  bytes >> head.version;

  qDebug() << "Loading civ2 file with version" << head.version;
  if (head.version != SUPPORTED_VERSION) {
    qCritical(_("Unsupported civ2 file version: %d"),
              static_cast<int>(head.version));
    return false;
  }

  if (head.version >= CIV2_TEST_OF_TIME_10) {
    bytes.skipRawData(640); // Unknown use
  }
  bytes.skipRawData(15); // Menu settings

  bytes >> head.turn >> head.year;
  bytes.skipRawData(13); // More game settings, some unknown
  bytes >> head.difficulty >> head.barbarians;
  bytes >> head.players_alive >> head.players_human;
  bytes.skipRawData(10); // More game settings, some unknown
  bytes >> head.unit_count >> head.city_count;
  bytes.skipRawData(204); // Unknown use

  fc_assert_ret_val(bytes.device()->pos() == 266, false);

  head.num_techs = head.version > CIV2_CLASSIC ? NUM_TECHS : NUM_TECHS_CIC;
  bytes.readRawData(head.first_to_discover.data(), head.num_techs);
  bytes.readRawData(head.discovered_by.data(), head.num_techs);
  for (auto &location : head.wonder_locations) {
    bytes >> location;
  }

  bytes.skipRawData(62); // Unknown/unused

  return bytes.status() == QDataStream::Ok;
}

/**
 * A civ2 nation
 */
struct tribe {
  std::int16_t city_style;        ///< Which city style is currently used
  std::array<char, 24> leader;    ///< Name of the tribe's leader
  std::array<char, 24> name;      ///< Name of the tribe
  std::array<char, 24> adjective; ///< Adjective for the tribe
  /// Title of the leader under each government
  std::array<std::array<char, 24>, 7> leader_title;
};

/**
 * Reads in a nation.
 */
bool read_tribe(QDataStream &bytes, tribe &t)
{
  bytes >> t.city_style;
  bytes.readRawData(t.leader.data(), t.leader.size());
  bytes.readRawData(t.name.data(), t.name.size());
  bytes.readRawData(t.adjective.data(), t.adjective.size());
  for (auto &title : t.leader_title) {
    bytes.readRawData(title.data(), title.size());
  }
  return bytes.status() == QDataStream::Ok;
}

/**
 * Information about a tile. This is a bitfield.
 */
enum tile_improvement : unsigned char {
  UNIT = 0X01,                  ///< A unit is present on the tile.
  CITY = 0X02,                  ///< A city is present on the tile.
  IRRIGATION = 0x04,            ///< The tile is irrigated.
  MINE = 0X08,                  ///< The tile has a mine.
  FARMLAND = IRRIGATION + MINE, ///< The tile has farmland.
  ROAD = 0X10,                  ///< The tile has a road.
  RAILROAD = 0X20, ///< The tile has a railroad (in principle with road).
  FORTRESS = 0X40, ///< A fortress is present.
  AIRBASE = CITY + FORTRESS,         ///< Air base.
  POLLUTION = 0X80,                  ///< Pollution on the tile.
  TRANSPORT_SITE = CITY + POLLUTION, ///< (Test of Time only.)
};

/**
 * civ2 tile info.
 */
struct tile {
  std::uint8_t terrain : 4;   ///< Terrain ID.
  std::byte _padding1 : 2;    ///< Unused?
  bool no_resources : 1;      ///< Cannot have resources.
  bool river : 1;             ///< Has a river.
  unsigned char improvements; ///< Tile improvements.
  std::byte _padding2 : 5;    ///< Unused?
  std::byte city_owner : 3;   ///< ID of civ that can work this tile.
  std::int8_t island;         ///< Island number.
  std::byte visibility;       ///< Who has explored the tile (bitfield).
  std::byte owner : 4;        ///< Tile owner (for bases).
  std::byte fertility : 4;    ///< ??
};
static_assert(sizeof(tile) == 6);

/**
 * A civ2 map.
 */
struct map {
  std::int16_t width;  ///< Map width.
  std::int16_t height; ///< Map height.
  std::int16_t shape;  ///< 0 = wrapx, 1 = no wrap (?)

  /*
  FIXME: For .MP files? We read .SAV/.SCN
  /// Starting position of each civ (x coordinate)
  std::array<std::int16_t, NUM_CIVS> start_pos_x;
  /// Starting position of each civ (y coordinate)
  std::array<std::int16_t, NUM_CIVS> start_pos_y;
  */

  /// Improvements visible by each player (except barbarians).
  std::array<std::vector<char>, NUM_PLAYERS> visible_improvements;
  /// Actual map tiles.
  std::vector<tile> tiles;
};

/**
 * Reads in civ2 map information.
 */
bool read_map(QDataStream &bytes, map &m)
{
  // Header
  bytes >> m.width >> m.height;
  m.width /= 2;
  bytes.skipRawData(2); // Surface
  bytes >> m.shape;
  bytes.skipRawData(2); // Seed
  std::uint16_t half_width, half_height;
  bytes >> half_width >> half_height;

  int tile_count = m.width * m.height;
  qDebug() << "Loading a map of" << m.width << "x" << m.height << "tiles";

  // Player maps (not for barbarians).
  for (int i = 1; i < NUM_PLAYERS; ++i) {
    m.visible_improvements[i].resize(tile_count);
    qDebug() << "Player map" << i << "starts at byte"
             << bytes.device()->pos();
    bytes.readRawData(m.visible_improvements[i].data(), tile_count);
  }

  // Main map.
  m.tiles.resize(tile_count);
  qDebug() << "Main map starts at byte" << bytes.device()->pos();
  bytes.readRawData(reinterpret_cast<char *>(m.tiles.data()),
                    tile_count * sizeof(tile));

  // Unknown map data, padding.
  bytes.skipRawData(2 * half_width * half_height + 1024);

  return bytes.status() == QDataStream::Ok;
}

/**
 * civ2 unit data.
 */
struct unit {
  /// Orders that can be given to a unit.
  enum orders : std::uint8_t {
    FORTIFY = 1,   ///< Fortifying.
    FORTIFIED,     ///< Remain fortified.
    SENTRY,        ///< Watch out for enemies.
    FORTRESS,      ///< Build a fortress.
    ROAD_OR_RAILS, ///< Build a road, or rails on a road.
    IRRIGATE,      ///< Build irrigation or farmland.
    MINE,          ///< Build a mine.
    TRANSFORM,     ///< Transform terrain.
    POLLUTION,     ///< Clean up pollution.
    AIRBASE,       ///< Build an airbase.
    GO_TO,         ///< Move.
    NONE = 0xff,   ///< No orders.
  };

  std::uint16_t x; ///< x coordinate of tile.
  std::uint16_t y; ///< y coordinate of tile.
  std::uint16_t _padding1 : 6;
  bool moved : 1; ///< Has the unit moved this turn?
  std::uint16_t _padding1_1 : 6;
  bool veteran : 1; ///< Whether the unit is veteran.
  bool _padding2 : 1;
  bool star : 1;           ///< Display a small star on top of the flag.
  std::uint8_t type;       ///< Unit type index.
  std::uint8_t owner;      ///< Index of unit owner.
  std::uint8_t moves_used; ///< Number of move fragments used.
  std::uint8_t _padding3;
  std::uint8_t hp_lost; ///< Number of HP _lost_.
  /// https://apolyton.net/forum/civilization-series/civilization-i-and-civilization-ii/130935-civilization-ii-sav-scn-file-format?p=4246962#post4246962
  /// However
  /// https://forums.civfanatics.com/threads/hex-editing-sav-file-for-units.637875/
  /// says Byte 12 would be facing direction.
  std::uint8_t work_progress;
  std::uint8_t _padding4;
  std::uint8_t commodity; ///< 00 hidden, 01 wool, ..., 0A uranium, F0 food.
  std::uint8_t _padding5;
  orders orders;           ///< Unit orders as an enum.
  std::uint8_t home_city;  ///< 0xff = unhomed.
  std::uint16_t goto_x;    ///< Goto destination tile x.
  std::uint16_t goto_y;    ///< Goto destination tile y.
  std::uint16_t _links[2]; ///< Which unit is shown on tile.
  std::uint16_t id;        ///< Some sort of unit ID
  std::uint32_t _unknown;  ///< Always 0?
};
// static_assert(sizeof(unit) == 26); // CiC
static_assert(sizeof(unit) == 32); // MGE+

/**
 * Reads in civ2 units information.
 */
std::vector<unit> read_units(QDataStream &bytes, int count)
{
  // TODO layout depends on version!
  // Here MGE
  // SizeOfUnit:=26; if MultiplayerVersion(Civ2Version) then SizeOfUnit:=32;
  auto units = std::vector<unit>(count);
  bytes.readRawData(reinterpret_cast<char *>(units.data()),
                    count * sizeof(unit));
  for (auto &u : units) {
    // Byte-swap 16-bits fields if needed.
    u.x = qFromLittleEndian(u.x);
    u.y = qFromLittleEndian(u.y);
    u.goto_x = qFromLittleEndian(u.goto_x);
    u.goto_y = qFromLittleEndian(u.goto_y);
    u.id = qFromLittleEndian(u.id);
  }
  return units;
}

/**
 * civ2 city data.
 */
struct city {
  std::uint16_t x; ///< x coordinate.
  std::uint16_t y; ///< y coordinate.
  std::array<std::byte, 4> flags;
  std::uint8_t owner;   ///< Current owner.
  std::int8_t size;     ///< City size.
  std::uint8_t founder; ///< Founder civ.
  std::byte _padding1[3];
  std::array<std::uint8_t, 7> citizens;
  /// Specialists in the city.
  std::array<std::uint8_t, 4> specialists;
  std::int16_t foodbox;             ///< Food in food box (0xff = famine)
  std::int16_t shieldbox;           ///< Number of shields in shields box.
  std::int16_t base_trade;          ///< Trade without trade routes.
  std::array<char, 16> name;        ///< City name.
  std::array<std::byte, 3> workers; ///< Citizen placement.
  std::uint8_t _padding2 : 2;
  std::uint8_t specialists_count : 6;    ///< Number of specialists.
  std::array<std::byte, 5> improvements; ///< Bitfield with improvements.
  /// Current production. Positive for units, negative for improvements.
  std::int8_t production;
  std::uint8_t trade_routes; ///< Number of active trade routes.
  std::array<std::uint8_t, 3> commodities_demand;
  std::array<std::uint8_t, 3> commodities_in_route;
  std::array<std::uint16_t, 3> partner_cities;
  std::int16_t science;     ///< Total science.
  std::int16_t tax;         ///< Total tax revenue.
  std::int16_t trade;       ///< Total trade incl. trade routes.
  std::int8_t total_food;   ///< Total food production.
  std::int8_t total_shield; ///< Total shield production.
  std::int8_t happy;        ///< Happy citizens.
  std::int8_t unhappy;      ///< Unhappy citizens, angry count double.
  std::byte _padding3[6];
};
static_assert(offsetof(city, name) == 32);
static_assert(sizeof(city) == 88);

/**
 * Reads in civ2 city information.
 */
std::vector<city> read_cities(QDataStream &bytes, int count)
{
  // TODO layout depends on version!
  // Here MGE
  auto cities = std::vector<city>(count);
  bytes.readRawData(reinterpret_cast<char *>(cities.data()),
                    count * sizeof(city));
  for (auto &c : cities) {
    // Byte-swap 16-bits fields if needed.
    c.x = qFromLittleEndian(c.x);
    c.y = qFromLittleEndian(c.y);
    c.foodbox = qFromLittleEndian(c.foodbox);
    c.shieldbox = qFromLittleEndian(c.shieldbox);
    c.base_trade = qFromLittleEndian(c.base_trade);
    for (auto &p : c.partner_cities) {
      p = qFromLittleEndian(p);
    }
    c.science = qFromLittleEndian(c.science);
    c.tax = qFromLittleEndian(c.tax);
    c.trade = qFromLittleEndian(c.trade);
  }
  return cities;
}

/**
 * Groups all the data present in a civ2 save.
 */
struct game
{
  /// Game header data.
  header head;
  /// Which nation is connected to which player slot (including barbarians).
  std::array<tribe, NUM_PLAYERS> tribes;
  /// Map information.
  civ2::map map;
  /// All units in the game.
  std::vector<unit> units;
  /// All cities in the game.
  std::vector<city> cities;
};

/**
 * Reads in a civ2 saved game.
 */
bool read_saved_game(const QString &path, game &g)
{
  // Open the file
  auto file = QFile(path);
  file.open(QIODevice::ReadOnly);
  QDataStream bytes(&file);
  bytes.setByteOrder(QDataStream::LittleEndian);

  if (!read_header(bytes, g.head)) {
    qCritical(_("Error loading civ2 savegame header."));
    return false;
  }

  fc_assert_ret_val(file.pos() == 584, false);

  for (int i = 1; i < NUM_PLAYERS; ++i) {
    if (!read_tribe(bytes, g.tribes[i])) {
      qCritical(_("Error loading civ2 tribe data."));
      return false;
    }
  }

  bytes.skipRawData(8); // Padding?
  fc_assert_ret_val(file.pos() == 2286, false);

  bytes.skipRawData(1427 * 8); // Techs, gold, diplomacy

  fc_assert_ret_val(file.pos() == 13702, false);
  if (!read_map(bytes, g.map)) {
    qCritical(_("Error loading civ2 savegame map data."));
    return false;
  }

  g.units = read_units(bytes, g.head.unit_count);
  if (g.units.size() != g.head.unit_count) {
    qCritical(_("Error loading civ2 savegame units."));
    return false;
  }

  g.cities = read_cities(bytes, g.head.city_count);
  if (g.cities.size() != g.head.city_count) {
    qCritical(_("Error loading civ2 savegame cities."));
    return false;
  }

  return bytes.status() == QDataStream::Ok;
}
} // namespace civ2

/**************************************************************************/

namespace /* anonymous */ {
/**
 * Data needed while loading the save.
 */
struct load_data {
  /// Freeciv21 players for the civ2 slots.
  std::array<player *, civ2::NUM_PLAYERS> players;
};

/**
 * Loads the ruleset for a civ2 game.
 */
bool setup_ruleset(const civ2::game &g, load_data &data)
{
  Q_UNUSED(g);
  Q_UNUSED(data);

  sz_strlcpy(game.server.rulesetdir, "civ2");
  if (!load_rulesets(nullptr, nullptr, false, nullptr, true, false, true)) {
    qCritical(_("Failed to load ruleset '%s' needed for savegame."), "civ2");
    return false;
  }

  return true;
}

/**
 * Loads players for a civ2 game.
 */
bool setup_players(const civ2::game &g, load_data &data)
{
  // TODO Move elsewhere
  game.scenario.is_scenario = true;
  game.scenario.players = true;
  game.scenario.allow_ai_type_fallback = true;

  // Remove all players. We will recreate them later.
  aifill(0);
  players_iterate(pplayer) { server_remove_player(pplayer); }
  players_iterate_end;

  // Ensure we get a meaningful AI level.
  const auto level = g.head.difficulty < AI_LEVEL_COUNT
                         ? static_cast<ai_level>(g.head.difficulty)
                         : AI_LEVEL_CHEATING;

  auto from_latin1 = QStringDecoder(QStringDecoder::Latin1);

  int slot = 0; // Freeciv21 player slot.
  for (int i = 0; i < g.tribes.size(); ++i) {
    if (!(g.head.players_alive & (1 << i))) {
      // Don't create dead players. They most often don't play any role in
      // scenarios.
      data.players[i] = nullptr;
      continue;
    }

    auto human = g.head.players_human & (1 << i);

    // Create the player.
    auto color = rgbcolor_new(civ2::CIV_COLORS[i] >> 16 & 0xff,
                              civ2::CIV_COLORS[i] >> 8 & 0xff,
                              civ2::CIV_COLORS[i] & 0xff);
    auto pplayer = server_create_player(slot, AI_MOD_DEFAULT, color, true);
    data.players[i] = pplayer;
    server_player_init(pplayer, false, false);
    rgbcolor_destroy(color);

    // Set usernames.
    auto leader_name = from_latin1(g.tribes[i].leader.data());
    server_player_set_name(pplayer, qUtf8Printable(leader_name));
    // Human players are unassigned.
    pplayer->unassigned_user = human;
    sz_strlcpy(pplayer->username, qUtf8Printable(leader_name));
    pplayer->server.orig_username[0] = '\0';
    pplayer->ranked_username[0] = '\0';
    player_delegation_set(pplayer, nullptr);

    // Index 0 is for barbarians.
    if (i == 0) {
      pplayer->ai_common.barbarian_type = LAND_AND_SEA_BARBARIAN;
      server.nbarbarians = 1;
    }

    // Pick a random nation.
    // TODO use info from the save.
    player_set_nation(
        pplayer, pick_a_nation(nullptr, false, true, NOT_A_BARBARIAN));
    ai_traits_init(pplayer);
    pplayer->style = style_of_nation(pplayer->nation);

    // Set some government. 0 is Anarchy so use 1.
    pplayer->government = government_by_number(1);

    // Add it to a team.
    team_add_player(pplayer, team_new(nullptr));

    // AI data.
    // Defaults taken from savegame3.cpp.
    pplayer->ai_common.fuzzy = 0;
    pplayer->ai_common.expand = 100;
    pplayer->ai_common.science_cost = 100;
    pplayer->ai_common.skill_level = level;
    if (!human) {
      set_as_ai(pplayer);
      set_ai_level_directer(pplayer, pplayer->ai_common.skill_level);
      CALL_PLR_AI_FUNC(gained_control, pplayer, pplayer);
    }

    slot++;
  }

  // Make sure the server doesn't remove the newly created players.
  game.info.aifill = player_count();
  // Prevent new players from being added.
  game.server.max_players = player_count();

  shuffle_players();

  return true;
}

/**
 * Loads the map for a civ2 game.
 */
bool setup_map(const civ2::game &g, load_data &data)
{
  Q_UNUSED(data);

  game.info.is_new_game = false;
  wld.map.server.have_huts = false;
  // We can't load civ2 resources.
  game.scenario.have_resources = false;
  wld.map.server.have_resources = false;
  wld.map.server.generator = MAPGEN_SCENARIO;

  // Map dimensions
  wld.map.xsize = g.map.width;
  wld.map.ysize = g.map.height;
  if (wld.map.ysize % 2 == 1) {
    // Freeciv21 really wants this to be even... skip the last row.
    wld.map.ysize--;
  }
  wld.map.topology_id = TF_ISO;
  // TODO wrap?

  map_init_topology();
  main_map_allocate();

  // Detect terrain types.
  std::array<terrain *, civ2::TERRAIN_IDS.size()> terrains;
  {
    terrain_type_iterate(pterrain)
    {
      auto i = civ2::TERRAIN_IDS.find(pterrain->identifier);
      fc_assert_ret_val(i != std::string::npos, false);
      terrains[i] = pterrain;
    }
    terrain_type_iterate_end;
  }

  // Detect extra numbers. This is based on heuristics.
  int irrigation_index = -1, farmland_index = -1, mine_index = -1,
      river_index = -1, road_index = -1, rails_index = -1, fort_index = -1,
      airbase_index = -1, pollution_index = -1;
  extra_type_iterate(pextra)
  {
    if (is_extra_caused_by(pextra, EC_IRRIGATION)) {
      if (BV_ISSET_ANY(pextra->hidden_by)) {
        irrigation_index = extra_index(pextra);
      } else {
        farmland_index = extra_index(pextra);
      }
    } else if (is_extra_caused_by(pextra, EC_MINE)) {
      mine_index = extra_index(pextra);
    } else if (is_extra_caused_by(pextra, EC_ROAD)) {
      if (road_has_flag(extra_road_get(pextra), RF_RIVER)
          && pextra->generated) {
        river_index = extra_index(pextra);
      } else if (extra_road_get(pextra)->move_cost == 0) {
        rails_index = extra_index(pextra);
      } else {
        road_index = extra_index(pextra);
      }
    } else if (is_extra_caused_by(pextra, EC_BASE)) {
      auto gui_type = extra_base_get(pextra)->gui_type;
      if (gui_type == BASE_GUI_FORTRESS) {
        fort_index = extra_index(pextra);
      } else if (gui_type == BASE_GUI_AIRBASE) {
        airbase_index = extra_index(pextra);
      }
    } else if (is_extra_caused_by(pextra, EC_POLLUTION)) {
      pollution_index = extra_index(pextra);
    }
  }
  extra_type_iterate_end;

  fc_assert_ret_val(mine_index >= 0, false);
  fc_assert_ret_val(river_index >= 0, false);
  fc_assert_ret_val(road_index >= 0, false);
  fc_assert_ret_val(rails_index >= 0, false);

  // Load tiles
  for (int y = 0; y < wld.map.ysize; ++y) {
    for (int x = 0; x < wld.map.xsize; ++x) {
      const auto &civ2tile = g.map.tiles[x + y * g.map.width];
      auto ptile = native_pos_to_tile(&(wld.map), x, y);

      // Set terrain
      fc_assert_ret_val(civ2tile.terrain < terrains.size(), false);
      ptile->terrain = terrains[civ2tile.terrain];

      // Set extras
      if (civ2tile.river) {
        BV_SET(ptile->extras, river_index);
      }
      if ((civ2tile.improvements & civ2::IRRIGATION) == civ2::IRRIGATION
          && ptile->terrain->irrigation_result == ptile->terrain) {
        // TODO: Second condition is a Freeciv21 limitation.
        BV_SET(ptile->extras, irrigation_index);
      }
      if ((civ2tile.improvements & civ2::FARMLAND) == civ2::FARMLAND
          && ptile->terrain->irrigation_result == ptile->terrain) {
        // TODO: Second condition is a Freeciv21 limitation.
        BV_SET(ptile->extras, farmland_index);
      }
      if ((civ2tile.improvements & civ2::MINE) == civ2::MINE
          && ptile->terrain->mining_result == ptile->terrain) {
        // TODO: Second condition is a Freeciv21 limitation.
        BV_SET(ptile->extras, mine_index);
      }
      if ((civ2tile.improvements & civ2::ROAD) == civ2::ROAD
          || (civ2tile.improvements & civ2::CITY) == civ2::CITY) {
        BV_SET(ptile->extras, road_index);
      }
      if ((civ2tile.improvements & civ2::RAILROAD) == civ2::RAILROAD) {
        BV_SET(ptile->extras, rails_index);
      }
      if ((civ2tile.improvements & civ2::FORTRESS) == civ2::FORTRESS) {
        BV_SET(ptile->extras, fort_index);
      }
      if ((civ2tile.improvements & civ2::AIRBASE) == civ2::AIRBASE) {
        BV_SET(ptile->extras, airbase_index);
      }
      if ((civ2tile.improvements & civ2::POLLUTION) == civ2::POLLUTION) {
        BV_SET(ptile->extras, pollution_index);
      }
    }
  }

  assign_continent_numbers();

  players_iterate(pplayer)
  {
    // Allocate player private map here; it is needed in different modules
    // besides this one.
    player_map_init(pplayer);
    pplayer->tile_known->fill(false);
  }
  players_iterate_end;

  // Initialize global warming levels.
  game_map_init();

  return true;
}

/**
 * Loads cities from a civ2 game.
 */
bool setup_cities(const civ2::game &g, load_data &data)
{
  auto from_latin1 = QStringDecoder(QStringDecoder::Latin1);

  for (int i = 0; i < g.cities.size(); ++i) {
    auto &c = g.cities[i];

    auto city_name = from_latin1(c.name.data());

    // Get the owner.
    fc_assert_ret_val(c.owner < data.players.size(), false);
    if (!data.players[c.owner]) {
      qWarning(_("Skipping city %s: unsupported owner"),
               qUtf8Printable(city_name));
      continue;
    }
    fc_assert_ret_val(data.players[c.owner], false);

    auto pplayer = data.players[c.owner];
    pplayer->server.got_first_city = true;

    // Create a dummy city.
    auto pcity =
        create_city_virtual(pplayer, nullptr, qUtf8Printable(city_name));
    pcity->id = i;
    adv_city_alloc(pcity);

    // Put it on the map.
    pcity->tile = native_pos_to_tile(&(wld.map), c.x / 2, c.y);
    fc_assert_ret_val(pcity->tile, false);

    // Two cities on the same tile?
    // FIXME: civ2 would apparently load this...
    fc_assert_ret_val(!tile_city(pcity->tile), false);
    pcity->tile->worked = pcity;

    // Tile ownership
    pcity->tile->owner = pplayer;

    // Citizens
    city_size_set(pcity, c.size);
    // TODO assign workers & specialists

    // Production
    city_choose_build_default(pcity);

    // TODO trade routes (second loop)

    identity_number_reserve(pcity->id);
    idex_register_city(&wld, pcity);

    // Vision
    auto pdcity = vision_site_new(0, nullptr, nullptr);
    pdcity->location = pcity->tile;
    pdcity->owner = pplayer;
    pdcity->identity = pcity->id;
    vision_site_size_set(pdcity, pcity->size);
    BV_CLR_ALL(pdcity->improvements);
    change_playertile_site(map_get_player_tile(pdcity->location, pplayer),
                           pdcity);
    identity_number_reserve(pdcity->identity);

    // After everything is loaded, but before vision.
    map_claim_ownership(city_tile(pcity), pplayer, city_tile(pcity), true);

    // adding the city contribution to fog-of-war
    pcity->server.vision = vision_new(pplayer, city_tile(pcity));
    vision_reveal_tiles(pcity->server.vision,
                        game.server.vision_reveal_tiles);
    city_refresh_vision(pcity);

    city_list_append(pplayer->cities, pcity);
  }

  return true;
}
} // anonymous namespace

/**
 * Checks if the file at path looks like a civ2 save.
 */
bool is_civ2_save(const QString &path)
{
  QFile input(path);
  if (!input.open(QIODevice::ReadOnly)) {
    return false;
  }

  return input.read(civ2::MAGIC.size()) == civ2::MAGIC;
}

/**
 * Loads a civ2 save.
 */
bool load_civ2_save(const QString &path)
{
  auto g = civ2::game();
  if (!read_saved_game(path, g)) {
    return false;
  }

  auto data = load_data();
  if (!setup_ruleset(g, data)) {
    return false;
  }
  if (!setup_players(g, data)) {
    return false;
  }
  if (!setup_map(g, data)) {
    return false;
  }
  if (!setup_cities(g, data)) {
    return false;
  }

  players_iterate(pplayer) { map_know_and_see_all(pplayer); }
  players_iterate_end;

  // Experimental!
  qWarning(_("Reading civ2 saves is experimental. Loading is incomplete and "
             "the game may not behave as expected."));
  return true;
}
