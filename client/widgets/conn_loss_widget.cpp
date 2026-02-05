// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "conn_loss_widget.h"

// utility
#include "fcintl.h"

// common
#include "featured_text.h"

// client
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h"

namespace freeciv {

/**
 * Constructor.
 */
conn_loss_widget::conn_loss_widget(QWidget *parent) : QFrame(parent)
{
  ui.setupUi(this);
  ui.label->setText(_("Connection to server lost"));
  ui.action->setText(_("Reconnect"));

  connect(ui.action, &QPushButton::pressed, [this] {
    disconnect_from_server();

    // client_url() already contains the password.
    char errbuf[512];
    if (connect_to_server(client_url(), errbuf, sizeof(errbuf)) >= 0) {
      // Success!
      setVisible(false);
    } else {
      output_window_append(ftc_client, errbuf);
    }
  });
}

} // namespace freeciv
