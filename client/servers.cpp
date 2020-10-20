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

// Qt
#include <QByteArray>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QtDebug>
#include <QNetworkDatagram>
/* dependencies */
#include "cvercmp.h"

/* utility */
#include "fcintl.h"
#include "fcthread.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "rand.h" /* fc_rand() */
#include "registry.h"
#include "support.h"

/* common */
#include "capstr.h"
#include "dataio.h"
#include "game.h"
#include "packets.h"
#include "version.h"

/* client */
#include "chatline_common.h"
#include "chatline_g.h"
#include "client_main.h"
#include "servers.h"

#include "gui_main_g.h"

struct server_scan {
  enum server_scan_type type;
  ServerScanErrorFunc error_func;

  struct srv_list srvrs;
  int sock;

  /* Only used for metaserver */
  struct {
    enum server_scan_status status;

    fc_thread thr;
    fc_mutex mutex;

    const char *urlpath;
    QByteArray mem;
  } meta;
};

fcUdpScan *fcUdpScan::m_instance = 0;
extern enum announce_type announce;

static bool begin_metaserver_scan(struct server_scan *scan);
static void delete_server_list(struct server_list *server_list);

fcUdpScan::fcUdpScan(QObject *parent) : QUdpSocket(parent) {}

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
  struct raw_data_out dout;
  char buffer[MAX_LEN_PACKET];
  const char *group;
  size_t size;

  if (announce == ANNOUNCE_NONE) {
    /* Succeeded in doing nothing */
    return TRUE;
  }
  group = get_multicast_group(announce == ANNOUNCE_IPV6);

  if (!bind(QHostAddress::AnyIPv4, SERVER_LAN_PORT + 1,
                        QAbstractSocket::ReuseAddressHint)) {
    char errstr[2048];

    fc_snprintf(errstr, sizeof(errstr),
                _("Binding socket to listen LAN announcements failed:\n%s"),
                fc_strerror(fc_get_errno()));
    scan->error_func(scan, errstr);
    return FALSE;
  }

  if(!joinMulticastGroup(QHostAddress(group))) {
      char errstr[2048];

      fc_snprintf(errstr, sizeof(errstr),
          _("Adding membership for LAN announcement group failed:\n%s"),
          fc_strerror(fc_get_errno()));
      scan->error_func(scan, errstr);
    }

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8_raw(&dout, SERVER_LAN_VERSION);
  size = dio_output_used(&dout);
  fcUdpScan::i()->writeDatagram(QByteArray(buffer, size),
                                QHostAddress::AnyIPv4, SERVER_LAN_PORT);

  fc_allocate_mutex(&scan->srvrs.mutex);
  scan->srvrs.servers = server_list_new();
  fc_release_mutex(&scan->srvrs.mutex);

  return TRUE;
}

/**********************************************************************/ /**
   Listens for UDP packets broadcasted from a server that responded
   to the request-packet sent from the client.
 **************************************************************************/
enum server_scan_status fcUdpScan::get_server_list(struct server_scan *scan)
{
  char *msgbuf;
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

  while (true) {
    struct server *pserver;
    QNetworkDatagram datagram;
    bool duplicate = FALSE;

    if (!fcUdpScan::i()->hasPendingDatagrams()) {
      break;
    }
    datagram = fcUdpScan::i()->receiveDatagram();
    msgbuf = datagram.data().data();
    dio_input_init(&din, msgbuf, datagram.data().size());

    dio_get_uint8_raw(&din, &type);
    if (type != SERVER_LAN_VERSION) {
      continue;
    }
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
      sz_strlcpy(servername, datagram.senderAddress().toString().toLocal8Bit());
    }

    /* UDP can send duplicate or delayed packets. */
    fc_allocate_mutex(&scan->srvrs.mutex);
    server_list_iterate(scan->srvrs.servers, aserver)
    {
      if (0 == fc_strcasecmp(aserver->host, servername)
          && aserver->port == port) {
        duplicate = TRUE;
        break;
      }
    }
    server_list_iterate_end;

    if (duplicate) {
      fc_release_mutex(&scan->srvrs.mutex);
      continue;
    }

    log_debug("Received a valid announcement from a server on the LAN.");

    pserver = static_cast<server *>(fc_malloc(sizeof(*pserver)));
    pserver->host = fc_strdup(servername);
    pserver->port = port;
    pserver->version = fc_strdup(version);
    pserver->state = fc_strdup(status);
    pserver->nplayers = atoi(players);
    pserver->humans = atoi(humans);
    pserver->message = fc_strdup(message);
    pserver->players = NULL;
    found_new = TRUE;

    server_list_prepend(scan->srvrs.servers, pserver);
    fc_release_mutex(&scan->srvrs.mutex);
  }

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

    log_verbose("Metaserver says latest '" FOLLOWTAG
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
    log_verbose("Mesaserver comment about '" FOLLOWTAG "': %s", comment);
    version_message(comment);
  }

  server_list = server_list_new();
  nservers = secfile_lookup_int_default(file, 0, "main.nservers");

  for (i = 0; i < nservers; i++) {
    const char *host, *port, *version, *state, *message, *nplayers, *nhumans;
    int n;
    struct server *pserver =
        (struct server *) fc_malloc(sizeof(struct server));

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
      pserver->players = static_cast<str_players *>(
          fc_malloc(pserver->nplayers * sizeof(*pserver->players)));
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
                metaserver);
    scan->error_func(scan, str);

    return FALSE;
  }

  /* parse message body */
  fc_allocate_mutex(&scan->srvrs.mutex);
  srvrs = parse_metaserver_data(f);
  scan->srvrs.servers = srvrs;
  fc_release_mutex(&scan->srvrs.mutex);

  /* 'f' (hence 'meta.mem') was closed in parse_metaserver_data(). */
  scan->meta.mem.clear();

  if (NULL == srvrs) {
    fc_snprintf(str, sizeof(str),
                _("Failed to parse the metaserver data from %s:\n"
                  "%s."),
                metaserver, secfile_error());
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
    fc_allocate_mutex(&scan->meta.mutex);
    scan->meta.status = SCAN_STATUS_ERROR;
  } else {
    if (!meta_read_response(scan)) {
      fc_allocate_mutex(&scan->meta.mutex);
      scan->meta.status = SCAN_STATUS_ERROR;
    } else {
      fc_allocate_mutex(&scan->meta.mutex);
      if (scan->meta.status == SCAN_STATUS_WAITING) {
        scan->meta.status = SCAN_STATUS_DONE;
      }
    }
  }

  fc_release_mutex(&scan->meta.mutex);
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

  QNetworkRequest request(QUrl(QString::fromUtf8(metaserver)));
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

    free(ptmp->host);
    free(ptmp->version);
    free(ptmp->state);
    free(ptmp->message);

    if (ptmp->players) {
      for (i = 0; i < n; i++) {
        free(ptmp->players[i].name);
        free(ptmp->players[i].type);
        free(ptmp->players[i].host);
        free(ptmp->players[i].nation);
      }
      free(ptmp->players);
    }

    free(ptmp);
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
  bool ok = FALSE;

  scan = new server_scan;
  scan->type = type;
  scan->error_func = error_func;
  scan->sock = -1;
  fc_init_mutex(&scan->srvrs.mutex);

  switch (type) {
  case SERVER_SCAN_GLOBAL: {
    int thr_ret;

    fc_init_mutex(&scan->meta.mutex);
    scan->meta.status = SCAN_STATUS_WAITING;
    thr_ret = fc_thread_start(&scan->meta.thr, metaserver_scan, scan);
    if (thr_ret) {
      ok = FALSE;
    } else {
      ok = TRUE;
    }
  } break;
  case SERVER_SCAN_LOCAL:
    ok = fcUdpScan::i()->begin_scan(scan);
    break;
  default:
    break;
  }

  if (!ok) {
    server_scan_finish(scan);
    scan = NULL;
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

    fc_allocate_mutex(&scan->meta.mutex);
    status = scan->meta.status;
    fc_release_mutex(&scan->meta.mutex);

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
struct srv_list *server_scan_get_list(struct server_scan *scan)
{
  if (!scan) {
    return NULL;
  }

  return &scan->srvrs;
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
    fc_allocate_mutex(&scan->meta.mutex);
    scan->meta.status = SCAN_STATUS_ABORT;
    fc_release_mutex(&scan->meta.mutex);

    /* Wait thread to stop */
    fc_thread_wait(&scan->meta.thr);
    fc_destroy_mutex(&scan->meta.mutex);

    /* This mainly duplicates code from below "else" block.
     * That's intentional, since they will be completely different in future
     * versions. We are better prepared for that by having them separately
     * already. */
    if (scan->sock >= 0) {
      fc_closesocket(scan->sock);
      scan->sock = -1;
    }

    if (scan->srvrs.servers) {
      fc_allocate_mutex(&scan->srvrs.mutex);
      delete_server_list(scan->srvrs.servers);
      scan->srvrs.servers = NULL;
      fc_release_mutex(&scan->srvrs.mutex);
    }
  } else {
    fcUdpScan::i()->drop();
    if (scan->srvrs.servers) {
      delete_server_list(scan->srvrs.servers);
      scan->srvrs.servers = NULL;
    }
  }

  fc_destroy_mutex(&scan->srvrs.mutex);

  free(scan);
}
