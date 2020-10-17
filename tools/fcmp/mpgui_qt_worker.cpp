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

// Qt
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// modinst
#include "download.h"
#include "mpgui_qt.h"

#include "mpgui_qt_worker.h"

/**********************************************************************/ /**
   Run download thread
 **************************************************************************/
void mpqt_worker::run()
{
  auto url_bytes = m_url.toEncoded();
  const char *errmsg = download_modpack(url_bytes.data(), m_fcmp,
                                        m_msg_callback, m_pb_callback);

  if (errmsg != nullptr) {
    m_msg_callback(errmsg);
  } else {
    m_msg_callback(_("Ready"));
  }

  m_gui->refresh_list_versions_thr();
}

/**********************************************************************/ /**
   Start thread to download and install given modpack.
 **************************************************************************/
void mpqt_worker::download(const QUrl &url, class mpgui *gui,
                           struct fcmp_params *fcmp,
                           const dl_msg_callback &msg_callback,
                           const dl_pb_callback &pb_callback)
{
  m_url = url;
  m_gui = gui;
  m_fcmp = fcmp;
  m_msg_callback = msg_callback;
  m_pb_callback = pb_callback;

  start();
}
