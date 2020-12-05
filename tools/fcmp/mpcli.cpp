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

#include <stdlib.h>

// Qt
#include <QCoreApplication>
#include <QString>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"

/* common */
#include "version.h"

/* modinst */
#include "download.h"
#include "mpcmdline.h"
#include "mpdb.h"

#include "modinst.h"

struct fcmp_params fcmp = {
    QUrl::fromUserInput(QStringLiteral(MODPACK_LIST_URL)), QLatin1String(),
    QLatin1String()};

/**********************************************************************/ /**
   Progress indications from downloader
 **************************************************************************/
static void msg_callback(const QString &msg)
{
  qInfo("%s", msg.toLocal8Bit().data());
}

/**********************************************************************/ /**
   Build main modpack list view
 **************************************************************************/
static void setup_modpack_list(const char *name, const char *URL,
                               const char *version, const char *license,
                               enum modpack_type type, const char *subtype,
                               const char *notes)
{
  const char *type_str;
  const char *lic_str;
  const char *inst_str;

  if (modpack_type_is_valid(type)) {
    type_str = _(modpack_type_name(type));
  } else {
    /* TRANS: Unknown modpack type */
    type_str = _("?");
  }

  if (license != NULL) {
    lic_str = license;
  } else {
    /* TRANS: License of modpack is not known */
    lic_str = Q_("?license:Unknown");
  }

  inst_str = mpdb_installed_version(name, type);
  if (inst_str == NULL) {
    inst_str = _("Not installed");
  }

  qInfo("%s", "");
  qInfo(_("Name=\"%s\""), name);
  qInfo(_("Version=\"%s\""), version);
  qInfo(_("Installed=\"%s\""), inst_str);
  qInfo(_("Type=\"%s\" / \"%s\""), type_str, subtype);
  qInfo(_("License=\"%s\""), lic_str);
  qInfo(_("URL=\"%s\""), URL);
  if (notes != NULL) {
    qInfo(_("Comment=\"%s\""), notes);
  }
}

/**********************************************************************/ /**
   Entry point of the freeciv-modpack program
 **************************************************************************/
int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(VERSION_STRING);

  // Delegate option parsing to the common function.
  fcmp_parse_cmdline(app);

  fcmp_init();

  const char *rev_ver;

  load_install_info_lists(&fcmp);

  qInfo(_("Freeciv modpack installer (command line version)"));

  qInfo("%s%s", word_version(), VERSION_STRING);

  rev_ver = fc_git_revision();
  if (rev_ver != NULL) {
    qInfo(_("commit: %s"), rev_ver);
  }

  qInfo("%s", "");

  if (fcmp.autoinstall == NULL) {
    download_modpack_list(&fcmp, setup_modpack_list, msg_callback);
  } else {
    const char *errmsg;

    errmsg = download_modpack(qPrintable(fcmp.autoinstall), &fcmp,
                              msg_callback, NULL);

    if (errmsg == NULL) {
      qInfo(_("Modpack installed successfully"));
    } else {
      qCritical(_("Modpack install failed: %s"), errmsg);
    }
  }

  close_mpdbs();

  fcmp_deinit();

  return EXIT_SUCCESS;
}
