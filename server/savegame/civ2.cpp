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
#include "fc_types.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "nation.h"
#include "player.h"
#include "rgbcolor.h"
#include "specialist.h"
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
#include "vision.h"

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
  std::int16_t city_style = 0;       ///< Which city style is currently used
  std::array<char, 24> leader{'\0'}; ///< Name of the tribe's leader
  std::array<char, 24> name{'\0'};   ///< Name of the tribe
  std::array<char, 24> adjective{'\0'}; ///< Adjective for the tribe
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
 * Diplomatic relations between players
 */
enum diplomacy : std::uint8_t {
  CONTACT = 0x01,    ///< Civs are in contact
  CEASE_FIRE = 0x02, ///< Civs have a cease-fire
  PEACE = 0x04,      ///< Civs are in peace
  ALLIANCE = 0x08,   ///< Civs are allied
  VENDETTA = 0x10,   ///< ?
  EMBASSY = 0x80,    ///< Civ has an embassy
  WAR = 0x20,        ///< Civs are at war (in 2nd byte)
};

/**
 * Additional information about a tribe (techs, money, treaties...)
 */
struct tribe_info {
  std::uint8_t gender;   ///< Ruler gender.
  std::int16_t money;    ///< Gold.
  std::uint8_t nation;   ///< Which civ is being played (index in RULES.TXT).
  std::uint8_t research; ///< Research progress.
  std::uint8_t current_research; ///< Current tech. 0xFF means no goal.
  std::uint8_t tax;              ///< Tax(?) rate 0-10.
  std::uint8_t science;          ///< Science(?) rate 0-10.
  std::uint8_t govermnent;       ///< 0 to 7 inclusive. 0 is Anarchy, 1 is
                                 ///< Despotism, 6 is Democracy.
  std::array<std::uint8_t[4], NUM_PLAYERS - 1>
      treaties;                       /// Treaties bitfield.
  std::array<std::uint8_t, 11> techs; ///< Known technologies.
  std::int16_t military_demo;         ///< "Military demographic value"
  std::array<std::int16_t, NUM_PLAYERS - 1> last_contact;
};

/**
 * Reads in techs, money, treaties, ...
 */
bool read_player(QDataStream &bytes, tribe_info &ti)
{
  const auto start = bytes.device()->pos();
  qDebug() << "Player block starting at byte" << start;

  std::uint8_t skip;
  std::uint16_t skip2;

  bytes >> skip >> ti.gender >> ti.money >> skip2 >> ti.nation >> skip
      >> ti.research >> skip >> ti.current_research;
  bytes.skipRawData(8);
  // NOTE: The format of tax and science is unclear in "Everything about Hex
  // Editing". What's below is a guess.
  bytes >> ti.science >> ti.tax >> ti.govermnent;
  bytes.skipRawData(14);
  bytes.readRawData(reinterpret_cast<char *>(ti.treaties.data()),
                    4 * (NUM_PLAYERS - 1));
  bytes.skipRawData(24);
  bytes.readRawData(reinterpret_cast<char *>(ti.techs.data()),
                    ti.techs.size());
  fc_assert_ret_val(bytes.device()->pos() == start + 99, false);
  bytes.skipRawData(897);
  fc_assert_ret_val(bytes.device()->pos() == start + 996, false);
  for (auto &lc : ti.last_contact) {
    bytes >> lc;
  }
  bytes.skipRawData(418);
  fc_assert_ret_val(bytes.device()->pos() == start + 1428, false);

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
  std::int8_t visibility;     ///< Who has explored the tile (bitfield).
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
  std::byte _padding1[10];
  /// Specialists in the city.
  std::array<std::uint8_t, 4> specialists;
  std::int16_t foodbox;             ///< Food in food box (0xff = famine)
  std::int16_t shieldbox;           ///< Number of shields in shields box.
  std::int16_t base_trade;          ///< Trade without trade routes.
  std::array<char, 16> name;        ///< City name.
  std::array<std::uint8_t, 3> workers; ///< Citizen placement.
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
  std::byte _padding3[6];
  std::uint8_t happy;       ///< Happy citizens.
  std::uint8_t unhappy;     ///< Unhappy citizens, angry count double.
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
  /// Player statistics.
  std::array<tribe_info, NUM_PLAYERS> tribe_infos;
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

  // NOTE: "Everything about Hex Editing states that the Techs & Money part
  // starts on byte 2287, but at least for MGE this is wrong! It starts 8
  // bytes earlier and the blocks are 1 byte longer than stated.
  fc_assert_ret_val(file.pos() == 2278, false);

  for (auto &ti : g.tribe_infos) {
    if (!read_player(bytes, ti)) {
      qCritical(_("Error loading civ2 player data."));
      return false;
    }
  }

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

    pplayer->is_male = g.tribe_infos[i].gender == 0;

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
    pplayer->economic.gold = g.tribe_infos[i].money;
    pplayer->economic.tax = 10 * g.tribe_infos[i].tax;
    pplayer->economic.science = 10 * g.tribe_infos[i].science;
    pplayer->economic.luxury =
        100 - pplayer->economic.tax - pplayer->economic.science;

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
 * Converts civ2 diplomatic states to Freeciv21.
 */
bool setup_diplomacy(const civ2::game &g, load_data &data)
{
  // No diplomacy with barbarians => start at 1
  for (int i = 1; i < civ2::NUM_PLAYERS; ++i) {
    auto pplayer = data.players[i];
    if (!pplayer) {
      continue;
    }

    BV_CLR_ALL(pplayer->real_embassy);

    for (int j = 1; j < civ2::NUM_PLAYERS; ++j) {
      auto pplayer2 = data.players[j];
      if (i == j || !pplayer2) {
        continue;
      }

      auto ds = player_diplstate_get(pplayer, pplayer2);
      auto treaties = g.tribe_infos[i].treaties[j - 1][0];
      if (treaties & civ2::EMBASSY) {
        BV_SET(pplayer->real_embassy, player_index(pplayer2));
      }
      if (treaties & civ2::ALLIANCE) {
        ds->type = DS_ALLIANCE;
      } else if (treaties & civ2::PEACE) {
        ds->type = DS_PEACE;
      } else if (treaties & civ2::CEASE_FIRE) {
        ds->type = DS_CEASEFIRE;
        ds->turns_left = 16; // FIXME How to import?
      } else if (treaties & civ2::CONTACT) {
        ds->type = DS_WAR; // FIXME This would be "None" in civ2
      }
      // Second status byte
      treaties = g.tribe_infos[i].treaties[j - 1][1];
      if (treaties & civ2::WAR) {
        ds->type = DS_WAR;
      }
      ds->max_state = ds->type;
    }
  }

  return true;
}

/**
 * Converts civ2-style bit fields to Freeciv21 extras.
 */
struct extra_data
{
  // Freeciv21 extra IDs
  int irrigation = -1, farmland = -1, mine = -1,
      river = -1, road = -1, rails = -1, fort = -1,
      airbase = -1, pollution = -1;

  /**
   * Detect extra numbers from the ruleset. This is based on heuristics.
   */
  extra_data() {
    extra_type_iterate(pextra)
    {
      if (is_extra_caused_by(pextra, EC_IRRIGATION)) {
        if (BV_ISSET_ANY(pextra->hidden_by)) {
          irrigation = extra_index(pextra);
        } else {
          farmland = extra_index(pextra);
        }
      } else if (is_extra_caused_by(pextra, EC_MINE)) {
        mine = extra_index(pextra);
      } else if (is_extra_caused_by(pextra, EC_ROAD)) {
        if (road_has_flag(extra_road_get(pextra), RF_RIVER)
            && pextra->generated) {
          river = extra_index(pextra);
        } else if (extra_road_get(pextra)->move_cost == 0) {
          rails = extra_index(pextra);
        } else {
          road = extra_index(pextra);
        }
      } else if (is_extra_caused_by(pextra, EC_BASE)) {
        auto gui_type = extra_base_get(pextra)->gui_type;
        if (gui_type == BASE_GUI_FORTRESS) {
          fort = extra_index(pextra);
        } else if (gui_type == BASE_GUI_AIRBASE) {
          airbase = extra_index(pextra);
        }
      } else if (is_extra_caused_by(pextra, EC_POLLUTION)) {
        pollution = extra_index(pextra);
      }
    }
    extra_type_iterate_end;
  }

  /**
   * Checks that all extra numbers are good.
   */
  bool check() const
  {
    fc_assert_ret_val(irrigation >= 0, false);
    fc_assert_ret_val(farmland >= 0, false);
    fc_assert_ret_val(mine >= 0, false);
    fc_assert_ret_val(river >= 0, false);
    fc_assert_ret_val(road >= 0, false);
    fc_assert_ret_val(rails >= 0, false);
    fc_assert_ret_val(fort >= 0, false);
    fc_assert_ret_val(airbase >= 0, false);
    fc_assert_ret_val(pollution >= 0, false);
    return true;
  }

  /**
   * Converts civ2 improvmeents to Freeciv21 extras.
   */
  void set(std::uint8_t improvements, bv_extras &extras,
           const terrain *pterrain) const
  {
    if ((improvements & civ2::IRRIGATION) == civ2::IRRIGATION
        && pterrain->irrigation_result == pterrain) {
      // TODO: Second condition is a Freeciv21 limitation.
      BV_SET(extras, irrigation);
    }
    if ((improvements & civ2::FARMLAND) == civ2::FARMLAND
        && pterrain->irrigation_result == pterrain) {
      // TODO: Second condition is a Freeciv21 limitation.
      BV_SET(extras, farmland);
    }
    if ((improvements & civ2::MINE) == civ2::MINE
        && pterrain->mining_result == pterrain) {
      // TODO: Second condition is a Freeciv21 limitation.
      BV_SET(extras, mine);
    }
    if ((improvements & civ2::ROAD) == civ2::ROAD
        || (improvements & civ2::CITY) == civ2::CITY) {
      BV_SET(extras, road);
    }
    if ((improvements & civ2::RAILROAD) == civ2::RAILROAD) {
      BV_SET(extras, rails);
    }
    if ((improvements & civ2::FORTRESS) == civ2::FORTRESS) {
      BV_SET(extras, fort);
    }
    if ((improvements & civ2::AIRBASE) == civ2::AIRBASE) {
      BV_SET(extras, airbase);
    }
    if ((improvements & civ2::POLLUTION) == civ2::POLLUTION) {
      BV_SET(extras, pollution);
    }
  }
};

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

  auto extras = extra_data();
  if (!extras.check()) {
    return false;
  }

  // Vision
  players_iterate(pplayer)
  {
    // Allocate player private map.
    player_map_init(pplayer);
    pplayer->tile_known->fill(false);
  }
  players_iterate_end;

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
        BV_SET(ptile->extras, extras.river);
      }
      extras.set(civ2tile.improvements, ptile->extras, ptile->terrain);

      // Vision. Skip barbarians, they see everything.
      for (int i = 1; i < civ2::NUM_PLAYERS; ++i) {
        if (!data.players[i] || !(civ2tile.visibility & (1 << i))) {
          continue;
        }

        map_set_known(ptile, data.players[i]);

        auto plrt = map_get_player_tile(ptile, data.players[i]);
        plrt->terrain = ptile->terrain; // Always visible in civ2.

        if (civ2tile.river) {
          BV_SET(plrt->extras, extras.river); // Always visible in civ2.
        }

        auto impr = g.map.visible_improvements[i][x + y * g.map.width];
        extras.set(impr, plrt->extras, ptile->terrain);
      }
    }
  }

  assign_continent_numbers();

  // Initialize global warming levels.
  game_map_init();

  return true;
}

/**
 * Imports a city's citizens: places workers on the map, creates specialists,
 * ...
 */
bool setup_citizens(const civ2::city &c, city *pcity)
{
  city_size_set(pcity, c.size);

  // civ2 citizens are encoded on 3 bytes where each bit maps to a tile. We
  // process one at a time with the following function.
  auto process = [pcity](std::uint8_t byte, const auto &tiles) {
    bool ok = true;
    for (int i = 0; i < tiles.size(); ++i) {
      bool worked = byte & (1 << i);
      if (!worked) {
        continue;
      }
      auto tile = tiles[i];
      // May happen next to the edge of the map.
      fc_assert_action(tile, ok = false; continue);
      fc_assert_action(!tile->worked, ok = false; continue);

      tile_set_worked(tile, pcity);
    }
    return ok;
  };

  // First byte = innermost 8 tiles.
  bool ok =
      process(c.workers[0],
              std::array{
                  // Directions in civ2 order.
                  mapstep(&(wld.map), city_tile(pcity), DIR8_NORTH),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_NORTHEAST),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_EAST),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_SOUTHEAST),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_SOUTH),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_SOUTHWEST),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_WEST),
                  mapstep(&(wld.map), city_tile(pcity), DIR8_NORTHWEST),
              });

  // Outermost tiles. The layout is as follows:
  //               11      4
  //           3       x       0
  //       10      x       x       5
  //           x      CITY     x
  //       9       x       x       6
  //           2       x       1
  //               8       7
  // Bit indices 0-7 are in byte 1, 8-11 are in byte 2.
  auto double_step = [pcity](direction8 dir1, direction8 dir2) {
    auto t1 = mapstep(&(wld.map), city_tile(pcity), dir1);
    return t1 ? mapstep(&(wld.map), t1, dir2) : nullptr;
  };
  // Byte 1
  ok &= process(c.workers[1], std::array{
                                  double_step(DIR8_NORTH, DIR8_NORTH),
                                  double_step(DIR8_EAST, DIR8_EAST),
                                  double_step(DIR8_SOUTH, DIR8_SOUTH),
                                  double_step(DIR8_WEST, DIR8_WEST),
                                  double_step(DIR8_NORTH, DIR8_NORTHWEST),
                                  double_step(DIR8_NORTH, DIR8_NORTHEAST),
                                  double_step(DIR8_EAST, DIR8_NORTHEAST),
                                  double_step(DIR8_EAST, DIR8_SOUTHEAST),
                              });
  // Byte 2.
  ok &= process(c.workers[2], std::array{
                                  double_step(DIR8_SOUTH, DIR8_SOUTHEAST),
                                  double_step(DIR8_SOUTH, DIR8_SOUTHWEST),
                                  double_step(DIR8_WEST, DIR8_SOUTHWEST),
                                  double_step(DIR8_WEST, DIR8_NORTHWEST),
                              });

  // Dumb specialist assignment.
  pcity->specialists[DEFAULT_SPECIALIST] = c.specialists_count;
  // TODO assign actual specialists

  // Dumb feeling assignment.
  for (int feel = FEELING_BASE; feel < FEELING_LAST; feel++) {
    pcity->feel[CITIZEN_HAPPY][feel] = c.happy;
    pcity->feel[CITIZEN_UNHAPPY][feel] = c.unhappy;
    pcity->feel[CITIZEN_ANGRY][feel] = 0;
    pcity->feel[CITIZEN_CONTENT][feel] =
        c.size - c.happy - c.unhappy - c.specialists_count;
  }

  return ok;
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
    if (!setup_citizens(c, pcity)) {
      qWarning(_("Problems loading citizens for %s"), pcity->name);
    }

    // Production
    city_choose_build_default(pcity);

    // TODO trade routes (second loop)

    identity_number_reserve(pcity->id);
    idex_register_city(&wld, pcity);

    // Who can see this city? Skip barbarians, they see everything.
    for (int j = 1; j < civ2::NUM_PLAYERS; ++j) {
      if (!data.players[j]) {
        continue; // No player in slot.
      }

      const auto &civ2tile = g.map.tiles[city_tile(pcity)->index];
      if (!(civ2tile.visibility & (1 << j))) {
        continue; // Player cannot see this tile.
      }

      auto impr = g.map.visible_improvements[j][city_tile(pcity)->index];
      if ((impr & civ2::CITY) != civ2::CITY) {
        continue; // Player doesn't know about the city.
      }

      auto plrt = map_get_player_tile(city_tile(pcity), data.players[j]);
      auto pdcity = vision_site_new_from_city(pcity);
      // BV_CLR_ALL(pdcity->improvements);
      change_playertile_site(plrt, pdcity);
    }

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

  // Get a random state.
  init_game_seed();

  auto data = load_data();
  if (!setup_ruleset(g, data)) {
    return false;
  }
  if (!setup_players(g, data)) {
    return false;
  }
  if (!setup_diplomacy(g, data)) {
    return false;
  }
  if (!setup_map(g, data)) {
    return false;
  }
  if (!setup_cities(g, data)) {
    return false;
  }
  // Barbarians can see everything
  if (data.players[0]) {
    map_know_and_see_all(data.players[0]);
  }

  // Experimental!
  qWarning(_("Reading civ2 saves is experimental. Loading is incomplete and "
             "the game may not behave as expected."));
  return true;
}
