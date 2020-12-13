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

#include <memory>

// Qt
#include <QBuffer>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

/* utility */
#include "fcintl.h"
#include "ioz.h"
#include "registry.h"

#include "netfile.h"

/*******************************************************************/ /**
   Fetch file from given URL to given file stream. This is core
   function of netfile module.
 ***********************************************************************/
static bool netfile_download_file_core(const QUrl &url, QIODevice *out,
                                       const nf_errmsg &cb)
{
  fc_assert_ret_val(out != nullptr, false);

  // Create a network manager
  auto manager = std::make_unique<QNetworkAccessManager>();

  // Initiate the request
  auto request = QNetworkRequest(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QLatin1String("Freeciv/" VERSION_STRING));

  auto reply = manager->get(request);
  bool retval = true;

  QEventLoop loop; // Need an event loop for QNetworkReply to work

  // Data are available
  QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
    while (reply->bytesAvailable() > 0) {
      out->write(reply->read(1 << 20)); // Read max 1MB at a time
    }
  });

  // It's over -- for good or bad reasons
  QObject::connect(reply, &QNetworkReply::finished, [&] {
    if (reply->error() != QNetworkReply::NoError) {
      // Error
      retval = false;

      if (cb != nullptr) {
        // TRANS: %1 is URL, %2 is Curl error message (not in Freeciv
        // translation domain)
        auto msg = QString::fromUtf8(_("Failed to fetch %1: %2"))
                       .arg(url.toDisplayString())
                       .arg(reply->errorString());
        cb(msg);
      }
    }

    // Clean up
    reply->deleteLater();
    manager->deleteLater();

    loop.quit();
  });

  // Perform the request
  loop.exec();

  return retval;
}

/*******************************************************************/ /**
   Fetch section file from net
 ***********************************************************************/
section_file *netfile_get_section_file(const QUrl &url, const nf_errmsg &cb)
{
  QBuffer buffer;
  buffer.open(QIODevice::WriteOnly);

  // Try to download into the buffer
  if (netfile_download_file_core(url, &buffer, cb)) {
    auto file = fz_from_memory(buffer.buffer());
    // Parse
    return secfile_from_stream(file, TRUE);
  }

  return nullptr;
}

/*******************************************************************/ /**
   Fetch file from given URL and save as given filename.
 ***********************************************************************/
bool netfile_download_file(const QUrl &url, const char *filename,
                           const nf_errmsg &cb)
{
  QFile out(QString::fromUtf8(filename));
  if (!out.open(QIODevice::WriteOnly)) {
    if (cb != NULL) {
      auto msg = QString::fromUtf8(_("Could not open %1 for writing"))
                     .arg(filename);
      cb(msg);
    }
    return false;
  }

  return netfile_download_file_core(url, &out, cb);
}
