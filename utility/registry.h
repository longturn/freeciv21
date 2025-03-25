// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

class QString;

void registry_module_init();
void registry_module_close();

struct section_file *secfile_new(bool allow_duplicates);
void secfile_destroy(struct section_file *secfile);
struct section_file *secfile_load(const QString &filename,
                                  bool allow_duplicates);

void secfile_allow_digital_boolean(struct section_file *secfile,
                                   bool allow_digital_boolean);

const char *secfile_error();
const char *section_name(const struct section *psection);
