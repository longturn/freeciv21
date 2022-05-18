/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Qt
#include <QHash>
#include <QString>

// utility
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "connection.h"
#include "packets.h"

// server
#include "connecthand.h"
#include "notify.h"
#include "sernet.h"
#include "srv_main.h"

/* server/scripting */
#include "script_fcdb.h"

#include "fcdb.h"

enum fcdb_option_source {
  AOS_DEFAULT, // Internal default, currently not used
  AOS_FILE,    // Read from config file
  AOS_SET      // Set, currently not used
};

struct fcdb_option {
  enum fcdb_option_source source;
  char *value;
};

QHash<QString, fcdb_option *> fcdb_config;

static bool fcdb_set_option(const char *key, const char *value,
                            enum fcdb_option_source source);
static bool fcdb_load_config(const char *filename);

/**
   Set one fcdb option (or delete it if value == nullptr).
   Replaces any previous setting.
 */
static bool fcdb_set_option(const char *key, const char *value,
                            enum fcdb_option_source source)
{
  struct fcdb_option *oldopt = nullptr;
  bool removed;

  if (value != nullptr) {
    auto newopt = new fcdb_option;

    newopt->value = fc_strdup(value);
    newopt->source = source;

    removed = fcdb_config.contains(key);
    if (removed) {
      oldopt = fcdb_config.value(key);
    }
    fcdb_config[key] = newopt;
  } else {
    removed = fcdb_config.contains(key);
    if (removed) {
      oldopt = fcdb_config.take(key);
    }
  }

  if (removed) {
    /* Overwritten/removed an existing value */
    fc_assert_ret_val(oldopt != nullptr, false);
    delete[] oldopt->value;
    delete oldopt;
    oldopt = nullptr;
  }

  return true;
}

/**
   Load fcdb configuration from file.
   We deliberately don't search datadirs for filename, as we don't want this
   overridden by modpacks etc.
 */
static bool fcdb_load_config(const char *filename)
{
  struct section_file *secfile;

  fc_assert_ret_val(nullptr != filename, false);

  if (!(secfile = secfile_load(filename, false))) {
    qCritical(_("Cannot load fcdb config file '%s':\n%s"), filename,
              secfile_error());
    return false;
  }

  entry_list_iterate(
      section_entries(secfile_section_by_name(secfile, "fcdb")), pentry)
  {
    if (entry_type_get(pentry) == ENTRY_STR) {
      const char *value;
      bool entry_str_get_success = entry_str_get(pentry, &value);

      fc_assert(entry_str_get_success);
      fcdb_set_option(entry_name(pentry), value, AOS_FILE);
    } else {
      qCritical("Value for '%s' in '%s' is not of string type, ignoring",
                entry_name(pentry), filename);
    }
  }
  entry_list_iterate_end;

  /* FIXME: we could arrange to call secfile_check_unused() and have it
   * complain about unused entries (e.g. those not in [fcdb]). */
  secfile_destroy(secfile);

  return true;
}

/**
   Initialize freeciv database system
 */
bool fcdb_init(const char *conf_file)
{
  if (conf_file && strcmp(conf_file, "-")) {
    if (!fcdb_load_config(conf_file)) {
      return false;
    }
  } else {
    log_debug("No fcdb config file.");
  }

  return script_fcdb_init(nullptr);
}

/**
   Return the selected fcdb config value.
 */
const char *fcdb_option_get(const char *type)
{
  if (fcdb_config.contains(type)) {
    return fcdb_config[type]->value;
  } else {
    return nullptr;
  }
}

/**
   Free resources allocated by fcdb system.
 */
void fcdb_free(void)
{
  script_fcdb_free();

  for (auto popt : qAsConst(fcdb_config)) {
    // Dangling pointers freed below
    delete[] popt->value;
    delete popt;
    popt = nullptr;
  }
  fcdb_config.clear();
}
