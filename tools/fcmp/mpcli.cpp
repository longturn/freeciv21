/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2022 Freeciv21 and Freeciv
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

#include <fc_config.h>

#include <cstdlib>

// Qt
#include <QCoreApplication>
#include <QString>

// utility
#include "fcintl.h"

// common
#include "version.h"

// modinst
#include "download.h"
#include "mpcmdline.h"
#include "mpdb.h"

#include "modinst.h"

struct fcmp_params fcmp = {
    QUrl::fromUserInput(QStringLiteral(MODPACK_LIST_URL)), QLatin1String(),
    QLatin1String()};

/**
   Progress indications from downloader
 */
static void msg_callback(const QString &msg)
{
  qInfo("%s", msg.toLocal8Bit().data());
}

/**
   Build main modpack list view
 */
static void setup_modpack_list(const QString &name, const QUrl &url,
                               const QString &version,
                               const QString &license,
                               enum modpack_type type,
                               const QString &subtype, const QString &notes)
{
  // TRANS: Unknown modpack type
  QString type_str =
      modpack_type_is_valid(type) ? (modpack_type_name(type)) : _("?");

  // TRANS: License of modpack is not known
  QString lic_str = license.isEmpty() ? Q_("?license:Unknown") : license;

  const char *tmp = mpdb_installed_version(qUtf8Printable(name), type);
  QString inst_str = tmp ? tmp : _("Not installed");

  qInfo();
  qInfo() << _("Name=") << name;
  qInfo() << _("Version=") << version;
  qInfo() << _("Installed=") << inst_str;
  qInfo() << _("Type=") << type_str << "/" << subtype;
  qInfo() << _("License=") << lic_str;
  qInfo() << _("URL=") << url.toDisplayString();
  if (!notes.isEmpty()) {
    qInfo() << _("Comment=") << notes;
  }
}

/**
   Entry point of the freeciv-modpack program
 */
int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(freeciv21_version());

  // Delegate option parsing to the common function.
  fcmp_parse_cmdline(app);

  fcmp_init();

  load_install_info_lists(&fcmp);

  qInfo(_("Freeciv21 modpack installer (command line version)"));
  qInfo(_("Version %s"), freeciv21_version());

  if (fcmp.autoinstall.isEmpty()) {
    if (auto msg =
            download_modpack_list(&fcmp, setup_modpack_list, msg_callback)) {
      qCritical() << msg;
    }
  } else {
    auto errmsg =
        download_modpack(fcmp.autoinstall, &fcmp, msg_callback, nullptr);

    if (errmsg == nullptr) {
      qInfo(_("Modpack installed successfully"));
    } else {
      qCritical(_("Modpack install failed: %s"), errmsg);
    }
  }

  close_mpdbs();

  fcmp_deinit();

  return EXIT_SUCCESS;
}
