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
#include <cstdarg>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "registry_ini.h"
#include "shared.h"
#include "support.h"

// common
#include "fc_types.h"
#include "requirements.h"
#include "worklist.h"

// client
#include "client_main.h"

#include "global_worklist.h"

enum global_worklist_status {
  /* Means we have a list of kind of universal and their name, but we
   * don't know the id they could have on the server's ruleset.  In this
   * case, we use the unbuilt part and not the worklist one. */
  STATUS_UNBUILT,

  // Means this worklist is plainly usable at running time.
  STATUS_WORKLIST
};

struct uni_name {
  char *kind;
  char *name;
};

// The global worklist structure.
struct global_worklist {
  int id;
  char name[MAX_LEN_NAME];
  enum global_worklist_status status;
  union {
    struct {
      int length;
      uni_name entries[MAX_LEN_WORKLIST];
    } unbuilt;
    struct worklist worklist;
  };
};

static bool global_worklist_load(struct section_file *file, const char *path,
                                 ...)
    fc__attribute((__format__(__printf__, 2, 3)));
static void global_worklist_save(const struct global_worklist *pgwl,
                                 struct section_file *file, int fill_until,
                                 const char *path, ...)
    fc__attribute((__format__(__printf__, 4, 5)));

/**
   Initialize the client global worklists.
 */
void global_worklists_init()
{
  if (!client.worklists) {
    client.worklists = global_worklist_list_new();
  }
}

/**
   Free the client global worklists.
 */
void global_worklists_free()
{
  if (client.worklists) {
    global_worklists_iterate_all(pgwl) { global_worklist_destroy(pgwl); }
    global_worklists_iterate_all_end;
    global_worklist_list_destroy(client.worklists);
    client.worklists = nullptr;
  }
}

/**
   Check if the global worklists are valid or not for the ruleset.
 */
void global_worklists_build()
{
  global_worklists_iterate_all(pgwl)
  {
    if (pgwl->status == STATUS_UNBUILT) {
      struct worklist worklist;
      struct uni_name *puni_name;
      int i;

      // Build a worklist.
      worklist_init(&worklist);
      for (i = 0; i < pgwl->unbuilt.length; i++) {
        struct universal source;

        puni_name = pgwl->unbuilt.entries + i;
        source = universal_by_rule_name(puni_name->kind, puni_name->name);
        if (source.kind == universals_n_invalid()) {
          /* This worklist is not valid on this ruleset.
           * N.B.: Don't remove it to resave it in client rc file. */
          break;
        } else {
          worklist_append(&worklist, &source);
        }
      }

      if (worklist_length(&worklist) != pgwl->unbuilt.length) {
        /* Somewhat in this worklist is not supported by the current
         * ruleset.  Don't try to build it, but keep it for later save. */
        continue;
      }

      // Now the worklist is built, change status.
      for (i = 0; i < pgwl->unbuilt.length; i++) {
        puni_name = pgwl->unbuilt.entries + i;
        delete[] puni_name->kind;
        delete[] puni_name->name;
      }
      pgwl->status = STATUS_WORKLIST;
      worklist_copy(&pgwl->worklist, &worklist);
    }
  }
  global_worklists_iterate_all_end;
}

/**
   Convert the universal pointers to strings to work out-ruleset.
 */
void global_worklists_unbuild()
{
  global_worklists_iterate_all(pgwl)
  {
    if (pgwl->status == STATUS_WORKLIST) {
      struct worklist worklist;
      struct uni_name *puni_name;
      int i;

      // Copy before over-write.
      worklist_copy(&worklist, &pgwl->worklist);
      pgwl->status = STATUS_UNBUILT;

      pgwl->unbuilt.length = worklist_length(&worklist);
      for (i = 0; i < worklist_length(&worklist); i++) {
        puni_name = pgwl->unbuilt.entries + i;
        puni_name->kind =
            fc_strdup(universal_type_rule_name(worklist.entries + i));
        puni_name->name =
            fc_strdup(universal_rule_name(worklist.entries + i));
      }
    }
  }
  global_worklists_iterate_all_end;
}

/**
   Returns a new created global worklist structure.
 */
static struct global_worklist *
global_worklist_alloc(enum global_worklist_status type)
{
  static int last_id = 0;
  struct global_worklist *pgwl = new global_worklist[1]();

  pgwl->id = ++last_id;
  pgwl->status = type;

  // Specific initializer.
  switch (pgwl->status) {
  case STATUS_UNBUILT:
    // All members set to 0 by fc_calloc.
    break;
  case STATUS_WORKLIST:
    worklist_init(&pgwl->worklist);
    break;
  }

  global_worklist_list_append(client.worklists, pgwl);

  return pgwl;
}

/**
   Destroys a glocal worklist.
 */
void global_worklist_destroy(struct global_worklist *pgwl)
{
  fc_assert_ret(nullptr != pgwl);

  global_worklist_list_remove(client.worklists, pgwl);

  // Specific descturctor.
  switch (pgwl->status) {
  case STATUS_UNBUILT: {
    struct uni_name *puni_name;
    int i;

    for (i = 0; i < pgwl->unbuilt.length; i++) {
      puni_name = pgwl->unbuilt.entries + i;
      puni_name->kind = pgwl->unbuilt.entries[i].kind;
      puni_name->name = pgwl->unbuilt.entries[i].name;
      delete[] puni_name->kind;
      delete[] puni_name->name;
    }
  } break;
  case STATUS_WORKLIST:
    break;
  }

  delete[] pgwl;
}

/**
   Creates a new global worklist form a normal worklist.
 */
struct global_worklist *global_worklist_new(const char *name)
{
  struct global_worklist *pgwl = global_worklist_alloc(STATUS_WORKLIST);

  global_worklist_set_name(pgwl, name);
  return pgwl;
}

/**
   Returns TRUE if this global worklist is valid.
 */
bool global_worklist_is_valid(const struct global_worklist *pgwl)
{
  return pgwl && pgwl->status == STATUS_WORKLIST;
}

/**
   Sets the worklist. Return TRUE on success.
 */
bool global_worklist_set(struct global_worklist *pgwl,
                         const struct worklist *pwl)
{
  if (pgwl && pgwl->status == STATUS_WORKLIST && pwl) {
    worklist_copy(&pgwl->worklist, pwl);
    return true;
  }
  return false;
}

/**
   Returns the worklist of this global worklist or nullptr if it's not valid.
 */
const struct worklist *
global_worklist_get(const struct global_worklist *pgwl)
{
  if (pgwl && pgwl->status == STATUS_WORKLIST) {
    return &pgwl->worklist;
  } else {
    return nullptr;
  }
}

/**
   Returns the id of the global worklist.
 */
int global_worklist_id(const struct global_worklist *pgwl)
{
  fc_assert_ret_val(nullptr != pgwl, 0);
  return pgwl->id;
}

/**
   Returns the global worklist corresponding to this id.
   N.B.: It can returns an invalid glocbal worklist.
 */
struct global_worklist *global_worklist_by_id(int id)
{
  global_worklists_iterate_all(pgwl)
  {
    if (pgwl->id == id) {
      return pgwl;
    }
  }
  global_worklists_iterate_all_end;

  return nullptr;
}

/**
   Sets the name of this global worklist.
 */
void global_worklist_set_name(struct global_worklist *pgwl, const char *name)
{
  if (name) {
    sz_strlcpy(pgwl->name, name);
  }
}

/**
   Return the name of the global worklist.
 */
const char *global_worklist_name(const struct global_worklist *pgwl)
{
  fc_assert_ret_val(nullptr != pgwl, nullptr);
  return pgwl->name;
}

/**
   Load a global worklist form a section file.  Returns FALSE if we
   reached the end of the array.
 */
static bool global_worklist_load(struct section_file *file, const char *path,
                                 ...)
{
  struct global_worklist *pgwl;
  const char *kind;
  const char *name;
  char path_str[1024];
  va_list ap;
  int i, length;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  fc_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  length = secfile_lookup_int_default(file, -1, "%s.wl_length", path_str);
  if (length == -1) {
    // Not set.
    return false;
  }
  length = MIN(length, MAX_LEN_WORKLIST);
  pgwl = global_worklist_alloc(STATUS_UNBUILT);
  global_worklist_set_name(
      pgwl, secfile_lookup_str_default(file, _("(noname)"), "%s.wl_name",
                                       path_str));

  for (i = 0; i < length; i++) {
    kind = secfile_lookup_str_default(file, nullptr, "%s.wl_kind%d",
                                      path_str, i);

    if (!kind) {
      // before 2.2.0 unit production was indicated by flag.
      bool is_unit = secfile_lookup_bool_default(
          file, false, "%s.wl_is_unit%d", path_str, i);
      kind = universals_n_name(is_unit ? VUT_UTYPE : VUT_IMPROVEMENT);
    }

    name = secfile_lookup_str_default(file, nullptr, "%s.wl_value%d",
                                      path_str, i);
    if (nullptr == kind || '\0' == kind[0] || nullptr == name
        || '\0' == name[0]) {
      break;
    } else {
      pgwl->unbuilt.entries[i].kind = fc_strdup(kind);
      pgwl->unbuilt.entries[i].name = fc_strdup(name);
      pgwl->unbuilt.length++;
    }
  }

  return true;
}

/**
   Load all global worklist from a section file.
 */
void global_worklists_load(struct section_file *file)
{
  int i;

  // Clear the current global worklists.
  global_worklists_iterate_all(pgwl) { global_worklist_destroy(pgwl); }
  global_worklists_iterate_all_end;

  for (i = 0; global_worklist_load(file, "worklists.worklist%d", i); i++) {
    // Nothing to do more.
  }

  if (C_S_RUNNING <= client_state()) {
    // We need to build the worklists immediately.
    global_worklists_build();
  }
}

/**
   Save one global worklist into a section file.
 */
static void global_worklist_save(const struct global_worklist *pgwl,
                                 struct section_file *file, int fill_until,
                                 const char *path, ...)
{
  char path_str[1024];
  int i = 0;
  va_list ap;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  fc_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  secfile_insert_str(file, pgwl->name, "%s.wl_name", path_str);

  switch (pgwl->status) {
  case STATUS_UNBUILT:
    secfile_insert_int(file, pgwl->unbuilt.length, "%s.wl_length", path_str);
    for (i = 0; i < pgwl->unbuilt.length; i++) {
      secfile_insert_str(file, pgwl->unbuilt.entries[i].kind, "%s.wl_kind%d",
                         path_str, i);
      secfile_insert_str(file, pgwl->unbuilt.entries[i].name,
                         "%s.wl_value%d", path_str, i);
    }
    break;
  case STATUS_WORKLIST:
    secfile_insert_int(file, worklist_length(&pgwl->worklist),
                       "%s.wl_length", path_str);
    for (i = 0; i < pgwl->unbuilt.length; i++) {
      secfile_insert_str(
          file, universal_type_rule_name(pgwl->worklist.entries + i),
          "%s.wl_kind%d", path_str, i);
      secfile_insert_str(file,
                         universal_rule_name(pgwl->worklist.entries + i),
                         "%s.wl_value%d", path_str, i);
    }
    break;
  }

  /* We want to keep savegame in tabular format, so each line has to be
   * of equal length.  Fill table up to maximum worklist size. */
  while (i < fill_until) {
    secfile_insert_str(file, "", "%s.wl_kind%d", path_str, i);
    secfile_insert_str(file, "", "%s.wl_value%d", path_str, i);
    i++;
  }
}

/**
   Save all global worklist into a section file.
 */
void global_worklists_save(struct section_file *file)
{
  int max_length = 0;
  int i = 0;

  /* We want to keep savegame in tabular format, so each line has to be
   * of equal length.  So we need to know about the biggest worklist. */
  global_worklists_iterate_all(pgwl)
  {
    switch (pgwl->status) {
    case STATUS_UNBUILT:
      max_length = MAX(max_length, pgwl->unbuilt.length);
      break;
    case STATUS_WORKLIST:
      max_length = MAX(max_length, worklist_length(&pgwl->worklist));
      break;
    }
  }
  global_worklists_iterate_all_end;

  global_worklists_iterate_all(pgwl)
  {
    global_worklist_save(pgwl, file, max_length, "worklists.worklist%d",
                         i++);
  }
  global_worklists_iterate_all_end;
}
