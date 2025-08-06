// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "civ2.h"

// utility
#include "log.h"

#include <QFile>
#include <QString>
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

namespace /* anonymous */ {
/// Magic string found at the beginning of a save.
const static auto MAGIC = QByteArrayLiteral("CIVILIZE\0\x1A");

/// Number of techs for Conflicts in Civliization
const static auto NUM_TECHS_CIC = 93;
/// Number of techs for most versions
const static auto NUM_TECHS = 100;
/// Number of Great Wonder slots
const static auto NUM_WONDERS = 28;
/// Number of civilizations (including barbarians)
const static auto NUM_PLAYERS = 8;
/// Total number of playable civs.
const static auto NUM_CIVS = 21;
/// Constant for wonders that haven't been built.
const static auto WONDER_NOT_BUILT = -1;
/// Constant for wonders that have been destroyed.
const static auto WONDER_DESTROYED = -2;

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
tribe read_tribe(QDataStream &bytes)
{
  tribe t;
  bytes >> t.city_style;
  bytes.readRawData(t.leader.data(), t.leader.size());
  bytes.readRawData(t.name.data(), t.name.size());
  bytes.readRawData(t.adjective.data(), t.adjective.size());
  for (auto &title : t.leader_title) {
    bytes.readRawData(title.data(), title.size());
  }
  return t;
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
  std::uint8_t terrain : 4; ///< Terrain ID.
  std::byte _padding1 : 2;  ///< Unused?
  bool no_resources : 1;    ///< Cannot have resources.
  bool river : 1;           ///< Has a river.
  std::byte improvements;   ///< Tile improvements.
  std::byte _padding2 : 5;  ///< Unused?
  std::byte city_owner : 3; ///< ID of civ that can work this tile.
  std::int8_t island;       ///< Island number.
  std::byte visibility;     ///< Who has explored the tile (bitfield).
  std::byte owner : 4;      ///< Tile owner (for bases).
  std::byte fertility : 4;  ///< ??
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
map read_map(QDataStream &bytes)
{
  map m;

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

  return m;
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
  }
  return units;
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

  return input.read(MAGIC.size()) == MAGIC;
}

/**
 * Loads a civ2 save.
 */
bool load_civ2_save(const QString &path)
{
  // Open the file
  auto file = QFile(path);
  file.open(QIODevice::ReadOnly);
  QDataStream bytes(&file);
  bytes.setByteOrder(QDataStream::LittleEndian);

  // Header
  auto head = header();

  bytes.skipRawData(MAGIC.size()); // CIVILIZE 0x00 0x1A
  bytes >> head.version;

  qDebug() << "Loading civ2 file with version" << head.version;
  if (head.version != SUPPORTED_VERSION) {
    qCritical("Unsupported civ2 file version: %d",
              static_cast<int>(head.version));
    return false;
  }

  if (head.version >= CIV2_TEST_OF_TIME_10) {
    bytes.skipRawData(640); // Unknown use
  }
  bytes.skipRawData(16); // Menu settings

  bytes >> head.turn >> head.year;
  bytes.skipRawData(26); // More game settings, unknown use
  bytes >> head.unit_count >> head.city_count;
  bytes.skipRawData(204); // Unknown use

  fc_assert_ret_val(file.pos() == 266, false);

  head.num_techs = head.version > CIV2_CLASSIC ? NUM_TECHS : NUM_TECHS_CIC;
  bytes.readRawData(head.first_to_discover.data(), head.num_techs);
  bytes.readRawData(head.discovered_by.data(), head.num_techs);
  for (auto &location : head.wonder_locations) {
    bytes >> location;
  }

  bytes.skipRawData(62); // Unknown/unused
  fc_assert_ret_val(file.pos() == 584, false);

  std::array<tribe, NUM_PLAYERS> tribes;
  for (int i = 1; i < NUM_PLAYERS; ++i) {
    tribes[i] = read_tribe(bytes);
  }

  bytes.skipRawData(8); // Padding?
  fc_assert_ret_val(file.pos() == 2286, false);

  bytes.skipRawData(1427 * 8); // Techs, gold, diplomacy

  fc_assert_ret_val(file.pos() == 13702, false);
  auto map = read_map(bytes);

  auto units = read_units(bytes, head.unit_count);

  // Unsupported!
  qCritical("Cannot read civ2 saves!");
  return false;
}
