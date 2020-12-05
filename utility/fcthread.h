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
#pragma once

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <QMutex>
#include <QThread>

class fcThread : public QThread {
public:
  fcThread(){};
  fcThread(void(tfunc)(void *), void *tdata);
  void set_func(void(tfunc)(void *), void *tdata);
  ~fcThread();
protected:
  void run() Q_DECL_OVERRIDE;

private:
  void (*func)(void *data);
  void *data;
  QMutex mutex;
};


