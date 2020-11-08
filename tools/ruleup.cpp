/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <signal.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

// Qt
#include <QCommandLineParser>
#include <QCoreApplication>

/* utility */
#include "fciconv.h"
#include "registry.h"
#include "string_vector.h"

/* common */
#include "fc_cmdhelp.h"
#include "fc_interface.h"

/* server */
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
static int fatal_assertions = -1;

/**********************************************************************/ /**
   Parse freeciv-ruleup commandline parameters.
 **************************************************************************/
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
    log_fatal("Adding command line arguments failed");
    exit(EXIT_FAILURE);
  }

  // Parse
  parser.process(app);

  // Process the parsed options
  if (parser.isSet("Fatal")) {
    fatal_assertions = SIGABRT;
  }
  if (parser.isSet("ruleset")) {
    if (parser.values("ruleset").size() >= 1) {
      fc_fprintf(stderr, _("Multiple rulesets requested. Only one ruleset "
                           "at time supported.\n"));
      exit(EXIT_FAILURE);
    } else {
      rs_selected = parser.value("ruleset");
    }
  }
  if (parser.isSet("output")) {
    if (parser.values("output").size() > 1) {
      fc_fprintf(stderr, _("Multiple output directories given.\n"));
      exit(EXIT_FAILURE);
    } else {
      od_selected = parser.value("output");
    }
  }
}

/**********************************************************************/ /**
   Conversion log callback
 **************************************************************************/
static void conv_log(const char *msg) { log_normal("%s", msg); }

/**********************************************************************/ /**
   Main entry point for freeciv-ruleup
 **************************************************************************/
int main(int argc, char **argv)
{
  enum log_level loglevel = LOG_NORMAL;

  /* Load win32 post-crash debugger */
#ifdef FREECIV_MSWINDOWS
  if (LoadLibrary("exchndl.dll") == NULL) {
#ifdef FREECIV_DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#endif /* FREECIV_DEBUG */
  }
#endif /* FREECIV_MSWINDOWS */

  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(VERSION_STRING);

  init_nls();

  init_character_encodings(FC_DEFAULT_DATA_ENCODING, FALSE);

  rup_parse_cmdline(app);

  log_init(NULL, loglevel, NULL, NULL, fatal_assertions);

  init_connections();

  settings_init(FALSE);

  game_init(FALSE);
  i_am_tool();

  /* Initialize the fc_interface functions needed to understand rules. */
  fc_interface_init_tool();

  /* Set ruleset user requested to use */
  if (rs_selected == NULL) {
    rs_selected = GAME_DEFAULT_RULESETDIR;
  }
  sz_strlcpy(game.server.rulesetdir, qUtf8Printable(rs_selected));

  /* Reset aifill to zero */
  game.info.aifill = 0;

  if (load_rulesets(NULL, NULL, TRUE, conv_log, FALSE, TRUE, TRUE)) {
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
      log_error(R__("Failed to load %s."), COMMENTS_FILE_NAME);
    }

    save_ruleset(qPrintable(tgt_dir), game.control.name, &data);
    log_normal("Saved %s", qPrintable(tgt_dir));
    comments_free();
  } else {
    log_error(_("Can't load ruleset %s"), qPrintable(rs_selected));
  }

  log_close();
  free_libfreeciv();
  free_nls();

  return EXIT_SUCCESS;
}
