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

// Qt
#include <QCommandLineParser>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "support.h"

/* common */
#include "version.h"

/* modinst */
#include "modinst.h"

#include "mpcmdline.h"

extern struct fcmp_params fcmp;

/**********************************************************************/ /**
   Parse commandline parameters. Modified argv[] so it contains only
   ui-specific options afterwards. Number of those ui-specific options is
   returned.
   Currently this is implemented in a way that it never fails. Either it
   returns with success or exit()s. Implementation can be changed so that
   this returns with value -1 in case program should be shut down instead
   of exiting itself. Callers are prepared for such implementation.
   This call initialises the log system.
 **************************************************************************/
void fcmp_parse_cmdline(const QCoreApplication &app)
{
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  bool ok = parser.addOptions(
      {{{"d", _("debug")},
        // TRANS: Do not translate "fatal", "critical", "warning", "info" or
        //        "debug". It's exactly what the user must type.
        _("Set debug log level (fatal/critical/warning/info/debug)"),
        _("LEVEL"),
        QStringLiteral("info")},
       {{"i", "install"},
        _("Automatically install modpack from a given URL"),
        // TRANS: Command line argument
        _("URL")},
       {{"L", "List"},
        _("Load modpack list from given URL"),
        // TRANS: Command line argument
        _("URL")},
       {{"p", "prefix"},
        _("Install modpacks to given directory hierarchy"),
        // TRANS: Command line argument
        _("DIR")}});
  if (!ok) {
    qFatal("Adding command line arguments failed");
    exit(EXIT_FAILURE);
  }

  // Parse
  parser.process(app);

  // Process the parsed options
  if (log_init(parser.value(QStringLiteral("debug")))) {
    exit(EXIT_FAILURE);
  }
  if (parser.isSet(QStringLiteral("List"))) {
    fcmp.list_url =
        QUrl::fromUserInput(parser.value(QStringLiteral("List")));
  }
  if (parser.isSet(QStringLiteral("prefix"))) {
    fcmp.inst_prefix = parser.value(QStringLiteral("prefix"));
  }
  if (parser.isSet(QStringLiteral("install"))) {
    fcmp.autoinstall = parser.value(QStringLiteral("install"));
  }

  if (fcmp.inst_prefix.isNull()) {
    fcmp.inst_prefix = freeciv_storage_dir();

    if (fcmp.inst_prefix.isNull()) {
      qCritical("Cannot determine freeciv storage directory");
    }
  }
}
