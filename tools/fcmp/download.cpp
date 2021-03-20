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

/**
   Download modpack from a given URL
 */
const char *download_modpack(const QUrl &url, const struct fcmp_params *fcmp,
                             const dl_msg_callback &mcb,
                             const dl_pb_callback &pbcb,
                             int recursion)
{
  if (recursion > 5) {
    return _("Recursive dependencies too deep");
  }

  if (!url.isValid()) {
    return _("No valid URL given");
  }

  if (!url.fileName().endsWith(QStringLiteral(MODPACKDL_SUFFIX))) {
    return _("This does not look like modpack URL");
  }

  qInfo().noquote() << QString::fromUtf8(_("Installing modpack %1 from %2"))
                           .arg(url.fileName())
                           .arg(url.toString());

  if (fcmp->inst_prefix.isEmpty()) {
    return _("Cannot install to given directory hierarchy");
  }

  if (mcb != NULL) {
    // TRANS: %s is a filename with suffix '.modpack'
    mcb(QString::fromUtf8(_("Downloading \"%1\" control file."))
            .arg(url.fileName()));
  }

  auto json = netfile_get_json_file(url, mcb);
  if (!json.isObject()) {
    return _("Cannot fetch and parse modpack list");
  }

  auto info_value = json["info"];
  if (!info_value.isObject()) {
    // TRANS: Do not translate "info"
    return _("\"info\" is not an object");
  }
  auto info = info_value.toObject();

  if (!info["options"].isString()) {
    // TRANS: Do not translate "info.options"
    return _("\"info.options\" is not a string");
  }

  auto list_capstr = info["options"].toString();

  if (!has_capabilities(MODLIST_CAPSTR, qUtf8Printable(list_capstr))) {
    qCritical() << "Incompatible control file:";
    qCritical() << "  control file options:" << list_capstr;
    qCritical() << "  supported options:" << MODLIST_CAPSTR;

    return _("Modpack control file is incompatible");
  }

  if (!info["name"].isString()) {
    // TRANS: Do not translate "info.name"
    return _("\"info.name\" is not a string");
  }
  auto mpname = info["name"].toString();
  if (mpname.isEmpty()) {
    return _("Modpack name is empty");
  }

  if (!info["version"].isString()) {
    // TRANS: Do not translate "info.version"
    return _("\"info.version\" is not a string");
  }
  auto mpver = info["version"].toString();

  if (!info["type"].isString()) {
    // TRANS: Do not translate "info.type"
    return _("\"info.type\" is not a string");
  }
  auto mptype = info["type"].toString();
  auto type = modpack_type_by_name(qUtf8Printable(mptype), fc_strcasecmp);
  if (!modpack_type_is_valid(type)) {
    return _("Illegal modpack type");
  }

  if (!info["base_url"].isString()) {
    // TRANS: Do not translate "info.type"
    return _("\"info.base_url\" is not a string");
  }
  auto base_url = QUrl::fromUserInput(info["base_url"].toString());
  if (base_url.isRelative()) {
    base_url = url.resolved(base_url);
  }
  // Make sure the url is treated as a directory by resolved()
  base_url.setPath(base_url.path(QUrl::FullyEncoded) + "/");

  /*
   * Fetch dependencies
   */
  auto deps = json["dependencies"];
  if (!deps.isNull()) {
    if (!deps.isArray()) {
      // TRANS: Do not translate "dependencies"
      return _("\"dependencies\" is not an array");
    }

    for (const auto &depref : deps.toArray()) {
      if (!depref.isObject()) {
        // TRANS: Do not translate "dependencies"
        return _("\"dependencies\" contains a non-object");
      }

      // QJsonValueRef doesn't support operator[], convert to a QJsonObject
      auto obj = depref.toObject();

      if (!obj.contains("url") || !obj["url"].isString()) {
        // TRANS: Do not translate "url"
        return _("Dependency has no \"url\" field or it is not a string");
      }
      auto dep_url = obj["url"].toString();

      if (!obj.contains("modpack") || !obj["modpack"].isString()) {
        // TRANS: Do not translate "modpack"
        return _("Dependency has no \"modpack\" field or it is not a string");
      }
      auto dep_name = obj["modpack"].toString();

      if (!obj.contains("type") || !obj["type"].isString()) {
        // TRANS: Do not translate "modpack"
        return _("Dependency has no \"type\" field or it is not a string");
      }
      auto dep_type_str = obj["type"].toString();

      auto dep_type =
          modpack_type_by_name(qUtf8Printable(dep_type_str), fc_strcasecmp);
      if (!modpack_type_is_valid(type)) {
        qCritical() << "Illegal modpack type" << dep_type_str;
        return _("Illegal modpack type");
      }


      if (!obj.contains("version") || !obj["version"].isString()) {
        // TRANS: Do not translate "version"
        return _("Dependency has no \"version\" field or it is not a string");
      }
      auto dep_version = obj["version"].toString();

      // We have everything
      auto inst_ver = mpdb_installed_version(qUtf8Printable(dep_name),
                                             dep_type);
      if (inst_ver != nullptr) {
        if (!cvercmp_max(qUtf8Printable(dep_version), inst_ver)) {
          log_debug() << "Dependency modpack" << dep_name <<  "needed.";

          if (mcb != nullptr) {
            mcb(_("Download dependency modpack"));
          }

          auto dep_qurl = QUrl::fromUserInput(dep_url);
          if (dep_qurl.isRelative()) {
            dep_qurl = url.resolved(dep_qurl);
          }

          auto msg = download_modpack(dep_url, fcmp, mcb, pbcb,
                                      recursion + 1);

          if (msg != nullptr) {
            return msg;
        }
        }
      }
    }
  }

  /*
   * Get the list of files
   */
  struct file_info {
    QString source, destination;
  };
  std::vector<file_info> required_files;

  auto files = json["files"];
  if (!files.isArray()) {
    // TRANS: Do not translate "files"
    return _("\"files\" is not an array");
  }

  for (const auto &fref : files.toArray()) {
    if (fref.isString()) {
      // Option 1: source and destination of the same name
      required_files.push_back({fref.toString(), fref.toString()});
    } else if (fref.isObject()) {
      // Option 2: source and destination separately
      // QJsonValueRef doesn't support operator[], convert to a QJsonObject
      auto obj = fref.toObject();

      file_info info;

      if (!obj.contains("dest") || !obj["dest"].isString()) {
        // TRANS: Do not translate "dest"
        return _("File has no destination (\"dest\")");
      }
      info.destination = obj["dest"].toString();

      if (obj.contains("url") && obj["url"].isString()) {
        info.source = obj["url"].toString();
      } else if (obj.contains("url")) {
        // TRANS: Do not translate "url"
        return _("File url is not a string");
      } else {
        info.source = info.destination;
      }

      required_files.emplace_back(info);
    } else {
      // TRANS: Do not translate "files"
      return _("Unsupported value in \"files\"");
    }
  }

  /*
   * Download files
   */
  // Prevent illegal names. The server will sanitize illegal source paths.
  for (auto info : required_files) {
    if (info.destination.isEmpty()) {
      // Probably a mistake. Don't accept it.
      mcb(_("Empty path"));
      return _("Empty path");
    } else if (info.destination.contains("..")) {
      // Big no, might overwrite system files...
      char buf[2048];

      fc_snprintf(buf, sizeof(buf), _("Illegal path for %s"),
                  qUtf8Printable(info.destination));
      mcb(buf);

      // Trying to trick us. Don't even try to download the rest.
      return _("Illegal file name");
    }
  }

  // Control file already downloaded
  int downloaded = 1;
  if (pbcb != NULL) {
    pbcb(downloaded, required_files.size() + 1);
  }

  // Where to install?
  auto local_dir = fcmp->inst_prefix
                   + ((type == MPT_SCENARIO) ? QStringLiteral("/scenarios/")
                                             : QStringLiteral("/"));

  // Download and install
  bool full_success = true;
  for (auto info : required_files) {
    auto destination = QFileInfo(local_dir + info.destination);

    // Create the destination directory if needed
    log_debug() << "Create directory:" << destination.absolutePath();
    if (!destination.absoluteDir().mkpath(".")) {
      return _("Cannot create required directories");
    }

    if (mcb != NULL) {
      char buf[2048];

      fc_snprintf(buf, sizeof(buf), _("Downloading %s"),
                  qUtf8Printable(info.source));
      mcb(buf);
    }

    // Resolve the URL
    auto source = base_url.resolved(info.source);
    log_debug() << "Download" << source.toDisplayString() << "to"
                << destination.absoluteFilePath();

    if (!netfile_download_file(
            source, qUtf8Printable(destination.absoluteFilePath()), mcb)) {
      if (mcb != NULL) {
        char buf[2048];

        fc_snprintf(buf, sizeof(buf), _("Failed to download %s"),
                    qUtf8Printable(info.source));
        mcb(buf);
      }
      full_success = false;
    }

    if (pbcb != NULL) {
      // Count download of control file also
      downloaded++;
      pbcb(downloaded, required_files.size() + 1);
    }
  }

  if (!full_success) {
    return _("Some parts of the modpack failed to install.");
  }

  mpdb_update_modpack(qUtf8Printable(mpname), type, qUtf8Printable(mpver));

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
