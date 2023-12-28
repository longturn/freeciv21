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

/**
 This module is for generic handling of help data, independent
 of gui considerations.
 */

#include <fc_config.h>

#include <QBitArray>
#include <QList>

#include <cstring>

// utility
#include "astring.h"
#include "bitvector.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "requirements.h"
#include "support.h"

// common
#include "effects.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "multipliers.h"
#include "nation.h"
#include "reqtext.h"
#include "research.h"
#include "server_settings.h"
#include "specialist.h"
#include "unit.h"
#include "version.h"

#include "helpdata.h"

// helper macro for easy conversion from snprintf and cat_snprintf
#define CATLSTR(_b, _s, _t) fc_strlcat(_b, _t, _s)

// This must be in same order as enum in helpdata.h
static const char *const help_type_names[] = {
    "(Any)",       "(Text)",  "Units",   "Improvements", "Wonders",
    "Techs",       "Terrain", "Extras",  "Goods",        "Specialists",
    "Governments", "Ruleset", "Tileset", "Nations",      "Multipliers",
    "Effects",     nullptr};

typedef QList<const struct help_item *> helpList;
helpList *help_nodes;
/* help_nodes_init is not quite the same as booted in boot_help_texts();
   latter can be FALSE even after call, eg if couldn't find helpdata.txt.
*/

/**
   Free all allocations associated with help_nodes.
 */
void free_help_texts()
{
  if (!help_nodes) {
    return;
  }
  for (const auto *ptmp : qAsConst(*help_nodes)) {
    delete[] ptmp->topic;
    delete[] ptmp->text;
    delete ptmp;
  }
  delete help_nodes;
  help_nodes = nullptr;
}

/**
   Insert fixed-width table describing veteran system.
   If only one veteran level, inserts 'nolevels' if non-nullptr.
   Otherwise, insert 'intro' then a table.
 */
static bool insert_veteran_help(char *outbuf, size_t outlen,
                                const struct veteran_system *veteran,
                                const char *intro, const char *nolevels)
{
  /* game.veteran can be nullptr in pregame; if so, keep quiet about
   * veteran levels */
  if (!veteran) {
    return false;
  }

  fc_assert_ret_val(veteran->levels >= 1, false);

  if (veteran->levels == 1) {
    // Only a single veteran level. Don't bother to name it.
    if (nolevels) {
      CATLSTR(outbuf, outlen, nolevels);
      return true;
    } else {
      return false;
    }
  } else {
    int i;
    fc_assert_ret_val(veteran->definitions != nullptr, false);
    if (intro) {
      CATLSTR(outbuf, outlen, intro);
      CATLSTR(outbuf, outlen, "\n\n");
    }
    // TODO: Report raise_chance and work_raise_chance
    CATLSTR(
        outbuf, outlen,
        /* TRANS: Header for fixed-width veteran level table.
         * TRANS: Translators cannot change column widths :(
         * TRANS: "Level name" left-justified, other two right-justified */
        _("Veteran level      Power factor   Move bonus\n"));
    CATLSTR(outbuf, outlen,
            // TRANS: Part of header for veteran level table.
            _("--------------------------------------------"));
    for (i = 0; i < veteran->levels; i++) {
      const struct veteran_level *level = &veteran->definitions[i];
      const char *name = name_translation_get(&level->name);
      /* Use get_internal_string_length() for correct alignment with
       * multibyte character encodings */
      cat_snprintf(
          outbuf, outlen, "\n%s%*s %4d%% %12s", name,
          MAX(0, 25 - (int) get_internal_string_length(name)), "",
          level->power_fact,
          /* e.g. "-    ", "+ 1/3", "+ 1    ", "+ 2 2/3" */
          move_points_text_full(level->move_bonus, true, "+ ", "-", true));
    }
    return true;
  }
}

/**
 * Formats the number of turns to transform between terrains so it can be
 * included in the terrain alterations table.
 */
static void format_change_terrain_string(char *buf, int bufsize,
                                         enum gen_action act, int time,
                                         terrain *from, const terrain *to)
{
  universal u = {.value = {.terrain = from}, .kind = VUT_TERRAIN};
  if (from == to || to == T_NONE
      || !action_id_univs_not_blocking(act, nullptr, &u)) {
    // Terrain cannot be changed
    fc_snprintf(buf, bufsize, "  -");
  } else {
    fc_snprintf(buf, bufsize, "%3d %s", time, terrain_name_translation(to));
  }
}

/**
   Insert generated text for the helpdata "name".
   Returns TRUE if anything was added.
 */
static bool insert_generated_text(char *outbuf, size_t outlen,
                                  const char *name)
{
  if (!game.client.ruleset_init) {
    return false;
  }

  if (0 == strcmp(name, "TerrainAlterations")) {
    int clean_pollution_time = -1, clean_fallout_time = -1,
        pillage_time = -1;
    bool terrain_independent_extras = false;

    CATLSTR(outbuf, outlen,
            /* TRANS: Header for fixed-width terrain alteration table.
             * TRANS: Translators cannot change column widths :( */
            _("Terrain           Cultivate        Plant            "
              "Transform\n"));
    CATLSTR(
        outbuf, outlen,
        "-------------------------------------------------------------------"
        "-\n");
    terrain_type_iterate(pterrain)
    {
      if (0 != qstrlen(terrain_rule_name(pterrain))) {
        char cultivate[MAX_LEN_NAME + 4], plant[MAX_LEN_NAME + 4],
            transform[MAX_LEN_NAME + 4];
        format_change_terrain_string(
            cultivate, sizeof(cultivate), ACTION_CULTIVATE,
            pterrain->cultivate_time, pterrain, pterrain->irrigation_result);
        format_change_terrain_string(plant, sizeof(plant), ACTION_PLANT,
                                     pterrain->plant_time, pterrain,
                                     pterrain->mining_result);
        format_change_terrain_string(
            transform, sizeof(transform), ACTION_TRANSFORM_TERRAIN,
            pterrain->transform_time, pterrain, pterrain->transform_result);

        auto terrain = terrain_name_translation(pterrain);

        /* Use get_internal_string_length() for correct alignment with
         * multibyte character encodings */
        cat_snprintf(
            outbuf, outlen, "%s%*s %s%*s %s%*s %s\n", terrain,
            MAX(0, 16 - (int) get_internal_string_length(terrain)), "",
            cultivate,
            MAX(0, 16 - (int) get_internal_string_length(cultivate)), "",
            plant, MAX(0, 16 - (int) get_internal_string_length(plant)), "",
            transform);

        if (clean_pollution_time != 0
            && pterrain->clean_pollution_time != 0) {
          if (clean_pollution_time < 0) {
            clean_pollution_time = pterrain->clean_pollution_time;
          } else {
            if (clean_pollution_time != pterrain->clean_pollution_time) {
              clean_pollution_time = 0; // give up
            }
          }
        }
        if (clean_fallout_time != 0 && pterrain->clean_fallout_time != 0) {
          if (clean_fallout_time < 0) {
            clean_fallout_time = pterrain->clean_fallout_time;
          } else {
            if (clean_fallout_time != pterrain->clean_fallout_time) {
              clean_fallout_time = 0; // give up
            }
          }
        }
        if (pillage_time != 0 && pterrain->pillage_time != 0) {
          if (pillage_time < 0) {
            pillage_time = pterrain->pillage_time;
          } else {
            if (pillage_time != pterrain->pillage_time) {
              pillage_time = 0; // give up
            }
          }
        }
      }
    }
    terrain_type_iterate_end;

    /* Examine extras to see if time of removal activities really is
     * terrain-independent, and take into account removal_time_factor.
     * XXX: this is rather overwrought to handle cases which the ruleset
     *      author could express much more simply for the same result */
    {
      int time = -1, factor = -1;

      extra_type_by_rmcause_iterate(ERM_CLEANPOLLUTION, pextra)
      {
        if (pextra->removal_time == 0) {
          if (factor < 0) {
            factor = pextra->removal_time_factor;
          } else if (factor != pextra->removal_time_factor) {
            factor = 0; // give up
          }
        } else {
          if (time < 0) {
            time = pextra->removal_time;
          } else if (time != pextra->removal_time) {
            time = 0; // give up
          }
        }
      }
      extra_type_by_rmcause_iterate_end;
      if (factor < 0) {
        // No extra has terrain-dependent clean time; use extra's time
        if (time >= 0) {
          clean_pollution_time = time;
        } else {
          clean_pollution_time = 0;
        }
      } else if (clean_pollution_time != 0) {
        // At least one extra's time depends on terrain
        fc_assert(clean_pollution_time > 0);
        if (time > 0 && factor > 0
            && time != clean_pollution_time * factor) {
          clean_pollution_time = 0;
        } else if (time >= 0) {
          clean_pollution_time = time;
        } else if (factor >= 0) {
          clean_pollution_time = clean_pollution_time * factor;
        } else {
          fc_assert(false);
        }
      }
    }

    {
      int time = -1, factor = -1;

      extra_type_by_rmcause_iterate(ERM_CLEANFALLOUT, pextra)
      {
        if (pextra->removal_time == 0) {
          if (factor < 0) {
            factor = pextra->removal_time_factor;
          } else if (factor != pextra->removal_time_factor) {
            factor = 0; // give up
          }
        } else {
          if (time < 0) {
            time = pextra->removal_time;
          } else if (time != pextra->removal_time) {
            time = 0; // give up
          }
        }
      }
      extra_type_by_rmcause_iterate_end;
      if (factor < 0) {
        // No extra has terrain-dependent clean time; use extra's time
        if (time >= 0) {
          clean_fallout_time = time;
        } else {
          clean_fallout_time = 0;
        }
      } else if (clean_fallout_time != 0) {
        // At least one extra's time depends on terrain
        fc_assert(clean_fallout_time > 0);
        if (time > 0 && factor > 0 && time != clean_fallout_time * factor) {
          clean_fallout_time = 0;
        } else if (time >= 0) {
          clean_fallout_time = time;
        } else if (factor >= 0) {
          clean_fallout_time = clean_fallout_time * factor;
        } else {
          fc_assert(false);
        }
      }
    }

    {
      int time = -1, factor = -1;

      extra_type_by_rmcause_iterate(ERM_PILLAGE, pextra)
      {
        if (pextra->removal_time == 0) {
          if (factor < 0) {
            factor = pextra->removal_time_factor;
          } else if (factor != pextra->removal_time_factor) {
            factor = 0; // give up
          }
        } else {
          if (time < 0) {
            time = pextra->removal_time;
          } else if (time != pextra->removal_time) {
            time = 0; // give up
          }
        }
      }
      extra_type_by_rmcause_iterate_end;
      if (factor < 0) {
        // No extra has terrain-dependent pillage time; use extra's time
        if (time >= 0) {
          pillage_time = time;
        } else {
          pillage_time = 0;
        }
      } else if (pillage_time != 0) {
        // At least one extra's time depends on terrain
        fc_assert(pillage_time > 0);
        if (time > 0 && factor > 0 && time != pillage_time * factor) {
          pillage_time = 0;
        } else if (time >= 0) {
          pillage_time = time;
        } else if (factor >= 0) {
          pillage_time = pillage_time * factor;
        } else {
          fc_assert(false);
        }
      }
    }

    /* Check whether there are any bases or roads whose build time is
     * independent of terrain */

    extra_type_by_cause_iterate(EC_BASE, pextra)
    {
      if (pextra->buildable && pextra->build_time > 0) {
        terrain_independent_extras = true;
        break;
      }
    }
    extra_type_by_cause_iterate_end;
    if (!terrain_independent_extras) {
      extra_type_by_cause_iterate(EC_ROAD, pextra)
      {
        if (pextra->buildable && pextra->build_time > 0) {
          terrain_independent_extras = true;
          break;
        }
      }
      extra_type_by_cause_iterate_end;
    }

    if (clean_pollution_time > 0 || clean_fallout_time > 0
        || pillage_time > 0 || terrain_independent_extras) {
      CATLSTR(outbuf, outlen, "\n");
      CATLSTR(outbuf, outlen,
              _("Time taken for the following activities is independent of "
                "terrain:\n"));
      CATLSTR(outbuf, outlen, "\n");
      CATLSTR(outbuf, outlen,
              /* TRANS: Header for fixed-width terrain alteration table.
               * TRANS: Translators cannot change column widths :( */
              _("Activity            Time\n"));
      CATLSTR(outbuf, outlen, "---------------------------");
      if (clean_pollution_time > 0) {
        cat_snprintf(outbuf, outlen, _("\nClean pollution    %3d"),
                     clean_pollution_time);
      }
      if (clean_fallout_time > 0) {
        cat_snprintf(outbuf, outlen, _("\nClean fallout      %3d"),
                     clean_fallout_time);
      }
      if (pillage_time > 0) {
        cat_snprintf(outbuf, outlen, _("\nPillage            %3d"),
                     pillage_time);
      }
      extra_type_by_cause_iterate(EC_ROAD, pextra)
      {
        if (pextra->buildable && pextra->build_time > 0) {
          const char *rname = extra_name_translation(pextra);

          cat_snprintf(outbuf, outlen, "\n%s%*s %3d", rname,
                       MAX(0, 18 - (int) get_internal_string_length(rname)),
                       "", pextra->build_time);
        }
      }
      extra_type_by_cause_iterate_end;
      extra_type_by_cause_iterate(EC_BASE, pextra)
      {
        if (pextra->buildable && pextra->build_time > 0) {
          const char *bname = extra_name_translation(pextra);

          cat_snprintf(outbuf, outlen, "\n%s%*s %3d", bname,
                       MAX(0, 18 - (int) get_internal_string_length(bname)),
                       "", pextra->build_time);
        }
      }
      extra_type_by_cause_iterate_end;
    }
    return true;
  } else if (0 == strcmp(name, "VeteranLevels")) {
    return insert_veteran_help(
        outbuf, outlen, game.veteran,
        _("In this ruleset, the following veteran levels are defined:"),
        _("This ruleset has no default veteran levels defined."));
  } else if (0 == strcmp(name, "Freeciv21Version")) {
    const char *ver = freeciv_name_version();

    cat_snprintf(
        outbuf, outlen,
        /* TRANS: First %s is version string, e.g.,
         * "Freeciv version 2.3.0-beta1 (beta version)" (translated). */
        _("This is %s."), ver);

    // There's also an separate entry about Qt.
    cat_snprintf(outbuf, outlen, _("\nBuilt against Qt %s, using %s"),
                 QT_VERSION_STR, qVersion());

    QString data_dirs_info = "\n\n";
    data_dirs_info += _("This instance of Freeciv21 searches the following "
                        "directories for data files:");
    data_dirs_info += "\n\n";
    for (const auto &path : qAsConst(get_data_dirs())) {
      QFileInfo info(path + "/");
      data_dirs_info += "* " + info.absolutePath() + " ";
      if (!info.exists()) {
        // TRANS: Folder does not exist
        data_dirs_info += _("(does not exist)");
      } else if (info.isWritable()) {
        // TRANS: Folder can be modified by the current user
        data_dirs_info += _("(user)");
      } else {
        // TRANS: Folder cannot be modified by the current user
        data_dirs_info += _("(system)");
      }
      data_dirs_info += "\n";
    }
    data_dirs_info += "\n";
    data_dirs_info += _("Note that for multiplayer games, the server may "
                        "use different paths.");

    cat_snprintf(outbuf, outlen, "%s", qUtf8Printable(data_dirs_info));

    return true;
  } else if (0 == strcmp(name, "DefaultMetaserver")) {
    cat_snprintf(outbuf, outlen, "  %s", FREECIV_META_URL);

    return true;
  }
  qCritical("Unknown directive '$%s' in help", name);
  return false;
}

/**
   Append text to 'buf' if the given requirements list 'subjreqs' contains
   'psource', implying that ability to build the subject 'subjstr' is
   affected by 'psource'.
   'strs' is an array of (possibly i18n-qualified) format strings covering
   the various cases where additional requirements apply.
 */
static void insert_allows_single(struct universal *psource,
                                 const requirement_vector *psubjreqs,
                                 const char *subjstr,
                                 const char *const *strs, char *buf,
                                 size_t bufsz, const char *prefix)
{
  QVector<QString> coreqs;
  QVector<QString> conoreqs;
  char *buf2 = new char[bufsz];

  // TODO: show other data like range and survives.

  requirement_vector_iterate(psubjreqs, req)
  {
    if (!req->quiet && are_universals_equal(psource, &req->source)) {
      // We're definitely going to print _something_.
      CATLSTR(buf, bufsz, prefix);
      if (req->present) {
        /* psource enables the subject, but other sources may
         * also be required (or required to be absent). */
        requirement_vector_iterate(psubjreqs, coreq)
        {
          if (!coreq->quiet
              && !are_universals_equal(psource, &coreq->source)) {
            universal_name_translation(&coreq->source, buf2, bufsz);
            if (coreq->present) {
              coreqs.append(buf2);
            } else {
              conoreqs.append(buf2);
            }
          }
        }
        requirement_vector_iterate_end;

        if (0 < coreqs.count()) {
          if (0 < conoreqs.count()) {
            cat_snprintf(buf, bufsz,
                         Q_(strs[0]), // "Allows %s (with %s but no %s)."
                         subjstr, qUtf8Printable(strvec_to_and_list(coreqs)),
                         qUtf8Printable(strvec_to_or_list(conoreqs)));
          } else {
            cat_snprintf(buf, bufsz, Q_(strs[1]), // "Allows %s (with %s)."
                         subjstr,
                         qUtf8Printable(strvec_to_and_list(coreqs)));
          }
        } else {
          if (0 < conoreqs.count()) {
            cat_snprintf(buf, bufsz, Q_(strs[2]), // "Allows %s (absent %s)."
                         subjstr,
                         qUtf8Printable(strvec_to_and_list(conoreqs)));
          } else {
            cat_snprintf(buf, bufsz, Q_(strs[3]), // "Allows %s."
                         subjstr);
          }
        }
      } else {
        // psource can, on its own, prevent the subject.
        cat_snprintf(buf, bufsz, Q_(strs[4]), // "Prevents %s."
                     subjstr);
      }
      cat_snprintf(buf, bufsz, "\n");
    }
  }
  requirement_vector_iterate_end;

  delete[] buf2;
}

/**
   Generate text for what this requirement source allows.  Something like

     "Allows Communism (with University).\n"
     "Allows Mfg. Plant (with Factory).\n"
     "Allows Library (absent Fundamentalism).\n"
     "Prevents Harbor.\n"

   This should be called to generate helptext for every possible source
   type.  Note this doesn't handle effects but rather requirements to
   create/maintain things (currently only building/government reqs).

   NB: This function overwrites any existing buffer contents by writing the
   generated text to the start of the given 'buf' pointer (i.e. it does
   NOT append like cat_snprintf).
 */
static void insert_allows(struct universal *psource, char *buf, size_t bufsz,
                          const char *prefix)
{
  buf[0] = '\0';

  for (const auto &pgov : governments) {
    static const char *const govstrs[] = {
        // TRANS: First %s is a government name.
        N_("?gov:Allows %s (with %s but no %s)."),
        // TRANS: First %s is a government name.
        N_("?gov:Allows %s (with %s)."),
        // TRANS: First %s is a government name.
        N_("?gov:Allows %s (absent %s)."),
        // TRANS: %s is a government name.
        N_("?gov:Allows %s."),
        // TRANS: %s is a government name.
        N_("?gov:Prevents %s.")};
    insert_allows_single(psource, &pgov.reqs,
                         government_name_translation(&pgov), govstrs, buf,
                         bufsz, prefix);
  } // governments iterate - gov

  improvement_iterate(pimprove)
  {
    static const char *const imprstrs[] = {
        // TRANS: First %s is a building name.
        N_("?improvement:Allows %s (with %s but no %s)."),
        // TRANS: First %s is a building name.
        N_("?improvement:Allows %s (with %s)."),
        // TRANS: First %s is a building name.
        N_("?improvement:Allows %s (absent %s)."),
        // TRANS: %s is a building name.
        N_("?improvement:Allows %s."),
        // TRANS: %s is a building name.
        N_("?improvement:Prevents %s.")};
    insert_allows_single(psource, &pimprove->reqs,
                         improvement_name_translation(pimprove), imprstrs,
                         buf, bufsz, prefix);
  }
  improvement_iterate_end;

  unit_type_iterate(putype)
  {
    static const char *const utstrs[] = {
        // TRANS: First %s is a unit type name.
        N_("?unittype:Allows %s (with %s but no %s)."),
        // TRANS: First %s is a unit type name.
        N_("?unittype:Allows %s (with %s)."),
        // TRANS: First %s is a unit type name.
        N_("?unittype:Allows %s (absent %s)."),
        // TRANS: %s is a unit type name.
        N_("?unittype:Allows %s."),
        // TRANS: %s is a unit type name.
        N_("?unittype:Prevents %s.")};
    insert_allows_single(psource, &putype->build_reqs,
                         utype_name_translation(putype), utstrs, buf, bufsz,
                         prefix);
  }
  unit_type_iterate_end;
}

/**
   Allocate and initialize new help item
 */
struct help_item *new_help_item(help_page_type type)
{
  struct help_item *pitem = new help_item;
  pitem->topic = nullptr;
  pitem->text = nullptr;
  pitem->type = type;
  return pitem;
}

/**
   For help_list_sort(); sort by topic via compare_strings()
   (sort topics with more leading spaces after those with fewer)
 */
static int help_item_compar(const struct help_item *v1,
                            const struct help_item *v2)
{
  if (QString(v1->topic) != QString(v2->topic)) {
    return QString(v1->topic) < QString(v2->topic);
  } else {
    return 0;
  }
}

/**
   pplayer may be nullptr.
 */
void boot_help_texts(const nation_set *nations_to_show,
                     help_item *tileset_help)
{
  static bool booted = false;

  struct section_file *sf;
  QString filename;
  struct help_item *pitem;
  struct section_list *sec;
  const char **paras;
  size_t npara;
  char empty[1];
  char long_buffer[64000]; // HACK: this may be overrun.

  empty[0] = '\0';

  // need to do something like this or bad things happen
  free_help_texts();
  help_nodes = new helpList;

  filename = fileinfoname(get_data_dirs(), "helpdata.txt");
  if (filename.isEmpty()) {
    qCritical("Did not read help texts");
    return;
  }
  /* after following call filename may be clobbered; use sf->filename instead
   */
  if (!(sf = secfile_load(filename, false))) {
    // this is now unlikely to happen
    qCritical("failed reading help-texts from '%s':\n%s",
              qUtf8Printable(filename), secfile_error());
    return;
  }

  sec = secfile_sections_by_name_prefix(sf, "help_");

  if (nullptr != sec) {
    section_list_iterate(sec, psection)
    {
      char help_text_buffer[MAX_LEN_PACKET];
      const char *sec_name = section_name(psection);
      const char *gen_str = secfile_lookup_str(sf, "%s.generate", sec_name);

      if (gen_str) {
        enum help_page_type current_type = HELP_ANY;
        int level = strspn(gen_str, " ");

        gen_str += level;

        for (int i = 2; help_type_names[i]; i++) {
          if (strcmp(gen_str, help_type_names[i]) == 0) {
            current_type = static_cast<help_page_type>(i);
            break;
          }
        }
        if (current_type == HELP_ANY) {
          qCritical("bad help-generate category \"%s\"", gen_str);
          continue;
        }

        if (!booted) {
          if (current_type == HELP_EXTRA) {
            size_t ncats;

            /* Avoid warnings about entries unused on this round,
             * when the entries in question are valid once help system has
             * been booted */
            const char **delete_me_pls = secfile_lookup_str_vec(
                sf, &ncats, "%s.categories", sec_name);
            delete[] delete_me_pls;
          }
          continue; // on initial boot data tables are empty
        }

        {
          /* Note these should really fill in pitem->text from auto-gen
             data instead of doing it later on the fly, but I don't want
             to change that now.  --dwp
          */
          char name[2048];
          helpList category_nodes;

          switch (current_type) {
          case HELP_UNIT:
            unit_type_iterate(punittype)
            {
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          utype_name_translation(punittype));
              pitem->topic = qstrdup(name);
              pitem->text = qstrdup(empty);
              category_nodes.append(pitem);
            }
            unit_type_iterate_end;
            break;
          case HELP_TECH:
            advance_index_iterate(A_FIRST, advi)
            {
              if (valid_advance_by_number(advi)) {
                pitem = new_help_item(current_type);
                fc_snprintf(
                    name, sizeof(name), "%*s%s", level, "",
                    advance_name_translation(advance_by_number(advi)));
                pitem->topic = qstrdup(name);
                pitem->text = qstrdup(empty);
                category_nodes.append(pitem);
              }
            }
            advance_index_iterate_end;
            break;
          case HELP_TERRAIN:
            terrain_type_iterate(pterrain)
            {
              if (0 != qstrlen(terrain_rule_name(pterrain))) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            terrain_name_translation(pterrain));
                pitem->topic = qstrdup(name);
                pitem->text = qstrdup(empty);
                category_nodes.append(pitem);
              }
            }
            terrain_type_iterate_end;
            break;
          case HELP_EXTRA: {
            const char **cats;
            size_t ncats;
            cats = secfile_lookup_str_vec(sf, &ncats, "%s.categories",
                                          sec_name);
            extra_type_iterate(pextra)
            {
              // If categories not specified, don't filter
              if (cats) {
                bool include = false;
                const char *cat = extra_category_name(pextra->category);
                int ci;

                for (ci = 0; ci < ncats; ci++) {
                  if (fc_strcasecmp(cats[ci], cat) == 0) {
                    include = true;
                    break;
                  }
                }
                if (!include) {
                  continue;
                }
              }
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          extra_name_translation(pextra));
              pitem->topic = qstrdup(name);
              pitem->text = qstrdup(empty);
              category_nodes.append(pitem);
            }
            extra_type_iterate_end;
            delete[] cats;
            cats = nullptr;
          } break;
          case HELP_GOODS:
            goods_type_iterate(pgood)
            {
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          goods_name_translation(pgood));
              pitem->topic = qstrdup(name);
              pitem->text = qstrdup(empty);
              category_nodes.append(pitem);
            }
            goods_type_iterate_end;
            break;
          case HELP_SPECIALIST:
            specialist_type_iterate(sp)
            {
              struct specialist *pspec = specialist_by_number(sp);

              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          specialist_plural_translation(pspec));
              pitem->topic = qstrdup(name);
              pitem->text = qstrdup(empty);
              category_nodes.append(pitem);
            }
            specialist_type_iterate_end;
            break;
          case HELP_GOVERNMENT:
            for (const auto &gov : governments) {
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          government_name_translation(&gov));
              pitem->topic = qstrdup(name);
              pitem->text = qstrdup(empty);
              category_nodes.append(pitem);
            };
            break;
          case HELP_IMPROVEMENT:
            improvement_iterate(pimprove)
            {
              if (valid_improvement(pimprove)
                  && !is_great_wonder(pimprove)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            improvement_name_translation(pimprove));
                pitem->topic = qstrdup(name);
                pitem->text = qstrdup(empty);
                category_nodes.append(pitem);
              }
            }
            improvement_iterate_end;
            break;
          case HELP_WONDER:
            improvement_iterate(pimprove)
            {
              if (valid_improvement(pimprove) && is_great_wonder(pimprove)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            improvement_name_translation(pimprove));
                pitem->topic = qstrdup(name);
                pitem->text = qstrdup(empty);
                category_nodes.append(pitem);
              }
            }
            improvement_iterate_end;
            break;
          case HELP_RULESET: {
            int desc_len;
            int len;

            pitem = new_help_item(HELP_RULESET);
            //           pitem->topic = qstrdup(_(game.control.name));
            fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                        Q_(HELP_RULESET_ITEM));
            pitem->topic = qstrdup(name);
            if (game.ruleset_description != nullptr) {
              desc_len = qstrlen("\n\n") + qstrlen(game.ruleset_description);
            } else {
              desc_len = 0;
            }
            if (game.ruleset_summary != nullptr) {
              if (game.control.version[0] != '\0') {
                len = qstrlen(_(game.control.name)) + qstrlen(" ")
                      + qstrlen(game.control.version) + qstrlen("\n\n")
                      + qstrlen(_(game.ruleset_summary)) + 1;

                pitem->text = new char[len + desc_len];
                fc_snprintf(pitem->text, len, "%s %s\n\n%s",
                            _(game.control.name), game.control.version,
                            _(game.ruleset_summary));
              } else {
                len = qstrlen(_(game.control.name)) + qstrlen("\n\n")
                      + qstrlen(_(game.ruleset_summary)) + 1;

                pitem->text = new char[len + desc_len];
                fc_snprintf(pitem->text, len, "%s\n\n%s",
                            _(game.control.name), _(game.ruleset_summary));
              }
            } else {
              const char *nodesc = _("Current ruleset contains no summary.");

              if (game.control.version[0] != '\0') {
                len = qstrlen(_(game.control.name)) + qstrlen(" ")
                      + qstrlen(game.control.version) + qstrlen("\n\n")
                      + qstrlen(nodesc) + 1;

                pitem->text = new char[len + desc_len];
                fc_snprintf(pitem->text, len, "%s %s\n\n%s",
                            _(game.control.name), game.control.version,
                            nodesc);
              } else {
                len = qstrlen(_(game.control.name)) + qstrlen("\n\n")
                      + qstrlen(nodesc) + 1;

                pitem->text = new char[len + desc_len];
                fc_snprintf(pitem->text, len, "%s\n\n%s",
                            _(game.control.name), nodesc);
              }
            }
            if (game.ruleset_description != nullptr) {
              fc_strlcat(pitem->text, "\n\n", len + desc_len);
              fc_strlcat(pitem->text, game.ruleset_description,
                         len + desc_len);
            }
            help_nodes->append(pitem);
          } break;
          case HELP_TILESET: {
            if (tileset_help) {
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          Q_(HELP_TILESET_ITEM));
              tileset_help->topic = qstrdup(name);
              help_nodes->append(tileset_help);
            }
          } break;
          case HELP_NATIONS:
            for (const auto &pnation : nations) {
              if (nations_to_show
                  && nation_is_in_set(&pnation, nations_to_show)) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            nation_plural_translation(&pnation));
                pitem->topic = qstrdup(name);
                pitem->text = qstrdup(empty);
                category_nodes.append(pitem);
              }
            } // iterate over nations - pnation
            break;
          case HELP_MULTIPLIER:
            multipliers_iterate(pmul)
            {
              help_text_buffer[0] = '\0';
              pitem = new_help_item(current_type);
              fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                          name_translation_get(&pmul->name));
              pitem->topic = qstrdup(name);
              if (pmul->helptext) {
                const char *sep = "";
                for (const auto &text : qAsConst(*pmul->helptext)) {
                  cat_snprintf(help_text_buffer, sizeof(help_text_buffer),
                               "%s%s", sep, qUtf8Printable(text));
                  sep = "\n\n";
                }
              }
              pitem->text = qstrdup(help_text_buffer);
              help_nodes->append(pitem);
            }
            multipliers_iterate_end;
            break;
          case HELP_EFFECT:
            for (int i = 0; i < EFT_COUNT; ++i) {
              auto effects = get_effects(static_cast<effect_type>(i));
              if (effect_list_size(effects) > 0) {
                pitem = new_help_item(current_type);
                fc_snprintf(name, sizeof(name), "%*s%s", level, "",
                            effect_type_name(static_cast<effect_type>(i)));
                pitem->topic = qstrdup(name);

                QString all_text = _("The following rules contribute to the "
                                     "value of this effect:\n");
                effect_list_iterate(effects, peffect)
                {
                  if (requirement_vector_size(&peffect->reqs) == 0) {
                    all_text += QString(_("* %1 by default\n"))
                                    .arg(effect_type_unit_text(
                                        peffect->type, peffect->value));
                  } else {
                    help_text_buffer[0] = '\0';
                    get_effect_req_text(peffect, help_text_buffer,
                                        sizeof(help_text_buffer));
                    all_text += QString(_("* %1 with %2\n"))
                                    .arg(effect_type_unit_text(
                                        peffect->type, peffect->value))
                                    .arg(help_text_buffer);
                  }
                }
                effect_list_iterate_end;

                pitem->text = qstrdup(qUtf8Printable(all_text));
                help_nodes->append(pitem);
              }
            }
            break;
          default:
            qCritical("Bad current_type: %d.", current_type);
            break;
          }
          std::sort(category_nodes.begin(), category_nodes.end(),
                    help_item_compar);
          helpList::iterator it = category_nodes.begin();
          while (it != category_nodes.end()) {
            help_nodes->append(*it);
            it++;
          }
          continue;
        }
      }

      // It wasn't a "generate" node:

      pitem = new_help_item(HELP_TEXT);
      pitem->topic =
          qstrdup(Q_(secfile_lookup_str(sf, "%s.name", sec_name)));

      paras = secfile_lookup_str_vec(sf, &npara, "%s.text", sec_name);

      long_buffer[0] = '\0';
      for (int i = 0; i < npara; i++) {
        bool inserted;
        const char *para = paras[i];

        if (strncmp(para, "$", 1) == 0) {
          inserted = insert_generated_text(long_buffer, sizeof(long_buffer),
                                           para + 1);
        } else {
          sz_strlcat(long_buffer, _(para));
          inserted = true;
        }
        if (inserted && i != npara - 1) {
          sz_strlcat(long_buffer, "\n\n");
        }
      }
      delete[] paras;
      paras = nullptr;
      pitem->text = qstrdup(long_buffer);
      help_nodes->append(pitem);
    }
    section_list_iterate_end;

    section_list_destroy(sec);
  }

  secfile_check_unused(sf);
  secfile_destroy(sf);
  booted = true;
  qDebug("Booted help texts ok");
}

/**
  The following few functions are essentially wrappers for the
  help_nodes help_list.  This allows us to avoid exporting the
  help_list, and instead only access it through a controlled
  interface.
 */

/**
   Find help item by name and type.
   Returns help item, and sets (*pos) to position in list.
   If no item, returns pointer to static internal item with
   some faked data, and sets (*pos) to -1.
 */
const struct help_item *
get_help_item_spec(const char *name, enum help_page_type htype, int *pos)
{
  int idx;
  const struct help_item *pitem = nullptr;
  static struct help_item vitem; // v = virtual
  static char vtopic[128];
  static char vtext[256];

  idx = 0;

  for (const auto *ptmp : qAsConst(*help_nodes)) {
    char *p = ptmp->topic;

    while (*p == ' ') {
      p++;
    }
    if (strcmp(name, p) == 0 && (htype == HELP_ANY || htype == ptmp->type)) {
      pitem = ptmp;
      break;
    }
    idx++;
  }

  if (!pitem) {
    idx = -1;
    vitem.topic = vtopic;
    sz_strlcpy(vtopic, name);
    vitem.text = vtext;
    if (htype == HELP_ANY || htype == HELP_TEXT) {
      fc_snprintf(vtext, sizeof(vtext), _("Sorry, no help topic for %s.\n"),
                  vitem.topic);
      vitem.type = HELP_TEXT;
    } else {
      fc_snprintf(vtext, sizeof(vtext),
                  _("Sorry, no help topic for %s.\n"
                    "This page was auto-generated.\n\n"),
                  vitem.topic);
      vitem.type = htype;
    }
    pitem = &vitem;
  }
  *pos = idx;
  return pitem;
}

/**
  FIXME:
  Also, in principle these could be auto-generated once, inserted
  into pitem->text, and then don't need to keep re-generating them.
  Only thing to be careful of would be changeable data, but don't
  have that here (for ruleset change or spacerace change must
  re-boot helptexts anyway).  Eg, genuinely dynamic information
  which could be useful would be if help system said which wonders
  have been built (or are being built and by who/where?)
 */

/**
   Write dynamic text for buildings (including wonders).  This includes
   the ruleset helptext as well as any automatically generated text.

   pplayer may be nullptr.
   user_text, if non-nullptr, will be appended to the text.
 */
char *helptext_building(char *buf, size_t bufsz, struct player *pplayer,
                        const char *user_text,
                        const struct impr_type *pimprove,
                        const nation_set *nations_to_show)
{
  bool reqs = false;
  struct universal source = {.value = {.building = pimprove},
                             .kind = VUT_IMPROVEMENT};

  fc_assert_ret_val(nullptr != buf && 0 < bufsz, nullptr);
  buf[0] = '\0';

  if (nullptr == pimprove) {
    return buf;
  }

  if (nullptr != pimprove->helptext) {
    for (const auto &text : qAsConst(*pimprove->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }

  // Add requirement text for improvement itself
  requirement_vector_iterate(&pimprove->reqs, preq)
  {
    if (req_text_insert_nl(buf, bufsz, pplayer, preq, VERB_DEFAULT, "")) {
      reqs = true;
    }
  }
  requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  requirement_vector_iterate(&pimprove->obsolete_by, pobs)
  {
    if (VUT_ADVANCE == pobs->source.kind && pobs->present) {
      cat_snprintf(buf, bufsz,
                   _("* The discovery of %s will make %s obsolete.\n"),
                   advance_name_translation(pobs->source.value.advance),
                   improvement_name_translation(pimprove));
    }
    if (VUT_IMPROVEMENT == pobs->source.kind && pobs->present) {
      cat_snprintf(buf, bufsz,
                   // TRANS: both %s are improvement names
                   _("* The presence of %s in the city will make %s "
                     "obsolete.\n"),
                   improvement_name_translation(pobs->source.value.building),
                   improvement_name_translation(pimprove));
    }
  }
  requirement_vector_iterate_end;

  if (is_small_wonder(pimprove)) {
    cat_snprintf(buf, bufsz,
                 _("* A 'small wonder': at most one of your cities may "
                   "possess this improvement.\n"));
  }
  /* (Great wonders are in their own help section explaining their
   * uniqueness, so we don't mention it here.) */

  if (building_has_effect(pimprove, EFT_ENABLE_NUKE)) {
    action_id nuke_actions[MAX_NUM_ACTIONS];
    struct unit_type *u = nullptr;

    {
      // Find Manhattan dependent nuke actions
      int i = 0;

      action_list_add_all_by_result(nuke_actions, &i, ACTRES_NUKE);
      action_list_add_all_by_result(nuke_actions, &i, ACTRES_NUKE_CITY);
      action_list_add_all_by_result(nuke_actions, &i, ACTRES_NUKE_UNITS);

      action_list_end(nuke_actions, i);
    }

    action_list_iterate(nuke_actions, act_id)
    {
      if (num_role_units(action_id_get_role(act_id)) > 0) {
        u = get_role_unit(action_id_get_role(act_id), 0);
        break;
      }
    }
    action_list_iterate_end;

    if (u) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: 'Allows all players with knowledge of atomic
                    * power to build nuclear units.' */
                   _("* Allows all players with knowledge of %s "
                     "to build %s units.\n"),
                   advance_name_translation(u->require_advance),
                   utype_name_translation(u));
    }
  }

  insert_allows(&source, buf + qstrlen(buf), bufsz - qstrlen(buf),
                // TRANS: bullet point; note trailing space
                Q_("?bullet:* "));

  // Actions that requires the building to target a city.
  action_iterate(act)
  {
    // Nothing is found yet.
    bool demanded = false;
    enum req_range max_range = REQ_RANGE_LOCAL;

    if (action_id_get_target_kind(act) != ATK_CITY) {
      // Not relevant
      continue;
    }

    if (action_by_number(act)->quiet) {
      // The ruleset it self documents this action.
      continue;
    }

    action_enabler_list_iterate(action_enablers_for_action(act), enabler)
    {
      if (universal_fulfills_requirements(true, &(enabler->target_reqs),
                                          &source)) {
        // The building is needed by this action enabler.
        demanded = true;

        // See if this enabler gives the building a wider range.
        requirement_vector_iterate(&(enabler->target_reqs), preq)
        {
          if (!universal_is_relevant_to_requirement(preq, &source)) {
            // Not relevant.
            continue;
          }

          if (!preq->present) {
            /* A !present larger range requirement would make the present
             * lower range illegal. */
            continue;
          }

          if (preq->range > max_range) {
            // Found a larger range.
            max_range = preq->range;
            /* Intentionally not breaking here. The requirement vector may
             * contain other requirements with a larger range.
             * Example: Building a GreatWonder in a city with a Palace. */
          }
        }
        requirement_vector_iterate_end;
      }
    }
    action_enabler_list_iterate_end;

    if (demanded) {
      switch (max_range) {
      case REQ_RANGE_LOCAL:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target the city building it "
                       "with the action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_CITY:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target its city with the "
                       "action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_TRADEROUTE:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target its city and its "
                       "trade partners with the action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_CONTINENT:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target all cities with its "
                       "owner on its continent with the action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_PLAYER:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target all cities with its "
                       "owner with the action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_TEAM:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target all cities on the "
                       "same team with the action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_ALLIANCE:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target all cities owned by "
                       "or allied to its owner with the action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_WORLD:
        /* At least one action enabler needed the building in its target
         * requirements. */
        cat_snprintf(buf, bufsz,
                     // TRANS: Help build Wonder
                     _("* Makes it possible to target all cities with the "
                       "action \'%s\'.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_CADJACENT:
      case REQ_RANGE_ADJACENT:
      case REQ_RANGE_COUNT:
        qCritical("The range %s is invalid for buildings.",
                  req_range_name(max_range));
        break;
      }
    }
  }
  action_iterate_end;

  // Building protects against action.
  action_iterate(act)
  {
    // Nothing is found yet.
    bool vulnerable = false;
    enum req_range min_range = REQ_RANGE_COUNT;

    if (action_id_get_target_kind(act) != ATK_CITY) {
      // Not relevant
      continue;
    }

    if (action_enabler_list_size(action_enablers_for_action(act)) == 0) {
      // This action isn't enabled at all.
      continue;
    }

    if (action_by_number(act)->quiet) {
      // The ruleset it self documents this action.
      continue;
    }

    // Must be immune in all cases.
    action_enabler_list_iterate(action_enablers_for_action(act), enabler)
    {
      if (requirement_fulfilled_by_improvement(pimprove,
                                               &(enabler->target_reqs))) {
        vulnerable = true;
        break;
      } else {
        enum req_range vector_max_range = REQ_RANGE_LOCAL;

        requirement_vector_iterate(&(enabler->target_reqs), preq)
        {
          if (!universal_is_relevant_to_requirement(preq, &source)) {
            // Not relevant.
            continue;
          }

          if (preq->present) {
            // Not what is looked for.
            continue;
          }

          if (preq->range > vector_max_range) {
            // Found a larger range.
            vector_max_range = preq->range;
          }
        }
        requirement_vector_iterate_end;

        if (vector_max_range < min_range) {
          // Found a smaller range.
          min_range = vector_max_range;
        }
      }
    }
    action_enabler_list_iterate_end;

    if (!vulnerable) {
      switch (min_range) {
      case REQ_RANGE_LOCAL:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "the city building it.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_CITY:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "its city.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_TRADEROUTE:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "its city or to its city's trade partners.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_CONTINENT:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "any city with its owner on its continent.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_PLAYER:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "any city with its owner.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_TEAM:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "any city on the same team.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_ALLIANCE:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "any city allied to or owned by its owner.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_WORLD:
        cat_snprintf(buf, bufsz,
                     // TRANS: Incite City
                     _("* Makes it impossible to do the action \'%s\' to "
                       "any city in the game.\n"),
                     qUtf8Printable(action_id_name_translation(act)));
        break;
      case REQ_RANGE_CADJACENT:
      case REQ_RANGE_ADJACENT:
      case REQ_RANGE_COUNT:
        qCritical("The range %s is invalid for buildings.",
                  req_range_name(min_range));
        break;
      }
    }
  }
  action_iterate_end;

  {
    int i;

    for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
      Impr_type_id n = game.rgame.global_init_buildings[i];
      if (n == B_LAST) {
        break;
      } else if (improvement_by_number(n) == pimprove) {
        cat_snprintf(buf, bufsz,
                     _("* All players start with this improvement in their "
                       "first city.\n"));
        break;
      }
    }
  }

  /* Assume no-one will set the same building in both global and nation
   * init_buildings... */
  for (const auto &pnation : nations) {
    // Avoid mentioning nations not in current set.
    if (nations_to_show && !nation_is_in_set(&pnation, nations_to_show)) {
      continue;
    }
    for (int n : pnation.init_buildings) {
      if (n == B_LAST) {
        break;
      } else if (improvement_by_number(n) == pimprove) {
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a nation plural
                     _("* The %s start with this improvement in their "
                       "first city.\n"),
                     nation_plural_translation(&pnation));
        break;
      }
    }
  } // iterate over nations - pnation

  if (improvement_has_flag(pimprove, IF_SAVE_SMALL_WONDER)) {
    cat_snprintf(buf, bufsz,
                 // TRANS: don't translate 'savepalace'
                 _("* If you lose the city containing this improvement, "
                   "it will be rebuilt for free in another of your cities "
                   "(if the 'savepalace' server setting is enabled).\n"));
  }

  if (user_text && user_text[0] != '\0') {
    cat_snprintf(buf, bufsz, "\n\n%s", user_text);
  }
  return buf;
}

/**
   Returns TRUE iff the specified unit type is able to perform an action
   that allows it to escape to the closest closest domestic city once done.

   See diplomat_escape() for more.
 */
static bool utype_may_do_escape_action(const struct unit_type *utype)
{
  action_iterate(act_id)
  {
    struct action *paction = action_by_number(act_id);

    if (action_get_actor_kind(paction) != AAK_UNIT) {
      // Not relevant.
      continue;
    }

    if (!utype_can_do_action(utype, paction->id)) {
      // Can't do it.
      continue;
    }

    if (utype_is_consumed_by_action(paction, utype)) {
      // No escape when dead.
      continue;
    }

    if (paction->actor.is_unit.moves_actor == MAK_ESCAPE) {
      // Survives and escapes.
      return true;
    }
  }
  action_iterate_end;

  return false;
}

/**
   Append misc dynamic text for units.
   Transport capacity, unit flags, fuel.

   pplayer may be nullptr.
 */
char *helptext_unit(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, const struct unit_type *utype,
                    const nation_set *nations_to_show)
{
  bool has_vet_levels;
  int flagid;
  struct unit_class *pclass;
  int fuel;

  fc_assert_ret_val(nullptr != buf && 0 < bufsz && nullptr != user_text,
                    nullptr);

  if (!utype) {
    qCritical("Unknown unit!");
    fc_strlcpy(buf, user_text, bufsz);
    return buf;
  }

  has_vet_levels = utype_veteran_levels(utype) > 1;

  buf[0] = '\0';

  pclass = utype_class(utype);
  cat_snprintf(buf, bufsz, _("* Belongs to %s unit class."),
               uclass_name_translation(pclass));
  if (nullptr != pclass->helptext) {
    for (const auto &text : qAsConst(*pclass->helptext)) {
      cat_snprintf(buf, bufsz, "\n%s\n", _(qUtf8Printable(text)));
    }
  } else {
    CATLSTR(buf, bufsz, "\n");
  }
  if (uclass_has_flag(pclass, UCF_CAN_OCCUPY_CITY)
      && !utype_has_flag(utype, UTYF_CIVILIAN)) {
    // TRANS: indented unit class property, preserve leading spaces
    CATLSTR(buf, bufsz, _("  * Can occupy empty enemy cities.\n"));
  }
  if (!uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    // TRANS: indented unit class property, preserve leading spaces
    CATLSTR(buf, bufsz, _("  * Speed is not affected by terrain.\n"));
  }
  if (!uclass_has_flag(pclass, UCF_TERRAIN_DEFENSE)) {
    // TRANS: indented unit class property, preserve leading spaces
    CATLSTR(buf, bufsz,
            _("  * Does not get defense bonuses from terrain.\n"));
  }
  if (!uclass_has_flag(pclass, UCF_ZOC)) {
    // TRANS: indented unit class property, preserve leading spaces
    CATLSTR(buf, bufsz, _("  * Not subject to zones of control.\n"));
  } else if (!utype_has_flag(utype, UTYF_IGZOC)) {
    // TRANS: indented unit class property, preserve leading spaces
    CATLSTR(buf, bufsz, _("  * Subject to zones of control.\n"));
  }
  if (uclass_has_flag(pclass, UCF_DAMAGE_SLOWS)) {
    // TRANS: indented unit class property, preserve leading spaces
    CATLSTR(buf, bufsz, _("  * Slowed down while damaged.\n"));
  }
  if (utype->defense_strength > 0) {
    struct universal unit_is_in_city[] = {
        {.value = {.utype = utype}, .kind = VUT_UTYPE},
        {.value = {.citytile = CITYT_CENTER}, .kind = VUT_CITYTILE},
    };
    int bonus = effect_value_from_universals(EFT_FORTIFY_DEFENSE_BONUS,
                                             unit_is_in_city,
                                             ARRAY_SIZE(unit_is_in_city));

    if (bonus > 0) {
      cat_snprintf(buf, bufsz,
                   /* TRANS: indented unit class property, preserve leading
                    * spaces */
                   _("  * Gets a %d%% defensive bonus while in cities.\n"),
                   bonus);
    }
  }
  if (uclass_has_flag(pclass, UCF_UNREACHABLE)) {
    CATLSTR(buf, bufsz,
            // TRANS: indented unit class property, preserve leading spaces
            _("  * Is unreachable. Most units cannot attack this one.\n"));
    if (utype_has_flag(utype, UTYF_NEVER_PROTECTS)) {
      CATLSTR(buf, bufsz,
              // TRANS: indented twice; preserve leading spaces
              _("    * Doesn't prevent enemy units from attacking other "
                "units on its tile.\n"));
    }
  }
  if (uclass_has_flag(pclass, UCF_DOESNT_OCCUPY_TILE)
      && !utype_has_flag(utype, UTYF_CIVILIAN)) {
    CATLSTR(buf, bufsz,
            // TRANS: indented unit class property, preserve leading spaces
            _("  * Doesn't prevent enemy cities from working the tile it's "
              "on.\n"));
  }
  if (can_attack_non_native(utype)) {
    CATLSTR(buf, bufsz,
            // TRANS: indented unit class property, preserve leading spaces
            _("  * Can attack units on non-native tiles.\n"));
  }
  for (flagid = UCF_USER_FLAG_1; flagid <= UCF_LAST_USER_FLAG; flagid++) {
    if (uclass_has_flag(pclass, static_cast<unit_class_flag_id>(flagid))) {
      const char *helptxt =
          unit_class_flag_helptxt(static_cast<unit_class_flag_id>(flagid));

      if (helptxt != nullptr) {
        // TRANS: indented unit class property, preserve leading spaces
        CATLSTR(buf, bufsz, Q_("?bullet:  * "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }

  /* The unit's combat bonuses. Won't mention that another unit type has a
   * combat bonus against this unit type. Doesn't handle complex cases like
   * when a unit type has multiple combat bonuses of the same kind. */
  combat_bonus_list_iterate(utype->bonuses, cbonus)
  {
    if (cbonus->quiet) {
      // Handled in the help text of the ruleset.
      continue;
    }
    QVector<QString> against;
    against.reserve(utype_count());

    // Find the unit types of the bonus targets.
    unit_type_iterate(utype2)
    {
      if (utype_has_flag(utype2, cbonus->flag)) {
        against.append(utype_name_translation(utype2));
      }
    }
    unit_type_iterate_end;

    if (!against.isEmpty()) {
      switch (cbonus->type) {
      case CBONUS_DEFENSE_MULTIPLIER:
        cat_snprintf(buf, bufsz,
                     // TRANS: percentage ... or-list of unit types
                     _("* %d%% defense bonus if attacked by %s.\n"),
                     cbonus->value * 100,
                     qUtf8Printable(strvec_to_or_list(against)));
        break;
      case CBONUS_DEFENSE_DIVIDER:
        cat_snprintf(buf, bufsz,
                     // TRANS: defense divider ... or-list of unit types
                     _("* Reduces target's defense to 1 / %d when "
                       "attacking %s.\n"),
                     cbonus->value + 1,
                     qUtf8Printable(strvec_to_or_list(against)));
        break;
      case CBONUS_FIREPOWER1:
        cat_snprintf(buf, bufsz,
                     // TRANS: or-list of unit types
                     _("* Reduces target's fire power to 1 when "
                       "attacking %s.\n"),
                     qUtf8Printable(strvec_to_and_list(against)));
        break;
      case CBONUS_DEFENSE_MULTIPLIER_PCT:
        cat_snprintf(buf, bufsz,
                     // TRANS: percentage ... or-list of unit types
                     _("* %d%% defense bonus if attacked by %s.\n"),
                     cbonus->value,
                     qUtf8Printable(strvec_to_or_list(against)));
        break;
      case CBONUS_DEFENSE_DIVIDER_PCT:
        cat_snprintf(buf, bufsz,
                     // TRANS: defense divider ... or-list of unit types
                     _("* Reduces target's defense to 1 / %.2f when "
                       "attacking %s.\n"),
                     (static_cast<float>(cbonus->value) + 100.0f) / 100.0f,
                     qUtf8Printable(strvec_to_or_list(against)));
        break;
      }
    }
  }
  combat_bonus_list_iterate_end;

  // Add requirement text for the unit type itself
  requirement_vector_iterate(&utype->build_reqs, preq)
  {
    (void) req_text_insert_nl(buf, bufsz, pplayer, preq, VERB_DEFAULT,
                              Q_("?bullet:* "));
  }
  requirement_vector_iterate_end;

  if (utype_has_flag(utype, UTYF_CANESCAPE)) {
    CATLSTR(buf, bufsz, _("* Can escape once stack defender is lost.\n"));
  }
  if (utype_has_flag(utype, UTYF_CANKILLESCAPING)) {
    CATLSTR(buf, bufsz, _("* Can pursue escaping units and kill them.\n"));
  }

  if (utype_has_flag(utype, UTYF_NOBUILD)) {
    CATLSTR(buf, bufsz, _("* May not be built in cities.\n"));
  }
  if (utype_has_flag(utype, UTYF_BARBARIAN_ONLY)) {
    CATLSTR(buf, bufsz, _("* Only barbarians may build this.\n"));
  }
  if (utype_has_flag(utype, UTYF_NEWCITY_GAMES_ONLY)) {
    CATLSTR(buf, bufsz,
            _("* Can only be built in games where new cities "
              "are allowed.\n"));
    if (game.scenario.prevent_new_cities) {
      // TRANS: indented; preserve leading spaces
      CATLSTR(buf, bufsz,
              _("  - New cities are not allowed in the current "
                "game.\n"));
    } else {
      // TRANS: indented; preserve leading spaces
      CATLSTR(buf, bufsz,
              _("  - New cities are allowed in the current "
                "game.\n"));
    }
  }
  for (const auto &pnation : nations) {
    int i, count = 0;

    // Avoid mentioning nations not in current set.
    if (nations_to_show && !nation_is_in_set(&pnation, nations_to_show)) {
      continue;
    }
    for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
      if (!pnation.init_units[i]) {
        break;
      } else if (pnation.init_units[i] == utype) {
        count++;
      }
    }
    if (count > 0) {
      cat_snprintf(buf, bufsz,
                   // TRANS: %s is a nation plural
                   PL_("* The %s start the game with %d of these units.\n",
                       "* The %s start the game with %d of these units.\n",
                       count),
                   nation_plural_translation(&pnation), count);
    }
  } // iterate over nations - pnation
  {
    QVector<QString> types;
    types.reserve(utype_count());

    unit_type_iterate(utype2)
    {
      if (utype2->converted_to == utype
          && utype_can_do_action_result(utype2, ACTRES_CONVERT)) {
        types.append(utype_name_translation(utype2));
      }
    }
    unit_type_iterate_end;
    if (!types.isEmpty()) {
      cat_snprintf(buf, bufsz,
                   // TRANS: %s is a list of unit types separated by "or".
                   _("* May be obtained by conversion of %s.\n"),
                   qUtf8Printable(strvec_to_or_list(types)));
    }
  }
  if (utype_has_flag(utype, UTYF_NOHOME)) {
    CATLSTR(buf, bufsz, _("* Never has a home city.\n"));
  }
  if (utype_has_flag(utype, UTYF_GAMELOSS)) {
    CATLSTR(buf, bufsz, _("* Losing this unit will lose you the game!\n"));
  }
  if (utype_has_flag(utype, UTYF_UNIQUE)) {
    CATLSTR(buf, bufsz,
            _("* Each player may only have one of this type of unit.\n"));
  }
  for (flagid = UTYF_USER_FLAG_1; flagid <= UTYF_LAST_USER_FLAG; flagid++) {
    if (utype_has_flag(utype, flagid)) {
      const char *helptxt =
          unit_type_flag_helptxt(static_cast<unit_type_flag_id>(flagid));

      if (helptxt != nullptr) {
        // TRANS: bullet point; note trailing space
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }
  if (utype->pop_cost > 0) {
    cat_snprintf(buf, bufsz,
                 PL_("* Costs %d population to build.\n",
                     "* Costs %d population to build.\n", utype->pop_cost),
                 utype->pop_cost);
  }
  if (0 < utype->transport_capacity) {
    QVector<QString> classes;
    classes.reserve(uclass_count());

    unit_class_iterate(uclass)
    {
      if (can_unit_type_transport(utype, uclass)) {
        classes.append(uclass_name_translation(uclass));
      }
    }
    unit_class_iterate_end;

    cat_snprintf(buf, bufsz,
                 // TRANS: %s is a list of unit classes separated by "or".
                 PL_("* Can carry and refuel %d %s unit.\n",
                     "* Can carry and refuel up to %d %s units.\n",
                     utype->transport_capacity),
                 utype->transport_capacity,
                 qUtf8Printable(strvec_to_or_list(classes)));
    if (uclass_has_flag(utype_class(utype), UCF_UNREACHABLE)) {
      /* Document restrictions on when units can load/unload */
      bool has_restricted_load = false, has_unrestricted_load = false,
           has_restricted_unload = false, has_unrestricted_unload = false;
      unit_type_iterate(pcargo)
      {
        if (can_unit_type_transport(utype, utype_class(pcargo))) {
          if (utype_can_freely_load(pcargo, utype)) {
            has_unrestricted_load = true;
          } else {
            has_restricted_load = true;
          }
          if (utype_can_freely_unload(pcargo, utype)) {
            has_unrestricted_unload = true;
          } else {
            has_restricted_unload = true;
          }
        }
      }
      unit_type_iterate_end;
      if (has_restricted_load) {
        if (has_unrestricted_load) {
          /* At least one type of cargo can load onto us freely.
           * The specific exceptions will be documented in cargo help. */
          CATLSTR(buf, bufsz,
                  // TRANS: indented; preserve leading spaces
                  _("  * Some cargo cannot be loaded except in a city or a "
                    "base native to this transport.\n"));
        } else {
          // No exceptions
          CATLSTR(buf, bufsz,
                  // TRANS: indented; preserve leading spaces
                  _("  * Cargo cannot be loaded except in a city or a "
                    "base native to this transport.\n"));
        }
      } // else, no restricted cargo exists; keep quiet
      if (has_restricted_unload) {
        if (has_unrestricted_unload) {
          // At least one type of cargo can unload from us freely.
          CATLSTR(
              buf, bufsz,
              // TRANS: indented; preserve leading spaces
              _("  * Some cargo cannot be unloaded except in a city or a "
                "base native to this transport.\n"));
        } else {
          // No exceptions
          CATLSTR(buf, bufsz,
                  // TRANS: indented; preserve leading spaces
                  _("  * Cargo cannot be unloaded except in a city or a "
                    "base native to this transport.\n"));
        }
      } // else, no restricted cargo exists; keep quiet
    }
  }
  if (utype_has_flag(utype, UTYF_COAST_STRICT)) {
    CATLSTR(buf, bufsz, _("* Must stay next to safe coast.\n"));
  }
  {
    /* Document exceptions to embark/disembark restrictions that we
     * have as cargo. */
    QBitArray embarks, disembarks;
    embarks.resize(UCL_LAST);
    disembarks.resize(UCL_LAST);
    /* Determine which of our transport classes have restrictions in the
     * first place (that is, contain at least one transport which carries at
     * least one type of cargo which is restricted). We'll suppress output
     * for classes not in this set, since this cargo type is not behaving
     * exceptionally in such cases. */
    unit_type_iterate(utrans)
    {
      const Unit_Class_id trans_class = uclass_index(utype_class(utrans));
      /* Don't waste time repeating checks on classes we've already checked,
       * or weren't under consideration in the first place */
      if (!embarks.at(trans_class)
          && BV_ISSET(utype->embarks, trans_class)) {
        unit_type_iterate(other_cargo)
        {
          if (can_unit_type_transport(utrans, utype_class(other_cargo))
              && !utype_can_freely_load(other_cargo, utrans)) {
            /* At least one load restriction in transport class, which
             * we aren't subject to */
            embarks.setBit(trans_class);
          }
        }
        unit_type_iterate_end; // cargo
      }
      if (!disembarks.at(trans_class)
          && BV_ISSET(utype->disembarks, trans_class)) {
        unit_type_iterate(other_cargo)
        {
          if (can_unit_type_transport(utrans, utype_class(other_cargo))
              && !utype_can_freely_unload(other_cargo, utrans)) {
            /* At least one load restriction in transport class, which
             * we aren't subject to */
            disembarks.setBit(trans_class);
          }
        }
        unit_type_iterate_end; // cargo
      }
    }
    unit_class_iterate_end; // transports

    if (is_any_set(embarks)) {
      // Build list of embark exceptions
      QVector<QString> eclasses;
      eclasses.reserve(uclass_count());

      unit_class_iterate(uclass)
      {
        if (embarks.at(uclass_index(uclass))) {
          eclasses.append(uclass_name_translation(uclass));
        }
      }
      unit_class_iterate_end;
      QString elist = strvec_to_or_list(eclasses);
      if (embarks == disembarks) {
        // A common case: the list of disembark exceptions is identical
        cat_snprintf(buf, bufsz,
                     /* TRANS: %s is a list of unit classes separated
                      * by "or". */
                     _("* May load onto and unload from %s transports even "
                       "when underway.\n"),
                     qUtf8Printable(elist));
      } else {
        cat_snprintf(
            buf, bufsz,
            /* TRANS: %s is a list of unit classes separated
             * by "or". */
            _("* May load onto %s transports even when underway.\n"),
            qUtf8Printable(elist));
      }
    }
    if (is_any_set(disembarks) && embarks != disembarks) {
      // Build list of disembark exceptions (if different from embarking)
      QVector<QString> dclasses;
      dclasses.reserve(uclass_count());

      unit_class_iterate(uclass)
      {
        if (disembarks.at(uclass_index(uclass))) {
          dclasses.append(uclass_name_translation(uclass));
        }
      }
      unit_class_iterate_end;
      QString dlist = strvec_to_or_list(dclasses);
      cat_snprintf(
          buf, bufsz,
          /* TRANS: %s is a list of unit classes separated
           * by "or". */
          _("* May unload from %s transports even when underway.\n"),
          qUtf8Printable(dlist));
    }
  }

  if (utype_has_flag(utype, UTYF_SPY)) {
    CATLSTR(buf, bufsz, _("* Strong in diplomatic battles.\n"));
  }
  if (utype_has_flag(utype, UTYF_DIPLOMAT)
      || utype_has_flag(utype, UTYF_SUPERSPY)) {
    CATLSTR(buf, bufsz, _("* Defends cities against diplomatic actions.\n"));
  }
  if (utype_has_flag(utype, UTYF_SUPERSPY)) {
    CATLSTR(buf, bufsz,
            _("* Will never lose a diplomat-versus-diplomat fight.\n"));
  }
  if (utype_may_do_escape_action(utype)
      && utype_has_flag(utype, UTYF_SUPERSPY)) {
    CATLSTR(buf, bufsz, _("* Will always survive a spy mission.\n"));
  }
  if (utype->vlayer == V_INVIS) {
    CATLSTR(
        buf, bufsz,
        _("* Is invisible except when next to an enemy unit or city.\n"));
  }
  if (utype_has_flag(utype, UTYF_ONLY_NATIVE_ATTACK)) {
    CATLSTR(buf, bufsz, _("* Can only attack units on native tiles.\n"));
  }
  if (utype_has_flag(utype, UTYF_CITYBUSTER)) {
    CATLSTR(buf, bufsz,
            _("* Gets double firepower when attacking cities.\n"));
  }
  if (utype_has_flag(utype, UTYF_IGTER)) {
    cat_snprintf(buf, bufsz,
                 /* TRANS: "MP" = movement points. %s may have a
                  * fractional part. */
                 _("* Ignores terrain effects (moving costs at most %s MP "
                   "per tile).\n"),
                 move_points_text(terrain_control.igter_cost, true));
  }
  if (utype_has_flag(utype, UTYF_NOZOC)) {
    CATLSTR(buf, bufsz, _("* Never imposes a zone of control.\n"));
  } else {
    CATLSTR(buf, bufsz,
            _("* May impose a zone of control on its adjacent "
              "tiles.\n"));
  }
  if (utype_has_flag(utype, UTYF_IGZOC)) {
    CATLSTR(buf, bufsz,
            _("* Not subject to zones of control imposed "
              "by other units.\n"));
  }
  if (utype_has_flag(utype, UTYF_CIVILIAN)) {
    CATLSTR(buf, bufsz, _("* A non-military unit:\n"));
    CATLSTR(buf, bufsz,
            // TRANS: indented; preserve leading spaces
            _("  * Cannot attack.\n"));
    CATLSTR(buf, bufsz,
            // TRANS: indented; preserve leading spaces
            _("  * Doesn't impose martial law.\n"));
    CATLSTR(
        buf, bufsz,
        // TRANS: indented; preserve leading spaces
        _("  * Can enter foreign territory regardless of peace treaty.\n"));
    CATLSTR(buf, bufsz,
            // TRANS: indented; preserve leading spaces
            _("  * Doesn't prevent enemy cities from working the tile it's "
              "on.\n"));
  }
  if (utype_has_flag(utype, UTYF_FIELDUNIT)) {
    CATLSTR(buf, bufsz,
            _("* A field unit: one unhappiness applies even when "
              "non-aggressive.\n"));
  }
  if (utype_has_flag(utype, UTYF_PROVOKING)
      && server_setting_value_bool_get(
          server_setting_by_name("autoattack"))) {
    CATLSTR(buf, bufsz,
            _("* An enemy unit considering to auto attack this unit will "
              "choose to do so even if it has better odds when defending "
              "against it than when attacking it.\n"));
  }
  if (utype_has_flag(utype, UTYF_SHIELD2GOLD)) {
    /* FIXME: the conversion shield => gold is activated if
     *        EFT_SHIELD2GOLD_FACTOR is not equal null; how to determine
     *        possible sources? */
    CATLSTR(
        buf, bufsz,
        _("* Under certain conditions the shield upkeep of this unit can "
          "be converted to gold upkeep.\n"));
  }

  unit_class_iterate(target)
  {
    if (uclass_has_flag(target, UCF_UNREACHABLE)
        && BV_ISSET(utype->targets, uclass_index(target))) {
      cat_snprintf(buf, bufsz,
                   _("* Can attack against %s units, which are usually not "
                     "reachable.\n"),
                   uclass_name_translation(target));
    }
  }
  unit_class_iterate_end;

  fuel = utype_fuel(utype);
  if (fuel > 0) {
    QVector<QString> types;
    types.reserve(utype_count() + 1);

    unit_type_iterate(transport)
    {
      if (can_unit_type_transport(transport, utype_class(utype))) {
        types.append(utype_name_translation(transport));
      }
    }
    unit_type_iterate_end;

    if (types.isEmpty()) {
      if (utype_has_flag(utype, UTYF_COAST)) {
        if (fuel == 1) {
          cat_snprintf(buf, bufsz,
                       _("* Unit has to end each turn next to safe coast or"
                         " in a city or a base.\n"));
        } else {
          cat_snprintf(
              buf, bufsz,
              /* Pluralization for the benefit of languages with
               * duals etc */
              // TRANS: Never called for 'turns = 1' case
              PL_("* Unit has to be next to safe coast, in a city or a base"
                  " after %d turn.\n",
                  "* Unit has to be next to safe coast, in a city or a base"
                  " after %d turns.\n",
                  fuel),
              fuel);
        }
      } else {
        cat_snprintf(buf, bufsz,
                     PL_("* Unit has to be in a city or a base"
                         " after %d turn.\n",
                         "* Unit has to be in a city or a base"
                         " after %d turns.\n",
                         fuel),
                     fuel);
      }
    } else {
      if (utype_has_flag(utype, UTYF_COAST)) {
        cat_snprintf(
            buf, bufsz,
            // TRANS: %s is a list of unit types separated by "or"
            PL_("* Unit has to be next to safe coast, in a city, a base, or "
                "on a %s"
                " after %d turn.\n",
                "* Unit has to be next to safe coast, in a city, a base, or "
                "on a %s"
                " after %d turns.\n",
                fuel),
            qUtf8Printable(strvec_to_or_list(types)), fuel);
      } else {
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a list of unit types separated by "or"
                     PL_("* Unit has to be in a city, a base, or on a %s"
                         " after %d turn.\n",
                         "* Unit has to be in a city, a base, or on a %s"
                         " after %d turns.\n",
                         fuel),
                     qUtf8Printable(strvec_to_or_list(types)), fuel);
      }
    }
  }

  // Auto attack immunity. (auto_attack.if_attacker ruleset setting)
  if (server_setting_value_bool_get(server_setting_by_name("autoattack"))) {
    bool not_an_auto_attacker = true;

    action_auto_perf_iterate(auto_action)
    {
      if (auto_action->cause != AAPC_UNIT_MOVED_ADJ) {
        // Not relevant for auto attack.
        continue;
      }

      if (requirement_fulfilled_by_unit_type(utype, &auto_action->reqs)) {
        // Can be forced to auto attack.
        not_an_auto_attacker = false;
        break;
      }
    }
    action_auto_perf_iterate_end;

    if (not_an_auto_attacker) {
      CATLSTR(buf, bufsz,
              _("* Will never be forced (by the autoattack server setting)"
                " to attack units moving to an adjacent tile.\n"));
    }
  }

  action_iterate(act)
  {
    struct action *paction = action_by_number(act);

    if (action_by_number(act)->quiet) {
      // The ruleset documents this action it self.
      continue;
    }

    if (utype_can_do_action(utype, act)) {
      const char *target_adjective;
      char sub_target_text[100];
      QVector<QString> blockers;
      blockers.reserve(MAX_NUM_ACTIONS);
      int i = 0;

      // Generic action information.
      cat_snprintf(buf, bufsz,
                   // TRANS: %s is the action's ruleset defined ui name
                   _("* Can do the action \'%s\'.\n"),
                   qUtf8Printable(action_id_name_translation(act)));

      switch (action_id_get_target_kind(act)) {
      case ATK_SELF:
        // No target.
        break;
      default:
        if (!can_utype_do_act_if_tgt_diplrel(utype, act, DRO_FOREIGN,
                                             true)) {
          // TRANS: describes the target of an action.
          target_adjective = _("domestic ");
        } else if (!can_utype_do_act_if_tgt_diplrel(utype, act, DRO_FOREIGN,
                                                    false)) {
          // TRANS: describes the target of an action.
          target_adjective = _("foreign ");
        } else {
          // Both foreign and domestic targets are acceptable.
          target_adjective = "";
        }

        sub_target_text[0] = '\0';
        if (action_get_sub_target_kind(paction) != ASTK_NONE) {
          cat_snprintf(sub_target_text, sizeof(sub_target_text), _("%s "),
                       _(action_sub_target_kind_name(
                           action_get_sub_target_kind(paction))));
        }

        cat_snprintf(
            buf, bufsz,
            /* TRANS: The first %s is the sub target kind. The next
             * may be an adjective (that includes a space). The
             * next is the name of the target kind.
             * Example: is done to extras on foreign tiles */
            _("  * is done to %s%s%s.\n"), sub_target_text, target_adjective,
            _(action_target_kind_name(action_id_get_target_kind(act))));
      }

      if (utype_is_consumed_by_action(paction, utype)) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: said about an action. %s is a unit type
                      * name. */
                     _("  * uses up the %s.\n"),
                     utype_name_translation(utype));
      }

      if (action_get_battle_kind(paction) != ABK_NONE) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: The %s is a kind of battle defined in
                      * actions.h. Example: "diplomatic battle". */
                     _("  * can lead to a %s against a defender.\n"),
                     action_battle_kind_translated_name(
                         action_get_battle_kind(paction)));
      }

      {
        int odds = action_dice_roll_initial_odds(paction);

        if (odds != ACTION_ODDS_PCT_DICE_ROLL_NA) {
          /* TODO: try to detect that the odds always will be 100% because
           * of the Action_Odds_Pct effect. */
          cat_snprintf(buf, bufsz,
                       _("  * may fail because of a dice throw.\n"));
        }
      }

      if (!utype_is_consumed_by_action(paction, utype)
          && paction->actor.is_unit.moves_actor == MAK_ESCAPE) {
        cat_snprintf(buf, bufsz,
                     /* TRANS: said about an action. %s is a unit type
                      * name. */
                     _("  * the %s may be captured while trying to"
                       " escape after completing the mission.\n"),
                     utype_name_translation(utype));
      }
      {
        struct universal req_pattern[] = {
            {.value = {.action = paction}, .kind = VUT_ACTION},
            {.value = {.utype = utype}, .kind = VUT_UTYPE},
        };
        int success_move_frag_cost = effect_value_from_universals(
            EFT_ACTION_SUCCESS_MOVE_COST, req_pattern,
            ARRAY_SIZE(req_pattern));

        success_move_frag_cost +=
            utype_pays_mp_for_action_base(paction, utype);

        /* Can't print the exact amount of move fragments. It isn't known.
         * The action performer function may subtract some movement itself
         * on top of what EFT_ACTION_SUCCESS_MOVE_COST takes. */
        if (MAX_MOVE_FRAGS <= success_move_frag_cost) {
          // A value this size takes all the actor unit's move fragments.
          cat_snprintf(buf, bufsz, _("  * ends this unit's turn.\n"));
        }
      }

      if (action_id_get_target_kind(act) != ATK_SELF) {
        // Distance to target is relevant.

        /* FIXME: move paratroopers_range to the action and remove this
         * variable once actions are generalized. */
        int relative_max =
            action_has_result(paction, ACTRES_PARADROP)
                ? MIN(paction->max_distance, utype->paratroopers_range)
                : paction->max_distance;

        if (paction->min_distance == relative_max) {
          // Only one distance to target is acceptable

          if (paction->min_distance == 0) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: distance between an actor unit and its
                          * target when performing a specific action. */
                         _("  * target must be at the same tile.\n"));
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: distance between an actor unit and its
                          * target when performing a specific action. */
                         PL_("  * target must be exactly %d tile away.\n",
                             "  * target must be exactly %d tiles away.\n",
                             paction->min_distance),
                         paction->min_distance);
          }
        } else if (relative_max == ACTION_DISTANCE_UNLIMITED) {
          // No max distance

          if (paction->min_distance == 0) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: distance between an actor unit and its
                          * target when performing a specific action. */
                         _("  * target can be anywhere.\n"));
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: distance between an actor unit and its
                          * target when performing a specific action. */
                         PL_("  * target must be at least %d tile away.\n",
                             "  * target must be at least %d tiles away.\n",
                             paction->min_distance),
                         paction->min_distance);
          }
        } else if (paction->min_distance == 0) {
          // No min distance

          cat_snprintf(buf, bufsz,
                       /* TRANS: distance between an actor unit and its
                        * target when performing a specific action. */
                       PL_("  * target can be max %d tile away.\n",
                           "  * target can be max %d tiles away.\n",
                           relative_max),
                       relative_max);
        } else {
          // Full range.

          cat_snprintf(
              buf, bufsz,
              /* TRANS: distance between an actor unit and its
               * target when performing a specific action. */
              PL_("  * target must be between %d and %d tile away.\n",
                  "  * target must be between %d and %d tiles away.\n",
                  relative_max),
              paction->min_distance, relative_max);
        }
      }

      // The action may be a Casus Belli.
      {
        const struct {
          const enum effect_type eft;
          const char *hlp_text;
        } casus_belli[] = {
            // TRANS: ...performing this action ... Casus Belli
            {EFT_CASUS_BELLI_SUCCESS, N_("successfully")},
            // TRANS: ...performing this action ... Casus Belli
            {EFT_CASUS_BELLI_CAUGHT, N_("getting caught before")},
        };

        struct universal req_pattern[] = {
            {.value = {.action = paction}, .kind = VUT_ACTION},
            {.kind = VUT_DIPLREL, /* value filled in later */},
        };

        /* First group by effect (currently getting caught and successfully
         * performing the action) */
        for (i = 0; i < ARRAY_SIZE(casus_belli); i++) {
          int diplrel;

          // DiplRel list of each Casus Belli size.
          QVector<QString> victim_diplrel_names;
          QVector<QString> outrage_diplrel_names;
          outrage_diplrel_names.reserve(DRO_LAST);
          victim_diplrel_names.reserve(DRO_LAST);

          // Ignore Team and everything in diplrel_other.
          for (diplrel = 0; diplrel < DS_NO_CONTACT; diplrel++) {
            int casus_belli_amount;

            if (!can_utype_do_act_if_tgt_diplrel(utype, act, diplrel,
                                                 true)) {
              // Can't do the action. Can't give Casus Belli.
              continue;
            }

            req_pattern[1].value.diplrel = diplrel;
            casus_belli_amount = effect_value_from_universals(
                casus_belli[i].eft, req_pattern, ARRAY_SIZE(req_pattern));

            if (CASUS_BELLI_OUTRAGE <= casus_belli_amount) {
              outrage_diplrel_names.append(
                  diplrel_name_translation(diplrel));
            } else if (CASUS_BELLI_VICTIM <= casus_belli_amount) {
              victim_diplrel_names.append(diplrel_name_translation(diplrel));
            }
          }

          /* Then group by Casus Belli size (currently victim and
           * international outrage) */
          if (!outrage_diplrel_names.isEmpty()) {
            cat_snprintf(
                buf, bufsz,
                // TRANS: successfully ... Peace, or Alliance
                _("  * %s performing this action during %s causes"
                  " international outrage: the whole world gets "
                  "Casus Belli against you.\n"),
                _(casus_belli[i].hlp_text),
                qUtf8Printable(strvec_to_or_list(outrage_diplrel_names)));
          }
          if (!victim_diplrel_names.isEmpty()) {
            cat_snprintf(
                buf, bufsz,
                // TRANS: successfully ... Peace, or Alliance
                _("  * %s performing this action during %s gives"
                  " the victim Casus Belli against you.\n"),
                _(casus_belli[i].hlp_text),
                qUtf8Printable(strvec_to_or_list(victim_diplrel_names)));
          }
        }
      }

      // Custom action result specific information.
      switch (paction->result) {
      case ACTRES_HELP_WONDER:
        cat_snprintf(buf, bufsz,
                     /* TRANS: the %d is the number of shields the unit can
                      * contribute. */
                     _("  * adds %d production.\n"),
                     utype_build_shield_cost_base(utype));
        break;
      case ACTRES_HEAL_UNIT:
        cat_snprintf(buf, bufsz,
                     _("  * restores up to 25%% of the target unit's"
                       " hit points.\n"));
        break;
      case ACTRES_FOUND_CITY:
        if (game.scenario.prevent_new_cities) {
          cat_snprintf(buf, bufsz,
                       // TRANS: is talking about an action.
                       _("  * is disabled in the current game.\n"));
        }
        cat_snprintf(buf, bufsz,
                     // TRANS: the %d is initial population.
                     PL_("  * initial population: %d.\n",
                         "  * initial population: %d.\n", utype->city_size),
                     utype->city_size);
        break;
      case ACTRES_JOIN_CITY:
        cat_snprintf(
            buf, bufsz,
            // TRANS: the %d is population.
            PL_("  * max target size: %d.\n", "  * max target size: %d.\n",
                game.info.add_to_size_limit - utype_pop_value(utype)),
            game.info.add_to_size_limit - utype_pop_value(utype));
        cat_snprintf(buf, bufsz,
                     // TRANS: the %d is the population added.
                     PL_("  * adds %d population.\n",
                         "  * adds %d population.\n",
                         utype_pop_value(utype)),
                     utype_pop_value(utype));
        break;
      case ACTRES_BOMBARD:
        cat_snprintf(buf, bufsz,
                     // TRANS: %d is bombard rate.
                     _("  * %d per turn.\n"), utype->bombard_rate);
        cat_snprintf(buf, bufsz,
                     // TRANS: talking about bombard
                     _("  * These attacks will only damage (never kill)"
                       " defenders, but damage all"
                       " defenders on a tile, and have no risk for the"
                       " attacker.\n"));
        break;
      case ACTRES_UPGRADE_UNIT:
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a unit type.
                     _("  * upgraded to %s or, when possible, to the unit "
                       "type it upgrades to.\n"),
                     utype_name_translation(utype->obsoleted_by));
        break;
      case ACTRES_ATTACK:
        if (game.info.tired_attack) {
          cat_snprintf(buf, bufsz,
                       _("  * weaker when tired. If performed with less "
                         "than a single move point left the attack power "
                         "is reduced accordingly.\n"));
        }
        break;
      case ACTRES_CONVERT:
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a unit type. "MP" = movement points.
                     PL_("  * is converted into %s (takes %d MP).\n",
                         "  * is converted into %s (takes %d MP).\n",
                         utype->convert_time),
                     utype_name_translation(utype->converted_to),
                     utype->convert_time);
        break;
      case ACTRES_SPY_NUKE:
      case ACTRES_NUKE:
      case ACTRES_NUKE_CITY:
      case ACTRES_NUKE_UNITS:
        if (game.info.nuke_pop_loss_pct > 0) {
          cat_snprintf(buf, bufsz,
                       // TRANS: percentage
                       _("  * %d%% of the population of each city inside"
                         " the nuclear blast dies.\n"),
                       game.info.nuke_pop_loss_pct);
        }
        if (game.info.nuke_defender_survival_chance_pct > 0) {
          cat_snprintf(buf, bufsz,
                       _("  * all units caught in the open by the nuclear"
                         " blast dies.\n"));
          cat_snprintf(buf, bufsz,
                       // TRANS: percentage
                       _("  * a unit caught in the nuclear blast while"
                         " inside a city has a %d%% chance of survival.\n"),
                       game.info.nuke_defender_survival_chance_pct);
        } else {
          cat_snprintf(buf, bufsz,
                       _("  * all units caught in the nuclear blast"
                         " dies.\n"));
        }

        break;
      case ACTRES_PLANT:
      case ACTRES_CULTIVATE:
      case ACTRES_TRANSFORM_TERRAIN:
        cat_snprintf(buf, bufsz,
                     _("  * converts target tile terrain to another"
                       " type.\n"));
        break;
      case ACTRES_ROAD:
      case ACTRES_MINE:
      case ACTRES_IRRIGATE:
      case ACTRES_BASE: {
        QVector<QString> extras_vec;

        extra_type_iterate(pextra)
        {
          if (action_creates_extra(paction, pextra)) {
            extras_vec.append(extra_name_translation(pextra));
          }
        }
        extra_type_iterate_end;
        if (extras_vec.count() > 0) {
          ;
          // TRANS: %s is list of extra types separated by ',' and 'and'
          cat_snprintf(buf, bufsz, _("  * builds %s on tiles.\n"),
                       qUtf8Printable(strvec_to_and_list(extras_vec)));
        }
      } break;
      case ACTRES_CLEAN_POLLUTION:
      case ACTRES_CLEAN_FALLOUT: {
        QVector<QString> extras_vec;

        extra_type_iterate(pextra)
        {
          if (action_removes_extra(paction, pextra)) {
            extras_vec.append(extra_name_translation(pextra));
          }
        }
        extra_type_iterate_end;

        if (extras_vec.count() > 0) {
          // TRANS: list of extras separated by "and"
          cat_snprintf(buf, bufsz, _("  * cleans %s from tiles.\n"),
                       qUtf8Printable(strvec_to_and_list(extras_vec)));
        }
      } break;
      case ACTRES_PILLAGE: {
        QVector<QString> extras_vec;

        extra_type_iterate(pextra)
        {
          if (action_removes_extra(paction, pextra)) {
            extras_vec.append(extra_name_translation(pextra));
          }
        }
        extra_type_iterate_end;

        if (extras_vec.count() > 0) {
          // TRANS: list of extras separated by "and"
          cat_snprintf(buf, bufsz, _("  * pillages %s from tiles.\n"),
                       qUtf8Printable(strvec_to_and_list(extras_vec)));
        }
      } break;
      case ACTRES_FORTIFY: {
        struct universal uni_cazfi = {.value = {.utype = utype},
                                      .kind = VUT_UTYPE};
        struct universal unit_is_fortified[] = {
            {.value = {.activity = ACTIVITY_FORTIFIED},
             .kind = VUT_ACTIVITY},
            {.value = {.utype = utype}, .kind = VUT_UTYPE},
        };
        int bonus = effect_value_from_universals(
            EFT_FORTIFY_DEFENSE_BONUS, unit_is_fortified,
            ARRAY_SIZE(unit_is_fortified));

        if (utype->defense_strength <= 0
            || (effect_cumulative_max(EFT_FORTIFY_DEFENSE_BONUS, &uni_cazfi)
                <= 0)) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: indented unit action property, preserve
                        * leading spaces */
                       _("  * to stay put. No defensive bonus.\n"));
        } else if (bonus > 0) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: indented unit action property, preserve
                        * leading spaces */
                       _("  * granting a %d%% defensive bonus.\n"), bonus);
        }
      } break;
      default:
        // No action specific details.
        break;
      }

      i = 0;
      action_iterate(blocker)
      {
        if (!utype_can_do_action(utype, blocker)) {
          // Can't block since never legal.
          continue;
        }

        if (action_id_would_be_blocked_by(act, blocker)) {
          /* action name alone can be MAX_LEN_NAME, leave space for extra
           * characters */
          int maxlen = MAX_LEN_NAME + 16;
          char *quoted = new char[maxlen];

          fc_snprintf(quoted, maxlen,
                      // TRANS: %s is an action that can block another.
                      _("\'%s\'"),
                      qUtf8Printable(action_id_name_translation(blocker)));
          blockers.append(quoted);
        }
      }
      action_iterate_end;

      if (!blockers.isEmpty()) {
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a list of actions separated by "or".
                     _("  * can't be done if %s is legal.\n"),
                     qUtf8Printable(strvec_to_or_list(blockers)));
      }
    }
  }
  action_iterate_end;
  action_iterate(act)
  {
    bool vulnerable;

    if (action_by_number(act)->quiet) {
      // The ruleset documents this action it self.
      continue;
    }

    // Not relevant
    if (action_id_get_target_kind(act) != ATK_UNIT
        && action_id_get_target_kind(act) != ATK_UNITS
        && action_id_get_target_kind(act) != ATK_SELF) {
      continue;
    }

    // All units are immune to this since its not enabled
    if (action_enabler_list_size(action_enablers_for_action(act)) == 0) {
      continue;
    }

    // Must be immune in all cases
    vulnerable = false;
    action_enabler_list_iterate(action_enablers_for_action(act), enabler)
    {
      if (requirement_fulfilled_by_unit_type(utype,
                                             &(enabler->target_reqs))) {
        vulnerable = true;
        break;
      }
    }
    action_enabler_list_iterate_end;

    if (!vulnerable) {
      cat_snprintf(buf, bufsz,
                   _("* Doing the action \'%s\' to this unit"
                     " is impossible.\n"),
                   qUtf8Printable(action_id_name_translation(act)));
    }
  }
  action_iterate_end;
  if (!has_vet_levels) {
    // Only mention this if the game generally does have veteran levels.
    if (game.veteran->levels > 1) {
      CATLSTR(buf, bufsz, _("* Will never achieve veteran status.\n"));
    }
  } else {
    // Not useful currently:
#if 0
    // Some units can never become veteran through combat in practice.
    bool veteran_through_combat =
      !(!utype_can_do_action(utype, ACTION_ATTACK)
        && utype->defense_strength == 0);
#endif
    /* FIXME: if we knew the raise chances on the client, we could be
     * more specific here about whether veteran status can be acquired
     * through combat/missions/work. Should also take into account
     * UTYF_NO_VETERAN when writing this text. (Gna patch #4794) */
    CATLSTR(buf, bufsz, _("* May acquire veteran status.\n"));
    if (utype_veteran_has_power_bonus(utype)) {
      if (utype_can_do_action(utype, ACTION_ATTACK)
          || utype->defense_strength > 0) {
        CATLSTR(buf, bufsz,
                // TRANS: indented; preserve leading spaces
                _("  * Veterans have increased strength in combat.\n"));
      }
      /* SUPERSPY always wins/escapes */
      if (utype_has_flag(utype, UTYF_DIPLOMAT)
          && !utype_has_flag(utype, UTYF_SUPERSPY)) {
        CATLSTR(buf, bufsz,
                // TRANS: indented; preserve leading spaces
                _("  * Veterans have improved chances in diplomatic "
                  "contests.\n"));
        if (utype_may_do_escape_action(utype)) {
          CATLSTR(buf, bufsz,
                  // TRANS: indented; preserve leading spaces
                  _("  * Veterans are more likely to survive missions.\n"));
        }
      }
      if (utype_has_flag(utype, UTYF_SETTLERS)) {
        CATLSTR(buf, bufsz,
                // TRANS: indented; preserve leading spaces
                _("  * Veterans work faster.\n"));
      }
    }
  }
  if (strlen(buf) > 0) {
    CATLSTR(buf, bufsz, "\n");
  }
  if (has_vet_levels && utype->veteran) {
    /* The case where the unit has only a single veteran level has already
     * been handled above, so keep quiet here if that happens */
    if (insert_veteran_help(
            buf, bufsz, utype->veteran,
            _("This type of unit has its own veteran levels:"), nullptr)) {
      CATLSTR(buf, bufsz, "\n\n");
    }
  }
  if (nullptr != utype->helptext) {
    for (const auto &text : qAsConst(*utype->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }
  CATLSTR(buf, bufsz, user_text);
  return buf;
}

/**
   Append misc dynamic text for advance/technology.

   pplayer may be nullptr.
 */
void helptext_advance(char *buf, size_t bufsz, struct player *pplayer,
                      const char *user_text, int i,
                      const nation_set *nations_to_show)
{
  struct advance *vap = valid_advance_by_number(i);
  struct universal source = {.value = {.advance = vap}, .kind = VUT_ADVANCE};
  int flagid;

  fc_assert_ret(nullptr != buf && 0 < bufsz && nullptr != user_text);
  fc_strlcpy(buf, user_text, bufsz);

  if (nullptr == vap) {
    qCritical("Unknown tech %d.", i);
    return;
  }

  if (game.control.num_tech_classes > 0) {
    if (vap->tclass == nullptr) {
      cat_snprintf(buf, bufsz, _("Belongs to the default tech class.\n\n"));
    } else {
      cat_snprintf(buf, bufsz, _("Belongs to tech class %s.\n\n"),
                   tech_class_name_translation(vap->tclass));
    }
  }

  if (nullptr != pplayer) {
    const struct research *presearch = research_get(pplayer);

    if (research_invention_state(presearch, i) != TECH_KNOWN) {
      if (research_invention_state(presearch, i) == TECH_PREREQS_KNOWN) {
        int bulbs = research_total_bulbs_required(presearch, i, false);

        cat_snprintf(buf, bufsz,
                     PL_("Starting now, researching %s would need %d bulb.",
                         "Starting now, researching %s would need %d bulbs.",
                         bulbs),
                     advance_name_translation(vap), bulbs);
      } else if (research_invention_reachable(presearch, i)) {
        /* Split string into two to allow localization of two pluralizations.
         */
        char buf2[MAX_LEN_MSG];
        int bulbs = research_goal_bulbs_required(presearch, i);

        fc_snprintf(
            buf2, ARRAY_SIZE(buf2),
            /* TRANS: appended to another sentence. Preserve the
             * leading space. */
            PL_(" The whole project will require %d bulb to complete.",
                " The whole project will require %d bulbs to complete.",
                bulbs),
            bulbs);
        cat_snprintf(buf, bufsz,
                     // TRANS: last %s is a sentence pluralized separately.
                     PL_("To research %s you need to research %d other"
                         " technology first.%s",
                         "To research %s you need to research %d other"
                         " technologies first.%s",
                         research_goal_unknown_techs(presearch, i) - 1),
                     advance_name_translation(vap),
                     research_goal_unknown_techs(presearch, i) - 1, buf2);
      } else {
        CATLSTR(buf, bufsz, _("You cannot research this technology."));
      }
      if (!techs_have_fixed_costs()
          && research_invention_reachable(presearch, i)) {
        CATLSTR(buf, bufsz,
                // TRANS: preserve leading space
                _(" This number may vary depending on what "
                  "other players research.\n"));
      } else {
        CATLSTR(buf, bufsz, "\n");
      }
    }

    CATLSTR(buf, bufsz, "\n");
  }

  if (requirement_vector_size(&vap->research_reqs) > 0) {
    CATLSTR(buf, bufsz, _("Requirements to research:\n"));
    requirement_vector_iterate(&vap->research_reqs, preq)
    {
      (void) req_text_insert_nl(buf, bufsz, pplayer, preq, VERB_DEFAULT, "");
    }
    requirement_vector_iterate_end;
    CATLSTR(buf, bufsz, "\n");
  }

  insert_allows(&source, buf + qstrlen(buf), bufsz - qstrlen(buf),
                // TRANS: bullet point; note trailing space
                Q_("?bullet:* "));

  {
    int j;

    for (j = 0; j < MAX_NUM_TECH_LIST; j++) {
      if (game.rgame.global_init_techs[j] == A_LAST) {
        break;
      } else if (game.rgame.global_init_techs[j] == i) {
        CATLSTR(buf, bufsz,
                _("* All players start the game with knowledge of this "
                  "technology.\n"));
        break;
      }
    }
  }

  /* Assume no-one will set the same tech in both global and nation
   * init_tech... */
  for (const auto &pnation : nations) {
    // Avoid mentioning nations not in current set.
    if (nations_to_show && !nation_is_in_set(&pnation, nations_to_show)) {
      continue;
    }
    for (int j = 0; j < MAX_NUM_TECH_LIST; j++) {
      if (pnation.init_techs[j] == A_LAST) {
        break;
      } else if (pnation.init_techs[j] == i) {
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a nation plural
                     _("* The %s start the game with knowledge of this "
                       "technology.\n"),
                     nation_plural_translation(&pnation));
        break;
      }
    }
  } // iterate over nations - pnation

  // Explain the effects of root_reqs.
  {
    bv_techs roots, rootsofroots;

    BV_CLR_ALL(roots);
    BV_CLR_ALL(rootsofroots);
    advance_root_req_iterate(vap, proot)
    {
      if (proot == vap) {
        /* Don't say anything at all if this tech is a self-root-req one;
         * assume that the ruleset help will explain how to get it. */
        BV_CLR_ALL(roots);
        break;
      }
      BV_SET(roots, advance_number(proot));
      if (advance_requires(proot, AR_ROOT) != proot) {
        /* Now find out what roots each of this tech's root_req has, so that
         * we can suppress them. If tech A has roots B/C, and B has root C,
         * it's not worth saying that A needs C, and can lead to overwhelming
         * lists. */
        /* (Special case: don't do this if the root is a self-root-req tech,
         * since it would appear in its own root iteration; in the scenario
         * where S is a self-root tech that is root for T, this would prevent
         * S appearing in T's help.) */
        // FIXME this is quite inefficient
        advance_root_req_iterate(proot, prootroot)
        {
          BV_SET(rootsofroots, advance_number(prootroot));
        }
        advance_root_req_iterate_end;
      }
    }
    advance_root_req_iterate_end;

    // Filter out all but the direct root reqs.
    BV_CLR_ALL_FROM(roots, rootsofroots);

    if (BV_ISSET_ANY(roots)) {
      QVector<QString> root_techs;
      root_techs.reserve(A_LAST);
      size_t n_roots = 0;

      advance_index_iterate(A_FIRST, root)
      {
        if (BV_ISSET(roots, root)) {
          root_techs.append(
              advance_name_translation(advance_by_number(root)));
        }
      }
      advance_index_iterate_end;
      fc_assert(n_roots > 0);
      cat_snprintf(buf, bufsz,
                   // TRANS: 'and'-separated list of techs
                   _("* Only those who know %s can acquire this "
                     "technology (by any means).\n"),
                   qUtf8Printable(strvec_to_and_list(root_techs)));
    }
  }

  if (advance_has_flag(i, TF_BONUS_TECH)) {
    cat_snprintf(buf, bufsz,
                 _("* The first player to learn %s gets"
                   " an immediate advance.\n"),
                 advance_name_translation(vap));
  }

  for (flagid = TECH_USER_1; flagid <= TECH_USER_LAST; flagid++) {
    if (advance_has_flag(i, static_cast<tech_flag_id>(flagid))) {
      const char *helptxt =
          tech_flag_helptxt(static_cast<tech_flag_id>(flagid));

      if (helptxt != nullptr) {
        // TRANS: bullet point; note trailing space
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }

  /* FIXME: bases -- but there is no good way to find out which bases a tech
   * can enable currently, so we have to remain silent. */

  if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
    CATLSTR(buf, bufsz,
            _("* To preserve this technology for our nation some bulbs "
              "are needed each turn.\n"));
  }

  if (nullptr != vap->helptext) {
    if (strlen(buf) > 0) {
      CATLSTR(buf, bufsz, "\n");
    }
    for (const auto &text : qAsConst(*vap->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }
}

/**
   Append text for terrain.
 */
void helptext_terrain(char *buf, size_t bufsz, struct player *pplayer,
                      const char *user_text, struct terrain *pterrain)
{
  Q_UNUSED(pplayer)
  struct universal source = {.value = {.terrain = pterrain},
                             .kind = VUT_TERRAIN};
  int flagid;

  fc_assert_ret(nullptr != buf && 0 < bufsz);
  buf[0] = '\0';

  if (!pterrain) {
    qCritical("Unknown terrain!");
    return;
  }

  insert_allows(&source, buf + qstrlen(buf), bufsz - qstrlen(buf),
                // TRANS: bullet point; note trailing space
                Q_("?bullet:* "));
  if (terrain_has_flag(pterrain, TER_NO_CITIES)) {
    CATLSTR(buf, bufsz, _("* You cannot build cities on this terrain."));
    CATLSTR(buf, bufsz, "\n");
  }
  if (pterrain->road_time == 0) {
    // Can't build roads; only mention if ruleset has buildable roads
    extra_type_by_cause_iterate(EC_ROAD, pextra)
    {
      if (pextra->buildable) {
        CATLSTR(buf, bufsz, _("* Paths cannot be built on this terrain."));
        CATLSTR(buf, bufsz, "\n");
        break;
      }
    }
    extra_type_by_cause_iterate_end;
  }
  if (pterrain->base_time == 0) {
    // Can't build bases; only mention if ruleset has buildable bases
    extra_type_by_cause_iterate(EC_BASE, pextra)
    {
      if (pextra->buildable) {
        CATLSTR(buf, bufsz, _("* Bases cannot be built on this terrain."));
        CATLSTR(buf, bufsz, "\n");
        break;
      }
    }
    extra_type_by_cause_iterate_end;
  }
  if (terrain_has_flag(pterrain, TER_UNSAFE_COAST)
      && terrain_type_terrain_class(pterrain) != TC_OCEAN) {
    CATLSTR(buf, bufsz, _("* The coastline of this terrain is unsafe."));
    CATLSTR(buf, bufsz, "\n");
  }
  {
    QVector<QString> classes;
    classes.reserve(uclass_count());

    unit_class_iterate(uclass)
    {
      if (is_native_to_class(uclass, pterrain, nullptr)) {
        classes.append(uclass_name_translation(uclass));
      }
    }
    unit_class_iterate_end;

    if (!classes.isEmpty()) {
      // TRANS: %s is a list of unit classes separated by "and".
      cat_snprintf(buf, bufsz, _("* Can be traveled by %s units.\n"),
                   qUtf8Printable(strvec_to_and_list(classes)));
    }
  }
  if (terrain_has_flag(pterrain, TER_NO_ZOC)) {
    CATLSTR(buf, bufsz,
            _("* Units on this terrain neither impose zones of control "
              "nor are restricted by them.\n"));
  } else {
    CATLSTR(buf, bufsz,
            _("* Units on this terrain may impose a zone of control, or "
              "be restricted by one.\n"));
  }
  for (flagid = TER_USER_1; flagid <= TER_USER_LAST; flagid++) {
    if (terrain_has_flag(pterrain, flagid)) {
      const char *helptxt =
          terrain_flag_helptxt(static_cast<terrain_flag_id>(flagid));

      if (helptxt != nullptr) {
        // TRANS: bullet point; note trailing space
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }

  if (nullptr != pterrain->helptext) {
    if (buf[0] != '\0') {
      CATLSTR(buf, bufsz, "\n");
    }
    for (const auto &text : qAsConst(*pterrain->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }
  if (user_text && user_text[0] != '\0') {
    CATLSTR(buf, bufsz, "\n\n");
    CATLSTR(buf, bufsz, user_text);
  }
}

/**
   Return a textual representation of the F/P/T bonus a road provides to a
   terrain if supplied, or the terrain-independent bonus if pterrain ==
   nullptr. e.g. "0/0/+1", "0/+50%/0", or for a complex road "+2/+1+50%/0".
   Returns a pointer to a static string, so caller should not free
   (or nullptr if there is no effect at all).
 */
const char *helptext_road_bonus_str(const struct terrain *pterrain,
                                    const struct road_type *proad)
{
  static char str[64];
  bool has_effect = false;

  str[0] = '\0';
  output_type_iterate(o)
  {
    switch (o) {
    case O_FOOD:
    case O_SHIELD:
    case O_TRADE: {
      int bonus = proad->tile_bonus[o];
      int incr = proad->tile_incr_const[o];

      if (pterrain) {
        incr +=
            proad->tile_incr[o] * pterrain->road_output_incr_pct[o] / 100;
      }
      if (str[0] != '\0') {
        CATLSTR(str, sizeof(str), "/");
      }
      if (incr == 0 && bonus == 0) {
        cat_snprintf(str, sizeof(str), "%d", incr);
      } else {
        has_effect = true;
        if (incr != 0) {
          cat_snprintf(str, sizeof(str), "%+d", incr);
        }
        if (bonus != 0) {
          cat_snprintf(str, sizeof(str), "%+d%%", bonus);
        }
      }
    } break;
    default:
      /* FIXME: there's nothing actually stopping roads having gold, etc
       * bonuses */
      fc_assert(proad->tile_incr_const[o] == 0 && proad->tile_incr[o] == 0
                && proad->tile_bonus[o] == 0);
      break;
    }
  }
  output_type_iterate_end;

  return has_effect ? str : nullptr;
}

/**
   Calculate any fixed food/prod/trade bonus that 'pextra' will always add
   to terrain type, independent of any other modifications. Does not
   consider percentage bonuses.
   Result written into 'bonus' which should hold 3 ints (F/P/T).
 */
static void extra_bonus_for_terrain(struct extra_type *pextra,
                                    struct terrain *pterrain, int *bonus)
{
  struct universal req_pattern[] = {
      {.value = {.extra = pextra}, .kind = VUT_EXTRA},
      {.value = {.terrain = pterrain}, .kind = VUT_TERRAIN},
      {.kind = VUT_OTYPE /* value filled in later */}};

  fc_assert_ret(bonus != nullptr);

  // Irrigation-like food bonuses
  bonus[0] = (pterrain->irrigation_food_incr
              * effect_value_from_universals(EFT_IRRIGATION_PCT, req_pattern,
                                             2 /* just extra+terrain */))
             / 100;

  // Mining-like shield bonuses
  bonus[1] = (pterrain->mining_shield_incr
              * effect_value_from_universals(EFT_MINING_PCT, req_pattern,
                                             2 /* just extra+terrain */))
             / 100;

  bonus[2] = 0; // no trade bonuses so far

  // Now add fixed bonuses from roads (but not percentage bonus)
  if (extra_road_get(pextra)) {
    const struct road_type *proad = extra_road_get(pextra);

    output_type_iterate(o)
    {
      switch (o) {
      case O_FOOD:
      case O_SHIELD:
      case O_TRADE:
        bonus[o] +=
            proad->tile_incr_const[o]
            + proad->tile_incr[o] * pterrain->road_output_incr_pct[o] / 100;
        break;
      default:
        // not dealing with other output types here
        break;
      }
    }
    output_type_iterate_end;
  }

  // Fixed bonuses for extra, possibly unrelated to terrain type

  output_type_iterate(o)
  {
    // Fill in rest of requirement template
    req_pattern[2].value.outputtype = static_cast<Output_type_id>(o);
    switch (o) {
    case O_FOOD:
    case O_SHIELD:
    case O_TRADE:
      bonus[o] += effect_value_from_universals(
          EFT_OUTPUT_ADD_TILE, req_pattern, ARRAY_SIZE(req_pattern));
      /* Any of the above bonuses is sufficient to trigger
       * Output_Inc_Tile, if underlying terrain does not */
      if (bonus[o] > 0 || pterrain->output[o] > 0) {
        bonus[o] += effect_value_from_universals(
            EFT_OUTPUT_INC_TILE, req_pattern, ARRAY_SIZE(req_pattern));
      }
      break;
    default:
      break;
    }
  }
  output_type_iterate_end;
}

/**
   Return a brief description specific to the extra and terrain, when
   extra is built by cause 'act'.
   Returns number of turns to build, and selected bonuses.
   Returns a pointer to a static string, so caller should not free.
 */
const char *helptext_extra_for_terrain_str(struct extra_type *pextra,
                                           struct terrain *pterrain,
                                           enum unit_activity act)
{
  static char buffer[256];
  int btime;
  int bonus[3];

  btime = terrain_extra_build_time(pterrain, act, pextra);
  fc_snprintf(buffer, sizeof(buffer), PL_("%d turn", "%d turns", btime),
              btime);
  extra_bonus_for_terrain(pextra, pterrain, bonus);
  if (bonus[0] > 0) {
    cat_snprintf(buffer, sizeof(buffer),
                 PL_(", +%d food", ", +%d food", bonus[0]), bonus[0]);
  }
  if (bonus[1] > 0) {
    cat_snprintf(buffer, sizeof(buffer),
                 PL_(", +%d shield", ", +%d shields", bonus[1]), bonus[1]);
  }
  if (bonus[2] > 0) {
    cat_snprintf(buffer, sizeof(buffer),
                 PL_(", +%d trade", ", +%d trade", bonus[2]), bonus[2]);
  }

  return buffer;
}

/**
   Append misc dynamic text for extras.
   Assumes build time and conflicts are handled in the GUI front-end.

   pplayer may be nullptr.
 */
void helptext_extra(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, struct extra_type *pextra)
{
  size_t group_start;
  struct base_type *pbase;
  struct road_type *proad;
  struct universal source = {.value = {.extra = pextra}, .kind = VUT_EXTRA};

  int flagid;

  fc_assert_ret(nullptr != buf && 0 < bufsz);
  buf[0] = '\0';

  if (!pextra) {
    qCritical("Unknown extra!");
    return;
  }

  if (is_extra_caused_by(pextra, EC_BASE)) {
    pbase = pextra->data.base;
  } else {
    pbase = nullptr;
  }

  if (is_extra_caused_by(pextra, EC_ROAD)) {
    proad = pextra->data.road;
  } else {
    proad = nullptr;
  }

  if (pextra->helptext != nullptr) {
    for (const auto &text : qAsConst(*pextra->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }

  // Describe how extra is created and destroyed

  group_start = qstrlen(buf);

  if (pextra->buildable) {
    if (is_extra_caused_by(pextra, EC_IRRIGATION)) {
      CATLSTR(buf, bufsz, _("Build by issuing an \"irrigate\" order.\n"));
    }
    if (is_extra_caused_by(pextra, EC_MINE)) {
      CATLSTR(buf, bufsz, _("Build by issuing a \"mine\" order.\n"));
    }
    if (is_extra_caused_by(pextra, EC_ROAD)) {
      CATLSTR(buf, bufsz, _("Build by issuing a \"road\" order.\n"));
    }
    if (is_extra_caused_by(pextra, EC_BASE)) {
      fc_assert(pbase);
      if (pbase->gui_type == BASE_GUI_OTHER) {
        cat_snprintf(buf, bufsz,
                     _("Build by issuing a \"build base\" order.\n"));
      } else {
        const char *order = "";

        switch (pbase->gui_type) {
        case BASE_GUI_FORTRESS:
          order = Q_(terrain_control.gui_type_base0);
          break;
        case BASE_GUI_AIRBASE:
          order = Q_(terrain_control.gui_type_base1);
          break;
        default:
          fc_assert(false);
          break;
        }
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is a gui_type base string from a ruleset
                     _("Build by issuing a \"%s\" order.\n"), order);
      }
    }
  }

  if (is_extra_caused_by(pextra, EC_POLLUTION)) {
    CATLSTR(buf, bufsz, _("May randomly appear around polluting city.\n"));
  }

  if (is_extra_caused_by(pextra, EC_FALLOUT)) {
    CATLSTR(buf, bufsz, _("May randomly appear around nuclear blast.\n"));
  }

  if (pextra->generated
      && (is_extra_caused_by(pextra, EC_HUT)
          || is_extra_caused_by(pextra, EC_RESOURCE)
          || (proad != nullptr && road_has_flag(proad, RF_RIVER)))) {
    CATLSTR(buf, bufsz, _("This resource is placed by map generator.\n"));
  }

  if (is_extra_removed_by(pextra, ERM_ENTER)) {
    CATLSTR(buf, bufsz, _("Can be explored by certain units.\n"));
  }

  if (is_extra_caused_by(pextra, EC_APPEARANCE)) {
    CATLSTR(buf, bufsz, _("May appear spontaneously.\n"));
  }

  if (requirement_vector_size(&pextra->reqs) > 0) {
    char reqsbuf[8192] = "";
    bool buildable =
        pextra->buildable && is_extra_caused_by_worker_action(pextra);

    requirement_vector_iterate(&pextra->reqs, preq)
    {
      (void) req_text_insert_nl(reqsbuf, sizeof(reqsbuf), pplayer, preq,
                                VERB_DEFAULT,
                                // TRANS: bullet point; note trailing space
                                buildable ? Q_("?bullet:* ") : "");
    }
    requirement_vector_iterate_end;
    if (reqsbuf[0] != '\0') {
      if (buildable) {
        CATLSTR(buf, bufsz, _("Requirements to build:\n"));
      }
      CATLSTR(buf, bufsz, reqsbuf);
    }
  }

  if (buf[group_start] != '\0') {
    CATLSTR(buf, bufsz, "\n"); // group separator
  }

  group_start = qstrlen(buf);

  if (is_extra_removed_by(pextra, ERM_PILLAGE)) {
    int pillage_time = -1;

    if (pextra->removal_time != 0) {
      pillage_time = pextra->removal_time;
    } else {
      terrain_type_iterate(pterrain)
      {
        int terr_pillage_time =
            pterrain->pillage_time * pextra->removal_time_factor;

        if (terr_pillage_time != 0) {
          if (pillage_time < 0) {
            pillage_time = terr_pillage_time;
          } else if (pillage_time != terr_pillage_time) {
            // Give up
            pillage_time = -1;
            break;
          }
        }
      }
      terrain_type_iterate_end;
    }
    if (pillage_time < 0) {
      CATLSTR(buf, bufsz,
              _("Can be pillaged by units (time is terrain-dependent).\n"));
    } else if (pillage_time > 0) {
      cat_snprintf(buf, bufsz,
                   PL_("Can be pillaged by units (takes %d turn).\n",
                       "Can be pillaged by units (takes %d turns).\n",
                       pillage_time),
                   pillage_time);
    }
  }
  if (is_extra_removed_by(pextra, ERM_CLEANPOLLUTION)
      || is_extra_removed_by(pextra, ERM_CLEANFALLOUT)) {
    int clean_time = -1;

    if (pextra->removal_time != 0) {
      clean_time = pextra->removal_time;
    } else {
      terrain_type_iterate(pterrain)
      {
        int terr_clean_time = -1;

        if (is_extra_removed_by(pextra, ERM_CLEANPOLLUTION)
            && pterrain->clean_pollution_time != 0) {
          terr_clean_time =
              pterrain->clean_pollution_time * pextra->removal_time_factor;
        }
        if (is_extra_removed_by(pextra, ERM_CLEANFALLOUT)
            && pterrain->clean_fallout_time != 0) {
          int terr_clean_fall_time =
              pterrain->clean_fallout_time * pextra->removal_time_factor;
          if (terr_clean_time > 0
              && terr_clean_time != terr_clean_fall_time) {
            /* Pollution/fallout cleaning activities taking different time
             * on same terrain. Give up. */
            clean_time = -1;
            break;
          }
          terr_clean_time = terr_clean_fall_time;
        }
        if (clean_time < 0) {
          clean_time = terr_clean_time;
        } else if (clean_time != terr_clean_time) {
          // Give up
          clean_time = -1;
          break;
        }
      }
      terrain_type_iterate_end;
    }
    if (clean_time < 0) {
      CATLSTR(buf, bufsz,
              _("Can be cleaned by units (time is terrain-dependent).\n"));
    } else if (clean_time > 0) {
      cat_snprintf(buf, bufsz,
                   PL_("Can be cleaned by units (takes %d turn).\n",
                       "Can be cleaned by units (takes %d turns).\n",
                       clean_time),
                   clean_time);
    }
  }

  if (requirement_vector_size(&pextra->rmreqs) > 0) {
    char reqsbuf[8192] = "";

    requirement_vector_iterate(&pextra->rmreqs, preq)
    {
      (void) req_text_insert_nl(reqsbuf, sizeof(reqsbuf), pplayer, preq,
                                VERB_DEFAULT, Q_("?bullet:* "));
    }
    requirement_vector_iterate_end;
    if (reqsbuf[0] != '\0') {
      CATLSTR(buf, bufsz, _("Requirements to remove:\n"));
      CATLSTR(buf, bufsz, reqsbuf);
    }
  }

  if (buf[group_start] != '\0') {
    CATLSTR(buf, bufsz, "\n"); // group separator
  }

  // Describe what other elements are enabled by extra

  group_start = qstrlen(buf);

  insert_allows(&source, buf + qstrlen(buf), bufsz - qstrlen(buf), "");

  if (buf[group_start] != '\0') {
    CATLSTR(buf, bufsz, "\n"); // group separator
  }

  // Describe other properties of extras

  if (pextra->visibility_req != A_NONE) {
    char vrbuf[1024];

    fc_snprintf(
        vrbuf, sizeof(vrbuf), _("* Visible only if %s known.\n"),
        advance_name_translation(advance_by_number(pextra->visibility_req)));
    CATLSTR(buf, bufsz, vrbuf);
  }

  if (pextra->eus == EUS_HIDDEN) {
    CATLSTR(buf, bufsz,
            _("* Units inside are hidden from non-allied players.\n"));
  }

  {
    QVector<QString> classes;
    classes.reserve(uclass_count());

    unit_class_iterate(uclass)
    {
      if (is_native_extra_to_uclass(pextra, uclass)) {
        classes.append(uclass_name_translation(uclass));
      }
    }
    unit_class_iterate_end;
    if (!classes.isEmpty()) {
      if (proad != nullptr) {
        // TRANS: %s is a list of unit classes separated by "and".
        cat_snprintf(buf, bufsz, _("* Can be traveled by %s units.\n"),
                     qUtf8Printable(strvec_to_and_list(classes)));
      } else {
        // TRANS: %s is a list of unit classes separated by "and".
        cat_snprintf(buf, bufsz, _("* Native to %s units.\n"),
                     qUtf8Printable(strvec_to_and_list(classes)));
      }

      if (extra_has_flag(pextra, EF_NATIVE_TILE)) {
        CATLSTR(buf, bufsz,
                // TRANS: indented; preserve leading spaces
                _("  * Such units can move onto this tile even if it would "
                  "not normally be suitable terrain.\n"));
      }
      if (pbase != nullptr) {
        if (base_has_flag(pbase, BF_NOT_AGGRESSIVE)) {
          // "3 tiles" is hardcoded in is_friendly_city_near()
          CATLSTR(
              buf, bufsz,
              // TRANS: indented; preserve leading spaces
              _("  * Such units situated here are not considered aggressive "
                "if this tile is within 3 tiles of a friendly city.\n"));
        }
        if (territory_claiming_base(pbase)) {
          CATLSTR(buf, bufsz,
                  // TRANS: indented; preserve leading spaces
                  _("  * Can be captured by such units if at war with the "
                    "nation that currently owns it.\n"));
        }
      }
      if (pextra->defense_bonus) {
        cat_snprintf(buf, bufsz,
                     // TRANS: indented; preserve leading spaces
                     _("  * Such units get a %d%% defense bonus on this "
                       "tile.\n"),
                     pextra->defense_bonus);
      }
    }
  }

  if (proad != nullptr && road_provides_move_bonus(proad)) {
    if (proad->move_cost == 0) {
      CATLSTR(buf, bufsz, _("* Allows infinite movement.\n"));
    } else {
      cat_snprintf(buf, bufsz,
                   /* TRANS: "MP" = movement points. Second %s may have a
                    * fractional part. */
                   _("* Movement cost along %s is %s MP.\n"),
                   extra_name_translation(pextra),
                   move_points_text(proad->move_cost, true));
    }
  }

  if (game.info.killstack && extra_has_flag(pextra, EF_NO_STACK_DEATH)) {
    CATLSTR(buf, bufsz,
            _("* Defeat of one unit does not cause death of all other units "
              "on this tile.\n"));
  }
  if (pbase != nullptr) {
    if (territory_claiming_base(pbase)) {
      CATLSTR(buf, bufsz,
              _("* Extends national borders of the building nation.\n"));
    }
    if (pbase->vision_main_sq >= 0) {
      CATLSTR(buf, bufsz,
              _("* Grants permanent vision of an area around the tile to "
                "its owner.\n"));
    }
    if (pbase->vision_invis_sq >= 0) {
      CATLSTR(buf, bufsz,
              _("* Allows the owner to see normally invisible stealth units "
                "in an area around the tile.\n"));
    }
    if (pbase->vision_subs_sq >= 0) {
      CATLSTR(
          buf, bufsz,
          _("* Allows the owner to see normally invisible subsurface units "
            "in an area around the tile.\n"));
    }
  }
  for (flagid = EF_USER_FLAG_1; flagid <= EF_LAST_USER_FLAG; flagid++) {
    if (extra_has_flag(pextra, static_cast<extra_flag_id>(flagid))) {
      const char *helptxt =
          extra_flag_helptxt(static_cast<extra_flag_id>(flagid));

      if (helptxt != nullptr) {
        // TRANS: bullet point; note trailing space
        CATLSTR(buf, bufsz, Q_("?bullet:* "));
        CATLSTR(buf, bufsz, _(helptxt));
        CATLSTR(buf, bufsz, "\n");
      }
    }
  }

  // Table of terrain-specific attributes, if needed
  if (proad != nullptr || pbase != nullptr) {
    bool road, do_time, do_bonus;

    road = (proad != nullptr);
    // Terrain-dependent build time?
    do_time = pextra->buildable && pextra->build_time == 0;
    if (road) {
      // Terrain-dependent output bonus?
      do_bonus = false;
      output_type_iterate(o)
      {
        if (proad->tile_incr[o] > 0) {
          do_bonus = true;
          fc_assert(o == O_FOOD || o == O_SHIELD || o == O_TRADE);
        }
      }
      output_type_iterate_end;
    } else {
      // Bases don't have output bonuses
      do_bonus = false;
    }

    if (do_time || do_bonus) {
      if (do_time && do_bonus) {
        CATLSTR(
            buf, bufsz,
            _("\nTime to build and output bonus depends on terrain:\n\n"));
        CATLSTR(buf, bufsz,
                /* TRANS: Header for fixed-width road properties table.
                 * TRANS: Translators cannot change column widths :( */
                _("Terrain       Time     Bonus F/P/T\n"
                  "----------------------------------\n"));
      } else if (do_time) {
        CATLSTR(buf, bufsz, _("\nTime to build depends on terrain:\n\n"));
        CATLSTR(buf, bufsz,
                /* TRANS: Header for fixed-width extra properties table.
                 * TRANS: Translators cannot change column widths :( */
                _("Terrain       Time\n"
                  "------------------\n"));
      } else {
        fc_assert(do_bonus);
        CATLSTR(buf, bufsz,
                /* TRANS: Header for fixed-width road properties table.
                 * TRANS: Translators cannot change column widths :( */
                _("\nYields an output bonus with some terrains:\n\n"));
        CATLSTR(buf, bufsz,
                _("Terrain       Bonus F/P/T\n"
                  "-------------------------\n"));
        ;
      }
      terrain_type_iterate(t)
      {
        int turns =
            road ? terrain_extra_build_time(t, ACTIVITY_GEN_ROAD, pextra)
                 : terrain_extra_build_time(t, ACTIVITY_BASE, pextra);
        const char *bonus_text =
            road ? helptext_road_bonus_str(t, proad) : nullptr;
        if (turns > 0 || bonus_text) {
          const char *terrain = terrain_name_translation(t);

          cat_snprintf(
              buf, bufsz, "%s%*s ", terrain,
              MAX(0, 12 - (int) get_internal_string_length(terrain)), "");
          if (do_time) {
            if (turns > 0) {
              cat_snprintf(buf, bufsz, "%3d      ", turns);
            } else {
              CATLSTR(buf, bufsz, "  -      ");
            }
          }
          if (do_bonus) {
            fc_assert(proad != nullptr);
            cat_snprintf(buf, bufsz, " %s", bonus_text ? bonus_text : "-");
          }
          CATLSTR(buf, bufsz, "\n");
        }
      }
      terrain_type_iterate_end;
    } // else rely on client-specific display
  }

  if (user_text && user_text[0] != '\0') {
    CATLSTR(buf, bufsz, "\n\n");
    CATLSTR(buf, bufsz, user_text);
  }
}

/**
   Append misc dynamic text for goods.
   Assumes effects are described in the help text.

   pplayer may be nullptr.
 */
void helptext_goods(char *buf, size_t bufsz, struct player *pplayer,
                    const char *user_text, struct goods_type *pgood)
{
  bool reqs = false;

  fc_assert_ret(nullptr != buf && 0 < bufsz);
  buf[0] = '\0';

  if (nullptr != pgood->helptext) {
    for (const auto &text : qAsConst(*pgood->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }

  if (pgood->onetime_pct == 0) {
    cat_snprintf(
        buf, bufsz,
        _("There's no bonuses paid when traderoute is established.\n\n"));
  } else if (pgood->onetime_pct != 100) {
    cat_snprintf(buf, bufsz,
                 _("When traderoute is established, %d%% of the normal "
                   "bonus is paid.\n"),
                 pgood->onetime_pct);
  }
  cat_snprintf(buf, bufsz,
               _("Sending city enjoys %d%% income from the route.\n"),
               pgood->from_pct);
  cat_snprintf(buf, bufsz,
               _("Receiving city enjoys %d%% income from the route.\n\n"),
               pgood->to_pct);

  // Requirements for this good.
  requirement_vector_iterate(&pgood->reqs, preq)
  {
    if (req_text_insert_nl(buf, bufsz, pplayer, preq, VERB_DEFAULT, "")) {
      reqs = true;
    }
  }
  requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  CATLSTR(buf, bufsz, user_text);
}

/**
   Append misc dynamic text for specialists.
   Assumes effects are described in the help text.

   pplayer may be nullptr.
 */
void helptext_specialist(char *buf, size_t bufsz, struct player *pplayer,
                         const char *user_text, struct specialist *pspec)
{
  bool reqs = false;

  fc_assert_ret(nullptr != buf && 0 < bufsz);
  buf[0] = '\0';

  if (nullptr != pspec->helptext) {
    for (const auto &text : qAsConst(*pspec->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }

  // Requirements for this specialist.
  requirement_vector_iterate(&pspec->reqs, preq)
  {
    if (req_text_insert_nl(buf, bufsz, pplayer, preq, VERB_DEFAULT, "")) {
      reqs = true;
    }
  }
  requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  CATLSTR(buf, bufsz, user_text);
}

/**
   Append text for government.

   pplayer may be nullptr.

   TODO: Generalize the effects code for use elsewhere. Add
   other requirements.
 */
void helptext_government(char *buf, size_t bufsz, struct player *pplayer,
                         const char *user_text, struct government *gov)
{
  bool reqs = false;
  QVector<QString> outputs;
  struct universal source = {.value = {.govern = gov},
                             .kind = VUT_GOVERNMENT};

  fc_assert_ret(nullptr != buf && 0 < bufsz);
  buf[0] = '\0';

  if (nullptr != gov->helptext) {
    for (const auto &text : qAsConst(*gov->helptext)) {
      cat_snprintf(buf, bufsz, "%s\n\n", _(qUtf8Printable(text)));
    }
  }

  // Add requirement text for government itself
  requirement_vector_iterate(&gov->reqs, preq)
  {
    if (req_text_insert_nl(buf, bufsz, pplayer, preq, VERB_DEFAULT, "")) {
      reqs = true;
    }
  }
  requirement_vector_iterate_end;
  if (reqs) {
    fc_strlcat(buf, "\n", bufsz);
  }

  // Effects
  CATLSTR(buf, bufsz, _("Features:\n"));
  insert_allows(&source, buf + qstrlen(buf), bufsz - qstrlen(buf),
                // TRANS: bullet point; note trailing space
                Q_("?bullet:* "));
  effect_list_iterate(get_req_source_effects(&source), peffect)
  {
    Output_type_id output_type = O_LAST;
    struct unit_class *unitclass = nullptr;
    const struct unit_type *unittype = nullptr;
    enum unit_type_flag_id unitflag = unit_type_flag_id_invalid();
    outputs.clear();
    QString or_outputs = Q_("?outputlist: Nothing ");
    QString and_outputs = Q_("?outputlist: Nothing ");
    bool too_complex = false;
    bool world_value_valid = true;

    // Grab output type, if there is one
    requirement_vector_iterate(&peffect->reqs, preq)
    {
      /* Treat an effect with any negated requirements as too complex for
       * us to explain here.
       * Also don't try to explain an effect with any requirements explicitly
       * marked as 'quiet' by ruleset author. */
      if (!preq->present || preq->quiet) {
        too_complex = true;
        continue;
      }
      switch (preq->source.kind) {
      case VUT_OTYPE:
        /* We should never have multiple outputtype requirements
         * in one list in the first place (it simply makes no sense,
         * output cannot be of multiple types)
         * Ruleset loading code should check against that. */
        fc_assert(output_type == O_LAST);
        output_type = preq->source.value.outputtype;
        outputs.append(get_output_name(output_type));
        break;
      case VUT_UCLASS:
        fc_assert(unitclass == nullptr);
        unitclass = preq->source.value.uclass;
        // FIXME: can't easily get world bonus for unit class
        world_value_valid = false;
        break;
      case VUT_UTYPE:
        fc_assert(unittype == nullptr);
        unittype = preq->source.value.utype;
        break;
      case VUT_UTFLAG:
        if (!unit_type_flag_id_is_valid(unitflag)) {
          unitflag =
              static_cast<unit_type_flag_id>(preq->source.value.unitflag);
          // FIXME: can't easily get world bonus for unit type flag
          world_value_valid = false;
        } else {
          /* Already have a unit flag requirement. More than one is too
           * complex for us to explain, so say nothing. */
          // FIXME: we could handle this
          too_complex = true;
        }
        break;
      case VUT_GOVERNMENT:
        /* This is government we are generating helptext for.
         * ...or if not, it's ruleset bug that should never make it
         * this far. Fix ruleset loading code. */
        fc_assert(preq->source.value.govern == gov);
        break;
      default:
        too_complex = true;
        world_value_valid = false;
        break;
      };
    }
    requirement_vector_iterate_end;

    if (!too_complex) {
      /* Only list effects that don't have extra requirements too complex
       * for us to handle.
       * Anything more complicated will have to be documented by hand by the
       * ruleset author. */

      /* Guard condition for simple player-wide effects descriptions.
       * (FIXME: in many cases, e.g. EFT_MAKE_CONTENT, additional
       * requirements like unittype will be ignored for gameplay, but will
       * affect our help here.) */
      const bool playerwide =
          world_value_valid && !unittype && (output_type == O_LAST);
      /* In some cases we give absolute values (world bonus + gov bonus).
       * We assume the fact that there's an effect with a gov requirement
       * is sufficient reason to list it in that gov's help.
       * Guard accesses to these with 'playerwide' or 'world_value_valid'. */
      int world_value = -999, net_value = -999;
      if (world_value_valid) {
        /* Get government-independent world value of effect if the extra
         * requirements were simple enough. */
        struct output_type *potype =
            output_type != O_LAST ? get_output_type(output_type) : nullptr;
        world_value = get_target_bonus_effects(
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            unittype, potype, nullptr, nullptr, peffect->type);
        net_value = peffect->value + world_value;
      }

      if (output_type == O_LAST) {
        /* There was no outputtype requirement. Effect is active for all
         * output types. Generate lists for that. */
        bool harvested_only = true; // Consider only output types from fields

        if (peffect->type == EFT_UPKEEP_FACTOR
            || peffect->type == EFT_UNIT_UPKEEP_FREE_PER_CITY
            || peffect->type == EFT_OUTPUT_BONUS
            || peffect->type == EFT_OUTPUT_BONUS_2) {
          // Effect can use or require any kind of output
          harvested_only = false;
        }

        output_type_iterate(ot)
        {
          struct output_type *pot =
              get_output_type(static_cast<Output_type_id>(ot));

          if (!harvested_only || pot->harvested) {
            outputs.append(_(pot->name));
          }
        }
        output_type_iterate_end;
      }
      if (outputs.count()) {
        or_outputs = strvec_to_or_list(outputs);
        and_outputs = strvec_to_and_list(outputs);
      }

      switch (peffect->type) {
      case EFT_UNHAPPY_FACTOR:
        if (playerwide) {
          /* FIXME: EFT_MAKE_CONTENT_MIL_PER would cancel this out. We assume
           * no-one will set both, so we don't bother handling it. */
          cat_snprintf(
              buf, bufsz,
              PL_("* Military units away from home and field units"
                  " will each cause %d citizen to become unhappy.\n",
                  "* Military units away from home and field units"
                  " will each cause %d citizens to become unhappy.\n",
                  net_value),
              net_value);
        } // else too complicated or silly ruleset
        break;
      case EFT_ENEMY_CITIZEN_UNHAPPY_PCT:
        if (playerwide && net_value != world_value) {
          if (world_value > 0) {
            if (net_value > 0) {
              cat_snprintf(buf, bufsz,
                           _("* Unhappiness from foreign citizens due to "
                             "war with their home state is %d%% the usual "
                             "value.\n"),
                           (net_value * 100) / world_value);
            } else {
              CATLSTR(buf, bufsz,
                      _("* No unhappiness from foreign citizens even when "
                        "at war with their home state.\n"));
            }
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: not pluralised as gettext doesn't support
                          * fractional numbers, which this might be */
                         _("* Each foreign citizen causes %.2g unhappiness "
                           "in their city while you are at war with their "
                           "home state.\n"),
                         static_cast<double>(net_value) / 100);
          }
        }
        break;
      case EFT_MAKE_CONTENT_MIL:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Each of your cities will avoid %d unhappiness"
                           " caused by units.\n",
                           "* Each of your cities will avoid %d unhappiness"
                           " caused by units.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_MAKE_CONTENT:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Each of your cities will avoid %d unhappiness,"
                           " not including that caused by aggression.\n",
                           "* Each of your cities will avoid %d unhappiness,"
                           " not including that caused by aggression.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_FORCE_CONTENT:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Each of your cities will avoid %d unhappiness,"
                           " including that caused by aggression.\n",
                           "* Each of your cities will avoid %d unhappiness,"
                           " including that caused by aggression.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_UPKEEP_FACTOR:
        if (world_value_valid && !unittype) {
          if (net_value == 0) {
            if (output_type != O_LAST) {
              cat_snprintf(buf, bufsz,
                           /* TRANS: %s is the output type, like 'shield'
                            * or 'gold'. */
                           _("* You pay no %s upkeep for your units.\n"),
                           qUtf8Printable(or_outputs));
            } else {
              CATLSTR(buf, bufsz,
                      _("* You pay no upkeep for your units.\n"));
            }
          } else if (net_value != world_value) {
            double ratio = static_cast<double>(net_value) / world_value;
            if (output_type != O_LAST) {
              cat_snprintf(
                  buf, bufsz,
                  /* TRANS: %s is the output type, like 'shield'
                   * or 'gold'. */
                  _("* You pay %.2g times normal %s upkeep for your "
                    "units.\n"),
                  ratio, qUtf8Printable(and_outputs));
            } else {
              cat_snprintf(buf, bufsz,
                           _("* You pay %.2g times normal upkeep for your "
                             "units.\n"),
                           ratio);
            }
          } // else this effect somehow has no effect; keep quiet
        }   // else there was some extra condition making it complicated
        break;
      case EFT_UNIT_UPKEEP_FREE_PER_CITY:
        if (!unittype) {
          if (output_type != O_LAST) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: %s is the output type, like 'shield' or
                          * 'gold'; pluralised in %d but there is currently
                          * no way to control the singular/plural name of the
                          * output type; sorry */
                         PL_("* Each of your cities will avoid paying %d %s"
                             " upkeep for your units.\n",
                             "* Each of your cities will avoid paying %d %s"
                             " upkeep for your units.\n",
                             peffect->value),
                         peffect->value, qUtf8Printable(and_outputs));
          } else {
            cat_snprintf(buf, bufsz,
                         /* TRANS: Amount is subtracted from upkeep cost
                          * for each upkeep type. */
                         PL_("* Each of your cities will avoid paying %d"
                             " upkeep for your units.\n",
                             "* Each of your cities will avoid paying %d"
                             " upkeep for your units.\n",
                             peffect->value),
                         peffect->value);
          }
        } // else too complicated
        break;
      case EFT_CIVIL_WAR_CHANCE:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       _("* If you lose your capital,"
                         " the base chance of civil war is %d%%.\n"),
                       net_value);
        }
        break;
      case EFT_EMPIRE_SIZE_BASE:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* You can have %d city before an "
                           "additional unhappy citizen appears in each city "
                           "due to civilization size.\n",
                           "* You can have up to %d cities before an "
                           "additional unhappy citizen appears in each city "
                           "due to civilization size.\n",
                           net_value),
                       net_value);
        }
        break;
      case EFT_EMPIRE_SIZE_STEP:
        if (playerwide) {
          cat_snprintf(
              buf, bufsz,
              PL_("* After the first unhappy citizen due to"
                  " civilization size, for each %d additional city"
                  " another unhappy citizen will appear.\n",
                  "* After the first unhappy citizen due to"
                  " civilization size, for each %d additional cities"
                  " another unhappy citizen will appear.\n",
                  net_value),
              net_value);
        }
        break;
      case EFT_MAX_RATES:
        if (playerwide && game.info.changeable_budget) {
          if (net_value < 100) {
            cat_snprintf(buf, bufsz,
                         _("* The maximum rate you can set for science,"
                           " gold, or luxuries is %d%%.\n"),
                         net_value);
          } else {
            CATLSTR(buf, bufsz,
                    _("* Has unlimited science/gold/luxuries rates.\n"));
          }
        }
        break;
      case EFT_MARTIAL_LAW_EACH:
        if (playerwide) {
          cat_snprintf(buf, bufsz,
                       PL_("* Your units may impose martial law."
                           " Each military unit inside a city will force %d"
                           " unhappy citizen to become content.\n",
                           "* Your units may impose martial law."
                           " Each military unit inside a city will force %d"
                           " unhappy citizens to become content.\n",
                           peffect->value),
                       peffect->value);
        }
        break;
      case EFT_MARTIAL_LAW_MAX:
        if (playerwide && net_value < 100) {
          cat_snprintf(buf, bufsz,
                       PL_("* A maximum of %d unit in each city can enforce"
                           " martial law.\n",
                           "* A maximum of %d units in each city can enforce"
                           " martial law.\n",
                           net_value),
                       net_value);
        }
        break;
      case EFT_RAPTURE_GROW:
        if (playerwide && net_value > 0) {
          cat_snprintf(buf, bufsz,
                       _("* You may grow your cities by means of "
                         "celebrations."));
          if (game.info.celebratesize > 1) {
            cat_snprintf(
                buf, bufsz,
                /* TRANS: Preserve leading space. %d should always be
                 * 2 or greater. */
                _(" (Cities below size %d cannot grow in this way.)"),
                game.info.celebratesize);
          }
          cat_snprintf(buf, bufsz, "\n");
        }
        break;
      case EFT_REVOLUTION_UNHAPPINESS:
        if (playerwide) {
          cat_snprintf(
              buf, bufsz,
              PL_("* If a city is in disorder for more than %d turn "
                  "in a row, government will fall into anarchy.\n",
                  "* If a city is in disorder for more than %d turns "
                  "in a row, government will fall into anarchy.\n",
                  net_value),
              net_value);
        }
        break;
      case EFT_HAS_SENATE:
        if (playerwide && net_value > 0) {
          CATLSTR(
              buf, bufsz,
              _("* Has a senate that may prevent declaration of war.\n"));
        }
        break;
      case EFT_INSPIRE_PARTISANS:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz,
                  _("* Allows partisans when cities are taken by the "
                    "enemy.\n"));
        }
        break;
      case EFT_HAPPINESS_TO_GOLD:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz,
                  _("* Buildings that normally confer bonuses against"
                    " unhappiness will instead give gold.\n"));
        }
        break;
      case EFT_FANATICS:
        if (playerwide && net_value > 0) {
          QVector<QString> fanatics;

          unit_type_iterate(putype)
          {
            if (utype_has_flag(putype, UTYF_FANATIC)) {
              fanatics.append(utype_name_translation(putype));
            }
          }
          unit_type_iterate_end;
          cat_snprintf(buf, bufsz,
                       // TRANS: %s is list of unit types separated by 'or'
                       _("* Pays no upkeep for %s.\n"),
                       qUtf8Printable(strvec_to_or_list(fanatics)));
        }
        break;
      case EFT_NO_UNHAPPY:
        if (playerwide && net_value > 0) {
          CATLSTR(buf, bufsz, _("* Has no unhappy citizens.\n"));
        }
        break;
      case EFT_VETERAN_BUILD: {
        int conditions = 0;
        if (unitclass) {
          conditions++;
        }
        if (unittype) {
          conditions++;
        }
        if (unit_type_flag_id_is_valid(unitflag)) {
          conditions++;
        }
        if (conditions > 1) {
          /* More than one requirement on units, too complicated for us
           * to describe. */
          break;
        }
        if (unitclass) {
          /* FIXME: account for multiple veteran levels, or negative
           * values. This might lie for complicated rulesets! */
          cat_snprintf(buf, bufsz,
                       // TRANS: %s is a unit class
                       Q_("?unitclass:* New %s units will be veteran.\n"),
                       uclass_name_translation(unitclass));
        } else if (unit_type_flag_id_is_valid(unitflag)) {
          // FIXME: same problems as unitclass
          cat_snprintf(buf, bufsz,
                       // TRANS: %s is a (translatable) unit type flag
                       Q_("?unitflag:* New %s units will be veteran.\n"),
                       unit_type_flag_id_translated_name(unitflag));
        } else if (unittype != nullptr) {
          if (world_value_valid && net_value > 0) {
            /* Here we can be specific about veteran level, and get
             * net value correct. */
            int maxlvl = utype_veteran_system(unittype)->levels - 1;
            const struct veteran_level *vlevel =
                utype_veteran_level(unittype, MIN(net_value, maxlvl));
            cat_snprintf(buf, bufsz,
                         /* TRANS: "* New Partisan units will have the rank
                          * of elite." */
                         Q_("?unittype:* New %s units will have the rank "
                            "of %s.\n"),
                         utype_name_translation(unittype),
                         name_translation_get(&vlevel->name));
          } // else complicated
        } else {
          // No extra criteria.
          // FIXME: same problems as above
          cat_snprintf(buf, bufsz, _("* New units will be veteran.\n"));
        }
      } break;
      case EFT_OUTPUT_PENALTY_TILE:
        if (world_value_valid) {
          cat_snprintf(
              buf, bufsz,
              /* TRANS: %s is list of output types, with 'or';
               * pluralised in %d but of course the output types
               * can't be pluralised; sorry */
              PL_("* Each worked tile that gives more than %d %s will"
                  " suffer a -1 penalty, unless the city working it"
                  " is celebrating.",
                  "* Each worked tile that gives more than %d %s will"
                  " suffer a -1 penalty, unless the city working it"
                  " is celebrating.",
                  net_value),
              net_value, qUtf8Printable(or_outputs));
          if (game.info.celebratesize > 1) {
            cat_snprintf(buf, bufsz,
                         /* TRANS: Preserve leading space. %d should always
                          * be 2 or greater. */
                         _(" (Cities below size %d will not celebrate.)"),
                         game.info.celebratesize);
          }
          cat_snprintf(buf, bufsz, "\n");
        }
        break;
      case EFT_OUTPUT_INC_TILE_CELEBRATE:
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is list of output types, with 'or'
                     PL_("* Each worked tile with at least 1 %s will yield"
                         " %d more of it while the city working it is"
                         " celebrating.",
                         "* Each worked tile with at least 1 %s will yield"
                         " %d more of it while the city working it is"
                         " celebrating.",
                         peffect->value),
                     qUtf8Printable(or_outputs), peffect->value);
        if (game.info.celebratesize > 1) {
          cat_snprintf(buf, bufsz,
                       /* TRANS: Preserve leading space. %d should always be
                        * 2 or greater. */
                       _(" (Cities below size %d will not celebrate.)"),
                       game.info.celebratesize);
        }
        cat_snprintf(buf, bufsz, "\n");
        break;
      case EFT_OUTPUT_INC_TILE:
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is list of output types, with 'or'
                     PL_("* Each worked tile with at least 1 %s will yield"
                         " %d more of it.\n",
                         "* Each worked tile with at least 1 %s will yield"
                         " %d more of it.\n",
                         peffect->value),
                     qUtf8Printable(or_outputs), peffect->value);
        break;
      case EFT_OUTPUT_BONUS:
      case EFT_OUTPUT_BONUS_2:
        // FIXME: makes most sense iff world_value == 0
        cat_snprintf(buf, bufsz,
                     // TRANS: %s is list of output types, with 'and'
                     _("* %s production is increased %d%%.\n"),
                     qUtf8Printable(and_outputs), peffect->value);
        break;
      case EFT_OUTPUT_WASTE:
        if (world_value_valid) {
          if (net_value > 30) {
            cat_snprintf(buf, bufsz,
                         // TRANS: %s is list of output types, with 'and'
                         _("* %s production will suffer massive losses.\n"),
                         qUtf8Printable(and_outputs));
          } else if (net_value >= 15) {
            cat_snprintf(buf, bufsz,
                         // TRANS: %s is list of output types, with 'and'
                         _("* %s production will suffer some losses.\n"),
                         qUtf8Printable(and_outputs));
          } else if (net_value > 0) {
            cat_snprintf(buf, bufsz,
                         // TRANS: %s is list of output types, with 'and'
                         _("* %s production will suffer a small amount "
                           "of losses.\n"),
                         qUtf8Printable(and_outputs));
          }
        }
        break;
      case EFT_HEALTH_PCT:
        if (playerwide) {
          if (peffect->value > 0) {
            CATLSTR(buf, bufsz,
                    _("* Increases the chance of plague"
                      " within your cities.\n"));
          } else if (peffect->value < 0) {
            CATLSTR(buf, bufsz,
                    _("* Decreases the chance of plague"
                      " within your cities.\n"));
          }
        }
        break;
      case EFT_OUTPUT_WASTE_BY_REL_DISTANCE:
        /* Semi-arbitrary scaling to get likely ruleset values in roughly
         * the same range as WASTE_BY_DISTANCE */
        // FIXME: use different wording?
        net_value = (net_value + 39) / 40; // round up
        fc__fallthrough;                   // fall through to:
      case EFT_OUTPUT_WASTE_BY_DISTANCE:
        if (world_value_valid) {
          if (net_value >= 300) {
            cat_snprintf(buf, bufsz,
                         // TRANS: %s is list of output types, with 'and'
                         _("* %s losses will increase quickly"
                           " with distance from capital.\n"),
                         qUtf8Printable(and_outputs));
          } else if (net_value >= 200) {
            cat_snprintf(buf, bufsz,
                         // TRANS: %s is list of output types, with 'and'
                         _("* %s losses will increase"
                           " with distance from capital.\n"),
                         qUtf8Printable(and_outputs));
          } else if (net_value > 0) {
            cat_snprintf(buf, bufsz,
                         // TRANS: %s is list of output types, with 'and'
                         _("* %s losses will increase slowly"
                           " with distance from capital.\n"),
                         qUtf8Printable(and_outputs));
          }
        }
        break;
      case EFT_MIGRATION_PCT:
        if (playerwide) {
          if (peffect->value > 0) {
            CATLSTR(buf, bufsz,
                    _("* Increases the chance of migration"
                      " into your cities.\n"));
          } else if (peffect->value < 0) {
            CATLSTR(buf, bufsz,
                    _("* Decreases the chance of migration"
                      " into your cities.\n"));
          }
        }
        break;
      case EFT_BORDER_VISION:
        if (game.info.borders == BORDERS_ENABLED && playerwide
            && net_value > 0) {
          CATLSTR(buf, bufsz,
                  _("* All tiles inside your borders are"
                    " monitored.\n"));
        }
        break;
      default:
        break;
      };
    }
  }
  effect_list_iterate_end;

  // Action immunity
  action_iterate(act)
  {
    if (action_by_number(act)->quiet) {
      // The ruleset documents this action it self.
      continue;
    }

    if (action_immune_government(gov, act)) {
      cat_snprintf(
          buf, bufsz,
          /* TRANS: action name ... action target
           * ("individual units", etc) */
          _("* Makes it impossible to do the action \'%s\'"
            " to your %s.\n"),
          qUtf8Printable(action_id_name_translation(act)),
          _(action_target_kind_name(action_id_get_target_kind(act))));
    }
  }
  action_iterate_end;

  if (user_text && user_text[0] != '\0') {
    cat_snprintf(buf, bufsz, "\n%s", user_text);
  }
}

/**
   Returns pointer to static string with eg: "1 shield, 1 unhappy"
 */
char *helptext_unit_upkeep_str(const struct unit_type *utype)
{
  static char buf[128];
  char *empty = new char;
  int any = 0;
  empty[0] = '\0';

  if (!utype) {
    qCritical("Unknown unit!");
    return empty;
  }

  buf[0] = '\0';
  output_type_iterate(o)
  {
    if (utype->upkeep[o] > 0) {
      // TRANS: "2 Food" or ", 1 Shield"
      cat_snprintf(buf, sizeof(buf), _("%s%d %s"),
                   (any > 0 ? Q_("?blistmore:, ") : ""), utype->upkeep[o],
                   get_output_name(static_cast<Output_type_id>(o)));
      any++;
    }
  }
  output_type_iterate_end;
  if (utype->happy_cost > 0) {
    // TRANS: "2 Unhappy" or ", 1 Unhappy"
    cat_snprintf(buf, sizeof(buf), _("%s%d Unhappy"),
                 (any > 0 ? Q_("?blistmore:, ") : ""), utype->happy_cost);
    any++;
  }

  if (any == 0) {
    // qstrcpy(buf, _("None"));
    fc_snprintf(buf, sizeof(buf), "%d", 0);
  }
  delete empty;
  return buf;
}

/**
   Returns nation legend and characteristics
 */
void helptext_nation(char *buf, size_t bufsz, struct nation_type *pnation,
                     const char *user_text)
{
  struct universal source = {.value = {.nation = pnation},
                             .kind = VUT_NATION};
  bool print_break = true;

#define PRINT_BREAK()                                                       \
  do {                                                                      \
    if (print_break) {                                                      \
      if (buf[0] != '\0') {                                                 \
        CATLSTR(buf, bufsz, "\n\n");                                        \
      }                                                                     \
      print_break = false;                                                  \
    }                                                                       \
  } while (false)

  fc_assert_ret(nullptr != buf && 0 < bufsz);
  buf[0] = '\0';

  if (pnation->legend[0] != '\0') {
    // Client side legend is stored already translated
    cat_snprintf(buf, bufsz, "%s", pnation->legend);
  }

  if (pnation->init_government) {
    PRINT_BREAK();
    cat_snprintf(buf, bufsz, _("Initial government is %s.\n"),
                 government_name_translation(pnation->init_government));
  }
  if (pnation->init_techs[0] != A_LAST) {
    QVector<QString> tech_names;
    tech_names.reserve(MAX_NUM_TECH_LIST);

    for (int init_tech : pnation->init_techs) {
      if (init_tech == A_LAST) {
        break;
      }
      tech_names.append(
          advance_name_translation(advance_by_number(init_tech)));
    }
    QString list = strvec_to_and_list(tech_names);
    PRINT_BREAK();
    if (game.rgame.global_init_techs[0] != A_LAST) {
      cat_snprintf(
          buf, bufsz,
          // TRANS: %s is an and-separated list of techs
          _("Starts with knowledge of %s in addition to the standard "
            "starting technologies.\n"),
          qUtf8Printable(list));
    } else {
      cat_snprintf(buf, bufsz,
                   // TRANS: %s is an and-separated list of techs
                   _("Starts with knowledge of %s.\n"),
                   qUtf8Printable(list));
    }
  }
  if (pnation->init_units[0]) {
    const struct unit_type *utypes[MAX_NUM_UNIT_LIST];
    int count[MAX_NUM_UNIT_LIST];
    int i, j, n = 0, total = 0;

    // Count how many of each type there is.
    for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
      if (!pnation->init_units[i]) {
        break;
      }
      for (j = 0; j < n; j++) {
        if (pnation->init_units[i] == utypes[j]) {
          count[j]++;
          total++;
          break;
        }
      }
      if (j == n) {
        utypes[n] = pnation->init_units[i];
        count[n] = 1;
        total++;
        n++;
      }
    }
    // Construct the list of unit types and counts.
    QVector<QString> utype_names;
    utype_names.reserve(MAX_NUM_UNIT_LIST);

    for (i = 0; i < n; i++) {
      if (count[i] > 1) {
        /* TRANS: a unit type followed by a count. For instance,
         * "Fighter (2)" means two Fighters. Count is never 1.
         * Used in a list. */
        utype_names.append(QString(_("%1 (%2)"))
                               .arg(utype_name_translation(utypes[i]),
                                    QString::number(count[i])));
      } else {
        utype_names.append(
            QStringLiteral("%s").arg(utype_name_translation(utypes[i])));
      }
    }
    QString list = strvec_to_and_list(utype_names);

    PRINT_BREAK();
    cat_snprintf(buf, bufsz,
                 /* TRANS: %s is an and-separated list of unit types
                  * possibly with counts. Plurality is in total number of
                  * units represented. */
                 PL_("Starts with the following additional unit: %s.\n",
                     "Starts with the following additional units: %s.\n",
                     total),
                 qUtf8Printable(list));
  }
  if (pnation->init_buildings[0] != B_LAST) {
    QVector<QString> impr_names;
    impr_names.reserve(MAX_NUM_BUILDING_LIST);

    for (int init_building : pnation->init_buildings) {
      if (init_building == B_LAST) {
        break;
      }
      impr_names.append(improvement_name_translation(
          improvement_by_number(init_building)));
    }
    QString list = strvec_to_and_list(impr_names);
    PRINT_BREAK();
    if (game.rgame.global_init_buildings[0] != B_LAST) {
      cat_snprintf(buf, bufsz,
                   // TRANS: %s is an and-separated list of improvements
                   _("First city will get %s for free in addition to the "
                     "standard improvements.\n"),
                   qUtf8Printable(list));
    } else {
      cat_snprintf(buf, bufsz,
                   // TRANS: %s is an and-separated list of improvements
                   _("First city will get %s for free.\n"),
                   qUtf8Printable(list));
    }
  }

  if (buf[0] != '\0') {
    CATLSTR(buf, bufsz, "\n");
  }
  insert_allows(&source, buf + qstrlen(buf), bufsz - qstrlen(buf), "");

  if (user_text && user_text[0] != '\0') {
    if (buf[0] != '\0') {
      CATLSTR(buf, bufsz, "\n");
    }
    CATLSTR(buf, bufsz, user_text);
  }
#undef PRINT_BREAK
}
