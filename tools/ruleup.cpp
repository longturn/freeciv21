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

#include <fc_config.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

// Qt
#include <QCommandLineParser>
#include <QCoreApplication>

// utility
#include "fciconv.h"
#include "version.h"

// common
#include "fc_interface.h"
#include "game.h"

// server
#include "ruleset.h"
#include "sernet.h"
#include "settings.h"

/* tools/shared */
#include "tools_fc_interface.h"

/* tools/ruleutil */
#include "comments.h"
#include "rulesave.h"

static QString rs_selected;
static QString od_selected;

/**
   Parse freeciv-ruleup commandline parameters.
 */
static void rup_parse_cmdline(const QCoreApplication &app)
{
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  bool ok = parser.addOptions({
      {{"F", "Fatal"}, _("Raise a signal on failed assertion")},
      {{"r", "ruleset"},
       _("Update RULESET"),
       // TRANS: Command-line argument
       _("RULESET")},
      {{"o", "output"},
       _("Create directory DIRECTORY for output"),
       // TRANS: Command-line argument
       _("DIRECTORY")},
  });
  if (!ok) {
    qFatal("Adding command line arguments failed");
    exit(EXIT_FAILURE);
  }

  // Parse
  parser.process(app);

  // Process the parsed options
  fc_assert_set_fatal(parser.isSet(QStringLiteral("Fatal")));
  if (parser.isSet(QStringLiteral("ruleset"))) {
    if (parser.values(QStringLiteral("ruleset")).size() > 1) {
      fc_fprintf(stderr, _("Multiple rulesets requested. Only one ruleset "
                           "at time supported.\n"));
      exit(EXIT_FAILURE);
    } else {
      rs_selected = parser.value(QStringLiteral("ruleset"));
    }
  }
  if (parser.isSet(QStringLiteral("output"))) {
    if (parser.values(QStringLiteral("output")).size() > 1) {
      fc_fprintf(stderr, _("Multiple output directories given.\n"));
      exit(EXIT_FAILURE);
    } else {
      od_selected = parser.value(QStringLiteral("output"));
    }
  }
}

/**
   Conversion log callback
 */
static void conv_log(const char *msg) { qInfo("%s", msg); }

/**
   Main entry point for freeciv-ruleup
 */
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(freeciv21_version());

  log_init();

  init_nls();

  init_character_encodings(FC_DEFAULT_DATA_ENCODING, false);

  rup_parse_cmdline(app);

  init_connections();

  settings_init(false);

  game_init(false);
  i_am_tool();

  // Initialize the fc_interface functions needed to understand rules.
  fc_interface_init_tool();

  // Set ruleset user requested to use
  if (rs_selected == nullptr) {
    rs_selected = GAME_DEFAULT_RULESETDIR;
  }
  sz_strlcpy(game.server.rulesetdir, qUtf8Printable(rs_selected));

  // Reset aifill to zero
  game.info.aifill = 0;

  if (load_rulesets(nullptr, nullptr, true, conv_log, false, true, true)) {
    struct rule_data data;
    QString tgt_dir;

    data.nationlist = game.server.ruledit.nationlist;

    if (!od_selected.isEmpty()) {
      tgt_dir = od_selected;
    } else {
      tgt_dir = rs_selected + ".ruleup";
    }

    if (!comments_load()) {
      /* TRANS: 'Failed to load comments-x.y.txt' where x.y is
       * freeciv version */
      qCritical(R__("Failed to load %s."), COMMENTS_FILE_NAME);
    }

    save_ruleset(qUtf8Printable(tgt_dir), game.control.name, &data);
    qInfo("Saved %s", qUtf8Printable(tgt_dir));
    comments_free();
  } else {
    qCritical(_("Can't load ruleset %s"), qUtf8Printable(rs_selected));
  }

  log_close();
  free_libfreeciv();
  free_nls();

  return EXIT_SUCCESS;
}
