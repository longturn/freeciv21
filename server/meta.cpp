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

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Qt
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

// utility
#include "fcintl.h"
#include "fcthread.h"
#include "log.h"
#include "support.h"
#include "timing.h"

// common
#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "game.h"
#include "map.h"
#include "nation.h"
#include "version.h"

// server
#include "console.h"
#include "plrhand.h"
#include "settings.h"
#include "srv_main.h"

#include "meta.h"

static bool server_is_open = false;
static bool persistent_meta_connection = false;
static int meta_retry_wait = 0;

static char meta_patches[256] = "";
static char meta_message[256] = "";

Q_GLOBAL_STATIC(fcThread, meta_srv_thread);

/**
   The default metaserver patches for this server
 */
const char *default_meta_patches_string() { return "none"; }

/**
   Return static string with default info line to send to metaserver.
 */
const char *default_meta_message_string()
{
#if IS_BETA_VERSION
  return "unstable pre-" NEXT_STABLE_VERSION ": beware";
#else // IS_BETA_VERSION
#if IS_DEVEL_VERSION
  return "development version: beware";
#else  // IS_DEVEL_VERSION
  return "-";
#endif // IS_DEVEL_VERSION
#endif // IS_BETA_VERSION
}

/**
   The metaserver patches
 */
const char *get_meta_patches_string() { return meta_patches; }

/**
   The metaserver message
 */
const char *get_meta_message_string() { return meta_message; }

/**
   The server metaserver type
 */
static const char *get_meta_type_string()
{
  if (game.server.meta_info.type[0] != '\0') {
    return game.server.meta_info.type;
  }

  return NULL;
}

/**
   The metaserver message set by user
 */
const char *get_user_meta_message_string()
{
  if (game.server.meta_info.user_message[0] != '\0') {
    return game.server.meta_info.user_message;
  }

  return NULL;
}

/**
   Update meta message. Set it to user meta message, if it is available.
   Otherwise use provided message.
   It is ok to call this with NULL message. Then it only replaces current
   meta message with user meta message if available.
 */
void maybe_automatic_meta_message(const char *automatic)
{
  const char *user_message;

  user_message = get_user_meta_message_string();

  if (user_message == NULL) {
    // No user message
    if (automatic != NULL) {
      set_meta_message_string(automatic);
    }
    return;
  }

  set_meta_message_string(user_message);
}

/**
   Set the metaserver patches string
 */
void set_meta_patches_string(const char *string)
{
  sz_strlcpy(meta_patches, string);
}

/**
   Set the metaserver message string
 */
void set_meta_message_string(const char *string)
{
  sz_strlcpy(meta_message, string);
}

/**
   Set user defined metaserver message string
 */
void set_user_meta_message_string(const char *string)
{
  if (string != NULL && string[0] != '\0') {
    sz_strlcpy(game.server.meta_info.user_message, string);
    set_meta_message_string(string);
  } else {
    // Remove user meta message. We will use automatic messages instead
    game.server.meta_info.user_message[0] = '\0';
    set_meta_message_string(default_meta_message_string());
  }
}

/**
   Return string describing both metaserver name and port.
 */
QString meta_addr_port() { return srvarg.metaserver_addr; }

/**
   We couldn't find or connect to the metaserver.
 */
static void metaserver_failed()
{
  if (!persistent_meta_connection) {
    con_puts(C_METAERROR,
             _("Not reporting to the metaserver in this game."));
    con_flush();

    server_close_meta();
  } else {
    con_puts(C_METAERROR, _("Metaserver connection currently failing."));
    meta_retry_wait = 1;
  }
}

/**
   Insert a setting in the metaserver message. Return TRUE if it succeded.
 */
static inline bool meta_insert_setting(QUrlQuery *query,
                                       const char *set_name)
{
  const struct setting *pset = setting_by_name(set_name);
  char buf[256];

  fc_assert_ret_val_msg(NULL != pset, false, "Setting \"%s\" not found!",
                        set_name);
  query->addQueryItem(QStringLiteral("vn[]"),
                      QString::fromUtf8(setting_name(pset)));
  query->addQueryItem(
      QStringLiteral("vv[]"),
      QString::fromUtf8(setting_value_name(pset, false, buf, sizeof(buf))));
  return true;
}

/**
   Send POST to metaserver. This runs in its own thread.
 */
static void send_metaserver_post(void *arg)
{
  // Create a network manager
  auto *manager = new QNetworkAccessManager;

  // Post the request
  auto *post = static_cast<QUrlQuery *>(arg);

  QNetworkRequest request(QUrl(srvarg.metaserver_addr));
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QLatin1String("Freeciv/" VERSION_STRING));
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    QLatin1String("application/x-www-form-urlencoded"));
  auto *reply =
      manager->post(request, post->toString(QUrl::FullyEncoded).toUtf8());

  // Wait for the reply
  QEventLoop loop;

  QObject::connect(reply, &QNetworkReply::finished, [&] {
    if (reply->error() != QNetworkReply::NoError) {
      con_puts(C_METAERROR, _("Error connecting to metaserver"));
      qCritical(_("Error message: %s"),
                qUtf8Printable(reply->errorString()));
      metaserver_failed();
    }

    // Clean up
    reply->deleteLater();
    manager->deleteLater();
    loop.exit();
  });

  delete post;
  loop.exec();
}

/**
   Construct the POST message and send info to metaserver.
 */
static bool send_to_metaserver(enum meta_flag flag)
{
  int players = 0;
  int humans = 0;
  char host[512];
  char state[20];
  char rs[256];

  switch (server_state()) {
  case S_S_INITIAL:
    sz_strlcpy(state, "Pregame");
    break;
  case S_S_RUNNING:
    sz_strlcpy(state, "Running");
    break;
  case S_S_OVER:
    sz_strlcpy(state, "Game Ended");
    break;
  }

  // get hostname
  if (!srvarg.identity_name.isEmpty()) {
    sz_strlcpy(host, qUtf8Printable(srvarg.identity_name));
  } else if (fc_gethostname(host, sizeof(host)) != 0) {
    sz_strlcpy(host, "unknown");
  }

  if (game.control.version[0] != '\0') {
    fc_snprintf(rs, sizeof(rs), "%s %s", game.control.name,
                game.control.version);
  } else {
    sz_strlcpy(rs, game.control.name);
  }

  QUrlQuery *post = new QUrlQuery;

  post->addQueryItem(QStringLiteral("host"), QString::fromUtf8(host));
  post->addQueryItem(QStringLiteral("port"),
                     QStringLiteral("%1").arg(srvarg.port));
  post->addQueryItem(QStringLiteral("state"), QString::fromUtf8(state));
  post->addQueryItem(QStringLiteral("ruleset"), QString::fromUtf8(rs));

  if (flag == META_GOODBYE) {
    post->addQueryItem(QStringLiteral("bye"), QStringLiteral("1"));
  } else {
    const char *srvtype = get_meta_type_string();

    if (srvtype != NULL) {
      post->addQueryItem(QStringLiteral("type"), QString::fromUtf8(srvtype));
    }
    post->addQueryItem(QStringLiteral("version"),
                       QStringLiteral(VERSION_STRING));
    post->addQueryItem(QStringLiteral("patches"),
                       QString::fromUtf8(get_meta_patches_string()));
    post->addQueryItem(QStringLiteral("capability"),
                       QString::fromUtf8(our_capability));

    post->addQueryItem(QStringLiteral("serverid"), srvarg.serverid);
    post->addQueryItem(QStringLiteral("message"),
                       QString::fromUtf8(get_meta_message_string()));

    // NOTE: send info for ALL players or none at all.
    if (normal_player_count() == 0) {
      post->addQueryItem(QStringLiteral("dropplrs"), QStringLiteral("1"));
    } else {
      players = 0; // a counter for players_available
      humans = 0;

      players_iterate(plr)
      {
        bool is_player_available = true;
        struct connection *pconn = conn_by_user(plr->username);

        QLatin1String type;
        if (!plr->is_alive) {
          type = QLatin1String("Dead");
        } else if (is_barbarian(plr)) {
          type = QLatin1String("Barbarian");
        } else if (is_ai(plr)) {
          type = QLatin1String("A.I.");
        } else if (is_human(plr)) {
          type = QLatin1String("Human");
        } else {
          type = QLatin1String("-");
        }

        post->addQueryItem(QStringLiteral("plu[]"),
                           QString::fromUtf8(plr->username));
        post->addQueryItem(QStringLiteral("plt[]"), type);
        post->addQueryItem(QStringLiteral("pll[]"),
                           QString::fromUtf8(player_name(plr)));
        post->addQueryItem(
            QStringLiteral("pln[]"),
            QString::fromUtf8(plr->nation != NO_NATION_SELECTED
                                  ? nation_plural_for_player(plr)
                                  : "none"));
        post->addQueryItem(
            QStringLiteral("plf[]"),
            QString::fromUtf8(plr->nation != NO_NATION_SELECTED
                                  ? nation_of_player(plr)->flag_graphic_str
                                  : "none"));
        post->addQueryItem(QStringLiteral("plh[]"),
                           pconn ? pconn->addr : QLatin1String(""));

        /* is this player available to take?
         * TODO: there's some duplication here with
         * stdinhand.c:is_allowed_to_take() */
        if (is_barbarian(plr) && !strchr(game.server.allow_take, 'b')) {
          is_player_available = false;
        } else if (!plr->is_alive && !strchr(game.server.allow_take, 'd')) {
          is_player_available = false;
        } else if (is_ai(plr)
                   && !strchr(game.server.allow_take,
                              (game.info.is_new_game ? 'A' : 'a'))) {
          is_player_available = false;
        } else if (is_human(plr)
                   && !strchr(game.server.allow_take,
                              (game.info.is_new_game ? 'H' : 'h'))) {
          is_player_available = false;
        }

        if (pconn) {
          is_player_available = false;
        }

        if (is_player_available) {
          players++;
        }

        if (is_human(plr) && plr->is_alive) {
          humans++;
        }
      }
      players_iterate_end;

      // send the number of available players.
      post->addQueryItem(QStringLiteral("available"),
                         QStringLiteral("%1").arg(players));
      post->addQueryItem(QStringLiteral("humans"),
                         QStringLiteral("%1").arg(humans));
    }

    // Send some variables: should be listed in inverted order?
    {
      static const char *settings[] = {"timeout",    "endturn", "minplayers",
                                       "maxplayers", "aifill",  "allowtake",
                                       "generator"};
      int i;

      for (i = 0; i < ARRAY_SIZE(settings); i++) {
        meta_insert_setting(post, settings[i]);
      }

      // HACK: send the most determinant setting for the map size.
      switch (wld.map.server.mapsize) {
      case MAPSIZE_FULLSIZE:
        meta_insert_setting(post, "size");
        break;
      case MAPSIZE_PLAYER:
        meta_insert_setting(post, "tilesperplayer");
        break;
      case MAPSIZE_XYSIZE:
        meta_insert_setting(post, "xsize");
        meta_insert_setting(post, "ysize");
        break;
      }
    }

    // Turn and year.
    post->addQueryItem(QStringLiteral("vn[]"), QStringLiteral("turn"));
    post->addQueryItem(QStringLiteral("vv[]"),
                       QStringLiteral("%1").arg(game.info.turn));
    post->addQueryItem(QStringLiteral("vn[]"), QStringLiteral("year"));

    if (server_state() != S_S_INITIAL) {
      post->addQueryItem(QStringLiteral("vv[]"),
                         QStringLiteral("%1").arg(game.info.year));
    } else {
      post->addQueryItem(QStringLiteral("vv[]"),
                         QStringLiteral("Calendar not set up"));
    }
  }

  // Send POST in new thread
  meta_srv_thread->set_func(send_metaserver_post, post);
  meta_srv_thread->start(QThread::NormalPriority);

  return true;
}

/**
   Stop sending updates to metaserver
 */
void server_close_meta()
{
  server_is_open = false;
  persistent_meta_connection = false;
}

/**
   Lookup the correct address for the metaserver.
 */
bool server_open_meta(bool persistent)
{
  if (meta_patches[0] == '\0') {
    set_meta_patches_string(default_meta_patches_string());
  }
  if (meta_message[0] == '\0') {
    set_meta_message_string(default_meta_message_string());
  }

  server_is_open = true;
  persistent_meta_connection = persistent;
  meta_retry_wait = 0;

  return true;
}

/**
   Are we sending info to the metaserver?
 */
bool is_metaserver_open() { return server_is_open; }

/**
   Control when we send info to the metaserver.
 */
bool send_server_info_to_metaserver(enum meta_flag flag)
{
  static civtimer *last_send_timer = NULL;
  static bool want_update;

  if (!server_is_open) {
    return false;
  }

  // Persistent connection temporary failures handling
  if (meta_retry_wait > 0) {
    if (meta_retry_wait++ > 5) {
      meta_retry_wait = 0;
    } else {
      return false;
    }
  }

  // if we're bidding farewell, ignore all timers
  if (flag == META_GOODBYE) {
    if (last_send_timer) {
      timer_destroy(last_send_timer);
      last_send_timer = NULL;
    }
    send_to_metaserver(flag);

    meta_srv_thread->wait();
    meta_srv_thread->quit();

    return true;
  }

  // don't allow the user to spam the metaserver with updates
  if (last_send_timer
      && (timer_read_seconds(last_send_timer)
          < METASERVER_MIN_UPDATE_INTERVAL)) {
    if (flag == META_INFO) {
      want_update = true; // we couldn't update now, but update a.s.a.p.
    }
    return false;
  }

  /* if we're asking for a refresh, only do so if
   * we've exceeded the refresh interval */
  if ((flag == META_REFRESH) && !want_update && last_send_timer
      && (timer_read_seconds(last_send_timer)
          < METASERVER_REFRESH_INTERVAL)) {
    return false;
  }

  // start a new timer if we haven't already
  if (!last_send_timer) {
    last_send_timer = timer_new(TIMER_USER, TIMER_ACTIVE);
  }

  timer_clear(last_send_timer);
  timer_start(last_send_timer);
  want_update = false;

  return send_to_metaserver(flag);
}
