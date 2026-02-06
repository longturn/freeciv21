// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "server_connection.h"

#include "game.h"

/**
   Find connection by exact user name, from game.all_connections,
   case-insensitve.  Returns nullptr if not found.
 */
server_connection *conn_by_user(const char *user_name)
{
  conn_list_iterate(game.all_connections, pconn)
  {
    if (fc_strcasecmp(user_name, pconn->username) == 0) {
      return static_cast<server_connection *>(pconn);
    }
  }
  conn_list_iterate_end;

  return nullptr;
}

/**
  Connection patterns.
 */
struct conn_pattern {
  enum conn_pattern_type type;
  char *wildcard;
};

/**
   Creates a new connection pattern.
 */
struct conn_pattern *conn_pattern_new(enum conn_pattern_type type,
                                      const char *wildcard)
{
  auto *ppattern = new conn_pattern;

  ppattern->type = type;
  ppattern->wildcard = fc_strdup(wildcard);

  return ppattern;
}

/**
   Free a connection pattern.
 */
void conn_pattern_destroy(struct conn_pattern *ppattern)
{
  fc_assert_ret(nullptr != ppattern);
  delete ppattern->wildcard;
  delete ppattern;
}

/**
   Returns TRUE whether the connection fits the connection pattern.
 */
bool conn_pattern_match(const conn_pattern *ppattern,
                        const server_connection *pconn)
{
  QString test;

  switch (ppattern->type) {
  case CPT_USER:
    test = pconn->username;
    break;
  case CPT_HOST:
    test = pconn->addr;
    break;
  case CPT_IP:
    test = pconn->ipaddr;
    break;
  }

  if (!test.isEmpty()) {
    return wildcard_fit_string(ppattern->wildcard, qUtf8Printable(test));
  } else {
    qCritical("%s(): Invalid pattern type (%d)", __FUNCTION__,
              ppattern->type);
    return false;
  }
}

/**
   Returns TRUE whether the connection fits one of the connection patterns.
 */
bool conn_pattern_list_match(const conn_pattern_list *plist,
                             const server_connection *pconn)
{
  conn_pattern_list_iterate(plist, ppattern)
  {
    if (conn_pattern_match(ppattern, pconn)) {
      return true;
    }
  }
  conn_pattern_list_iterate_end;

  return false;
}

/**
   Put a string reprentation of the pattern in 'buf'.
 */
size_t conn_pattern_to_string(const conn_pattern *ppattern, char *buf,
                              size_t buf_len)
{
  return fc_snprintf(buf, buf_len, "<%s=%s>",
                     conn_pattern_type_name(ppattern->type),
                     ppattern->wildcard);
}

/**
   Creates a new connection pattern from the string. If the type is not
   specified in 'pattern', then 'prefer' type will be used. If the type
   is needed, then pass conn_pattern_type_invalid() for 'prefer'.
 */
struct conn_pattern *conn_pattern_from_string(const char *pattern,
                                              conn_pattern_type prefer,
                                              char *error_buf,
                                              size_t error_buf_len)
{
  conn_pattern_type type = conn_pattern_type_invalid();
  const char *p;

  // Determine pattern type.
  if ((p = strchr(pattern, '='))) {
    // Special character to separate the type of the pattern.
    auto pattern_type = QString(pattern).trimmed();
    type = conn_pattern_type_by_name(qUtf8Printable(pattern_type),
                                     fc_strcasecmp);
    if (!conn_pattern_type_is_valid(type)) {
      if (nullptr != error_buf) {
        fc_snprintf(error_buf, error_buf_len,
                    _("\"%s\" is not a valid pattern type"),
                    qUtf8Printable(pattern_type));
      }
      return nullptr;
    }
  } else {
    // Use 'prefer' type.
    p = pattern;
    type = prefer;
    if (!conn_pattern_type_is_valid(type)) {
      if (nullptr != error_buf) {
        fc_strlcpy(error_buf, _("Missing pattern type"), error_buf_len);
      }
      return nullptr;
    }
  }

  // Remove leading spaces.
  while (QChar::isSpace(*p)) {
    p++;
  }

  if ('\0' == *p) {
    if (nullptr != error_buf) {
      fc_strlcpy(error_buf, _("Missing pattern"), error_buf_len);
    }
    return nullptr;
  }

  return conn_pattern_new(type, p);
}
