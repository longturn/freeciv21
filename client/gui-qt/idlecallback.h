/********************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
                              .___.
          /)               ,-^     ^-.
         //               /           \
.-------| |--------------/  __     __  \-------------------.__
|WMWMWMW| |>>>>>>>>>>>>> | />>\   />>\ |>>>>>>>>>>>>>>>>>>>>>>:>
`-------| |--------------| \__/   \__/ |-------------------'^^
         \\               \    /|\    / You should have received
          \)               \   \_/   / a copy of the GNU General
                            |       | Public License along with this
    ⒻⓇⒺⒺⒸⒾⓋ ②①        |+H+H+H+| program; if not, write to
      GPLv3                 \       / 51 Franklin Street,Fifth Floor
                             ^-----^  Boston, MA  02110-1301, USA.
********************************************************************/

#ifndef FC__IDLECALLBACK_H
#define FC__IDLECALLBACK_H

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
class mr_idle : public QObject {
  Q_OBJECT
public:
  mr_idle();
  ~mr_idle();

  void add_callback(call_me_back *cb);
private slots:
  void idling();

private:
  QQueue<call_me_back *> callback_list;
  QTimer timer;
};

#endif  /* FC__IDLECALLBACK_H */
