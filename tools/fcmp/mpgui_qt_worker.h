/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__MPGUI_QT_WORKER_H
#define FC__MPGUI_QT_WORKER_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QString>
#include <QThread>
#include <QUrl>

// tools
#include "download.h"

class mpgui;
struct fcmp_params;

class mpqt_worker : public QThread {
  Q_OBJECT

public:
  mpqt_worker() : m_gui(nullptr), m_fcmp(nullptr) {};
  void run();
  void download(const QUrl &url, mpgui *gui, fcmp_params *fcmp,
                const dl_msg_callback &msg_callback,
                const dl_pb_callback &pb_callback);

private:
  QUrl m_url;
  mpgui *m_gui;
  fcmp_params *m_fcmp;
  dl_msg_callback m_msg_callback;
  dl_pb_callback m_pb_callback;
};

#endif // FC__MPGUI_QT_WORKER_H
