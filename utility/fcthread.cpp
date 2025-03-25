// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "fcthread.h"

// Qt
#include <QMutexLocker>

fcThread::fcThread(void(tfunc)(void *), void *tdata)
    : func(tfunc), data(tdata)
{
}

void fcThread::set_func(void(tfunc)(void *), void *tdata)
{
  func = tfunc;
  data = tdata;
}

fcThread::~fcThread() = default;

void fcThread::run()
{
  QMutexLocker locker(&mutex);
  if (func) {
    (func)(data);
  }
}
