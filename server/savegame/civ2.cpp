// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "civ2.h"

#include <QFile>
#include <QString>
#include <QtDebug>

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
const static auto NUM_WONDERS = 56;
/// Number of civilizations (including barbarians)
const static auto NUM_CIVS = 8;
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
const static auto MAX_VERSION = CIV2_MULTIPLAYER;

/**
 * The civ2 file header.
 */
struct header {
  std::int16_t version; ///< Save format version number
  std::int16_t turn;    ///< Current turn
  std::int16_t year;    ///< Current year
  int num_techs;        ///< Number of techs in the current version
  /// First nation that discovered each tech
  std::array<char, NUM_TECHS> first_to_discover;
  /// Which nations have discoved which techs (bitfield)
  std::array<char, NUM_TECHS> discovered_by;
  /// City ID for Great Wonders
  std::array<std::int16_t, NUM_WONDERS> wonder_locations;
};
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
  if (head.version > MAX_VERSION) {
    qCritical("Unsupported civ2 file version: %d",
              static_cast<int>(head.version));
    return false;
  }

  if (head.version >= CIV2_TEST_OF_TIME_10) {
    bytes.skipRawData(640); // Unknown use
  }
  bytes.skipRawData(16); // Menu settings

  bytes >> head.turn >> head.year;
  bytes.skipRawData(34); // More game settings, unknown use

  head.num_techs = head.version > CIV2_CLASSIC ? NUM_TECHS :NUM_TECHS_CIC;
  bytes.readRawData(head.first_to_discover.data(), head.num_techs);
  bytes.readRawData(head.discovered_by.data(), head.num_techs);
  for (auto& location: head.wonder_locations) {
    bytes >> location;
  }

  // Unsupported!
  qCritical("Cannot read civ2 saves!");
  return false;
}
