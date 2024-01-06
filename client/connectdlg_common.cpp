/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
 */
#include <fc_config.h>

// Qt
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTcpServer>
#include <QUrl>
#include <QUuid>

#include <cstring>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

// utility
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "registry.h"
#include "registry_ini.h"
#include "shared.h"
#include "support.h"

// client
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h" // connect_to_server()
#include "connectdlg_common.h"
#include "packhand_gen.h"

// gui-qt
#include "qtg_cxxside.h"

#define WAIT_BETWEEN_TRIES 100000 // usecs
#define NUMBER_OF_TRIES 500

bool server_quitting = false;

static char challenge_fullname[MAX_LEN_PATH];
static bool client_has_hack = false;

int internal_server_port;

class serverProcess : public QProcess {
  Q_DISABLE_COPY(serverProcess);

public:
  static serverProcess *i();

private:
  void drop();
  serverProcess();
  static serverProcess *m_instance;
};
serverProcess *serverProcess::m_instance = 0;

// Server process constructor
serverProcess::serverProcess()
{
  connect(this,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [=](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode)
            qInfo() << _("Freeciv21 Server") << exitStatus;
            drop();
          });
}
// Server process instance
serverProcess *serverProcess::i()
{
  if (!m_instance) {
    m_instance = new serverProcess;
  }
  return m_instance;
}
// Removes server process
void serverProcess::drop()
{
  if (m_instance) {
    m_instance->deleteLater();
    m_instance = 0;
  }
}

/**
The general chain of events:

Two distinct paths are taken depending on the choice of mode:

if the user selects the multi- player mode, then a packet_req_join_game
packet is sent to the server. It is either successful or not. The end.

If the user selects a single- player mode (either a new game or a save game)
then:
   1. the packet_req_join_game is sent.
   2. on receipt, if we can join, then a challenge packet is sent to the
      server, so we can get hack level control.
   3. if we can't get hack, then we get dumped to multi- player mode. If
      we can, then:
      a. for a new game, we send a series of packet_generic_message packets
         with commands to start the game.
      b. for a saved game, we send the load command with a
         packet_generic_message, then we send a PACKET_PLAYER_LIST_REQUEST.
         the response to this request will tell us if the game was loaded or
         not. if not, then we send another load command. if so, then we send
         a series of packet_generic_message packets with commands to start
         the game.
 */

/**
   Tests if the client has started the server.
 */
bool is_server_running()
{
  if (server_quitting) {
    return false;
  }
  return serverProcess::i()->state() > QProcess::NotRunning;
}

/**
   Returns TRUE if the client has hack access.
 */
bool can_client_access_hack() { return client_has_hack; }

/**
   Kills the server if the client has started it.

   If the 'force' parameter is unset, we just do a /quit.  If it's set, then
   we'll send a signal to the server to kill it (use this when the socket
   is disconnected already).
 */
void client_kill_server(bool force)
{
  if (is_server_running()) {
    if (client.conn.used && client_has_hack) {
      /* This does a "soft" shutdown of the server by sending a /quit.
       *
       * This is useful when closing the client or disconnecting because it
       * doesn't kill the server prematurely.  In particular, killing the
       * server in the middle of a save can have disastrous results.  This
       * method tells the server to quit on its own.  This is safer from a
       * game perspective, but more dangerous because if the kill fails the
       * server will be left running.
       *
       * Another potential problem is because this function is called atexit
       * it could potentially be called when we're connected to an unowned
       * server.  In this case we don't want to kill it. */
      send_chat("/quit");
      server_quitting = true;
    } else if (force) {
      /* Either we already disconnected, or we didn't get control of the
       * server. In either case, the only thing to do is a "hard" kill of
       * the server. */
      serverProcess::i()->kill();
      server_quitting = false;
    }
  }

  client_has_hack = false;
}

/**
   Forks a server if it can. Returns FALSE if we find we
   couldn't start the server.
 */
bool client_start_server(const QString &user_name)
{
  QStringList arguments;
  QString trueFcser, storage, port_buf, savesdir, scensdir;
  char buf[512];
  int connect_tries = 0;

  // only one server (forked from this client) shall be running at a time
  // This also resets client_has_hack.
  client_kill_server(true);

  output_window_append(ftc_client, _("Starting local server..."));

  storage = freeciv_storage_dir();
  if (storage == nullptr) {
    output_window_append(ftc_client,
                         _("Cannot find Freeciv21 storage directory"));
    output_window_append(
        ftc_client, _("You'll have to start server manually. Sorry..."));
    return false;
  }

  // Generate a (random) unique name for the local socket
  auto uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

  // Set up the command-line parameters.
  savesdir = QStringLiteral("%1/saves").arg(storage);
  scensdir = QStringLiteral("%1/scenarios").arg(storage);

  arguments << QStringLiteral("--local") << uuid << QStringLiteral("-q")
            << QStringLiteral("1") << QStringLiteral("-e")
            << QStringLiteral("--saves") << savesdir
            << QStringLiteral("--scenarios") << scensdir
            << QStringLiteral("-A") << QStringLiteral("none");
  if (!logfile.isEmpty()) {
    arguments << QStringLiteral("--debug") << log_get_level()
              << QStringLiteral("--log") << logfile;
  }
  if (scriptfile.isEmpty()) {
    arguments << QStringLiteral("--read") << scriptfile;
  }
  if (savefile.isEmpty()) {
    arguments << QStringLiteral("--file") << savefile;
  }

  // Look for a server binary
  const QString server_name = QStringLiteral("freeciv21-server");

  // First next to the client binary
  // NOTE On Windows findExecutable adds the .exe automatically
  QString location = QStandardPaths::findExecutable(
      server_name, {QCoreApplication::applicationDirPath()});
  if (location.isEmpty()) {
    // Then in PATH
    location = QStandardPaths::findExecutable(server_name);
  }

  // Start it
  qInfo(_("Starting freeciv21-server at %s"), qUtf8Printable(location));

  serverProcess::i()->start(location, arguments);
  if (!serverProcess::i()->waitForStarted(3000)) {
    output_window_append(ftc_client, _("Couldn't start the server."));
    output_window_append(ftc_client,
                         _("We probably couldn't start it from here."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));
    return false;
  }

  // Wait for the server to print its welcome screen
  serverProcess::i()->waitForReadyRead();
  serverProcess::i()->waitForStarted();
  server_quitting = false;

  // Local server URL
  auto url = QUrl();
  url.setScheme(QStringLiteral("fc21+local"));
  url.setUserName(user_name);
  url.setPath(uuid);

  // a reasonable number of tries
  while (connect_to_server(
             url, buf,
             sizeof(buf) && serverProcess::i()->state() == QProcess::Running)
         == -1) {
    fc_usleep(WAIT_BETWEEN_TRIES);
    if (connect_tries++ > NUMBER_OF_TRIES) {
      qCritical("Last error from connect attempts: '%s'", buf);
      break;
    }
  }
  /* weird, but could happen, if server doesn't support new startup stuff
   * capabilities won't help us here... */
  if (!client.conn.used || serverProcess::i()->processId() == 0) {
    // possible that server is still running. kill it, kill it with Igni
    client_kill_server(true);

    qCritical("Failed to connect to spawned server!");
#ifdef FREECIV_DEBUG
    qDebug("Tried with commandline: '%s'",
           QString(trueFcser + arguments.join(QStringLiteral(" ")))
               .toLocal8Bit()
               .data());
#endif
    output_window_append(ftc_client, _("Couldn't connect to the server."));
    output_window_append(ftc_client,
                         _("We probably couldn't start it from here."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));

    return false;
  }

  return true;
}

/**
   Generate a random string.
 */
static void randomize_string(char *str, size_t n)
{
  const char chars[] =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;

  for (i = 0; i < n - 1; i++) {
    str[i] = chars[fc_rand(sizeof(chars) - 1)];
  }
  str[i] = '\0';
}

/**
   If the client is capable of 'wanting hack', then the server will
   send the client a filename in the packet_join_game_reply packet.

   This function creates the file with a suitably random string in it
   and then sends the string to the server. If the server can open
   and read the string, then the client is given hack access.
 */
void send_client_wants_hack(const char *filename)
{
  if (filename[0] != '\0') {
    struct packet_single_want_hack_req req;
    struct section_file *file;
    auto sdir = freeciv_storage_dir();

    if (sdir.isEmpty()) {
      return;
    }

    if (!is_safe_filename(filename)) {
      return;
    }

    QDir().mkpath(sdir);

    fc_snprintf(challenge_fullname, sizeof(challenge_fullname), "%s/%s",
                qUtf8Printable(sdir), filename);

    // generate an authentication token
    randomize_string(req.token, sizeof(req.token));

    file = secfile_new(false);
    secfile_insert_str(file, req.token, "challenge.token");
    if (!secfile_save(file, challenge_fullname)) {
      qCritical("Couldn't write token to temporary file: %s",
                challenge_fullname);
    }
    secfile_destroy(file);

    // tell the server what we put into the file
    send_packet_single_want_hack_req(&client.conn, &req);
  }
}

/**
   Handle response (by the server) if the client has got hack or not.
 */
void handle_single_want_hack_reply(bool you_have_hack)
{
  // remove challenge file
  if (challenge_fullname[0] != '\0') {
    if (fc_remove(challenge_fullname) == -1) {
      qCritical("Couldn't remove temporary file: %s", challenge_fullname);
    }
    challenge_fullname[0] = '\0';
  }

  if (you_have_hack) {
    output_window_append(ftc_client,
                         _("Established control over the server. "
                           "You have command access level 'hack'."));
    client_has_hack = true;
  } else if (is_server_running()) {
    // only output this if we started the server and we NEED hack
    output_window_append(ftc_client,
                         _("Failed to obtain the required access "
                           "level to take control of the server. "
                           "Attempting to shut down server."));
    client_kill_server(true);
  }
}

/**
   Send server command to save game.
 */
void send_save_game(const char *filename)
{
  if (filename) {
    send_chat_printf("/save %s", filename);
  } else {
    send_chat("/save");
  }
}

/**
   Handle the list of rulesets sent by the server.
 */
void handle_ruleset_choices(const struct packet_ruleset_choices *packet)
{
  QStringList rulesets;
  int i;

  for (i = 0; i < packet->ruleset_count; i++) {
    QString r = packet->rulesets[i];

    int id = r.lastIndexOf(RULESET_SUFFIX);
    if (id >= 0) {
      r = r.left(id);
    }
    rulesets.append(r);
  }
  set_rulesets(packet->ruleset_count, rulesets);
}

/**
   Called by the GUI code when the user sets the ruleset.  The ruleset
   passed in here should match one of the strings given to set_rulesets().
 */
void set_ruleset(const char *ruleset)
{
  char buf[4096];

  fc_snprintf(buf, sizeof(buf), "/read %s%s", ruleset, RULESET_SUFFIX);
  log_debug("Executing '%s'", buf);
  send_chat(buf);
}
