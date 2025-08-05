// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "civ2.h"

#include <QFile>
#include <QString>

/**
 * Checks if the file at path looks like a civ2 save.
 */
bool is_civ2_save(const QString &path)
{
  QFile input(path);
  if (!input.open(QIODevice::ReadOnly)) {
    return false;
  }

  const auto magic = QByteArrayLiteral("CIVILIZE");
  return input.read(magic.size()) == magic;
}

/**
 * Loads a civ2 save.
 */
bool load_civ2_save(const QString &path)
{
  // Unsupported!
  qCritical("Cannot read civ2 saves!");
  return false;
}
