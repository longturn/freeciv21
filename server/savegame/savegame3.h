// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

void savegame3_load(struct section_file *sfile);
void savegame3_save(struct section_file *sfile, const char *save_reason,
                    bool scenario);
