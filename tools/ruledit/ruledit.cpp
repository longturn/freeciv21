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

/* ANSI */
#include <stdlib.h>

#include <signal.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

// Qt
#include <QApplication>
#include <QCommandLineParser>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"

/* common */
#include "fc_cmdhelp.h"
#include "fc_interface.h"
#include "version.h"

/* server */
#include "sernet.h"
#include "settings.h"

/* tools/shared */
#include "tools_fc_interface.h"

/* ruledit */
#include "comments.h"
#include "ruledit_qt.h"

#include "ruledit.h"

static void re_parse_cmdline(const QCoreApplication &app);

struct ruledit_arguments reargs;

static int fatal_assertions = -1;

/**********************************************************************/ /**
   Main entry point for freeciv-ruledit
 **************************************************************************/
int main(int argc, char **argv)
{
  QtMsgType loglevel = LOG_NORMAL;
  int ui_options;

  /* Load win32 post-crash debugger */
#ifdef FREECIV_MSWINDOWS
  if (LoadLibrary("exchndl.dll") == NULL) {
#ifdef FREECIV_DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#endif /* FREECIV_DEBUG */
  }
#endif /* FREECIV_MSWINDOWS */

  QApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(VERSION_STRING);

  init_nls();

#ifdef ENABLE_NLS
  (void) bindtextdomain("freeciv-ruledit", get_locale_dir());
#endif

  init_character_encodings(FC_DEFAULT_DATA_ENCODING, FALSE);
#ifdef ENABLE_NLS
  bind_textdomain_codeset("freeciv-ruledit", get_internal_encoding());
#endif

  log_init(NULL, loglevel, NULL, NULL, fatal_assertions);

  /* Initialize command line arguments. */
  re_parse_cmdline(app);

  init_connections();

  settings_init(FALSE);

  /* Reset aifill to zero */
  game.info.aifill = 0;

  game_init(FALSE);
  i_am_tool();

  /* Initialize the fc_interface functions needed to understand rules. */
  fc_interface_init_tool();

  if (comments_load()) {
    auto main = new ruledit_main;
    auto gui = new ruledit_gui(main);

    main->show();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, main,
                     &QObject::deleteLater);

    app.exec();

    comments_free();
  } else {
    /* TRANS: 'Failed to load comments-x.y.txt' where x.y is
     * freeciv version */
    log_error(R__("Failed to load %s."), COMMENTS_FILE_NAME);
  }

  log_close();
  free_libfreeciv();
  free_nls();

  return EXIT_SUCCESS;
}

/**********************************************************************/ /**
   Parse freeciv-ruledit commandline.
 **************************************************************************/
static void re_parse_cmdline(const QCoreApplication &app)
{
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  bool ok = parser.addOptions({
      {{"F", "Fatal"}, _("Raise a signal on failed assertion")},
      {{"r", "ruleset"},
       R__("Ruleset to use as the starting point."),
       // TRANS: Command-line argument
       R__("RULESET")},
  });
  if (!ok) {
    qFatal("Adding command line arguments failed");
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
      fc_fprintf(stderr, R__("Can only edit one ruleset at a time.\n"));
      exit(EXIT_FAILURE);
    } else {
      reargs.ruleset = parser.value("ruleset");
    }
  }
}

/**********************************************************************/ /**
   Show widget if experimental features enabled, hide otherwise
 **************************************************************************/
void show_experimental(QWidget *wdg)
{
#ifdef RULEDIT_EXPERIMENTAL
  wdg->show();
#else
  wdg->hide();
#endif
}
