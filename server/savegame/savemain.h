// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "support.h"

struct section_file;
class QString;

bool savegame_load(const QString &path);
void savegame_save(struct section_file *sfile, const char *save_reason,
                   bool scenario);

void save_game(const char *orig_filename, const char *save_reason,
               bool scenario);

void save_system_close();
