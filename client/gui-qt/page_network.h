/**************************************************************************
\^~~~~\   )  (   /~~~~^/ *     _      Copyright (c) 1996-2020 Freeciv21 and
 ) *** \  {**}  / *** (  *  _ {o} _      Freeciv contributors. This file is
  ) *** \_ ^^ _/ *** (   * {o}{o}{o}   part of Freeciv21. Freeciv21 is free
  ) ****   vv   **** (   *  ~\ | /~software: you can redistribute it and/or
   )_****      ****_(    *    OoO      modify it under the terms of the GNU
     )*** m  m ***(      *    /|\      General Public License  as published
       by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version. You should have received  a copy of
                        the GNU General Public License along with Freeciv21.
                                 If not, see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__PAGE_NETWORK_H
#define FC__PAGE_NETWORK_H

#include <QWidget>
#include "ui_page_network.h"
//client
#include "servers.h"

#include "fc_client.h" // conn state
class fc_client;
class QItemSelection;


class page_network : public QWidget
{
    Q_OBJECT
    public:
    page_network(QWidget *, fc_client *);
    ~page_network();
    void update_network_page(void);
    void update_network_lists(void);
    void set_connection_state(enum connection_state state);
    void destroy_server_scans(void);
    void handle_authentication_req(enum authentication_type type,
                                          const char *message);
private slots:
  void slot_meta_scan();
  void slot_connect();
  void slot_lan_scan();
  void slot_selection_changed(const QItemSelection &,
                              const QItemSelection &);
private:
  void update_server_list(enum server_scan_type sstype,
                                   const struct server_list *list);
  bool check_server_scan(server_scan *scan_data);

  fc_client* king; // protect The King
  QTimer *meta_scan_timer;
  QTimer *lan_scan_timer;
  Ui::FormPageNetwork ui;
};

void server_scan_error(server_scan *, const char *);

#endif /* FC__PAGE_NETWORK_H */

