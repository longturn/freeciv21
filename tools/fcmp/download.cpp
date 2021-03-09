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

// Qt
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QUrl>

// dependencies
#include "cvercmp.h"

// utility
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "netfile.h"
#include "registry.h"

// tools
#include "mpdb.h"

#include "download.h"

static const char *download_modpack_recursive(const char *URL,
                                              const struct fcmp_params *fcmp,
                                              const dl_msg_callback &mcb,
                                              const dl_pb_callback &pbcb,
                                              int recursion);

/**
   Download modpack from a given URL
 */
const char *download_modpack(const char *URL, const struct fcmp_params *fcmp,
                             const dl_msg_callback &mcb,
                             const dl_pb_callback &pbcb)
{
  return download_modpack_recursive(URL, fcmp, mcb, pbcb, 0);
}

/**
   Download modpack and its recursive dependencies.
 */
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
    // Nothing
  }

  qInfo(_("Installing modpack %s from %s"), URL + start_idx, URL);

  if (fcmp->inst_prefix == NULL) {
    return _("Cannot install to given directory hierarchy");
  }

  if (mcb != NULL) {
    char buf[2048];

    // TRANS: %s is a filename with suffix '.modpack'
    fc_snprintf(buf, sizeof(buf), _("Downloading \"%s\" control file."),
                URL + start_idx);
    mcb(QString::fromUtf8(buf));
  }

  std::unique_ptr<section_file, void (*)(section_file *)> control(
      netfile_get_section_file(QUrl::fromUserInput(QString::fromUtf8(URL)),
                               mcb),
      // Make sure the control file is destroyed properly
      [](section_file *section) { secfile_destroy(section); });

  if (control == nullptr) {
    return _("Failed to get and parse modpack control file");
  }

  control_capstr = secfile_lookup_str(control.get(), "info.options");
  if (control_capstr == NULL) {
    return _("Modpack control file has no capability string");
  }

  if (!has_capabilities(MODPACK_CAPSTR, control_capstr)) {
    qCritical("Incompatible control file:");
    qCritical("  control file options: %s", control_capstr);
    qCritical("  supported options:    %s", MODPACK_CAPSTR);

    return _("Modpack control file is incompatible");
  }

  mpname = secfile_lookup_str(control.get(), "info.name");
  if (mpname == NULL) {
    return _("Modpack name not defined in control file");
  }
  mpver = secfile_lookup_str(control.get(), "info.version");
  if (mpver == NULL) {
    return _("Modpack version not defined in control file");
  }

  typestr = secfile_lookup_str(control.get(), "info.type");
  type = modpack_type_by_name(typestr, fc_strcasecmp);
  if (!modpack_type_is_valid(type)) {
    return _("Illegal modpack type");
  }

  if (type == MPT_SCENARIO) {
    fc_snprintf(local_dir, sizeof(local_dir), "%s/scenarios",
                qPrintable(fcmp->inst_prefix));
  } else {
    fc_snprintf(local_dir, sizeof(local_dir), "%s/" DATASUBDIR,
                qPrintable(fcmp->inst_prefix));
  }

  baseURLpart = secfile_lookup_str(control.get(), "info.baseURL");

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
        control.get(), NULL, "dependencies.list%d.modpack", dep);
    if (dep_name != NULL) {
      const char *dep_URL;
      const char *inst_ver;
      const char *dep_typestr;
      enum modpack_type dep_type;
      bool needed = true;

      dep_URL = secfile_lookup_str_default(control.get(), NULL,
                                           "dependencies.list%d.URL", dep);

      if (dep_URL == NULL) {
        return _("Dependency has no download URL");
      }

      dep_typestr =
          secfile_lookup_str(control.get(), "dependencies.list%d.type", dep);
      dep_type = modpack_type_by_name(dep_typestr, fc_strcasecmp);
      if (!modpack_type_is_valid(dep_type)) {
        return _("Illegal dependency modpack type");
      }

      inst_ver = mpdb_installed_version(dep_name, type);

      if (inst_ver != NULL) {
        const char *dep_ver;

        dep_ver = secfile_lookup_str_default(
            control.get(), NULL, "dependencies.list%d.version", dep);

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
    src_name = secfile_lookup_str_default(control.get(), NULL,
                                          "files.list%d.src", total_files);

    if (src_name != NULL) {
      total_files++;
    }
  } while (src_name != NULL);

  if (pbcb != NULL) {
    // Control file already downloaded
    pbcb(1, total_files + 1);
  }

  filenbr = 0;
  for (filenbr = 0; filenbr < total_files; filenbr++) {
    const char *dest_name;

    int i;
    bool illegal_filename = false;

    src_name = secfile_lookup_str_default(control.get(), NULL,
                                          "files.list%d.src", filenbr);
    fc_assert_ret_val(src_name != nullptr, nullptr);

    dest_name = secfile_lookup_str_default(control.get(), NULL,
                                           "files.list%d.dest", filenbr);

    if (dest_name == NULL || dest_name[0] == '\0') {
      // Missing dest name is ok, we just default to src_name
      dest_name = src_name;
    }

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
    }

    if (!illegal_filename) {
      fc_snprintf(local_name, sizeof(local_name), "%s/%s", local_dir,
                  dest_name);

      for (i = qstrlen(local_name) - 1; local_name[i] != '/'; i--) {
        // Nothing
      }
      local_name[i] = '\0';
      log_debug("Create directory \"%s\"", local_name);
      if (!make_dir(local_name)) {
        return _("Cannot create required directories");
      }
      local_name[i] = '/';

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
    }

    if (pbcb != NULL) {
      // Count download of control file also
      pbcb(filenbr + 2, total_files + 1);
    }
  }

  if (partial_failure) {
    return _("Some parts of the modpack failed to install.");
  }

  mpdb_update_modpack(mpname, type, mpver);

  return NULL;
}

/**
   Download modpack list
 */
const char *download_modpack_list(const struct fcmp_params *fcmp,
                                  const modpack_list_setup_cb &cb,
                                  const dl_msg_callback &mcb)
{
  auto json = netfile_get_json_file(fcmp->list_url, mcb);
  if (!json.isObject()) {
    return _("Cannot fetch and parse modpack list");
  }

  auto info = json["info"];
  if (!info.isObject()) {
    // TRANS: Do not translate "info"
    return _("\"info\" is not an object");
  }

  if (!info["options"].isString()) {
    // TRANS: Do not translate "info.options"
    return _("\"info.options\" is not a string");
  }

  auto list_capstr = info["options"].toString();

  if (!has_capabilities(MODLIST_CAPSTR, qUtf8Printable(list_capstr))) {
    qCritical() << "Incompatible modpack list file:";
    qCritical() << "  list file options:" << list_capstr;
    qCritical() << "  supported options:" << MODLIST_CAPSTR;

    return _("Modpack list is incompatible");
  }

  if (info["message"].isString()) {
    mcb(info["message"].toString());
  }

  auto modpacks = json["modpacks"];
  if (!modpacks.isArray()) {
    // TRANS: Do not translate "modpacks"
    return _("\"modpacks\" is not an array");
  }

  for (const auto &mpref : modpacks.toArray()) {
    // QJsonValueRef doesn't support operator[], convert to a QJsonObject
    if (!mpref.isObject()) {
      // TRANS: Do not translate "modpacks"
      return _("\"modpacks\" contains a non-object");
    }
    auto mp = mpref.toObject();

    // Modpack name (required)
    if (!mp["name"].isString()) {
      // TRANS: Do not translate "name"
      return _("Modpack \"name\" is missing or is not a string");
    }
    auto name = mp["name"].toString();
    if (name.isEmpty()) {
      return _("Modpack name is empty");
    }

    // Modpack version (required, can be empty)
    if (!mp["version"].isString()) {
      // TRANS: Do not translate "version"
      return _("Modpack \"version\" is missing or is not a string");
    }
    auto version = mp["version"].toString();

    // Modpack license (required, can be empty)
    if (!mp["license"].isString()) {
      // TRANS: Do not translate "license"
      return _("Modpack \"license\" is missing or is not a string");
    }
    auto license = mp["license"].toString();

    // Modpack type (required, validated)
    if (!mp["type"].isString()) {
      // TRANS: Do not translate "type"
      return _("Modpack \"type\" is missing or is not a string");
    }
    auto type_str = mp["type"].toString();

    auto type =
        modpack_type_by_name(qUtf8Printable(type_str), fc_strcasecmp);
    if (!modpack_type_is_valid(type)) {
      qCritical() << "Illegal modpack type" << type_str;
      return _("Illegal modpack type");
    }

    // Modpack subtype (optional, free text)
    if (mp.contains("subtype") && !mp["subtype"].isString()) {
      // TRANS: Do not translate "subtype"
      return _("Modpack \"subtype\" is not a string");
    }
    auto subtype = mp["subtype"].toString(QStringLiteral("-"));

    // Modpack URL (optional, validated)
    if (!mp["url"].isString()) {
      // TRANS: Do not translate "url"
      return _("Modpack \"url\" is missing or is not a string");
    }
    auto url = QUrl::fromUserInput(mp["url"].toString());
    if (!url.isValid()) {
      qCritical() << "Invalid URL" << mp["url"].toString() << ":"
                  << url.errorString();
      return _("Invalid URL");
    }
    auto resolved = url.isRelative() ? fcmp->list_url.resolved(url) : url;

    // Modpack notes (optional)
    if (mp.contains("notes") && !mp["notes"].isString()) {
      // TRANS: Do not translate "notes"
      return _("Modpack \"notes\" is not a string");
    }
    auto notes = mp["notes"].toString(QStringLiteral(""));

    // Call the callback with the modpack info we just parsed
    cb(name, resolved, version, license, type,
       QString::fromUtf8(_(qPrintable(subtype))), notes);
  }

  return nullptr;
}
