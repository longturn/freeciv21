// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "registry.h"

// utility
#include "registry_ini.h"

/**
   Create a section file from a file.  Returns nullptr on error.
 */
struct section_file *secfile_load(const QString &filename,
                                  bool allow_duplicates)
{
  return secfile_load_section(filename, nullptr, allow_duplicates);
}
