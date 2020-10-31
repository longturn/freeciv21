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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

// Qt
#include <QCommandLineParser>
#include <QCoreApplication>

/* utility */
#include "deprecations.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"
#include "timing.h"

/* common */
#include "capstr.h"
#include "fc_cmdhelp.h"
#include "game.h"
#include "version.h"

/* server */
#include "aiiface.h"
#include "console.h"
#include "meta.h"
#include "sernet.h"
#include "server.h"
#include "srv_main.h"

#define save_and_exit(sig)                                                  \
  if (S_S_RUNNING == server_state()) {                                      \
    save_game_auto(#sig, AS_INTERRUPT);                                     \
  }                                                                         \
  QCoreApplication::exit(EXIT_SUCCESS);

/**********************************************************************/ /**
   This function is called when a SIGINT (ctrl-c) is received.  It will exit
   only if two SIGINTs are received within a second.
 **************************************************************************/
static void signal_handler(int sig)
{
  static struct timer *timer = NULL;

  switch (sig) {
  case SIGINT:
    if (timer && timer_read_seconds(timer) <= 1.0) {
      save_and_exit(SIGINT);
    } else {
      if (game.info.timeout == -1) {
        log_normal(_("Setting timeout to 0. Autogame will stop."));
        game.info.timeout = 0;
      }
      if (!timer) {
        log_normal(_("You must interrupt Freeciv twice "
                     "within one second to make it exit."));
      }
    }
    timer = timer_renew(timer, TIMER_USER, TIMER_ACTIVE);
    timer_start(timer);
    break;

#ifdef SIGHUP
  case SIGHUP:
    save_and_exit(SIGHUP);
    break;
#endif /* SIGHUP */

  case SIGTERM:
    save_and_exit(SIGTERM);
    break;

#ifdef SIGPIPE
  case SIGPIPE:
    if (signal(SIGPIPE, signal_handler) == SIG_ERR) {
      /* Because the signal may have interrupted arbitrary code, we use
       * fprintf() and _exit() here instead of log_*() and exit() so
       * that we don't accidentally call any "unsafe" functions here
       * (see the manual page for the signal function). */
      fprintf(stderr, "\nFailed to reset SIGPIPE handler "
                      "while handling SIGPIPE.\n");
      _exit(EXIT_FAILURE);
    }
    break;
#endif /* SIGPIPE */
  }
}

/**********************************************************************/ /**
  Entry point for Freeciv server.  Basically, does two things:
   1. Parses command-line arguments (possibly dialog, on mac).
   2. Calls the main server-loop routine.
 **************************************************************************/
int main(int argc, char *argv[])
{
  int inx;
  bool showhelp = FALSE;
  bool showvers = FALSE;
  char *option = NULL;

  /* Load win32 post-crash debugger */
#ifdef FREECIV_MSWINDOWS
#ifndef FREECIV_NDEBUG
  if (LoadLibrary("exchndl.dll") == NULL) {
#ifdef FREECIV_DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#endif /* FREECIV_DEBUG */
  }
#endif /* FREECIV_NDEBUG */
#endif /* FREECIV_MSWINDOWS */

  if (SIG_ERR == signal(SIGINT, signal_handler)) {
    fc_fprintf(stderr, _("Failed to install SIGINT handler: %s\n"),
               fc_strerror(fc_get_errno()));
    exit(EXIT_FAILURE);
  }

#ifdef SIGHUP
  if (SIG_ERR == signal(SIGHUP, signal_handler)) {
    fc_fprintf(stderr, _("Failed to install SIGHUP handler: %s\n"),
               fc_strerror(fc_get_errno()));
    exit(EXIT_FAILURE);
  }
#endif /* SIGHUP */

  if (SIG_ERR == signal(SIGTERM, signal_handler)) {
    fc_fprintf(stderr, _("Failed to install SIGTERM handler: %s\n"),
               fc_strerror(fc_get_errno()));
    exit(EXIT_FAILURE);
  }

#ifdef SIGPIPE
  /* Ignore SIGPIPE, the error is handled by the return value
   * of the write call. */
  if (SIG_ERR == signal(SIGPIPE, signal_handler)) {
    fc_fprintf(stderr, _("Failed to ignore SIGPIPE: %s\n"),
               fc_strerror(fc_get_errno()));
    exit(EXIT_FAILURE);
  }
#endif /* SIGPIPE */

  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(VERSION_STRING);

  /* initialize server */
  srv_init();

  /* parse command-line arguments... */
  srvarg.announce = ANNOUNCE_DEFAULT;

  game.server.meta_info.type[0] = '\0';

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  // List of all the supported options
  bool ok = parser.addOptions({
      {{"A", "Announce"},
       // TRANS: Do not translate IPv4, IPv6 and none
       _("Announce game in LAN using protocol PROTO (IPv4/IPv6/none)"),
       // TRANS: Command-line argument
       _("PROTO"),
       "IPv4"},
      {{"B", "Bind-meta"},
       _("Connect to metaserver from this address"),
       // TRANS: Command-line argument
       _("ADDR")},
      {{"b", "bind"},
       _("Listen for clients on ADDR"),
       // TRANS: Command-line argument
       _("ADDR")},
      {{"d", "debug"},
#ifdef FREECIV_DEBUG
       _("Set debug log level (one of f,e,w,n,v,d, or d:file1,min,max:...)"),
#else
       QString::asprintf(_("Set debug log level (%d to %d)"), LOG_FATAL,
                         LOG_VERBOSE),
#endif
       // TRANS: Command-line argument
       _("LEVEL")},
      {{"e", "exit-on-end"},
       _("When a game ends, exit instead of restarting")},
#ifndef FREECIV_NDEBUG
      {{"F", "Fatal"}, _("Raise a signal on failed assertion")},
#endif
      {{"f", "file"},
       _("Load saved game"),
       // TRANS: Command-line argument
       _("FILE")},
      {{"i", "identity"},
       _("Be known as ADDR at metaserver or LAN client"),
       // TRANS: Command-line argument
       _("ADDR")},
      {{"k", "keep"},
       _("Keep updating game information on metaserver even after failure")},
      {{"l", "log"},
       _("Use FILE as logfile"),
       // TRANS: Command-line argument
       _("FILE")},
      {{"M", "Metaserver"},
       _("Set ADDR as metaserver address"),
       // TRANS: Command-line argument
       _("ADDR")},
      {{"m", "meta"}, _("Notify metaserver and send server's info")},
      {{"p", "port"},
       _("Listen for clients on port PORT"),
       // TRANS: Command-line argument
       _("PORT")},
      {{"q", "quitidle"},
       _("Quit if no players for TIME seconds"),
       // TRANS: Command-line argument
       _("TIME")},
      {{"R", "Ranklog"},
       _("Use FILE as ranking logfile"),
       // TRANS: Command-line argument
       _("FILE")},
      {{"r", "read"},
       _("Read startup file FILE"),
       // TRANS: Command-line argument
       _("FILE")},
      {{"S", "Serverid"},
       _("Sets the server id to ID"),
       // TRANS: Command-line argument
       _("ID")},
      {{"s", "saves"},
       _("Sage games to directory DIR"),
       // TRANS: Command-line argument
       _("DIR")},
      {{"t", "timetrack"},
       _("Prints stats about elapsed time on misc tasks")},
      {{"w", "warnings"}, _("Warn about deprecated modpack constructs")},
      {"ruleset", _("Load ruleset RULESET"),
       // TRANS: Command-line argument
       _("RULESET")},
      {"scenarios", _("Save scenarios to directory DIR"),
       // TRANS: Command-line argument
       _("DIR")},
#ifdef HAVE_FCDB
      {{"a", "auth"},
       _("Enable database authentication (requires --Database).")},
      {{"D", "Database"},
       _("Enable database connection with configuration from FILE."),
       // TRANS: Command-line argument
       _("FILE")},
      {{"G", "Guests"}, _("Allow guests to login if auth is enabled.")},
      {{"N", "Newusers"}, _("Allow new users to login if auth is enabled.")},
#endif // HAVE_FCDB
#ifdef AI_MODULES
      {"LoadAI", _("Load ai module MODULE. Can appear multiple times"),
       // TRANS: Command-line argument
       _("MODULE")},
#endif // AI_MODULES
  });
  if (!ok) {
    log_fatal("Adding command line arguments failed");
    exit(EXIT_FAILURE);
  }

  // Parse
  parser.process(app);

  // Process the parsed options
  if (parser.isSet("file")) {
    srvarg.load_filename = parser.value("file");
  }
  if (parser.isSet("log")) {
    srvarg.log_filename = parser.value("log");
  }
  if (parser.isSet("Fatal")) {
    srvarg.fatal_assertions = SIGABRT;
  }
  if (parser.isSet("Ranklog")) {
    srvarg.ranklog_filename = parser.value("Ranklog");
  }
  if (parser.isSet("keep")) {
    srvarg.metaconnection_persistent = true;
    // Implies --meta
    srvarg.metaserver_no_send = false;
  }
  if (parser.isSet("meta")) {
    srvarg.metaserver_no_send = false;
  }
  if (parser.isSet("Metaserver")) {
    srvarg.ranklog_filename = parser.value("Metaserver");
    // Implies --meta
    srvarg.metaserver_no_send = false;
  }
  if (parser.isSet("identity")) {
    srvarg.ranklog_filename = parser.value("identity");
  }
  if (parser.isSet("port")) {
    bool conversion_ok;
    srvarg.port = parser.value("port").toUInt(&conversion_ok);
    if (!conversion_ok) {
      log_fatal(_("Invalid port number %s"),
                qPrintable(parser.value("port")));
      exit(EXIT_FAILURE);
    }
  }
  if (parser.isSet("bind")) {
    srvarg.bind_addr = parser.value("bind");
  }
  if (parser.isSet("Bind-meta")) {
    srvarg.bind_meta_addr = parser.value("Bind-meta");
  }
  if (parser.isSet("read")) {
    srvarg.script_filename = parser.value("read");
  }
  if (parser.isSet("quitidle")) {
    bool conversion_ok;
    srvarg.quitidle = parser.value("quitidle").toUInt(&conversion_ok);
    if (!conversion_ok) {
      log_fatal(_("Invalid number %s"),
                qPrintable(parser.value("quitidle")));
      exit(EXIT_FAILURE);
    }
  }
  if (parser.isSet("exit-on-end")) {
    srvarg.exit_on_end = true;
  }
  if (parser.isSet("timetrack")) {
    srvarg.timetrack = true;
  }
  if (parser.isSet("debug")) {
    if (!log_parse_level_str(parser.value("debug"), &srvarg.loglevel)) {
      exit(EXIT_FAILURE);
    }
  }
#ifdef HAVE_FCDB
  if (parser.isSet("Database")) {
    srvarg.fcdb_enabled = true;
    srvarg.fcdb_conf = parser.value("Database");
  }
  if (parser.isSet("auth")) {
    srvarg.auth_enabled = true;
  }
  if (parser.isSet("Guests")) {
    srvarg.auth_allow_guests = true;
  }
  if (parser.isSet("Newusers")) {
    srvarg.auth_allow_newusers = true;
  }
#endif // HAVE_FCDB
  if (parser.isSet("Serverid")) {
    srvarg.serverid = parser.value("Serverid");
  }
  if (parser.isSet("saves")) {
    srvarg.saves_pathname = parser.value("saves");
  }
  if (parser.isSet("scenarios")) {
    srvarg.scenarios_pathname = parser.value("scenarios_pathname");
  }
  if (parser.isSet("ruleset")) {
    srvarg.ruleset = parser.value("scenarios_pathname");
  }
  if (parser.isSet("Announce")) {
    auto value = parser.value("Announce").toLower();
    if (value == QLatin1String("ipv4")) {
      srvarg.announce = ANNOUNCE_IPV4;
    } else if (value == QLatin1String("ipv6")) {
      srvarg.announce = ANNOUNCE_IPV6;
    } else if (value == QLatin1String("none")) {
      srvarg.announce = ANNOUNCE_NONE;
    } else {
      log_error(_("Illegal value \"%s\" for --Announce"),
                qPrintable(parser.value("Announce")));
    }
  }
  if (parser.isSet("warnings")) {
    deprecation_warnings_enable();
  }
#ifdef AI_MODULES
  if (parser.isSet("LoadAI")) {
    for (const auto &mod : parser.values("LoadAI")) {
      if (!load_ai_module(qUtf8Printable(mod)) {
        fc_fprintf(stderr, _("Failed to load AI module \"%s\"\n"), option);
        exit(EXIT_FAILURE);
      }
    }
  }
#endif

  con_write(C_VERSION, _("This is the server for %s"),
            freeciv_name_version());
  /* TRANS: No full stop after the URL, could cause confusion. */
  con_write(C_COMMENT, _("You can learn a lot about Freeciv at %s"),
            WIKI_URL);

#ifdef HAVE_FCDB
  if (srvarg.auth_enabled && !srvarg.fcdb_enabled) {
    fc_fprintf(stderr, _("Requested authentication with --auth, "
                         "but no --Database given\n"));
    exit(EXIT_FAILURE);
  }
#endif /* HAVE_FCDB */

  /* disallow running as root -- too dangerous */
  dont_run_as_root(argv[0], "freeciv_server");

  init_our_capability();

  /* have arguments, call the main server loop... */
  auto server = new freeciv::server;
  if (!server->is_ready()) {
    delete server;
    exit(EXIT_FAILURE);
  }
  QObject::connect(&app, &QCoreApplication::aboutToQuit, server,
                   &QObject::deleteLater);

  return app.exec();
}
