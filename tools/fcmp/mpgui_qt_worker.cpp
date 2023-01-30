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

// Qt
#include <QApplication>
#include <QHeaderView>
#include <QLabel>

// utility
#include "fcintl.h"
// modinst
#include "download.h"
#include "mpgui_qt.h"

#include "mpgui_qt_worker.h"

/**
   Run download thread
 */
void mpqt_worker::run()
{
  auto errmsg =
      download_modpack(m_url, m_fcmp, m_msg_callback, m_pb_callback);

  if (errmsg != nullptr) {
    m_msg_callback(errmsg);
  } else {
    m_msg_callback(_("Ready"));
  }

  m_gui->refresh_list_versions_thr();
}

/**
   Start thread to download and install given modpack.
 */
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
