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

// utility
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "registry_ini.h"

// common
#include "actions.h"
#include "base.h"
#include "map.h"
#include "specialist.h"
#include "team.h"

// server
#include "aiiface.h"
#include "unittools.h"

#include "savecompat.h"

bool sg_success;

static const char *special_names[] = {"Irrigation", "Mine",     "Pollution",
                                      "Hut",        "Farmland", "Fallout",
                                      nullptr};

/*
  For each savefile format after 2.3.0, compatibility functions are defined
  which translate secfile structures from previous version to that version;
  all necessary compat functions are called in order to
  translate between the file and current version. See sg_load_compat().

  The integer version ID should be increased every time the format is
  changed. If the change is not backwards compatible, please state the
  changes in the following list and update the compat functions at the end of
  this file.

  - what was added / removed
  - when was it added / removed (date and version)
  - when can additional capability checks be set to mandatory (version)
  - which compatibility checks are needed and till when (version)

  freeciv | what                                           | date       | id
  --------+------------------------------------------------+------------+----
  current | (mapped to current savegame format)            | ----/--/-- |  0
          | first version (svn17538)                       | 2010/07/05 |  -
  2.3.0   | 2.3.0 release                                  | 2010/11/?? |  3
  2.4.0   | 2.4.0 release                                  | 201./../.. | 10
          | * player ai type                               |            |
          | * delegation                                   |            |
          | * citizens                                     |            |
          | * save player color                            |            |
          | * "known" info format change                   |            |
  2.5.0   | 2.5.0 release                                  | 201./../.. | 20
  2.6.0   | 2.6.0 release                                  | 201./../.. | 30
  3.0.0   | 3.0.0 release                                  | 201./../.. | 40
  3.1.0   | 3.0.0 release (development)                    | 201./../.. | 50
          |                                                |            |
*/

static void compat_load_020400(struct loaddata *loading,
                               enum sgf_version format_class);
static void compat_load_020500(struct loaddata *loading,
                               enum sgf_version format_class);
static void compat_load_020600(struct loaddata *loading,
                               enum sgf_version format_class);
static void compat_load_030000(struct loaddata *loading,
                               enum sgf_version format_class);
static void compat_load_030100(struct loaddata *loading,
                               enum sgf_version format_class);
static void compat_post_load_030100(struct loaddata *loading,
                                    enum sgf_version format_class);

#ifdef FREECIV_DEV_SAVE_COMPAT
static void compat_load_dev(struct loaddata *loading);
static void compat_post_load_dev(struct loaddata *loading);
#endif // FREECIV_DEV_SAVE_COMPAT

typedef void (*load_version_func_t)(struct loaddata *loading,
                                    enum sgf_version format_class);

struct compatibility {
  int version;
  const load_version_func_t load;
  const load_version_func_t post_load;
};

/* The struct below contains the information about the savegame versions. It
 * is identified by the version number (first element), which should be
 * steadily increasing. It is saved as 'savefile.version'. The support
 * string (first element of 'name') is not saved in the savegame; it is
 * saved in settings files (so, once assigned, cannot be changed). The
 * 'pretty' string (second element of 'name') can be changed if necessary
 * For changes in the development version, edit the definitions above and
 * add the needed code to load the old version below. Thus, old
 * savegames can still be loaded while the main definition
 * represents the current state of the art. */
/* While developing freeciv 3.1.0, add the compatibility functions to
 * - compat_load_030100 to load old savegame. */
static struct compatibility compat[] = {
    // dummy; equal to the current version (last element)
    {0, nullptr, nullptr},
    // version 1 and 2 is not used
    /* version 3: first savegame2 format, so no compat functions for
     * translation from previous format */
    {3, nullptr, nullptr},
    // version 4 to 9 are reserved for possible changes in 2.3.x
    {10, compat_load_020400, nullptr},
    // version 11 to 19 are reserved for possible changes in 2.4.x
    {20, compat_load_020500, nullptr},
    // version 21 to 29 are reserved for possible changes in 2.5.x
    {30, compat_load_020600, nullptr},
    // version 31 to 39 are reserved for possible changes in 2.6.x
    {40, compat_load_030000, nullptr},
    // version 41 to 49 are reserved for possible changes in 3.0.x
    {50, compat_load_030100, compat_post_load_030100},
    /* Current savefile version is listed above this line; it corresponds to
       the definitions in this file. */
};

static const int compat_num = ARRAY_SIZE(compat);
#define compat_current (compat_num - 1)

/**
   Compatibility functions for loaded game.

   This function is called at the beginning of loading a savegame. The data
 in loading->file should be change such, that the current loading functions
 can be executed without errors.
 */
void sg_load_compat(struct loaddata *loading, enum sgf_version format_class)
{
  int i;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  loading->version =
      secfile_lookup_int_default(loading->file, -1, "savefile.version");
#ifdef FREECIV_DEBUG
  sg_failure_ret(0 < loading->version,
                 "Invalid savefile format version (%d).", loading->version);
  if (loading->version > compat[compat_current].version) {
    // Debug build can (TRY TO!) load newer versions but ...
    qCritical("Savegame version newer than this build found (%d > %d). "
              "Trying to load the game nevertheless ...",
              loading->version, compat[compat_current].version);
  }
#else  // FREECIV_DEBUG
  sg_failure_ret(0 < loading->version
                     && loading->version <= compat[compat_current].version,
                 "Unknown savefile format version (%d).", loading->version);
#endif // FREECIV_DEBUG

  for (i = 0; i < compat_num; i++) {
    if (loading->version < compat[i].version && compat[i].load != nullptr) {
      qInfo(_("Run compatibility function for version: <%d "
              "(save file: %d; server: %d)."),
            compat[i].version, loading->version,
            compat[compat_current].version);
      compat[i].load(loading, format_class);
    }
  }

#ifdef FREECIV_DEV_SAVE_COMPAT
  if (loading->version == compat[compat_current].version) {
    compat_load_dev(loading);
  }
#endif // FREECIV_DEV_SAVE_COMPAT
}

/**
   Compatibility functions for loaded game that needs game state.

   Some compatibility needs access to game state not available in
   sg_load_compat(). Do those here.

   This function is called after a savegame has loaded the game state. The
   data should be changed in the game state since the game already is done
   loading. Prefer using sg_load_compat() when possible.
 */
void sg_load_post_load_compat(struct loaddata *loading,
                              enum sgf_version format_class)
{
  int i;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  for (i = 0; i < compat_num; i++) {
    if (loading->version < compat[i].version
        && compat[i].post_load != nullptr) {
      qInfo(_("Run post load compatibility function for version: <%d "
              "(save file: %d; server: %d)."),
            compat[i].version, loading->version,
            compat[compat_current].version);
      compat[i].post_load(loading, format_class);
    }
  }

#ifdef FREECIV_DEV_SAVE_COMPAT
  if (loading->version == compat[compat_current].version) {
    compat_post_load_dev(loading);
  }
#endif // FREECIV_DEV_SAVE_COMPAT
}

/**
   Return current compatibility version
 */
int current_compat_ver() { return compat[compat_current].version; }

/**
   This returns an ascii hex value of the given half-byte of the binary
   integer. See ascii_hex2bin().
   example: bin2ascii_hex(0xa00, 2) == 'a'
 */
char bin2ascii_hex(int value, int halfbyte_wanted)
{
  return hex_chars[((value) >> ((halfbyte_wanted) * 4)) & 0xf];
}

/**
   This returns a binary integer value of the ascii hex char, offset by the
   given number of half-bytes. See bin2ascii_hex().
   example: ascii_hex2bin('a', 2) == 0xa00
   This is only used in loading games, and it requires some error checking so
   it's done as a function.
 */
int ascii_hex2bin(char ch, int halfbyte)
{
  const char *pch;

  if (ch == ' ') {
    /* Sane value. It is unknow if there are savegames out there which
     * need this fix. Savegame.c doesn't write such savegames
     * (anymore) since the inclusion into CVS (2000-08-25). */
    return 0;
  }

  pch = strchr(hex_chars, ch);

  sg_failure_ret_val(nullptr != pch && '\0' != ch, 0,
                     "Unknown hex value: '%c' %d", ch, ch);
  return (pch - hex_chars) << (halfbyte * 4);
}

static const char num_chars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-+";

/**
   Converts single character into numerical value. This is not hex
 conversion.
 */
int char2num(char ch)
{
  const char *pch;

  pch = strchr(num_chars, ch);

  sg_failure_ret_val(nullptr != pch, 0,
                     "Unknown ascii value for num: '%c' %d", ch, ch);

  return pch - num_chars;
}

/**
   Return the special with the given name, or S_LAST.
 */
enum tile_special_type special_by_rule_name(const char *name)
{
  int i;

  for (i = 0; special_names[i] != nullptr; i++) {
    if (!strcmp(name, special_names[i])) {
      return static_cast<tile_special_type>(i);
    }
  }

  return S_LAST;
}

/**
   Return the untranslated name of the given special.
 */
const char *special_rule_name(enum tile_special_type type)
{
  fc_assert(type >= 0 && type < S_LAST);

  return special_names[type];
}

/**
   Get extra of the given special
 */
struct extra_type *special_extra_get(int spe)
{
  struct extra_type_list *elist =
      extra_type_list_by_cause(static_cast<extra_cause>(EC_SPECIAL));

  if (spe < extra_type_list_size(elist)) {
    return extra_type_list_get(elist, spe);
  }

  return nullptr;
}

/**
   Return the resource type matching the identifier, or nullptr when none
 matches.
 */
struct extra_type *resource_by_identifier(const char identifier)
{
  extra_type_by_cause_iterate(EC_RESOURCE, presource)
  {
    if (presource->data.resource->id_old_save == identifier) {
      return presource;
    }
  }
  extra_type_by_cause_iterate_end;

  return nullptr;
}

/* =======================================================================
 * Compatibility functions for loading a game.
 * ======================================================================= */

/**
   Translate savegame secfile data from 2.3.x to 2.4.0 format.
 */
static void compat_load_020400(struct loaddata *loading,
                               enum sgf_version format_class)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  log_debug("Upgrading data from savegame to version 2.4.0");

  // Add the default player AI.
  player_slots_iterate(pslot)
  {
    int ncities, i, plrno = player_slot_index(pslot);

    if (nullptr
        == secfile_section_lookup(loading->file, "player%d", plrno)) {
      continue;
    }

    secfile_insert_str(loading->file, default_ai_type_name(),
                       "player%d.ai_type", player_slot_index(pslot));

    /* Create dummy citizens informations. We do not know if citizens are
     * activated due to the fact that this information
     * (game.info.citizen_nationality) is not available, but adding the
     * information does no harm. */
    ncities = secfile_lookup_int_default(loading->file, 0,
                                         "player%d.ncities", plrno);
    if (ncities > 0) {
      for (i = 0; i < ncities; i++) {
        int size = secfile_lookup_int_default(loading->file, 0,
                                              "player%d.c%d.size", plrno, i);
        if (size > 0) {
          secfile_insert_int(loading->file, size, "player%d.c%d.citizen%d",
                             plrno, i, plrno);
        }
      }
    }
  }
  player_slots_iterate_end;

  /* Player colors are assigned at the end of player loading, as this
   * needs information not available here. */

  /* Deal with buggy known tiles information from 2.3.0/2.3.1 (and the
   * workaround in later 2.3.x); see gna bug #19029.
   * (The structure of this code is odd as it avoids relying on knowledge of
   * xsize/ysize, which haven't been extracted from the savefile yet.) */
  {
    if (has_capability("knownv2", secfile_lookup_str(loading->file,
                                                     "savefile.options"))) {
      /* This savefile contains known information in a sane format.
       * Just move any entries to where 2.4.x+ expect to find them. */
      struct section *map = secfile_section_by_name(loading->file, "map");
      if (map) {
        entry_list_iterate(section_entries(map), pentry)
        {
          const char *name = entry_name(pentry);
          if (strncmp(name, "kvb", 3) == 0) {
            // Rename the "kvb..." entry to "k..."
            char *name2 = fc_strdup(name), *newname = name2 + 2;
            *newname = 'k';
            /* Savefile probably contains existing "k" entries, which are
             * bogus so we trash them */
            secfile_entry_delete(loading->file, "map.%s", newname);
            entry_set_name(pentry, newname);
            delete[] name2;
            name2 = nullptr;
          }
        }
        entry_list_iterate_end;
      }
      /* Could remove "knownv2" from savefile.options, but it's doing
       * no harm there. */
    } else {
      /* This savefile only contains known information in the broken
       * format. Try to recover it to a sane format. */
      // MAX_NUM_PLAYER_SLOTS in 2.3.x was 128
      // MAP_MAX_LINEAR_SIZE in 2.3.x was 512
      const int maxslots = 128, maxmapsize = 512;
      const int lines = maxslots / 32;
      int xsize = 0, y, l, j, x;
      unsigned int known_row_old[lines * maxmapsize],
          known_row[lines * maxmapsize];
      // Process a map row at a time
      for (y = 0; y < maxmapsize; y++) {
        // Look for broken info to convert
        bool found = false;
        memset(known_row_old, 0, sizeof(known_row_old));
        for (l = 0; l < lines; l++) {
          for (j = 0; j < 8; j++) {
            const char *s = secfile_lookup_str_default(
                loading->file, nullptr, "map.k%02d_%04d", l * 8 + j, y);
            if (s) {
              found = true;
              if (xsize == 0) {
                xsize = qstrlen(s);
              }
              sg_failure_ret(xsize == qstrlen(s),
                             "Inconsistent xsize in map.k%02d_%04d",
                             l * 8 + j, y);
              for (x = 0; x < xsize; x++) {
                known_row_old[l * xsize + x] |= ascii_hex2bin(s[x], j);
              }
            }
          }
        }
        if (found) {
          /* At least one entry found for this row. Let's hope they were
           * all there. */
          // Attempt to munge into sane format
          int p;
          memset(known_row, 0, sizeof(known_row));
          // Iterate over possible player slots
          for (p = 0; p < maxslots; p++) {
            l = p / 32;
            for (x = 0; x < xsize; x++) {
              /* This test causes bit-shifts of >=32 (undefined behaviour),
               * but on common platforms, information happens not to be lost,
               * just oddly arranged. */
              if (known_row_old[l * xsize + x] & (1u << (p - l * 8))) {
                known_row[l * xsize + x] |= (1u << (p - l * 32));
              }
            }
          }
          /* Save sane format back to memory representation of secfile for
           * real loading code to pick up */
          for (l = 0; l < lines; l++) {
            for (j = 0; j < 8; j++) {
              /* Save info for all slots (not just used ones). It's only
               * memory, after all. */
              char row[xsize + 1];
              for (x = 0; x < xsize; x++) {
                row[x] = bin2ascii_hex(known_row[l * xsize + x], j);
              }
              row[xsize] = '\0';
              secfile_replace_str(loading->file, row, "map.k%02d_%04d",
                                  l * 8 + j, y);
            }
          }
        }
      }
    }
  }

  // Server setting migration.
  {
    int set_count;
    if (secfile_lookup_int(loading->file, &set_count,
                           "settings.set_count")) {
      int i, new_opt = set_count;
      bool gamestart_valid = secfile_lookup_bool_default(
          loading->file, false, "settings.gamestart_valid");
      for (i = 0; i < set_count; i++) {
        const char *name =
            secfile_lookup_str(loading->file, "settings.set%d.name", i);
        if (!name) {
          continue;
        }

        /* In 2.3.x and prior, saveturns=0 meant no turn-based saves.
         * This is now controlled by the "autosaves" setting. */
        if (!fc_strcasecmp("saveturns", name)) {
          /* XXX: hardcodes details from GAME_AUTOSAVES_DEFAULT
           * and settings.c:autosaves_name() (but these defaults reflect
           * 2.3's behaviour). */
          const char *const nosave = "GAMEOVER|QUITIDLE|INTERRUPT";
          const char *const save = "TURN|GAMEOVER|QUITIDLE|INTERRUPT";
          int nturns;

          if (secfile_lookup_int(loading->file, &nturns,
                                 "settings.set%d.value", i)) {
            if (nturns == 0) {
              // Invent a new "autosaves" setting
              secfile_insert_str(loading->file, nosave,
                                 "settings.set%d.value", new_opt);
              // Pick something valid for saveturns
              secfile_replace_int(loading->file, GAME_DEFAULT_SAVETURNS,
                                  "settings.set%d.value", i);
            } else {
              secfile_insert_str(loading->file, save, "settings.set%d.value",
                                 new_opt);
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
          if (gamestart_valid) {
            if (secfile_lookup_int(loading->file, &nturns,
                                   "settings.set%d.gamestart", i)) {
              if (nturns == 0) {
                // Invent a new "autosaves" setting
                secfile_insert_str(loading->file, nosave,
                                   "settings.set%d.gamestart", new_opt);
                // Pick something valid for saveturns
                secfile_replace_int(loading->file, GAME_DEFAULT_SAVETURNS,
                                    "settings.set%d.gamestart", i);
              } else {
                secfile_insert_str(loading->file, save,
                                   "settings.set%d.gamestart", new_opt);
              }
            } else {
              log_sg("Setting '%s': %s", name, secfile_error());
            }
          }
        } else if (!fc_strcasecmp("autosaves", name)) {
          /* Sanity check. This won't trigger on an option we've just
           * invented, as the loop won't include it. */
          log_sg("Unexpected \"autosaves\" setting found in pre-2.4 "
                 "savefile. It may have been overridden.");
        }
      }
    }
  }
}

/**
   Callback to get name of old killcitizen setting bit.
 */
static const char *killcitizen_enum_str(secfile_data_t data, int bit)
{
  switch (bit) {
  case UMT_LAND:
    return "LAND";
  case UMT_SEA:
    return "SEA";
  case UMT_BOTH:
    return "BOTH";
  }

  return nullptr;
}

/**
   Translate savegame secfile data from 2.4.x to 2.5.0 format.
 */
static void compat_load_020500(struct loaddata *loading,
                               enum sgf_version format_class)
{
  const char *modname[] = {"Road", "Railroad"};
  const char *old_activities_names[] = {
      "Idle",     "Pollution",       "Unused Road",
      "Mine",     "Irrigate",        "Mine",
      "Irrigate", "Fortified",       "Fortress",
      "Sentry",   "Unused Railroad", "Pillage",
      "Goto",     "Explore",         "Transform",
      "Unused",   "Unused Airbase",  "Fortifying",
      "Fallout",  "Unused Patrol",   "Base"};

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  log_debug("Upgrading data from savegame to version 2.5.0");

  secfile_insert_int(loading->file, 2, "savefile.roads_size");
  secfile_insert_int(loading->file, 0, "savefile.trait_size");

  secfile_insert_str_vec(loading->file, modname, 2, "savefile.roads_vector");

  secfile_insert_int(loading->file, 19, "savefile.activities_size");
  secfile_insert_str_vec(loading->file, old_activities_names, 19,
                         "savefile.activities_vector");

  // Server setting migration.
  {
    int set_count;

    if (secfile_lookup_int(loading->file, &set_count,
                           "settings.set_count")) {
      int i;
      bool gamestart_valid = secfile_lookup_bool_default(
          loading->file, false, "settings.gamestart_valid");
      for (i = 0; i < set_count; i++) {
        const char *name =
            secfile_lookup_str(loading->file, "settings.set%d.name", i);
        if (!name) {
          continue;
        }
        /* In 2.4.x and prior, "killcitizen" listed move types that
         * killed citizens after succesfull attack. Now killcitizen
         * is just boolean and classes affected are defined in ruleset. */
        if (!fc_strcasecmp("killcitizen", name)) {
          int value;

          if (secfile_lookup_enum_data(loading->file, &value, true,
                                       killcitizen_enum_str, nullptr,
                                       "settings.set%d.value", i)) {
            /* Lowest bit of old killcitizen value indicates if
             * land units should kill citizens. We take that as
             * new boolean killcitizen value. */
            if (value & 0x1) {
              secfile_replace_bool(loading->file, true,
                                   "settings.set%d.value", i);
            } else {
              secfile_replace_bool(loading->file, false,
                                   "settings.set%d.value", i);
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
          if (gamestart_valid) {
            if (secfile_lookup_enum_data(loading->file, &value, true,
                                         killcitizen_enum_str, nullptr,
                                         "settings.set%d.gamestart", i)) {
              /* Lowest bit of old killcitizen value indicates if
               * land units should kill citizens. We take that as
               * new boolean killcitizen value. */
              if (value & 0x1) {
                secfile_replace_bool(loading->file, true,
                                     "settings.set%d.gamestart", i);
              } else {
                secfile_replace_bool(loading->file, false,
                                     "settings.set%d.gamestart", i);
              }
            } else {
              log_sg("Setting '%s': %s", name, secfile_error());
            }
          }
        }
      }
    }
  }
}

/**
   Return string representation of revolentype
 */
static const char *revolentype_str(enum revolen_type type)
{
  switch (type) {
  case REVOLEN_FIXED:
    return "FIXED";
  case REVOLEN_RANDOM:
    return "RANDOM";
  case REVOLEN_QUICKENING:
    return "QUICKENING";
  case REVOLEN_RANDQUICK:
    return "RANDQUICK";
  }

  return "";
}

/**
   Translate savegame secfile data from 2.5.x to 2.6.0 format.
 */
static void compat_load_020600(struct loaddata *loading,
                               enum sgf_version format_class)
{
  bool team_pooled_research = GAME_DEFAULT_TEAM_POOLED_RESEARCH;
  int tsize;
  int ti;
  int turn;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  log_debug("Upgrading data from savegame to version 2.6.0");

  /* Terrain mapping table - use current ruleset as we have no way to know
   * any other old values. */
  ti = 0;
  terrain_type_iterate(pterr)
  {
    char buf[2];

    secfile_insert_str(loading->file, terrain_rule_name(pterr),
                       "savefile.terrident%d.name", ti);
    buf[0] = terrain_identifier(pterr);
    buf[1] = '\0';
    secfile_insert_str(loading->file, buf, "savefile.terrident%d.identifier",
                       ti++);
  }
  terrain_type_iterate_end;

  // Server setting migration.
  {
    int set_count;

    if (secfile_lookup_int(loading->file, &set_count,
                           "settings.set_count")) {
      char value_buffer[1024] = "";
      char gamestart_buffer[1024] = "";
      int i;
      int dcost = -1;
      int dstartcost = -1;
      enum revolen_type rlt = GAME_DEFAULT_REVOLENTYPE;
      enum revolen_type gsrlt = GAME_DEFAULT_REVOLENTYPE;
      bool gamestart_valid = secfile_lookup_bool_default(
          loading->file, false, "settings.gamestart_valid");
      int new_set_count;

      for (i = 0; i < set_count; i++) {
        const char *name =
            secfile_lookup_str(loading->file, "settings.set%d.name", i);
        if (!name) {
          continue;
        }

        /* In 2.5.x and prior, "spacerace" boolean controlled if
         * spacerace victory condition was active. */
        if (!fc_strcasecmp("spacerace", name)) {
          bool value;

          if (secfile_lookup_bool(loading->file, &value,
                                  "settings.set%d.value", i)) {
            if (value) {
              if (value_buffer[0] != '\0') {
                sz_strlcat(value_buffer, "|");
              }
              sz_strlcat(value_buffer, "SPACERACE");
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
          if (secfile_lookup_bool(loading->file, &value,
                                  "settings.set%d.gamestart", i)) {
            if (value) {
              if (gamestart_buffer[0] != '\0') {
                sz_strlcat(gamestart_buffer, "|");
              }
              sz_strlcat(gamestart_buffer, "SPACERACE");
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }

          /* We cannot delete old values from the secfile, or rather cannot
           * change index of the later settings. Renumbering them is not easy
           * as we don't know type of each setting we would encounter.
           * So we keep old setting values and only add new "victories"
           * setting. */
        } else if (!fc_strcasecmp("alliedvictory", name)) {
          bool value;

          if (secfile_lookup_bool(loading->file, &value,
                                  "settings.set%d.value", i)) {
            if (value) {
              if (value_buffer[0] != '\0') {
                sz_strlcat(value_buffer, "|");
              }
              sz_strlcat(value_buffer, "ALLIED");
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
          if (secfile_lookup_bool(loading->file, &value,
                                  "settings.set%d.gamestart", i)) {
            if (value) {
              if (gamestart_buffer[0] != '\0') {
                sz_strlcat(gamestart_buffer, "|");
              }
              sz_strlcat(gamestart_buffer, "ALLIED");
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
        } else if (!fc_strcasecmp("revolen", name)) {
          int value;

          if (secfile_lookup_int(loading->file, &value,
                                 "settings.set%d.value", i)) {
            // 0 meant RANDOM 1-5
            if (value == 0) {
              rlt = REVOLEN_RANDOM;
              secfile_replace_int(loading->file, 5, "settings.set%d.value",
                                  i);
            } else {
              rlt = REVOLEN_FIXED;
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
          if (secfile_lookup_int(loading->file, &value,
                                 "settings.set%d.gamestart", i)) {
            // 0 meant RANDOM 1-5
            if (value == 0) {
              gsrlt = REVOLEN_RANDOM;
              secfile_replace_int(loading->file, 5,
                                  "settings.set%d.gamestart", i);
            } else {
              gsrlt = REVOLEN_FIXED;
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
        } else if (!fc_strcasecmp("happyborders", name)) {
          bool value;

          if (secfile_lookup_bool(loading->file, &value,
                                  "settings.set%d.value", i)) {
            secfile_entry_delete(loading->file, "settings.set%d.value", i);
            if (value) {
              secfile_insert_str(loading->file, "NATIONAL",
                                 "settings.set%d.value", i);
            } else {
              secfile_insert_str(loading->file, "DISABLED",
                                 "settings.set%d.value", i);
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
          if (secfile_lookup_bool(loading->file, &value,
                                  "settings.set%d.gamestart", i)) {
            secfile_entry_delete(loading->file, "settings.set%d.gamestart",
                                 i);
            if (value) {
              secfile_insert_str(loading->file, "NATIONAL",
                                 "settings.set%d.gamestart", i);
            } else {
              secfile_insert_str(loading->file, "DISABLED",
                                 "settings.set%d.gamestart", i);
            }
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
        } else if (!fc_strcasecmp("team_pooled_research", name)) {
          sg_warn(secfile_lookup_bool(loading->file, &team_pooled_research,
                                      "settings.set%d.value", i),
                  "%s", secfile_error());
        } else if (!fc_strcasecmp("diplcost", name)) {
          // Old 'diplcost' split to 'diplbulbcost' and 'diplgoldcost'
          if (secfile_lookup_int(loading->file, &dcost,
                                 "settings.set%d.value", i)) {
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }

          if (secfile_lookup_int(loading->file, &dstartcost,
                                 "settings.set%d.gamestart", i)) {
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }
        } else if (!fc_strcasecmp("huts", name)) {
          // Scale of 'huts' changed.
          int hcount;

          if (secfile_lookup_int(loading->file, &hcount,
                                 "settings.set%d.value", i)) {
          } else {
            log_sg("Setting '%s': %s", name, secfile_error());
          }

          // Store old-style absolute value.
          wld.map.server.huts_absolute = hcount;
        }
      }

      new_set_count = set_count + 2; // 'victories' and 'revolentype'

      if (dcost >= 0) {
        new_set_count += 2;
      }

      secfile_replace_int(loading->file, new_set_count,
                          "settings.set_count");

      secfile_insert_str(loading->file, "victories", "settings.set%d.name",
                         set_count);
      secfile_insert_str(loading->file, value_buffer, "settings.set%d.value",
                         set_count);
      secfile_insert_str(loading->file, "revolentype", "settings.set%d.name",
                         set_count + 1);
      secfile_insert_str(loading->file, revolentype_str(rlt),
                         "settings.set%d.value", set_count + 1);

      if (dcost >= 0) {
        secfile_insert_str(loading->file, "diplbulbcost",
                           "settings.set%d.name", set_count + 2);
        secfile_insert_int(loading->file, dcost, "settings.set%d.value",
                           set_count + 2);
        secfile_insert_str(loading->file, "diplgoldcost",
                           "settings.set%d.name", set_count + 3);
        secfile_insert_int(loading->file, dcost, "settings.set%d.value",
                           set_count + 3);
      }

      if (gamestart_valid) {
        secfile_insert_str(loading->file, gamestart_buffer,
                           "settings.set%d.gamestart", set_count);
        secfile_insert_str(loading->file, revolentype_str(gsrlt),
                           "settings.set%d.gamestart", set_count + 1);

        if (dcost >= 0) {
          secfile_insert_int(loading->file, dstartcost,
                             "settings.set%d.gamestart", set_count + 2);
          secfile_insert_int(loading->file, dstartcost,
                             "settings.set%d.gamestart", set_count + 3);
        }
      }
    }
  }

  sg_failure_ret(
      secfile_lookup_int(loading->file, &tsize, "savefile.trait_size"),
      "Trait size: %s", secfile_error());

  turn = secfile_lookup_int_default(loading->file, 0, "game.turn");

  player_slots_iterate(pslot)
  {
    int plrno = player_slot_index(pslot);
    bool got_first_city;
    int old_barb_type;
    enum barbarian_type new_barb_type;
    int i;
    const char *name;
    int score;
    int units_num;

    if (nullptr
        == secfile_section_lookup(loading->file, "player%d", plrno)) {
      continue;
    }

    // Renamed 'capital' to 'got_first_city'.
    if (secfile_lookup_bool(loading->file, &got_first_city,
                            "player%d.capital", plrno)) {
      secfile_insert_bool(loading->file, got_first_city,
                          "player%d.got_first_city", plrno);
    }

    // Add 'anonymous' qualifiers for user names
    name = secfile_lookup_str_default(loading->file, "", "player%d.username",
                                      plrno);
    secfile_insert_bool(loading->file, (!strcmp(name, ANON_USER_NAME)),
                        "player%d.unassigned_user", plrno);

    name = secfile_lookup_str_default(loading->file, "",
                                      "player%d.ranked_username", plrno);
    secfile_insert_bool(loading->file, (!strcmp(name, ANON_USER_NAME)),
                        "player%d.unassigned_ranked", plrno);

    // Convert numeric barbarian type to textual
    old_barb_type = secfile_lookup_int_default(
        loading->file, 0, "player%d.ai.is_barbarian", plrno);
    new_barb_type = barb_type_convert(old_barb_type);
    secfile_insert_str(loading->file, barbarian_type_name(new_barb_type),
                       "player%d.ai.barb_type", plrno);

    /* Pre-2.6 didn't record when a player was created or died, so we have
     * to assume they lived from the start of the game until last turn */
    secfile_insert_int(loading->file, turn, "player%d.turns_alive", plrno);

    // As if there never has been a war.
    secfile_insert_int(loading->file, -1, "player%d.last_war", plrno);

    // Assume people were playing until current reload
    secfile_insert_int(loading->file, 0, "player%d.idle_turns", plrno);

    for (i = 0; i < tsize; i++) {
      int val;

      val = secfile_lookup_int_default(loading->file, -1,
                                       "player%d.trait.val%d", plrno, i);
      if (val != -1) {
        secfile_insert_int(loading->file, val, "player%d.trait%d.val", plrno,
                           i);
      }

      sg_failure_ret(secfile_lookup_int(loading->file, &val,
                                        "player%d.trait.mod%d", plrno, i),
                     "Trait mod: %s", secfile_error());
      secfile_insert_int(loading->file, val, "player%d.trait%d.mod", plrno,
                         i);
    }

    score = secfile_lookup_int_default(loading->file, -1,
                                       "player%d.units_built", plrno);
    if (score >= 0) {
      secfile_insert_int(loading->file, score, "score%d.units_built", plrno);
    }

    score = secfile_lookup_int_default(loading->file, -1,
                                       "player%d.units_killed", plrno);
    if (score >= 0) {
      secfile_insert_int(loading->file, score, "score%d.units_killed",
                         plrno);
    }

    score = secfile_lookup_int_default(loading->file, -1,
                                       "player%d.units_lost", plrno);
    if (score >= 0) {
      secfile_insert_int(loading->file, score, "score%d.units_lost", plrno);
    }

    // Units orders.
    units_num = secfile_lookup_int_default(loading->file, 0,
                                           "player%d.nunits", plrno);

    for (i = 0; i < units_num; i++) {
      int len;

      if (secfile_lookup_bool_default(loading->file, false,
                                      "player%d.u%d.orders_last_move_safe",
                                      plrno, i)) {
        continue;
      }

      len = secfile_lookup_int_default(
          loading->file, 0, "player%d.u%d.orders_length", plrno, i);
      if (len > 0) {
        char orders_str[len + 1];
        char *p;

        sz_strlcpy(orders_str, secfile_lookup_str_default(
                                   loading->file, "",
                                   "player%d.u%d.orders_list", plrno, i));
        if ((p = strrchr(orders_str, 'm'))
            || (p = strrchr(orders_str, 'M'))) {
          *p = 'x'; // ORDER_MOVE -> ORDER_ACTION_MOVE
          secfile_replace_str(loading->file, orders_str,
                              "player%d.u%d.orders_list", plrno, i);
        }
      }
    }
  }
  player_slots_iterate_end;

  /* Add specialist order - loading time order is ok here, as we will use
   * that when we in later part of compatibility conversion use the
   * specialist values */
  secfile_insert_int(loading->file, specialist_count(),
                     "savefile.specialists_size");
  {
    const char **modname;
    int i = 0;

    modname = new const char *[specialist_count()]();
    specialist_type_iterate(sp)
    {
      modname[i++] = specialist_rule_name(specialist_by_number(sp));
    }
    specialist_type_iterate_end;

    secfile_insert_str_vec(loading->file, modname, specialist_count(),
                           "savefile.specialists_vector");

    delete[] modname;
  }

  // Replace all city specialist count fields with correct names
  player_slots_iterate(pslot)
  {
    int plrno = player_slot_index(pslot);
    int ncities;
    int i;

    if (nullptr
        == secfile_section_lookup(loading->file, "player%d", plrno)) {
      continue;
    }

    ncities = secfile_lookup_int_default(loading->file, 0,
                                         "player%d.ncities", plrno);

    for (i = 0; i < ncities; i++) {
      int k = 0;

      specialist_type_iterate(sp)
      {
        struct specialist *psp = specialist_by_number(sp);
        int count;

        sg_failure_ret(secfile_lookup_int(loading->file, &count,
                                          "player%d.c%d.n%s", plrno, i,
                                          specialist_rule_name(psp)),
                       "specialist error: %s", secfile_error());
        secfile_entry_delete(loading->file, "player%d.c%d.n%s", plrno, i,
                             specialist_rule_name(psp));
        secfile_insert_int(loading->file, count, "player%d.c%d.nspe%d",
                           plrno, i, k++);
      }
      specialist_type_iterate_end;
    }
  }
  player_slots_iterate_end;

  // Build [research].
  {
    const struct {
      const char *name;
      enum entry_type type;
    } entries[] = {{"goal_name", ENTRY_STR},  {"techs", ENTRY_INT},
                   {"futuretech", ENTRY_INT}, {"bulbs_before", ENTRY_INT},
                   {"saved_name", ENTRY_STR}, {"bulbs", ENTRY_INT},
                   {"now_name", ENTRY_STR},   {"got_tech", ENTRY_BOOL},
                   {"done", ENTRY_STR}};

    int researches[MAX(MAX_NUM_PLAYER_SLOTS, team_slot_count())];
    int count = 0;
    int i;

    for (i = 0; i < ARRAY_SIZE(researches); i++) {
      researches[i] = -1;
    }

    player_slots_iterate(pslot)
    {
      int plrno = player_slot_index(pslot);
      int ival;
      bool bval;
      const char *sval;
      int j;

      if (secfile_section_lookup(loading->file, "player%d", plrno)
          == nullptr) {
        continue;
      }

      // Get the research number.
      if (team_pooled_research) {
        i = secfile_lookup_int_default(loading->file, plrno,
                                       "player%d.team_no", plrno);
      } else {
        i = plrno;
      }

      sg_failure_ret(i >= 0 && i < ARRAY_SIZE(researches),
                     "Research out of bounds (%d)!", i);

      // Find the index in [research] section.
      if (researches[i] == -1) {
        // This is the first player for this research.
        secfile_insert_int(loading->file, i, "research.r%d.number", count);
        researches[i] = count;
        count++;
      }
      i = researches[i];

      // Move entries.
      for (j = 0; j < ARRAY_SIZE(entries); j++) {
        switch (entries[j].type) {
        case ENTRY_BOOL:
          if (secfile_lookup_bool(loading->file, &bval,
                                  "player%d.research.%s", plrno,
                                  entries[j].name)) {
            secfile_insert_bool(loading->file, bval, "research.r%d.%s", i,
                                entries[j].name);
          }
          break;
        case ENTRY_INT:
          if (secfile_lookup_int(loading->file, &ival,
                                 "player%d.research.%s", plrno,
                                 entries[j].name)) {
            secfile_insert_int(loading->file, ival, "research.r%d.%s", i,
                               entries[j].name);
          }
          break;
        case ENTRY_STR:
          if ((sval =
                   secfile_lookup_str(loading->file, "player%d.research.%s",
                                      plrno, entries[j].name))) {
            secfile_insert_str(loading->file, sval, "research.r%d.%s", i,
                               entries[j].name);
          }
          break;
        case ENTRY_FLOAT:
          sg_failure_ret(entries[j].type != ENTRY_FLOAT,
                         "Research related entry marked as float.");
          break;
        case ENTRY_FILEREFERENCE:
          fc_assert(entries[j].type != ENTRY_FILEREFERENCE);
          break;
        case ENTRY_ILLEGAL:
          fc_assert(entries[j].type != ENTRY_ILLEGAL);
          break;
        }
      }
    }
    player_slots_iterate_end;
    secfile_insert_int(loading->file, count, "research.count");
  }

  // Add diplstate type order.
  secfile_insert_int(loading->file, DS_LAST, "savefile.diplstate_type_size");
  const char **modname;
  int i;
  int j;
  i = 0;
  modname = new const char *[DS_LAST]();
  for (j = 0; j < DS_LAST; j++) {
    modname[i++] = diplstate_type_name(static_cast<diplstate_type>(j));
  }
  secfile_insert_str_vec(loading->file, modname, DS_LAST,
                         "savefile.diplstate_type_vector");
  delete[] modname;

  /* Fix save games from legacy freeciv versions with a bug that made it view
   * "Never met" as closer than "Peace" or "Alliance". */
  player_slots_iterate(pslot)
  {
    int plrno = player_slot_index(pslot);

    if (nullptr
        == secfile_section_lookup(loading->file, "player%d", plrno)) {
      continue;
    }

    player_slots_iterate(pslot2)
    {
      int i = player_slot_index(pslot2);
      char buf[32];
      int current;
      int closest;

      if (nullptr == secfile_section_lookup(loading->file, "player%d", i)) {
        continue;
      }

      fc_snprintf(buf, sizeof(buf), "player%d.diplstate%d", plrno, i);

      // Read the current diplomatic state.
      current = secfile_lookup_int_default(loading->file, DS_NO_CONTACT,
                                           "%s.type", buf);

      // Read the closest diplomatic state.
      closest = secfile_lookup_int_default(loading->file, DS_NO_CONTACT,
                                           "%s.max_state", buf);

      if (closest == DS_NO_CONTACT
          && (current == DS_PEACE || current == DS_ALLIANCE)) {
        const char *name1 = secfile_lookup_str_default(
            loading->file, "", "player%d.name", plrno);
        const char *name2 = secfile_lookup_str_default(loading->file, "",
                                                       "player%d.name", i);
        /* The current relationship is closer than what the save game
         * claims is the closes relationship ever. */

        log_sg(
            _("The save game is wrong about what the closest"
              " relationship %s (player %d) and %s (player %d) have had is."
              " Fixing it..."),
            name1, plrno, name2, i);

        secfile_replace_int(loading->file, current, "%s.max_state", buf);
      }
    }
    player_slots_iterate_end;
  }
  player_slots_iterate_end;
}

/**
   Increase turn value in secfile by one.
 */
static int increase_secfile_turn_int(struct loaddata *loading,
                                     const char *key, int old_def,
                                     bool keep_default)
{
  int value;

  value = secfile_lookup_int_default(loading->file, old_def, "%s", key);

  if (value != old_def || !keep_default) {
    value++;
    secfile_replace_int(loading->file, value, "%s", key);
  }

  return value;
}

/**
   Translate savegame secfile data from 2.6.x to 3.0.0 format.
   Note that even after 2.6 savegame has gone through this compatibility
   function, it's still 2.6 savegame in the sense that savegame2.c, and not
   savegame3.c, handles it.
 */
static void compat_load_030000(struct loaddata *loading,
                               enum sgf_version format_class)
{
  bool randsaved;
  int num_settings;
  bool started;
  int old_turn = 0;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  log_debug("Upgrading data from savegame to version 3.0.0");

  /* Rename "random.save" as "random.saved"
   * Note that it's not an error if a scenario does not have [random] at all.
   */
  if (secfile_lookup_bool(loading->file, &randsaved, "random.save")) {
    secfile_insert_bool(loading->file, randsaved, "random.saved");
  }

  // Already started games should have their turn counts increased by 1
  if (secfile_lookup_bool_default(loading->file, true,
                                  "game.save_players")) {
    started = true;

    old_turn = increase_secfile_turn_int(loading, "game.turn", 0, false) - 1;
    increase_secfile_turn_int(loading, "game.scoreturn",
                              old_turn + GAME_DEFAULT_SCORETURN, false);
    increase_secfile_turn_int(loading, "history.turn", -2, true);
  } else {
    started = false;
  }

  player_slots_iterate(pslot)
  {
    int plrno = player_slot_index(pslot);
    const char *flag_names[1];

    if (secfile_section_lookup(loading->file, "player%d", plrno)
        == nullptr) {
      continue;
    }

    if (secfile_lookup_bool_default(loading->file, false,
                                    "player%d.ai.control", plrno)) {
      flag_names[0] = plr_flag_id_name(PLRF_AI);

      secfile_insert_str_vec(loading->file, flag_names, 1, "player%d.flags",
                             plrno);
    }

    if (started) {
      int num = secfile_lookup_int_default(loading->file, 0,
                                           "player%d.nunits", plrno);
      int i;

      for (i = 0; i < num; i++) {
        char buf[64];

        fc_snprintf(buf, sizeof(buf), "player%d.u%d.born", plrno, i);

        increase_secfile_turn_int(loading, buf, old_turn, false);
      }

      num = secfile_lookup_int_default(loading->file, 0, "player%d.ncities",
                                       plrno);

      for (i = 0; i < num; i++) {
        char buf[64];

        fc_snprintf(buf, sizeof(buf), "player%d.c%d.turn_founded", plrno, i);

        increase_secfile_turn_int(loading, buf, -2, true);
      }
    }
  }
  player_slots_iterate_end;

  // Settings
  num_settings =
      secfile_lookup_int_default(loading->file, 0, "settings.set_count");

  // User meta server message is now a setting.
  if (secfile_lookup_bool_default(loading->file, false,
                                  "game.meta_usermessage")) {
    const char *metamessage;

    metamessage =
        secfile_lookup_str_default(loading->file, "", "game.meta_message");

    // Insert the meta message as a setting
    secfile_insert_str(loading->file, "metamessage", "settings.set%d.name",
                       num_settings);
    secfile_insert_str(loading->file, metamessage, "settings.set%d.value",
                       num_settings);
    secfile_insert_str(loading->file, "", "settings.set%d.gamestart",
                       num_settings);
    num_settings++;
  }

  secfile_replace_int(loading->file, num_settings, "settings.set_count");
}

/*
   Insert server side agent information.
 */
static void insert_server_side_agent(struct loaddata *loading,
                                     enum sgf_version format_class)
{
  int ssa_size;

  if (format_class == SAVEGAME_2) {
    // Handled in savegame2
    return;
  }

  ssa_size = secfile_lookup_int_default(loading->file, 0,
                                        "savefile.server_side_agent_size");

  if (ssa_size != 0) {
    // Already inserted.
    return;
  }

  // Add server side agent order.
  secfile_insert_int(loading->file, SSA_COUNT,
                     "savefile.server_side_agent_size");
  const char **modname;
  int i;
  int j;
  i = 0;
  modname = new const char *[SSA_COUNT]();
  for (j = 0; j < SSA_COUNT; j++) {
    modname[i++] = server_side_agent_name(static_cast<server_side_agent>(j));
  }
  secfile_insert_str_vec(loading->file, modname, SSA_COUNT,
                         "savefile.server_side_agent_list");
  delete[] modname;

  // Insert server_side_agent unit field.
  player_slots_iterate(pslot)
  {
    int unit;
    int units_num;
    int plrno = player_slot_index(pslot);

    if (secfile_section_lookup(loading->file, "player%d", plrno)
        == nullptr) {
      continue;
    }

    // Number of units the player has.
    units_num = secfile_lookup_int_default(loading->file, 0,
                                           "player%d.nunits", plrno);

    for (unit = 0; unit < units_num; unit++) {
      bool ai;

      if (secfile_section_lookup(
              loading->file, "player%d.u%d.server_side_agent", plrno, unit)
          != nullptr) {
        // Already updated?
        continue;
      }

      ai = secfile_lookup_bool_default(loading->file, false,
                                       "player%d.u%d.ai", plrno, unit);

      if (ai) {
        /* Autosettler and Auotexplore are separated by
         * compat_post_load_030100() when set to SSA_AUTOSETTLER */
        secfile_insert_int(loading->file, SSA_AUTOSETTLER,
                           "player%d.u%d.server_side_agent", plrno, unit);
      } else {
        secfile_insert_int(loading->file, SSA_NONE,
                           "player%d.u%d.server_side_agent", plrno, unit);
      }
    }
  }
  player_slots_iterate_end;
}

/**
   Translate savegame secfile data from 3.0.x to 3.1.0 format.
   Note that even after 2.6 savegame has gone through all the compatibility
   functions, it's still 2.6 savegame in the sense that savegame2.c, and not
   savegame3.c, handles it.
 */
static void compat_load_030100(struct loaddata *loading,
                               enum sgf_version format_class)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  log_debug("Upgrading data from savegame to version 3.1.0");

  // Actions are now stored by number.
  player_slots_iterate(pslot)
  {
    int unit;
    int units_num;
    int plrno = player_slot_index(pslot);

    if (secfile_section_lookup(loading->file, "player%d", plrno)
        == nullptr) {
      continue;
    }

    // Number of units the player has.
    units_num = secfile_lookup_int_default(loading->file, 0,
                                           "player%d.nunits", plrno);

    for (unit = 0; unit < units_num; unit++) {
      const char *action_unitstr;
      int order_len;

      order_len = secfile_lookup_int_default(
          loading->file, 0, "player%d.u%d.orders_length", plrno, unit);
      action_unitstr = secfile_lookup_str_default(
          loading->file, "", "player%d.u%d.action_list", plrno, unit);
      if (action_unitstr) {
        int order_num;

        if (order_len > qstrlen(action_unitstr)) {
          order_len = qstrlen(action_unitstr);
        }

        for (order_num = 0; order_num < order_len; order_num++) {
          int unconverted_action_id;

          if (action_unitstr[order_num] == '?') {
            unconverted_action_id = -1;
          } else {
            unconverted_action_id = char2num(action_unitstr[order_num]);
          }

          if (order_num == 0) {
            // The start of a vector has no number.
            secfile_insert_int(loading->file, unconverted_action_id,
                               "player%d.u%d.action_vec", plrno, unit);
          } else {
            secfile_insert_int(loading->file, unconverted_action_id,
                               "player%d.u%d.action_vec,%d", plrno, unit,
                               order_num);
          }
        }
      }
    }
  }
  player_slots_iterate_end;

  // Explicit server side agent was new in 3.1
  insert_server_side_agent(loading, format_class);
}

/**
   Upgrade unit activity orders to unit action orders.
 */
static void unit_order_activity_to_action(struct unit *act_unit)
{
  int i;

  for (i = 0; i < act_unit->orders.length; i++) {
    struct unit_order *order = &act_unit->orders.list[i];

    if (order->order != ORDER_ACTIVITY) {
      continue;
    }

    switch (order->activity) {
    case ACTIVITY_FALLOUT:
    case ACTIVITY_POLLUTION:
    case ACTIVITY_MINE:
    case ACTIVITY_IRRIGATE:
    case ACTIVITY_PLANT:
    case ACTIVITY_CULTIVATE:
    case ACTIVITY_TRANSFORM:
    case ACTIVITY_CONVERT:
    case ACTIVITY_FORTIFYING:
    case ACTIVITY_BASE:
    case ACTIVITY_GEN_ROAD:
    case ACTIVITY_PILLAGE:
      action_iterate(act_id)
      {
        struct action *paction = action_by_number(act_id);
        if (action_get_activity(paction) == order->activity) {
          order->order = ORDER_PERFORM_ACTION;
          order->action = action_number(paction);
          order->activity = ACTIVITY_LAST;
          break;
        }
      }
      action_iterate_end;
      break;
    case ACTIVITY_SENTRY:
      // Not an action
      break;
    case ACTIVITY_EXPLORE:
    case ACTIVITY_IDLE:
    case ACTIVITY_GOTO:
    case ACTIVITY_FORTIFIED:
    case ACTIVITY_OLD_ROAD:
    case ACTIVITY_OLD_RAILROAD:
    case ACTIVITY_FORTRESS:
    case ACTIVITY_AIRBASE:
    case ACTIVITY_PATROL_UNUSED:
    case ACTIVITY_LAST:
    case ACTIVITY_UNKNOWN:
      qCritical("Activity %d is not supposed to appear in unit orders",
                order->activity);
      break;
    }
  }
}

/**
   Returns the opposite direction.
 */
static enum direction8 dir_opposite(enum direction8 dir)
{
  switch (dir) {
  case DIR8_NORTH:
    return DIR8_SOUTH;
  case DIR8_NORTHEAST:
    return DIR8_SOUTHWEST;
  case DIR8_EAST:
    return DIR8_WEST;
  case DIR8_SOUTHEAST:
    return DIR8_NORTHWEST;
  case DIR8_SOUTH:
    return DIR8_NORTH;
  case DIR8_SOUTHWEST:
    return DIR8_NORTHEAST;
  case DIR8_WEST:
    return DIR8_EAST;
  case DIR8_NORTHWEST:
    return DIR8_SOUTHEAST;
  }

  return DIR8_ORIGIN;
}

/**
   Upgrade unit action order target encoding.
 */
static void upgrade_unit_order_targets(struct unit *act_unit)
{
  int i;
  struct tile *current_tile;
  struct tile *tgt_tile;

  if (!unit_has_orders(act_unit)) {
    return;
  }

  // The order index is for the unit at its current tile.
  current_tile = unit_tile(act_unit);

  // Rewind to the beginning of the orders
  for (i = act_unit->orders.index; i > 0; i--) {
    struct unit_order *prev_order = &act_unit->orders.list[i - 1];

    if (!(prev_order->order == ORDER_PERFORM_ACTION
          && utype_is_unmoved_by_action(action_by_number(prev_order->action),
                                        unit_type_get(act_unit)))
        && direction8_is_valid(prev_order->dir)) {
      current_tile =
          mapstep(&(wld.map), current_tile, dir_opposite(prev_order->dir));
    }
  }

  // Upgrade to explicit target tile
  for (i = 0; i < act_unit->orders.length; i++) {
    struct unit_order *order = &act_unit->orders.list[i];

    if (order->order == ORDER_PERFORM_ACTION && order->target != NO_TARGET) {
      // The target is already specified in the new format.
      tgt_tile = index_to_tile(&(wld.map), order->target);
      fc_assert(tgt_tile != nullptr);
      return;
    }

    if (!direction8_is_valid(order->dir)) {
      // The target of the action is on the actor's tile.
      tgt_tile = current_tile;
    } else {
      // The target of the action is on a tile next to the actor.
      tgt_tile = mapstep(&(wld.map), current_tile, order->dir);
    }

    if (order->order == ORDER_PERFORM_ACTION) {
      struct action *paction = action_by_number(order->action);

      order->target = tgt_tile->index;
      // Leave no traces.
      order->dir = DIR8_ORIGIN;

      if (!utype_is_unmoved_by_action(paction, unit_type_get(act_unit))) {
        /* The action moves the unit to the target tile (unless this is the
         * final order) */
        fc_assert(
            utype_is_moved_to_tgt_by_action(paction, unit_type_get(act_unit))
            || i == act_unit->orders.length - 1);
        current_tile = tgt_tile;
      }
    } else {
      current_tile = tgt_tile;
    }
  }
}

/*
   Correct the server side agent information.
 */
static void upgrade_server_side_agent(struct loaddata *loading)
{
  players_iterate_alive(pplayer)
  {
    unit_list_iterate(pplayer->units, punit)
    {
      if (punit->activity == ACTIVITY_EXPLORE) {
        punit->ssa_controller = SSA_AUTOEXPLORE;
      }
    }
    unit_list_iterate_end;
  }
  players_iterate_alive_end;
}

/**
   Update loaded game data from 3.0.x to something usable by 3.1.0.
 */
static void compat_post_load_030100(struct loaddata *loading,
                                    enum sgf_version format_class)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Action orders were new in 3.0
  if (format_class == SAVEGAME_3) {
    // Only 3.0 savegames may have "Attack" action orders.
    players_iterate_alive(pplayer)
    {
      unit_list_iterate(pplayer->units, punit)
      {
        int i;

        if (!punit->has_orders) {
          continue;
        }

        fc_assert_action(punit->orders.length == 0
                             || punit->orders.list != nullptr,
                         continue);

        for (i = 0; i < punit->orders.length; i++) {
          // "Attack" was split in "Suicide Attack" and "Attack" in 3.1.
          if (punit->orders.list[i].order == ORDER_PERFORM_ACTION
              && punit->orders.list[i].action == ACTION_ATTACK
              && !unit_can_do_action(punit, ACTION_ATTACK)
              && unit_can_do_action(punit, ACTION_SUICIDE_ATTACK)) {
            punit->orders.list[i].action = ACTION_SUICIDE_ATTACK;
          }

          /* Production targeted actions were split from building targeted
           * actions in 3.1. The building sub target encoding changed. */
          if (punit->orders.list[i].order == ORDER_PERFORM_ACTION
              && ((punit->orders.list[i].action
                   == ACTION_SPY_TARGETED_SABOTAGE_CITY)
                  || (punit->orders.list[i].action
                      == ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC))) {
            punit->orders.list[i].sub_target -= 1;
          }
          if (punit->orders.list[i].order == ORDER_PERFORM_ACTION
              && (punit->orders.list[i].action
                  == ACTION_SPY_TARGETED_SABOTAGE_CITY)
              && punit->orders.list[i].sub_target == -1) {
            punit->orders.list[i].action =
                ACTION_SPY_SABOTAGE_CITY_PRODUCTION;
          }
          if (punit->orders.list[i].order == ORDER_PERFORM_ACTION
              && (punit->orders.list[i].action
                  == ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC)
              && punit->orders.list[i].sub_target == -1) {
            punit->orders.list[i].action =
                ACTION_SPY_SABOTAGE_CITY_PRODUCTION_ESC;
          }
        }
      }
      unit_list_iterate_end;
    }
    players_iterate_alive_end;
  }

  // Explicit server side agent was new in 3.1
  upgrade_server_side_agent(loading);

  // Some activities should only be ordered in action orders.
  players_iterate_alive(pplayer)
  {
    unit_list_iterate(pplayer->units, punit)
    {
      unit_order_activity_to_action(punit);
    }
    unit_list_iterate_end;
  }
  players_iterate_alive_end;

  // Unit order action target isn't dir anymore
  players_iterate_alive(pplayer)
  {
    unit_list_iterate(pplayer->units, punit)
    {
      upgrade_unit_order_targets(punit);
    }
    unit_list_iterate_end;
  }
  players_iterate_alive_end;

  /* Backward compatibility: if we had any open-ended orders (pillage)
   * in the savegame, assign specific targets now */
  players_iterate_alive(pplayer)
  {
    unit_list_iterate(pplayer->units, punit)
    {
      unit_assign_specific_activity_target(punit, &punit->activity,
                                           &punit->activity_target);
    }
    unit_list_iterate_end;
  }
  players_iterate_alive_end;
}

/**
   Translate savegame secfile data from earlier development version format
   to current one.
 */
#ifdef FREECIV_DEV_SAVE_COMPAT
static void compat_load_dev(struct loaddata *loading)
{
  int game_version;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  log_debug("Upgrading data between development revisions");

  if (!secfile_lookup_int(loading->file, &game_version,
                          "scenario.game_version")) {
    game_version = 2060000;
  }

#ifdef FREECIV_DEV_SAVE_COMPAT_3_1

  if (game_version < 3009200) {
    // Before version number bump to 3.0.92, beginning of 2020

    // Renamed actions.
    loading->action.size =
        secfile_lookup_int_default(loading->file, 0, "savefile.action_size");
    if (loading->action.size) {
      const char **modname;
      int j;

      modname = secfile_lookup_str_vec(loading->file, &loading->action.size,
                                       "savefile.action_vector");

      for (j = 0; j < loading->action.size; j++) {
        if (fc_strcasecmp(modname[j], "Targeted Steal Tech Escape") == 0) {
          secfile_replace_str(loading->file,
                              "Targeted Steal Tech Escape Expected",
                              "savefile.action_vector,%d", j);
        } else if (fc_strcasecmp(modname[j], "Steal Tech Escape") == 0) {
          secfile_replace_str(loading->file, "Steal Tech Escape Expected",
                              "savefile.action_vector,%d", j);
        } else if (fc_strcasecmp(modname[j], "Road") == 0) {
          secfile_replace_str(loading->file, "Build Road",
                              "savefile.action_vector,%d", j);
        }
      }

      delete[] modname;
    }

    // Old unit order tgt_vec refers to order sub targets
    player_slots_iterate(pslot)
    {
      int unit;
      int units_num;
      int plrno = player_slot_index(pslot);

      if (secfile_section_lookup(loading->file, "player%d", plrno)
          == nullptr) {
        continue;
      }

      // Number of units the player has.
      units_num = secfile_lookup_int_default(loading->file, 0,
                                             "player%d.nunits", plrno);

      for (unit = 0; unit < units_num; unit++) {
        size_t old_tgt_size;
        int *old_tgt_vec;
        old_tgt_vec =
            secfile_lookup_int_vec(loading->file, &old_tgt_size,
                                   "player%d.u%d.tgt_vec", plrno, unit);
        if (old_tgt_vec) {
          secfile_insert_int_vec(loading->file, old_tgt_vec, old_tgt_size,
                                 "player%d.u%d.sub_tgt_vec", plrno, unit);
          delete[] old_tgt_vec;
          old_tgt_vec = nullptr;
        }
      }
    }
    player_slots_iterate_end;

    /* Unit order extra sub targets was for a while stored separate from tech
     * and building sub targets. */
    player_slots_iterate(pslot)
    {
      int unit;
      int units_num;
      int plrno = player_slot_index(pslot);

      if (secfile_section_lookup(loading->file, "player%d", plrno)
          == nullptr) {
        continue;
      }

      // Number of units the player has.
      units_num = secfile_lookup_int_default(loading->file, 0,
                                             "player%d.nunits", plrno);

      for (unit = 0; unit < units_num; unit++) {
        size_t extra_vec_size;
        int *extra_vec;

        if ((extra_vec = secfile_lookup_int_vec(
                 loading->file, &extra_vec_size, "player%d.u%d.extra_vec",
                 plrno, unit))) {
          int order_num;

          for (order_num = 0; order_num < extra_vec_size; order_num++) {
            if (extra_vec[order_num] != -1) {
              if (order_num) {
                secfile_replace_int(loading->file, extra_vec[order_num],
                                    "player%d.u%d.sub_tgt_vec,%d", plrno,
                                    unit, order_num);
              } else {
                secfile_replace_int(loading->file, extra_vec[order_num],
                                    "player%d.u%d.sub_tgt_vec", plrno, unit);
              }
            }
          }
          delete[] extra_vec;
        }
      }
    }
    player_slots_iterate_end;

    player_slots_iterate(pslot)
    {
      int plrno = player_slot_index(pslot);
      int history;

      history = secfile_lookup_int_default(loading->file, 0,
                                           "player%d.culture", plrno);

      if (history > 0) {
        /* Savefile had player history value saved to field named 'culture'.
         * Save it to 'history'. */
        secfile_insert_int(loading->file, history, "player%d.history",
                           plrno);
      }
    }
    player_slots_iterate_end;

    {
      int action_count;

      action_count = secfile_lookup_int_default(loading->file, 0,
                                                "savefile.action_size");
      if (action_count > 0) {
        const char **modname;
        const char **modname_new;
        const char *plant_name = "Plant";
        const char *cultivate_name = "Cultivate";
        int j;

        modname = secfile_lookup_str_vec(
            loading->file, &loading->action.size, "savefile.action_vector");

        modname_new = new action_count();
        for (j = 0; j < action_count; j++) {
          const char *aname = modname[j];

          if (!fc_strcasecmp("Mine TF", aname)) {
            modname_new[j] = plant_name;
          } else if (!fc_strcasecmp("Irrigate TF", aname)) {
            modname_new[j] = cultivate_name;
          } else {
            modname_new[j] = aname;
          }
        }

        secfile_replace_str_vec(loading->file, modname_new, action_count,
                                "savefile.action_vector");

        free(modname_new);
      }
    }

    // Actions are now stored by number.
    player_slots_iterate(pslot)
    {
      int unit;
      int units_num;
      int plrno = player_slot_index(pslot);

      if (secfile_section_lookup(loading->file, "player%d", plrno)
          == nullptr) {
        continue;
      }

      // Number of units the player has.
      units_num = secfile_lookup_int_default(loading->file, 0,
                                             "player%d.nunits", plrno);

      for (unit = 0; unit < units_num; unit++) {
        const char *action_unitstr;
        int order_len;

        order_len = secfile_lookup_int_default(
            loading->file, 0, "player%d.u%d.orders_length", plrno, unit);
        action_unitstr = secfile_lookup_str_default(
            loading->file, "", "player%d.u%d.action_list", plrno, unit);
        if (action_unitstr) {
          int order_num;

          if (order_len > qstrlen(action_unitstr)) {
            order_len = qstrlen(action_unitstr);
          }

          for (order_num = 0; order_num < order_len; order_num++) {
            int unconverted_action_id;

            if (action_unitstr[order_num] == '?') {
              unconverted_action_id = -1;
            } else {
              unconverted_action_id = char2num(action_unitstr[order_num]);
            }

            if (order_num == 0) {
              // The start of a vector has no number.
              secfile_insert_int(loading->file, unconverted_action_id,
                                 "player%d.u%d.action_vec", plrno, unit);
            } else {
              secfile_insert_int(loading->file, unconverted_action_id,
                                 "player%d.u%d.action_vec,%d", plrno, unit,
                                 order_num);
            }
          }
        }
      }
    }
    player_slots_iterate_end;
    player_slots_iterate(pslot)
    {
      int city;
      int city_num;
      int plrno = player_slot_index(pslot);

      if (secfile_section_lookup(loading->file, "player%d", plrno)
          == nullptr) {
        continue;
      }

      // Number of cities the player has.
      city_num = secfile_lookup_int_default(loading->file, 0,
                                            "player%d.ncities", plrno);

      for (city = 0; city < city_num; city++) {
        const char *action_citystr;
        int order_len;

        order_len = secfile_lookup_int_default(
            loading->file, 0, "player%d.c%d.rally_point_length", plrno,
            city);
        action_citystr = secfile_lookup_str_default(
            loading->file, "", "player%d.c%d.rally_point_actions", plrno,
            city);
        if (action_citystr) {
          int order_num;

          if (order_len > qstrlen(action_citystr)) {
            order_len = qstrlen(action_citystr);
          }

          for (order_num = 0; order_num < order_len; order_num++) {
            int unconverted_action_id;

            if (action_citystr[order_num] == '?') {
              unconverted_action_id = -1;
            } else {
              unconverted_action_id = char2num(action_citystr[order_num]);
            }

            if (order_num == 0) {
              // The start of a vector has no number.
              secfile_insert_int(loading->file, unconverted_action_id,
                                 "player%d.c%d.rally_point_action_vec",
                                 plrno, city);
            } else {
              secfile_insert_int(loading->file, unconverted_action_id,
                                 "player%d.c%d.rally_point_action_vec,%d",
                                 plrno, city, order_num);
            }
          }
        }
      }
    }
    player_slots_iterate_end;
  } // Version < 3.0.92

  if (game_version < 3009300) {
    // Before version number bump to 3.0.93

    // Explicit server side agent was new in 3.1
    insert_server_side_agent(loading, SAVEGAME_3);
  } // Version < 3.0.93

#endif // FREECIV_DEV_SAVE_COMPAT_3_1
}

/**
   Update loaded game data from earlier development version to something
   usable by current Freeciv21.
 */
static void compat_post_load_dev(struct loaddata *loading)
{
  int game_version;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (!secfile_lookup_int(loading->file, &game_version,
                          "scenario.game_version")) {
    game_version = 2060000;
  }

  if (game_version < 3009300) {
    players_iterate_alive(pplayer)
    {
      unit_list_iterate(pplayer->units, punit)
      {
        int i;

        if (!punit->has_orders) {
          continue;
        }

        fc_assert_action(punit->orders.length == 0
                             || punit->orders.list != nullptr,
                         continue);

        for (i = 0; i < punit->orders.length; i++) {
          // "Attack" was split in "Suicide Attack" and "Attack" in 3.1.
          if (punit->orders.list[i].order == ORDER_PERFORM_ACTION
              && punit->orders.list[i].action == ACTION_ATTACK
              && !unit_can_do_action(punit, ACTION_ATTACK)
              && unit_can_do_action(punit, ACTION_SUICIDE_ATTACK)) {
            punit->orders.list[i].action = ACTION_SUICIDE_ATTACK;
          }
        }
      }
      unit_list_iterate_end;
    }
    players_iterate_alive_end;

    // Explicit server side agent was new in 3.1
    upgrade_server_side_agent(loading);

    // Some activities should only be ordered in action orders.
    players_iterate_alive(pplayer)
    {
      unit_list_iterate(pplayer->units, punit)
      {
        unit_order_activity_to_action(punit);
      }
      unit_list_iterate_end;
    }
    players_iterate_alive_end;

    // Unit order action target isn't dir anymore
    players_iterate_alive(pplayer)
    {
      unit_list_iterate(pplayer->units, punit)
      {
        upgrade_unit_order_targets(punit);
      }
      unit_list_iterate_end;
    }
    players_iterate_alive_end;

    /* Backward compatibility: if we had any open-ended orders (pillage)
     * in the savegame, assign specific targets now */
    players_iterate_alive(pplayer)
    {
      unit_list_iterate(pplayer->units, punit)
      {
        unit_assign_specific_activity_target(punit, &punit->activity,
                                             &punit->activity_target);
      }
      unit_list_iterate_end;
    }
    players_iterate_alive_end;
  } // Version < 3.0.93
}
#endif // FREECIV_DEV_SAVE_COMPAT

/**
   Convert old ai level value to ai_level
 */
enum ai_level ai_level_convert(int old_level)
{
  switch (old_level) {
  case 1:
    return AI_LEVEL_AWAY;
  case 2:
    return AI_LEVEL_NOVICE;
  case 3:
    return AI_LEVEL_EASY;
  case 5:
    return AI_LEVEL_NORMAL;
  case 7:
    return AI_LEVEL_HARD;
  case 8:
    return AI_LEVEL_CHEATING;
  case 10:
#ifdef FREECIV_DEBUG
    return AI_LEVEL_EXPERIMENTAL;
#else  // FREECIV_DEBUG
    return AI_LEVEL_HARD;
#endif // FREECIV_DEBUG
  }

  return ai_level_invalid();
}

/**
   Convert old barbarian type value to barbarian_type
 */
enum barbarian_type barb_type_convert(int old_type)
{
  switch (old_type) {
  case 0:
    return NOT_A_BARBARIAN;
  case 1:
    return LAND_BARBARIAN;
  case 2:
    return SEA_BARBARIAN;
  }

  return barbarian_type_invalid();
}

/**
   Assign a new base building task to unit
 */
void set_unit_activity_base(struct unit *punit, Base_type_id base)
{
  set_unit_activity_targeted(punit, ACTIVITY_BASE,
                             base_extra_get(base_by_number(base)));
}

/**
   Assign a new road building task to unit
 */
void set_unit_activity_road(struct unit *punit, Road_type_id road)
{
  set_unit_activity_targeted(punit, ACTIVITY_GEN_ROAD,
                             road_extra_get(road_by_number(road)));
}

/**
   Returns the action id corresponding to the specified order id. If no
   corresponding action is found ACTION_NONE is returned.

   Relevant tile content information must be loaded before this function is
   called. Tile content information is relevant if it determines what action
   an old order result in. Example: a 2.6 ORDER_BUILD_CITY would result in
   Join City inside a domestic city and in Found City on a tile without a
   city. That makes domestic cities relevant tile content information.

   Intended to be used while loading unit orders from pre Freeciv 3.0.0
   save games (savegame and savegame2). Should be deleted with savegame2.

   Temporarily used to provide development version internal save game
   compatibility for what will become Freeciv21. This use should cease
   before Freeciv21 3.0.0 is released.

   Should never be called from savegame3 after the 3.0 development version
   internal save game compatibility is removed.
 */
int sg_order_to_action(int order, struct unit *act_unit,
                       struct tile *tgt_tile)
{
  switch (order) {
  case ORDER_OLD_BUILD_CITY:
    if (tile_city(tgt_tile)
        && city_owner(tile_city(tgt_tile)) == unit_owner(act_unit)) {
      /* The player's cities are loaded right before his units. It wasn't
       * possible for rulesets to allow joining foreign cities before 3.0.
       * This means that a converted build city order only can be a Join
       * City order if it targets a domestic city. */
      return ACTION_JOIN_CITY;
    } else {
      // Assume that the intention was to found a new city.
      return ACTION_FOUND_CITY;
    }
  case ORDER_OLD_BUILD_WONDER:
    // Maps one to one with each other.
    return ACTION_HELP_WONDER;
  case ORDER_OLD_TRADE_ROUTE:
    // Maps one to one with each other.
    return ACTION_TRADE_ROUTE;
  case ORDER_OLD_DISBAND:
    /* Added to the order system in the same commit as Help Wonder. Assume
     * that anyone that intended to order Help Wonder used Help Wonder. */
    /* Could in theory be intended as an order to disband in the field. Why
     * would the player give a unit an order to go to a non city location
     * and disband there? Assume the intention was to recycle the unit
     * until a non recycle disband order is found. */
    return ACTION_RECYCLE_UNIT;
  case ORDER_OLD_HOMECITY:
    return ACTION_HOME_CITY;
  }

  // The order hasn't been replaced by an action.
  return ACTION_NONE;
}
