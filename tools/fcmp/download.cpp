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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cerrno>
#include <unistd.h>

// Qt
#include <QString>
#include <QUrl>

/* dependencies */
#include "cvercmp.h"

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "netfile.h"
#include "registry.h"

/* tools */
#include "mpdb.h"

#include "download.h"

static const char *download_modpack_recursive(const char *URL,
                                              const struct fcmp_params *fcmp,
                                              const dl_msg_callback &mcb,
                                              const dl_pb_callback &pbcb,
                                              int recursion);

/**********************************************************************/ /**
   Download modpack from a given URL
 **************************************************************************/
const char *download_modpack(const char *URL, const struct fcmp_params *fcmp,
                             const dl_msg_callback &mcb,
                             const dl_pb_callback &pbcb)
{
  return download_modpack_recursive(URL, fcmp, mcb, pbcb, 0);
}

/**********************************************************************/ /**
   Download modpack and its recursive dependencies.
 **************************************************************************/
static const char *download_modpack_recursive(const char *URL,
                                              const struct fcmp_params *fcmp,
                                              const dl_msg_callback &mcb,
                                              const dl_pb_callback &pbcb,
                                              int recursion)
{
  char local_dir[2048];
  char local_name[2048];
  int start_idx;
  int filenbr, total_files;
  struct section_file *control;
  const char *control_capstr;
  const char *baseURLpart;
  enum modpack_type type;
  const char *typestr;
  const char *mpname;
  const char *mpver;
  char baseURL[2048];
  char fileURL[2048];
  const char *src_name;
  bool partial_failure = false;
  int dep;
  const char *dep_name;

  if (recursion > 5) {
    return _("Recursive dependencies too deep");
  }

  if (URL == NULL || URL[0] == '\0') {
    return _("No URL given");
  }

  if (strlen(URL) < qstrlen(MODPACKDL_SUFFIX)
      || strcmp(URL + qstrlen(URL) - qstrlen(MODPACKDL_SUFFIX),
                MODPACKDL_SUFFIX)) {
    return _("This does not look like modpack URL");
  }

  for (start_idx = qstrlen(URL) - qstrlen(MODPACKDL_SUFFIX);
       start_idx > 0 && URL[start_idx - 1] != '/'; start_idx--) {
    /* Nothing */
  }

  qInfo(_("Installing modpack %s from %s"), URL + start_idx, URL);

  if (fcmp->inst_prefix == NULL) {
    return _("Cannot install to given directory hierarchy");
  }

  if (mcb != NULL) {
    char buf[2048];

    /* TRANS: %s is a filename with suffix '.modpack' */
    fc_snprintf(buf, sizeof(buf), _("Downloading \"%s\" control file."),
                URL + start_idx);
    mcb(QString::fromUtf8(buf));
  }

  control = netfile_get_section_file(
      QUrl::fromUserInput(QString::fromUtf8(URL)), mcb);

  if (control == NULL) {
    return _("Failed to get and parse modpack control file");
  }

  control_capstr = secfile_lookup_str(control, "info.options");
  if (control_capstr == NULL) {
    secfile_destroy(control);
    return _("Modpack control file has no capability string");
  }

  if (!has_capabilities(MODPACK_CAPSTR, control_capstr)) {
    qCritical("Incompatible control file:");
    qCritical("  control file options: %s", control_capstr);
    qCritical("  supported options:    %s", MODPACK_CAPSTR);

    secfile_destroy(control);

    return _("Modpack control file is incompatible");
  }

  mpname = secfile_lookup_str(control, "info.name");
  if (mpname == NULL) {
    return _("Modpack name not defined in control file");
  }
  mpver = secfile_lookup_str(control, "info.version");
  if (mpver == NULL) {
    return _("Modpack version not defined in control file");
  }

  typestr = secfile_lookup_str(control, "info.type");
  type = modpack_type_by_name(typestr, fc_strcasecmp);
  if (!modpack_type_is_valid(type)) {
    return _("Illegal modpack type");
  }

  if (type == MPT_SCENARIO) {
    fc_snprintf(local_dir, sizeof(local_dir), "%s%cscenarios",
                qPrintable(fcmp->inst_prefix), DIR_SEPARATOR_CHAR);
  } else {
    fc_snprintf(local_dir, sizeof(local_dir), "%s%c" DATASUBDIR,
                qPrintable(fcmp->inst_prefix), DIR_SEPARATOR_CHAR);
  }

  baseURLpart = secfile_lookup_str(control, "info.baseURL");

  if (baseURLpart[0] == '.') {
    char URLstart[start_idx];

    qstrncpy(URLstart, URL, start_idx - 1);
    URLstart[start_idx - 1] = '\0';
    fc_snprintf(baseURL, sizeof(baseURL), "%s%s", URLstart, baseURLpart + 1);
  } else {
    sz_strlcpy(baseURL, baseURLpart);
  }

  dep = 0;
  do {
    dep_name = secfile_lookup_str_default(
        control, NULL, "dependencies.list%d.modpack", dep);
    if (dep_name != NULL) {
      const char *dep_URL;
      const char *inst_ver;
      const char *dep_typestr;
      enum modpack_type dep_type;
      bool needed = true;

      dep_URL = secfile_lookup_str_default(control, NULL,
                                           "dependencies.list%d.URL", dep);

      if (dep_URL == NULL) {
        secfile_destroy(control);
        return _("Dependency has no download URL");
      }

      dep_typestr =
          secfile_lookup_str(control, "dependencies.list%d.type", dep);
      dep_type = modpack_type_by_name(dep_typestr, fc_strcasecmp);
      if (!modpack_type_is_valid(dep_type)) {
        return _("Illegal dependency modpack type");
      }

      inst_ver = mpdb_installed_version(dep_name, type);

      if (inst_ver != NULL) {
        const char *dep_ver;

        dep_ver = secfile_lookup_str_default(
            control, NULL, "dependencies.list%d.version", dep);

        if (dep_ver != NULL && cvercmp_max(dep_ver, inst_ver)) {
          needed = false;
        }
      }

      if (needed) {
        const char *msg;
        char dep_URL_full[2048];

        log_debug("Dependency modpack \"%s\" needed.", dep_name);

        if (mcb != NULL) {
          mcb(_("Download dependency modpack"));
        }

        if (dep_URL[0] == '.') {
          char URLstart[start_idx];

          qstrncpy(URLstart, URL, start_idx - 1);
          URLstart[start_idx - 1] = '\0';
          fc_snprintf(dep_URL_full, sizeof(dep_URL_full), "%s%s", URLstart,
                      dep_URL + 1);
        } else {
          sz_strlcpy(dep_URL_full, dep_URL);
        }

        msg = download_modpack_recursive(dep_URL_full, fcmp, mcb, pbcb,
                                         recursion + 1);

        if (msg != NULL) {
          return msg;
        }
      }
    }

    dep++;
  } while (dep_name != NULL);

  total_files = 0;
  do {
    src_name = secfile_lookup_str_default(control, NULL, "files.list%d.src",
                                          total_files);

    if (src_name != NULL) {
      total_files++;
    }
  } while (src_name != NULL);

  if (pbcb != NULL) {
    /* Control file already downloaded */
    pbcb(1, total_files + 1);
  }

  filenbr = 0;
  for (filenbr = 0; filenbr < total_files; filenbr++) {
    const char *dest_name;

#ifndef DIR_SEPARATOR_IS_DEFAULT
    char *dest_name_copy;
#else /* DIR_SEPARATOR_IS_DEFAULT */
#define dest_name_copy dest_name
#endif /* DIR_SEPARATOR_IS_DEFAULT */

    int i;
    bool illegal_filename = false;

    src_name = secfile_lookup_str_default(control, NULL, "files.list%d.src",
                                          filenbr);

    dest_name = secfile_lookup_str_default(control, NULL,
                                           "files.list%d.dest", filenbr);

    if (dest_name == NULL || dest_name[0] == '\0') {
      /* Missing dest name is ok, we just default to src_name */
      dest_name = src_name;
    }

#ifndef DIR_SEPARATOR_IS_DEFAULT
    dest_name_copy = new char[strlen(dest_name) + 1];
#endif /* DIR_SEPARATOR_IS_DEFAULT */

    for (i = 0; dest_name[i] != '\0'; i++) {
      if (dest_name[i] == '.' && dest_name[i + 1] == '.') {
        if (mcb != NULL) {
          char buf[2048];

          fc_snprintf(buf, sizeof(buf), _("Illegal path for %s"), dest_name);
          mcb(buf);
        }
        partial_failure = true;
        illegal_filename = true;
      }

#ifndef DIR_SEPARATOR_IS_DEFAULT
      if (dest_name[i] == '/') {
        dest_name_copy[i] = DIR_SEPARATOR_CHAR;
      } else {
        dest_name_copy[i] = dest_name[i];
      }
#endif /* DIR_SEPARATOR_IS_DEFAULT */
    }

#ifndef DIR_SEPARATOR_IS_DEFAULT
    dest_name_copy[i] = '\0';
#endif /* DIR_SEPARATOR_IS_DEFAULT */

    if (!illegal_filename) {
      fc_snprintf(local_name, sizeof(local_name), "%s%c%s", local_dir,
                  DIR_SEPARATOR_CHAR, dest_name_copy);

#ifndef DIR_SEPARATOR_IS_DEFAULT
      free(dest_name_copy);
#endif /* DIR_SEPARATOR_IS_DEFAULT */

      for (i = qstrlen(local_name) - 1; local_name[i] != DIR_SEPARATOR_CHAR;
           i--) {
        /* Nothing */
      }
      local_name[i] = '\0';
      log_debug("Create directory \"%s\"", local_name);
      if (!make_dir(local_name)) {
        secfile_destroy(control);
        return _("Cannot create required directories");
      }
      local_name[i] = DIR_SEPARATOR_CHAR;

      if (mcb != NULL) {
        char buf[2048];

        fc_snprintf(buf, sizeof(buf), _("Downloading %s"), src_name);
        mcb(buf);
      }

      fc_snprintf(fileURL, sizeof(fileURL), "%s/%s", baseURL, src_name);
      log_debug("Download \"%s\" as \"%s\".", fileURL, local_name);
      if (!netfile_download_file(
              QUrl::fromUserInput(QString::fromUtf8(fileURL)), local_name,
              mcb)) {
        if (mcb != NULL) {
          char buf[2048];

          fc_snprintf(buf, sizeof(buf), _("Failed to download %s"),
                      src_name);
          mcb(buf);
        }
        partial_failure = true;
      }
    } else {
#ifndef DIR_SEPARATOR_IS_DEFAULT
      free(dest_name_copy);
#endif /* DIR_SEPARATOR_IS_DEFAULT */
    }

    if (pbcb != NULL) {
      /* Count download of control file also */
      pbcb(filenbr + 2, total_files + 1);
    }
  }

  if (partial_failure) {
    secfile_destroy(control);

    return _("Some parts of the modpack failed to install.");
  }

  mpdb_update_modpack(mpname, type, mpver);

  secfile_destroy(control);

  return NULL;
}

/**********************************************************************/ /**
   Download modpack list
 **************************************************************************/
const char *download_modpack_list(const struct fcmp_params *fcmp,
                                  const modpack_list_setup_cb &cb,
                                  const dl_msg_callback &mcb)
{
  struct section_file *list_file;
  const char *list_capstr;
  int modpack_count;
  const char *msg;
  const char *mp_name;

  list_file = netfile_get_section_file(fcmp->list_url, mcb);

  if (list_file == NULL) {
    return _("Cannot fetch and parse modpack list");
  }

  list_capstr = secfile_lookup_str(list_file, "info.options");
  if (list_capstr == NULL) {
    secfile_destroy(list_file);
    return _("Modpack list has no capability string");
  }

  if (!has_capabilities(MODLIST_CAPSTR, list_capstr)) {
    qCritical("Incompatible modpack list file:");
    qCritical("  list file options: %s", list_capstr);
    qCritical("  supported options: %s", MODLIST_CAPSTR);

    secfile_destroy(list_file);

    return _("Modpack list is incompatible");
  }

  msg = secfile_lookup_str_default(list_file, NULL, "info.message");

  if (msg != NULL) {
    mcb(msg);
  }

  modpack_count = 0;
  do {
    const char *mpURL;
    const char *mpver;
    const char *mplic;
    const char *mp_type_str;
    const char *mp_subtype;
    const char *mp_notes;

    mp_name = secfile_lookup_str_default(
        list_file, NULL, "modpacks.list%d.name", modpack_count);
    mpver = secfile_lookup_str_default(
        list_file, NULL, "modpacks.list%d.version", modpack_count);
    mplic = secfile_lookup_str_default(
        list_file, NULL, "modpacks.list%d.license", modpack_count);
    mp_type_str = secfile_lookup_str_default(
        list_file, NULL, "modpacks.list%d.type", modpack_count);
    mp_subtype = secfile_lookup_str_default(
        list_file, NULL, "modpacks.list%d.subtype", modpack_count);
    mpURL = secfile_lookup_str_default(list_file, NULL,
                                       "modpacks.list%d.URL", modpack_count);
    mp_notes = secfile_lookup_str_default(
        list_file, NULL, "modpacks.list%d.notes", modpack_count);

    if (mp_name != NULL && mpURL != NULL) {
      enum modpack_type type =
          modpack_type_by_name(mp_type_str, fc_strcasecmp);

      if (!modpack_type_is_valid(type)) {
        qCritical("Illegal modpack type \"%s\"",
                  mp_type_str ? mp_type_str : "NULL");
      }
      if (mpver == NULL) {
        mpver = "-";
      }
      if (mp_subtype == NULL) {
        mp_subtype = "-";
      }

      QUrl from_list = QUrl::fromUserInput(mpURL);
      QUrl resolved = from_list.isRelative()
                          ? fcmp->list_url.resolved(from_list)
                          : from_list;

      cb(mp_name, qPrintable(resolved.toString()), mpver, mplic, type,
         _(mp_subtype), mp_notes);
    }
    modpack_count++;
  } while (mp_name != NULL);

  return NULL;
}
