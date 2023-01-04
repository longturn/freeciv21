/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include <vector>

class QMenu;

struct unit;

namespace freeciv {

void add_quick_unit_actions(QMenu *menu, const std::vector<unit *> &units);

} // namespace freeciv
