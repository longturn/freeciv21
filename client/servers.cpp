/***********************************************************************
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
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

#include "servers.h"

// Qt
#include <QByteArray>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

/* dependencies */
#include "cvercmp.h"

/* utility */
#include "net_types.h"

/* common */
#include "capstr.h"
#include "dataio.h"
#include "version.h"

/* client */
#include "chatline_g.h"
#include "client_main.h"

struct server_scan {
  enum server_scan_type type;
  ServerScanErrorFunc error_func;

  struct server_list *servers;
  int sock;

  /* Only used for metaserver */
  struct {
    enum server_scan_status status;

    const char *urlpath;
    QByteArray mem;
  } meta;
};

fcUdpScan *fcUdpScan::m_instance = 0;
extern enum announce_type announce;

static bool begin_metaserver_scan(struct server_scan *scan);
static void delete_server_list(struct server_list *server_list);

fcUdpScan::fcUdpScan(QObject *parent) : QUdpSocket(parent)
{
  fcudp_scan = nullptr;
  connect(this, &QUdpSocket::readyRead, this,
          &fcUdpScan::readPendingDatagrams);
  connect(
      this,
      QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
      this, &fcUdpScan::sockError);
}

/**************************************************************************
  returns fcUdpScan instance, use it to initizalize fcUdpScan
  or for calling methods
**************************************************************************/
fcUdpScan *fcUdpScan::i()
{
  if (!m_instance) {
    m_instance = new fcUdpScan;
  }
  return m_instance;
}

/**************************************************************************
  Pass errors to main scan
**************************************************************************/
void fcUdpScan::sockError(QAbstractSocket::SocketError socketError)
{
  char *errstr;
  if (!fcudp_scan)
    return;
  errstr = errorString().toLocal8Bit().data();
  fcudp_scan->error_func(fcudp_scan, errstr);
}

/**************************************************************************
  deletes fcUdpScan
**************************************************************************/
void fcUdpScan::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

/**********************************************************************/ /**
   Broadcast an UDP package to all servers on LAN, requesting information
   about the server. The packet is send to all Freeciv servers in the same
   multicast group as the client.
 **************************************************************************/
bool fcUdpScan::begin_scan(struct server_scan *scan)
{
  const char *group;
  struct raw_data_out dout;
  char buffer[MAX_LEN_PACKET];
  enum QHostAddress::SpecialAddress address_type;
  size_t size;

  fcudp_scan = scan;
  if (announce == ANNOUNCE_NONE) {
    /* Succeeded in doing nothing */
    return TRUE;
  }
  group = get_multicast_group(announce == ANNOUNCE_IPV6);

  switch (announce) {
  case ANNOUNCE_IPV6:
    address_type = QHostAddress::AnyIPv6;
    break;
  case ANNOUNCE_IPV4:
  default:
    address_type = QHostAddress::AnyIPv4;
  }

  if (!bind(address_type, SERVER_LAN_PORT + 1,
            QAbstractSocket::ReuseAddressHint)) {
    return false;
  }

  joinMulticastGroup(QHostAddress(group));

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8_raw(&dout, SERVER_LAN_VERSION);
  size = dio_output_used(&dout);
  writeDatagram(QByteArray(buffer, size), QHostAddress(group),
                SERVER_LAN_PORT);
  scan->servers = server_list_new();

  return TRUE;
}

void fcUdpScan::readPendingDatagrams()
{
  while (hasPendingDatagrams()) {
    bool add = true;
    QNetworkDatagram qn = receiveDatagram();
    for (auto d : datagram_list) {
      if (d.data().data() == qn.data().data()) {
        add = false;
      }
    }
    if (add) {
      datagram_list.append(qn);
    }
  }
}

/**********************************************************************/ /**
   Listens for UDP packets broadcasted from a server that responded
   to the request-packet sent from the client.
 **************************************************************************/
enum server_scan_status fcUdpScan::get_server_list(struct server_scan *scan)
{
  int type;
  struct data_in din;
  char servername[512];
  char portstr[256];
  int port;
  char version[256];
  char status[256];
  char players[256];
  char humans[256];
  char message[1024];
  bool found_new = FALSE;

  struct server *pserver;
  bool duplicate = FALSE;

  for (auto datagram : datagram_list) {
    if (datagram.isNull() || !datagram.isValid()) {
      continue;
    }
    auto data = datagram.data();
    dio_input_init(&din, data.constData(), data.size());

    dio_get_uint8_raw(&din, &type);
    dio_get_string_raw(&din, servername, sizeof(servername));
    dio_get_string_raw(&din, portstr, sizeof(portstr));
    port = atoi(portstr);
    dio_get_string_raw(&din, version, sizeof(version));
    dio_get_string_raw(&din, status, sizeof(status));
    dio_get_string_raw(&din, players, sizeof(players));
    dio_get_string_raw(&din, humans, sizeof(humans));
    dio_get_string_raw(&din, message, sizeof(message));

    if (!fc_strcasecmp("none", servername)) {
      bool nameinfo = FALSE;

      const char *dst = NULL;
      struct hostent *from;
      const char *host = NULL;
      sz_strlcpy(servername,
                 datagram.senderAddress().toString().toLocal8Bit());
    }

    log_debug("Received a valid announcement from a server on the LAN.");

    pserver = new server;
    pserver->host = fc_strdup(servername);
    pserver->port = port;
    pserver->version = fc_strdup(version);
    pserver->state = fc_strdup(status);
    pserver->nplayers = atoi(players);
    pserver->humans = atoi(humans);
    pserver->message = fc_strdup(message);
    pserver->players = NULL;
    found_new = TRUE;

    server_list_prepend(scan->servers, pserver);
  }
  datagram_list.clear();
  if (found_new) {
    return SCAN_STATUS_PARTIAL;
  }
  return SCAN_STATUS_WAITING;
}

/**********************************************************************/ /**
   The server sends a stream in a registry 'ini' type format.
   Read it using secfile functions and fill the server_list structs.
 **************************************************************************/
static struct server_list *parse_metaserver_data(fz_FILE *f)
{
  struct server_list *server_list;
  struct section_file *file;
  int nservers, i, j;
  const char *latest_ver;
  const char *comment;

  /* This call closes f. */
  if (!(file = secfile_from_stream(f, TRUE))) {
    return NULL;
  }

  latest_ver = secfile_lookup_str_default(file, NULL, "versions." FOLLOWTAG);
  comment =
      secfile_lookup_str_default(file, NULL, "version_comments." FOLLOWTAG);

  if (latest_ver != NULL) {
    const char *my_comparable = fc_comparable_version();
    char vertext[2048];

    qDebug("Metaserver says latest '" FOLLOWTAG
           "' version is '%s'; we have '%s'",
           latest_ver, my_comparable);
    if (cvercmp_greater(latest_ver, my_comparable)) {
      const char *const followtag = "?vertag:" FOLLOWTAG;
      fc_snprintf(vertext, sizeof(vertext),
                  /* TRANS: Type is version tag name like "stable", "S2_4",
                   * "win32" (which can also be localised -- msgids start
                   * '?vertag:') */
                  _("Latest %s release of Freeciv is %s, this is %s."),
                  Q_(followtag), latest_ver, my_comparable);

      version_message(vertext);
    } else if (comment == NULL) {
      fc_snprintf(vertext, sizeof(vertext),
                  _("There is no newer %s release of Freeciv available."),
                  FOLLOWTAG);

      version_message(vertext);
    }
  }

  if (comment != NULL) {
    qDebug("Mesaserver comment about '" FOLLOWTAG "': %s", comment);
    version_message(comment);
  }

  server_list = server_list_new();
  nservers = secfile_lookup_int_default(file, 0, "main.nservers");

  for (i = 0; i < nservers; i++) {
    const char *host, *port, *version, *state, *message, *nplayers, *nhumans;
    int n;
    struct server *pserver = new server;

    host = secfile_lookup_str_default(file, "", "server%d.host", i);
    pserver->host = fc_strdup(host);

    port = secfile_lookup_str_default(file, "", "server%d.port", i);
    pserver->port = atoi(port);

    version = secfile_lookup_str_default(file, "", "server%d.version", i);
    pserver->version = fc_strdup(version);

    state = secfile_lookup_str_default(file, "", "server%d.state", i);
    pserver->state = fc_strdup(state);

    message = secfile_lookup_str_default(file, "", "server%d.message", i);
    pserver->message = fc_strdup(message);

    nplayers = secfile_lookup_str_default(file, "0", "server%d.nplayers", i);
    n = atoi(nplayers);
    pserver->nplayers = n;

    nhumans = secfile_lookup_str_default(file, "-1", "server%d.humans", i);
    n = atoi(nhumans);
    pserver->humans = n;

    if (pserver->nplayers > 0) {
      pserver->players = new str_players[pserver->nplayers];
    } else {
      pserver->players = NULL;
    }

    for (j = 0; j < pserver->nplayers; j++) {
      const char *name, *nation, *type, *plrhost;

      name = secfile_lookup_str_default(file, "", "server%d.player%d.name",
                                        i, j);
      pserver->players[j].name = fc_strdup(name);

      type = secfile_lookup_str_default(file, "", "server%d.player%d.type",
                                        i, j);
      pserver->players[j].type = fc_strdup(type);

      plrhost = secfile_lookup_str_default(file, "",
                                           "server%d.player%d.host", i, j);
      pserver->players[j].host = fc_strdup(plrhost);

      nation = secfile_lookup_str_default(file, "",
                                          "server%d.player%d.nation", i, j);
      pserver->players[j].nation = fc_strdup(nation);
    }

    server_list_append(server_list, pserver);
  }

  secfile_destroy(file);

  return server_list;
}

/**********************************************************************/ /**
   Read the reply string from the metaserver.
 **************************************************************************/
static bool meta_read_response(struct server_scan *scan)
{
  fz_FILE *f;
  char str[4096];
  struct server_list *srvrs;

  f = fz_from_memory(scan->meta.mem);
  if (NULL == f) {
    fc_snprintf(str, sizeof(str),
                _("Failed to read the metaserver data from %s."),
                qUtf8Printable(cmd_metaserver));
    scan->error_func(scan, str);

    return FALSE;
  }

  /* parse message body */
  srvrs = parse_metaserver_data(f);
  scan->servers = srvrs;

  /* 'f' (hence 'meta.mem') was closed in parse_metaserver_data(). */
  scan->meta.mem.clear();

  if (NULL == srvrs) {
    fc_snprintf(str, sizeof(str),
                _("Failed to parse the metaserver data from %s:\n"
                  "%s."),
                qUtf8Printable(cmd_metaserver), secfile_error());
    scan->error_func(scan, str);

    return FALSE;
  }

  return TRUE;
}

/**********************************************************************/ /**
   Metaserver scan thread entry point
 **************************************************************************/
static void metaserver_scan(void *arg)
{
  struct server_scan *scan = static_cast<server_scan *>(arg);

  if (!begin_metaserver_scan(scan)) {
    scan->meta.status = SCAN_STATUS_ERROR;
  } else {
    if (!meta_read_response(scan)) {
      scan->meta.status = SCAN_STATUS_ERROR;
    } else {
      if (scan->meta.status == SCAN_STATUS_WAITING) {
        scan->meta.status = SCAN_STATUS_DONE;
      }
    }
  }
}

/**********************************************************************/ /**
   Begin a metaserver scan for servers.

   Returns FALSE on error (in which case errbuf will contain an error
   message).
 **************************************************************************/
static bool begin_metaserver_scan(struct server_scan *scan)
{
  // Create a network manager
  auto manager = new QNetworkAccessManager;

  // Post the request
  QUrlQuery post;
  post.addQueryItem(QLatin1String("client_cap"),
                    QString::fromUtf8(our_capability));

  QNetworkRequest request(cmd_metaserver);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QLatin1String("Freeciv/" VERSION_STRING));
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    QLatin1String("application/x-www-form-urlencoded"));
  auto reply =
      manager->post(request, post.toString(QUrl::FullyEncoded).toUtf8());

  // Read from the reply
  bool retval = true;
  QEventLoop loop; // Need an event loop for QNetworkReply to work

  QObject::connect(reply, &QNetworkReply::finished, [&] {
    if (reply->error() == QNetworkReply::NoError) {
      scan->meta.mem = reply->readAll();
    } else {
      // Error
      scan->error_func(scan, _("Error connecting to metaserver"));
      qCritical(_("Error message: %s"),
                qUtf8Printable(reply->errorString()));

      scan->meta.mem.clear();
      retval = false;
    }

    // Clean up
    reply->deleteLater();
    manager->deleteLater();

    loop.quit();
  });

  loop.exec();

  return retval;
}

/**********************************************************************/ /**
   Frees everything associated with a server list including
   the server list itself (so the server_list is no longer
   valid after calling this function)
 **************************************************************************/
static void delete_server_list(struct server_list *server_list)
{
  if (!server_list) {
    return;
  }

  server_list_iterate(server_list, ptmp)
  {
    int i;
    int n = ptmp->nplayers;

    delete[] ptmp->host;
    delete[] ptmp->version;
    delete[] ptmp->state;
    delete[] ptmp->message;

    if (ptmp->players) {
      for (i = 0; i < n; i++) {
        delete[] ptmp->players[i].name;
        delete[] ptmp->players[i].type;
        delete[] ptmp->players[i].host;
        delete[] ptmp->players[i].nation;
      }
      delete[] ptmp->players;
    }

    delete ptmp;
  }
  server_list_iterate_end;

  server_list_destroy(server_list);
}

/**********************************************************************/ /**
   Creates a new server scan and returns it, or NULL if impossible.

   Depending on 'type' the scan will look for either local or internet
   games.

   error_func provides a callback to be used in case of error; this
   callback probably should call server_scan_finish.

   NB: You must call server_scan_finish() when you are done with the
   scan to free the memory and resources allocated by it.
 **************************************************************************/
struct server_scan *server_scan_begin(enum server_scan_type type,
                                      ServerScanErrorFunc error_func)
{
  struct server_scan *scan;

  scan = new server_scan;
  scan->type = type;
  scan->error_func = error_func;

  switch (type) {
  case SERVER_SCAN_GLOBAL:
    metaserver_scan(scan);
    break;
  case SERVER_SCAN_LOCAL:
    fcUdpScan::i()->begin_scan(scan);
    break;
  default:
    break;
  }

  return scan;
}

/**********************************************************************/ /**
   A simple query function to determine the type of a server scan (previously
   allocated in server_scan_begin).
 **************************************************************************/
enum server_scan_type server_scan_get_type(const struct server_scan *scan)
{
  if (!scan) {
    return SERVER_SCAN_LAST;
  }
  return scan->type;
}

/**********************************************************************/ /**
   A function to query servers of the server scan. This will check any
   pending network data and update the server list.

   The return value indicates the status of the server scan:
     SCAN_STATUS_ERROR   - The scan failed and should be aborted.
     SCAN_STATUS_WAITING - The scan is in progress (continue polling).
     SCAN_STATUS_PARTIAL - The scan received some data, with more expected.
                           Get the servers with server_scan_get_list(), and
                           continue polling.
     SCAN_STATUS_DONE    - The scan received all data it expected to receive.
                           Get the servers with server_scan_get_list(), and
                           stop calling this function.
     SCAN_STATUS_ABORT   - The scan has been aborted
 **************************************************************************/
enum server_scan_status server_scan_poll(struct server_scan *scan)
{
  if (!scan) {
    return SCAN_STATUS_ERROR;
  }

  switch (scan->type) {
  case SERVER_SCAN_GLOBAL: {
    enum server_scan_status status;
    status = scan->meta.status;
    return status;
  } break;
  case SERVER_SCAN_LOCAL:
    return fcUdpScan::i()->get_server_list(scan);
    break;
  default:
    break;
  }

  return SCAN_STATUS_ERROR;
}

/**********************************************************************/ /**
   Returns the srv_list currently held by the scan (may be NULL).
 **************************************************************************/
struct server_list *server_scan_get_list(struct server_scan *scan)
{
  if (!scan) {
    return NULL;
  }

  return scan->servers;
}

/**********************************************************************/ /**
   Closes the socket listening on the scan, frees the list of servers, and
   frees the memory allocated for 'scan' by server_scan_begin().
 **************************************************************************/
void server_scan_finish(struct server_scan *scan)
{
  if (!scan) {
    return;
  }

  if (scan->type == SERVER_SCAN_GLOBAL) {
    /* Signal metaserver scan thread to stop */
    scan->meta.status = SCAN_STATUS_ABORT;

    if (scan->servers) {
      delete_server_list(scan->servers);
      scan->servers = NULL;
    }
  } else {
    fcUdpScan::i()->drop();
    if (scan->servers) {
      delete_server_list(scan->servers);
      scan->servers = NULL;
    }
  }

  delete scan;
}
