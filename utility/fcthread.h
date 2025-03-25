// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// Qt
#include <QMutex>
#include <QThread>

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
