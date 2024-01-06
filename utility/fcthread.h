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

#include <QMutex>
#include <QThread>
#include <QtGlobal>

class fcThread : public QThread {
public:
  fcThread() = default;
  ;
  fcThread(void(tfunc)(void *), void *tdata);
  void set_func(void(tfunc)(void *), void *tdata);
  ~fcThread() override;

protected:
  void run() Q_DECL_OVERRIDE;

private:
  void (*func)(void *data) = nullptr;
  void *data = nullptr;
  QMutex mutex;
};
