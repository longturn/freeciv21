/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include <sqlite3.h>

// utility
#include "capability.h"
#include "fcintl.h"
#include "registry.h"
#include "registry_ini.h"

#include "mpdb.h"

#define MPDB_CAPSTR "+mpdb"

#define MPDB_FORMAT_VERSION "1"

static sqlite3 *main_handle = nullptr;
static sqlite3 *scenario_handle = nullptr;

static int mpdb_query(sqlite3 *handle, const char *query);

/**
   Construct install info list from file.
 */
void load_install_info_list(const char *filename)
{
  struct section_file *file;
  const char *caps;

  file = secfile_load(filename, false);

  if (file == nullptr) {
    /* This happens in first run - or actually all runs until something is
     * installed. Previous run has not saved database. */
    log_debug("No install info file");

    return;
  }

  caps = secfile_lookup_str(file, "info.options");

  if (caps == nullptr) {
    qCritical("MPDB %s missing capability information", filename);
    secfile_destroy(file);
    return;
  }

  if (!has_capabilities(MPDB_CAPSTR, caps)) {
    qCritical("Incompatible mpdb file %s:", filename);
    qCritical("  file options:      %s", caps);
    qCritical("  supported options: %s", MPDB_CAPSTR);

    secfile_destroy(file);

    return;
  }

  if (file != nullptr) {
    bool all_read = false;
    int i;

    for (i = 0; !all_read; i++) {
      const char *str;
      char buf[80];

      fc_snprintf(buf, sizeof(buf), "modpacks.mp%d", i);

      str = secfile_lookup_str_default(file, nullptr, "%s.name", buf);

      if (str != nullptr) {
        const char *type;
        const char *ver;

        type = secfile_lookup_str(file, "%s.type", buf);
        ver = secfile_lookup_str(file, "%s.version", buf);

        auto mptype = modpack_type_by_name(type, fc_strcasecmp);
        fc_assert_action(modpack_type_is_valid(mptype), break);
        mpdb_update_modpack(str, mptype, ver);
      } else {
        all_read = true;
      }
    }

    secfile_destroy(file);
  }
}

/**
   SQL query to database
 */
static int mpdb_query(sqlite3 *handle, const char *query)
{
  int ret;
  sqlite3_stmt *stmt;

  ret = sqlite3_prepare_v2(handle, query, -1, &stmt, nullptr);

  if (ret == SQLITE_OK) {
    ret = sqlite3_step(stmt);
  }

  if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
    qCritical("Query \"%s\" failed. (%d)", query, ret);
  }

  if (int errcode = sqlite3_finalize(stmt); errcode != SQLITE_OK) {
    qCritical("Finalizing query \"%s\" returned error. (%d)", query, ret);
  }

  return ret;
}

/**
   Create modpack database
 */
void create_mpdb(const char *filename, bool scenario_db)
{
  sqlite3 **handle;
  int ret;
  int llen = qstrlen(filename) + 1;
  char *local_name = new char[llen];
  int i;

  qstrncpy(local_name, filename, llen);
  for (i = llen - 1; local_name[i] != '/'; i--) {
    // Nothing
  }
  local_name[i] = '\0';
  if (!make_dir(local_name)) {
    qCritical(_("Can't create directory \"%s\" for modpack database."),
              local_name);
    return;
  }

  if (scenario_db) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  ret = sqlite3_open_v2(filename, handle,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

  if (ret == SQLITE_OK) {
    ret = mpdb_query(
        *handle,
        "create table meta (version INTEGER default " MPDB_FORMAT_VERSION
        ");");
  }

  if (ret == SQLITE_DONE) {
    ret = mpdb_query(*handle,
                     "create table modpacks (name VARCHAR(60) NOT null, "
                     "type VARCHAR(32), version VARCHAR(32) NOT null);");
  }

  if (ret == SQLITE_DONE) {
    log_debug("Created %s", filename);
  } else {
    qCritical(_("Creating \"%s\" failed: %s"), filename,
              sqlite3_errstr(ret));
  }
}

/**
   Open existing database
 */
void open_mpdb(const char *filename, bool scenario_db)
{
  sqlite3 **handle;
  int ret;

  if (scenario_db) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  ret = sqlite3_open_v2(filename, handle, SQLITE_OPEN_READWRITE, nullptr);

  if (ret != SQLITE_OK) {
    qCritical(_("Opening \"%s\" failed: %s"), filename, sqlite3_errstr(ret));
  }
}

/**
   Close open databases
 */
void close_mpdbs()
{
  sqlite3_close(main_handle);
  main_handle = nullptr;
  sqlite3_close(scenario_handle);
  scenario_handle = nullptr;
}

/**
   Update modpack information in database
 */
bool mpdb_update_modpack(const char *name, enum modpack_type type,
                         const char *version)
{
  sqlite3 **handle;
  int ret;
  char qbuf[2048];

  if (type == MPT_SCENARIO) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  sqlite3_snprintf(sizeof(qbuf), qbuf,
                   "select * from modpacks where name is '%q';", name);
  ret = mpdb_query(*handle, qbuf);

  if (ret == SQLITE_ROW) {
    sqlite3_snprintf(sizeof(qbuf), qbuf,
                     "update modpacks set type = '%q', version = '%q' where "
                     "name is '%q';",
                     modpack_type_name(type), version, name);
    ret = mpdb_query(*handle, qbuf);
  } else {
    // Completely new modpack
    sqlite3_snprintf(sizeof(qbuf), qbuf,
                     "insert into modpacks values ('%q', '%q', '%q');", name,
                     modpack_type_name(type), version);
    ret = mpdb_query(*handle, qbuf);
  }

  if (ret != SQLITE_DONE) {
    qCritical(_("Failed to insert modpack '%s' information"), name);
  }

  return ret != SQLITE_DONE;
}

/**
   Return version of modpack. The caller is responsible to free the returned
   string.
 */
const char *mpdb_installed_version(const char *name, enum modpack_type type)
{
  sqlite3 **handle;
  int ret;
  char qbuf[2048];
  const char *version = nullptr;
  sqlite3_stmt *stmt;

  if (type == MPT_SCENARIO) {
    handle = &scenario_handle;
  } else {
    handle = &main_handle;
  }

  sqlite3_snprintf(sizeof(qbuf), qbuf,
                   "select * from modpacks where name is '%q';", name);
  ret = sqlite3_prepare_v2(*handle, qbuf, -1, &stmt, nullptr);

  if (ret == SQLITE_OK) {
    ret = sqlite3_step(stmt);
  }

  if (ret == SQLITE_ROW) {
    version = qstrdup((const char *) sqlite3_column_text(stmt, 2));
  }

  if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
    qCritical("Query to get installed version for \"%s\" failed. (%d)", name,
              ret);
  }

  if (int errcode = sqlite3_finalize(stmt); errcode != SQLITE_OK) {
    qCritical(
        "Finalizing query to get installed version for \"%s\" failed. (%d)",
        name, ret);
  }

  return version;
}
