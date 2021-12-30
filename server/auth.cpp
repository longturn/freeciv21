/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

// common
#include "connection.h"
#include "packets.h"

/* common/scripting */
#include "luascript_types.h"

// server
#include "connecthand.h"
#include "fcdb.h"
#include "notify.h"
#include "sernet.h"
#include "srv_main.h"

/* server/scripting */
#include "script_fcdb.h"

#include "auth.h"

#define GUEST_NAME "guest"

#define MIN_PASSWORD_LEN 6 // minimum length of password
#define MIN_PASSWORD_CAPS                                                   \
  0                         /* minimum number of capital letters required   \
                             */
#define MIN_PASSWORD_NUMS 0 // minimum number of numbers required

#define MAX_AUTH_TRIES 3
#define MAX_WAIT_TIME 300 // max time we'll wait on a password

/* after each wrong guess for a password, the server waits this
 * many seconds to reply to the client */
static const int auth_fail_wait[] = {1, 1, 2, 3};

static bool is_guest_name(const char *name);
static void get_unique_guest_name(char *name);
static bool is_good_password(const char *password, char *msg);

/**
   Handle authentication of a user; called by handle_login_request() if
   authentication is enabled.

   If the connection is rejected right away, return FALSE, otherwise this
   function will return TRUE.
 */
bool auth_user(struct connection *pconn, char *username)
{
  char tmpname[MAX_LEN_NAME] = "\0";

  /* assign the client a unique guest name/reject if guests aren't allowed */
  if (is_guest_name(username)) {
    if (srvarg.auth_allow_guests) {
      sz_strlcpy(tmpname, username);
      get_unique_guest_name(username);

      if (strncmp(tmpname, username, MAX_LEN_NAME) != 0) {
        notify_conn_early(pconn->self, NULL, E_CONNECTION, ftc_warning,
                          _("Warning: the guest name '%s' has been "
                            "taken, renaming to user '%s'."),
                          tmpname, username);
      }
      sz_strlcpy(pconn->username, username);
      establish_new_connection(pconn);
    } else {
      reject_new_connection(_("Guests are not allowed on this server. "
                              "Sorry."),
                            pconn);
      qInfo(_("%s was rejected: Guests not allowed."), username);
      return false;
    }
  } else {
    /* we are not a guest, we need an extra check as to whether a
     * connection can be established: the client must authenticate itself */
    char buffer[MAX_LEN_MSG];
    bool exists = false;

    sz_strlcpy(pconn->username, username);

    if (!script_fcdb_user_exists(pconn, exists)) {
      if (srvarg.auth_allow_guests) {
        sz_strlcpy(tmpname, pconn->username);
        get_unique_guest_name(tmpname); // don't pass pconn->username here
        sz_strlcpy(pconn->username, tmpname);

        qCritical("Error reading database; connection -> guest");
        notify_conn_early(
            pconn->self, NULL, E_CONNECTION, ftc_warning,
            _("There was an error reading the user "
              "database, logging in as guest connection '%s'."),
            pconn->username);
        establish_new_connection(pconn);
      } else {
        reject_new_connection(
            _("There was an error reading the user database "
              "and guest logins are not allowed. Sorry"),
            pconn);
        qInfo(_("%s was rejected: Database error and guests not "
                "allowed."),
              pconn->username);
        return false;
      }
    } else if (exists) {
      // we found a user
      fc_snprintf(buffer, sizeof(buffer), _("Enter password for %s:"),
                  pconn->username);
      dsend_packet_authentication_req(pconn, AUTH_LOGIN_FIRST, buffer);
      pconn->server.auth_settime = time(NULL);
      pconn->server.status = AS_REQUESTING_OLD_PASS;
    } else {
      // we couldn't find the user, he is new
      if (srvarg.auth_allow_newusers) {
        /* TRANS: Try not to make the translation much longer than the
         * original. */
        sz_strlcpy(
            buffer,
            _("First time login. Set a new password and confirm it."));
        dsend_packet_authentication_req(pconn, AUTH_NEWUSER_FIRST, buffer);
        pconn->server.auth_settime = time(NULL);
        pconn->server.status = AS_REQUESTING_NEW_PASS;
      } else {
        reject_new_connection(_("This server allows only preregistered "
                                "users. Sorry."),
                              pconn);
        qInfo(_("%s was rejected: Only preregistered users allowed."),
              pconn->username);

        return false;
      }
    }
  }
  return true;
}

/**
   Receives a password from a client and verifies it.
 */
bool auth_handle_reply(struct connection *pconn, char *password)
{
  char msg[MAX_LEN_MSG];

  if (pconn->server.status == AS_REQUESTING_NEW_PASS) {
    // check if the new password is acceptable
    if (!is_good_password(password, msg)) {
      if (pconn->server.auth_tries++ >= MAX_AUTH_TRIES) {
        reject_new_connection(_("Sorry, too many wrong tries..."), pconn);
        qInfo(_("%s was rejected: Too many wrong password "
                "verifies for new user."),
              pconn->username);

        return false;
      } else {
        dsend_packet_authentication_req(pconn, AUTH_NEWUSER_RETRY, msg);
        return true;
      }
    }

    if (!script_fcdb_user_save(pconn, password)) {
      notify_conn(pconn->self, NULL, E_CONNECTION, ftc_warning,
                  _("Warning: There was an error in saving to the database. "
                    "Continuing, but your stats will not be saved."));
      qCritical("Error writing to database for: %s", pconn->username);
    }

    establish_new_connection(pconn);
  } else if (pconn->server.status == AS_REQUESTING_OLD_PASS) {
    bool success = false;

    if (script_fcdb_user_verify(pconn, password, success) && success) {
      establish_new_connection(pconn);
    } else {
      pconn->server.status = AS_FAILED;
      pconn->server.auth_tries++;
      pconn->server.auth_settime =
          time(NULL) + auth_fail_wait[pconn->server.auth_tries];
    }
  } else {
    qDebug("%s is sending unrequested auth packets", pconn->username);
    return false;
  }

  return true;
}

/**
   Checks on where in the authentication process we are.
 */
void auth_process_status(struct connection *pconn)
{
  switch (pconn->server.status) {
  case AS_NOT_ESTABLISHED:
    // nothing, we're not ready to do anything here yet.
    break;
  case AS_FAILED:
    /* the connection gave the wrong password, we kick 'em off or
     * we're throttling the connection to avoid password guessing */
    if (pconn->server.auth_settime > 0
        && time(NULL) >= pconn->server.auth_settime) {
      if (pconn->server.auth_tries >= MAX_AUTH_TRIES) {
        pconn->server.status = AS_NOT_ESTABLISHED;
        reject_new_connection(_("Sorry, too many wrong tries..."), pconn);
        qInfo(_("%s was rejected: Too many wrong password tries."),
              pconn->username);
        connection_close_server(pconn, _("auth failed"));
      } else {
        struct packet_authentication_req request;

        pconn->server.status = AS_REQUESTING_OLD_PASS;
        request.type = AUTH_LOGIN_RETRY;
        sz_strlcpy(request.message,
                   _("Your password is incorrect. Try again."));
        send_packet_authentication_req(pconn, &request);
      }
    }
    break;
  case AS_REQUESTING_OLD_PASS:
  case AS_REQUESTING_NEW_PASS:
    // waiting on the client to send us a password... don't wait too long
    if (time(NULL) >= pconn->server.auth_settime + MAX_WAIT_TIME) {
      pconn->server.status = AS_NOT_ESTABLISHED;
      reject_new_connection(_("Sorry, your connection timed out..."), pconn);
      qInfo(_("%s was rejected: Connection timeout waiting for "
              "password."),
            pconn->username);
      connection_close_server(pconn, _("auth failed"));
    }
    break;
  case AS_ESTABLISHED:
    // this better fail bigtime
    fc_assert(pconn->server.status != AS_ESTABLISHED);
    break;
  }
}

/**
   See if the name qualifies as a guest login name
 */
static bool is_guest_name(const char *name)
{
  return (fc_strncasecmp(name, GUEST_NAME, qstrlen(GUEST_NAME)) == 0);
}

/**
   Return a unique guest name
   WARNING: do not pass pconn->username to this function: it won't return!
 */
static void get_unique_guest_name(char *name)
{
  unsigned int i;

  // first see if the given name is suitable
  if (is_guest_name(name) && !conn_by_user(name)) {
    return;
  }

  // next try bare guest name
  fc_strlcpy(name, GUEST_NAME, MAX_LEN_NAME);
  if (!conn_by_user(name)) {
    return;
  }

  // bare name is taken, append numbers
  for (i = 1;; i++) {
    fc_snprintf(name, MAX_LEN_NAME, "%s%u", GUEST_NAME, i);

    // attempt to find this name; if we can't we're good to go
    if (!conn_by_user(name)) {
      break;
    }

    // Prevent endless loops.
    fc_assert_ret(i < 2 * MAX_NUM_PLAYERS);
  }
}

/**
   Verifies that a password is valid. Does some [very] rudimentary safety
   checks. TODO: do we want to frown on non-printing characters?
   Fill the msg (length MAX_LEN_MSG) with any worthwhile information that
   the client ought to know.
 */
static bool is_good_password(const char *password, char *msg)
{
  int i, num_caps = 0, num_nums = 0;

  // check password length
  if (strlen(password) < MIN_PASSWORD_LEN) {
    fc_snprintf(msg, MAX_LEN_MSG,
                _("Your password is too short, the minimum length is %d. "
                  "Try again."),
                MIN_PASSWORD_LEN);
    return false;
  }

  fc_snprintf(msg, MAX_LEN_MSG,
              _("The password must have at least %d capital letters, %d "
                "numbers, and be at minimum %d [printable] characters long. "
                "Try again."),
              MIN_PASSWORD_CAPS, MIN_PASSWORD_NUMS, MIN_PASSWORD_LEN);

  for (i = 0; i < qstrlen(password); i++) {
    if (QChar::isUpper(password[i])) {
      num_caps++;
    }
    if (QChar::isDigit(password[i])) {
      num_nums++;
    }
  }

  // check number of capital letters
  if (num_caps < MIN_PASSWORD_CAPS) {
    return false;
  }

  // check number of numbers
  if (num_nums < MIN_PASSWORD_NUMS) {
    Q_UNREACHABLE();
    return false;
  }

  if (!is_ascii_name(password)) {
    return false;
  }

  return true;
}

/**
   Get username for connection
 */
const char *auth_get_username(struct connection *pconn)
{
  fc_assert_ret_val(pconn != NULL, NULL);

  return pconn->username;
}

/**
   Get connection ip address
 */
const char *auth_get_ipaddr(struct connection *pconn)
{
  fc_assert_ret_val(pconn != NULL, NULL);

  return pconn->server.ipaddr;
}
