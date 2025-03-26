// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

// self
#include "demographic.h"

/**
 * \class demographic
 *
 * Demographics are statistics providing information about players. Each
 * demographic object contains the information for a piece of information.
 */

/**
 * Constructs a demographic.
 */
demographic::demographic(const char *name, int (*get_value)(const player *),
                         const char *(*get_text)(int value),
                         bool larger_is_better)
    : m_name(name), m_get_value(get_value), m_get_text(get_text),
      m_larger_is_better(larger_is_better)
{
}

/**
 * Evaluates the current value of the demographic for a player.
 */
int demographic::evaluate(const player *pplayer) const
{
  return m_get_value(pplayer);
}

/**
 * Turns a demographic value into displayable text.
 */
const char *demographic::text(int value) const { return m_get_text(value); }
