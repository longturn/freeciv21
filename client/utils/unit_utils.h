// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
// SPDX-License-Identifier: GPLv3-or-later

#pragma once

#include <vector>

struct unit;
struct unit_list;

std::vector<unit *> sorted(const unit_list *units);
