/**************************************************************************
  /\ ___ /\        Copyright (c) 1996-2020 ＦＲＥＥＣＩＶ ２１ and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                  If not, see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <QQueue>
#include <QTimer>

/**************************************************************************
  Struct used for idle callback to execute some callbacks later
**************************************************************************/
struct call_me_back {
  void (*callback)(void *data);
  void *data;
};

/**************************************************************************
  Class to handle idle callbacks
**************************************************************************/
class mrIdle : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(mrIdle);

public:
  ~mrIdle();
  static mrIdle *idlecb();
  static void drop();
  void addCallback(call_me_back *cb);
  void runNow();
private slots:
  void idling();

private:
  static mrIdle *m_instance;
  mrIdle();
  QQueue<call_me_back *> callback_list;
  QTimer timer;
};
