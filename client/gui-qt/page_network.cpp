/**************************************************************************
             .--~~,__        Copyright (c) 1996-2020 Freeciv21 and Freeciv
:-....,-------`~~'._.'       contributors. This file is part of Freeciv21.
 `-,,,  ,_      ;'~U'  Freeciv21 is free software: you can redistribute it
  _,-' ,'`-__; '--.            and/or modify it under the terms of the GNU
 (_/'~~      ''''(;            General Public License  as published by the
                             Free Software Foundation, either version 3 of
       the License, or (at your option) any later version. You should have
   received a copy of the GNU General Public License along with Freeciv21.
                                If not, see https://www.gnu.org/licenses/.
**************************************************************************/
#include "page_network.h"
// utility
#include "fcintl.h"
// client
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"
// gui-qt
#include "dialogs.h"
#include "fc_client.h"
#include "qtg_cxxside.h"

static enum connection_state connection_status;
static struct server_scan *meta_scan, *lan_scan;

page_network::page_network(QWidget *parent, fc_client *gui)
    : QWidget(parent), meta_scan_timer(nullptr), lan_scan_timer(nullptr)
{
  king = gui;
  ui.setupUi(this);

  QHeaderView *header;

  ui.connect_password_edit->setEchoMode(QLineEdit::Password);
  ui.connect_confirm_password_edit->setEchoMode(QLineEdit::Password);
  ui.connect_password_edit->setDisabled(true);
  ui.connect_confirm_password_edit->setDisabled(true);

  QStringList servers_list;
  servers_list << _("Server Name") << _("Port") << _("Version")
               << _("Status") << Q_("?count:Players") << _("Comment");
  QStringList server_info;
  server_info << _("Name") << _("Type") << _("Host") << _("Nation");
  // TODO put it to designer
  ui.lan_widget->setRowCount(0);
  ui.lan_widget->setColumnCount(servers_list.count());
  ui.lan_widget->verticalHeader()->setVisible(false);
  ui.lan_widget->setAutoScroll(false);
  ui.wan_widget->setRowCount(0);
  ui.wan_widget->setColumnCount(servers_list.count());
  ui.wan_widget->verticalHeader()->setVisible(false);
  ui.wan_widget->setAutoScroll(false);
  ui.info_widget->setRowCount(0);
  ui.info_widget->setColumnCount(server_info.count());
  ui.info_widget->verticalHeader()->setVisible(false);
  ui.lan_widget->setHorizontalHeaderLabels(servers_list);
  ui.lan_widget->setProperty("showGrid", "false");
  ui.lan_widget->setProperty("selectionBehavior", "SelectRows");
  ui.lan_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.lan_widget->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.wan_widget->setHorizontalHeaderLabels(servers_list);
  ui.wan_widget->setProperty("showGrid", "false");
  ui.wan_widget->setProperty("selectionBehavior", "SelectRows");
  ui.wan_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.wan_widget->setSelectionMode(QAbstractItemView::SingleSelection);

  connect(ui.wan_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &page_network::slot_selection_changed);

  connect(ui.lan_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &page_network::slot_selection_changed);
  connect(ui.wan_widget, &QTableWidget::itemDoubleClicked, this,
          &page_network::slot_connect);
  connect(ui.lan_widget, &QTableWidget::itemDoubleClicked, this,
          &page_network::slot_connect);

  ui.info_widget->setHorizontalHeaderLabels(server_info);
  ui.info_widget->setProperty("selectionBehavior", "SelectRows");
  ui.info_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.info_widget->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.info_widget->setProperty("showGrid", "false");
  ui.info_widget->setAlternatingRowColors(true);

  header = ui.lan_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  header = ui.wan_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  header = ui.info_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);

  ui.lhost->setText(_("Connect"));
  ui.lport->setText(_("Port"));
  ui.lname->setText(_("Password"));
  ui.lpass->setText(_("Connect"));
  ui.lconfpass->setText(_("Confirm Password"));

  ui.refresh_button->setText(_("Refresh"));
  ui.cancel_button->setText(_("Cancel"));
  ui.connect_button->setText(_("Connect"));
  QObject::connect(ui.refresh_button, &QAbstractButton::clicked, this,
                   &page_network::update_network_lists);
  QObject::connect(ui.cancel_button, &QPushButton::clicked,
                   [gui]() { gui->switch_page(PAGE_MAIN); });
  connect(ui.connect_button, &QAbstractButton::clicked, this,
          &page_network::slot_connect);
  connect(ui.connect_login_edit, &QLineEdit::returnPressed, this,
          &page_network::slot_connect);
  connect(ui.connect_password_edit, &QLineEdit::returnPressed, this,
          &page_network::slot_connect);
  connect(ui.connect_confirm_password_edit, &QLineEdit::returnPressed, this,
          &page_network::slot_connect);

  ui.lan_label->setText(_("Internet servers:"));
  ui.wan_label->setText(_("Local servers:"));

  ui.connect_host_edit->setText(server_host);
  ui.connect_port_edit->setText(QString::number(server_port));
  ui.connect_login_edit->setText(user_name);
  ui.connect_password_edit->setDisabled(true);
  ui.connect_confirm_password_edit->setDisabled(true);
  setLayout(ui.gridLayout);
}

page_network::~page_network() {}

/**********************************************************************/ /**
   Update network page connection state.
 **************************************************************************/
void page_network::set_connection_state(enum connection_state state)
{
  switch (state) {
  case LOGIN_TYPE:
    king->set_status_bar("");
    ui.connect_password_edit->setText("");
    ui.connect_password_edit->setDisabled(true);
    ui.connect_confirm_password_edit->setText("");
    ui.connect_confirm_password_edit->setDisabled(true);
    break;
  case NEW_PASSWORD_TYPE:
    ui.connect_password_edit->setText("");
    ui.connect_confirm_password_edit->setText("");
    ui.connect_password_edit->setDisabled(false);
    ui.connect_confirm_password_edit->setDisabled(false);
    ui.connect_password_edit->setFocus(Qt::OtherFocusReason);
    break;
  case ENTER_PASSWORD_TYPE:
    ui.connect_password_edit->setText("");
    ui.connect_confirm_password_edit->setText("");
    ui.connect_password_edit->setDisabled(false);
    ui.connect_confirm_password_edit->setDisabled(true);
    ui.connect_password_edit->setFocus(Qt::OtherFocusReason);

    break;
  case WAITING_TYPE:
    king->set_status_bar("");
    break;
  }

  connection_status = state;
}

/**********************************************************************/ /**
   Updates list of servers in network page in proper QTableViews
 **************************************************************************/
void page_network::update_server_list(enum server_scan_type sstype,
                                      const struct server_list *list)
{
  QTableWidget *sel = NULL;
  QString host, portstr;
  int port;
  int row;
  int old_row_count;

  switch (sstype) {
  case SERVER_SCAN_LOCAL:
    sel = ui.lan_widget;
    break;
  case SERVER_SCAN_GLOBAL:
    sel = ui.wan_widget;
    break;
  default:
    break;
  }

  if (!sel) {
    return;
  }

  if (!list) {
    return;
  }

  host = ui.connect_host_edit->text();
  portstr = ui.connect_port_edit->text();
  port = portstr.toInt();
  old_row_count = sel->rowCount();
  sel->clearContents();
  row = 0;
  server_list_iterate(list, pserver)
  {
    char buf[20];
    int tmp;
    QString tstring;

    if (old_row_count <= row) {
      sel->insertRow(row);
    }

    if (pserver->humans >= 0) {
      fc_snprintf(buf, sizeof(buf), "%d", pserver->humans);
    } else {
      strncpy(buf, _("Unknown"), sizeof(buf) - 1);
    }

    tmp = pserver->port;
    tstring = QString::number(tmp);

    for (int col = 0; col < 6; col++) {
      QTableWidgetItem *item;

      item = new QTableWidgetItem();

      switch (col) {
      case 0:
        item->setText(pserver->host);
        break;
      case 1:
        item->setText(tstring);
        break;
      case 2:
        item->setText(pserver->version);
        break;
      case 3:
        item->setText(_(pserver->state));
        break;
      case 4:
        item->setText(buf);
        break;
      case 5:
        item->setText(pserver->message);
        break;
      default:
        break;
      }
      sel->setItem(row, col, item);
    }

    if (host == pserver->host && port == pserver->port) {
      sel->selectRow(row);
    }

    row++;
  }
  server_list_iterate_end;

  /* Remove unneeded rows, if there are any */
  while (old_row_count - row > 0) {
    sel->removeRow(old_row_count - 1);
    old_row_count--;
  }
}

/**********************************************************************/ /**
   Callback function for when there's an error in the server scan.
 **************************************************************************/
void server_scan_error(struct server_scan *scan, const char *message)
{
  qtg_version_message(message);
  log_error("%s", message);
  qobject_cast<page_network *>(king()->pages[PAGE_NETWORK])
      ->destroy_server_scans();
}

/**********************************************************************/ /**
   Free the server scans.
 **************************************************************************/
void page_network::destroy_server_scans(void)
{
  if (meta_scan) {
    server_scan_finish(meta_scan);
    meta_scan = NULL;
  }

  if (meta_scan_timer != NULL) {
    meta_scan_timer->stop();
    meta_scan_timer->disconnect();
    delete meta_scan_timer;
    meta_scan_timer = NULL;
  }

  if (lan_scan) {
    server_scan_finish(lan_scan);
    lan_scan = NULL;
  }

  if (lan_scan_timer != NULL) {
    lan_scan_timer->stop();
    lan_scan_timer->disconnect();
    delete lan_scan_timer;
    lan_scan_timer = NULL;
  }
}

/**********************************************************************/ /**
   Stop and restart the metaserver and lan server scans.
 **************************************************************************/
void page_network::update_network_lists(void)
{
  destroy_server_scans();

  lan_scan_timer = new QTimer(this);
  lan_scan = server_scan_begin(SERVER_SCAN_LOCAL, server_scan_error);
  connect(lan_scan_timer, &QTimer::timeout, this,
          &page_network::slot_lan_scan);
  lan_scan_timer->start(500);

  meta_scan_timer = new QTimer(this);
  meta_scan = server_scan_begin(SERVER_SCAN_GLOBAL, server_scan_error);
  connect(meta_scan_timer, &QTimer::timeout, this,
          &page_network::slot_meta_scan);
  meta_scan_timer->start(800);
}

/**********************************************************************/ /**
   This function updates the list of servers every so often.
 **************************************************************************/
bool page_network::check_server_scan(server_scan *scan_data)
{
  struct server_scan *scan = scan_data;
  enum server_scan_status stat;

  if (!scan) {
    return false;
  }

  stat = server_scan_poll(scan);

  if (stat >= SCAN_STATUS_PARTIAL) {
    enum server_scan_type type;
    struct srv_list *srvrs;

    type = server_scan_get_type(scan);
    srvrs = server_scan_get_list(scan);
    update_server_list(type, srvrs->servers);
  }

  if (stat == SCAN_STATUS_ERROR || stat == SCAN_STATUS_DONE) {
    return false;
  }

  return true;
}

/**********************************************************************/ /**
   Executes lan scan network
 **************************************************************************/
void page_network::slot_lan_scan()
{
  if (lan_scan_timer == NULL) {
    return;
  }
  check_server_scan(lan_scan);
}

/**********************************************************************/ /**
   Executes metaserver scan network
 **************************************************************************/
void page_network::slot_meta_scan()
{
  if (meta_scan_timer == NULL) {
    return;
  }
  check_server_scan(meta_scan);
}
/**********************************************************************/ /**
   Configure the dialog depending on what type of authentication request the
   server is making.
 **************************************************************************/
void page_network::handle_authentication_req(enum authentication_type type,
                                             const char *message)
{
  king->set_status_bar(QString::fromUtf8(message));
  destroy_server_scans();

  switch (type) {
  case AUTH_NEWUSER_FIRST:
  case AUTH_NEWUSER_RETRY:
    set_connection_state(NEW_PASSWORD_TYPE);
    return;
  case AUTH_LOGIN_FIRST:
    /* if we magically have a password already present in 'password'
     * then, use that and skip the password entry dialog */
    if (password[0] != '\0') {
      struct packet_authentication_reply reply;

      sz_strlcpy(reply.password, password);
      send_packet_authentication_reply(&client.conn, &reply);
      return;
    } else {
      set_connection_state(ENTER_PASSWORD_TYPE);
    }

    return;
  case AUTH_LOGIN_RETRY:
    set_connection_state(ENTER_PASSWORD_TYPE);
    return;
  }

  log_error("Unsupported authentication type %d: %s.", type, message);
}

/**********************************************************************/ /**
   If on the network page, switch page to the login page (with new server
   and port). if on the login page, send connect and/or authentication
   requests to the server.
 **************************************************************************/
void page_network::slot_connect()
{
  char errbuf[512];
  struct packet_authentication_reply reply;
  QByteArray ba_bytes;

  switch (connection_status) {
  case LOGIN_TYPE:
    user_name = ui.connect_login_edit->text().toLocal8Bit();
    server_host = ui.connect_host_edit->text().toLocal8Bit();
    server_port = ui.connect_port_edit->text().toInt();

    if (connect_to_server(user_name, server_host, server_port, errbuf,
                          sizeof(errbuf))
        != -1) {
    } else {
      king->set_status_bar(QString::fromUtf8(errbuf));
      output_window_append(ftc_client, errbuf);
    }

    return;
  case NEW_PASSWORD_TYPE:
    ba_bytes = ui.connect_password_edit->text().toLatin1();
    sz_strlcpy(password, ba_bytes.data());
    ba_bytes = ui.connect_confirm_password_edit->text().toLatin1();
    sz_strlcpy(reply.password, ba_bytes.data());

    if (strncmp(reply.password, password, MAX_LEN_NAME) == 0) {
      password[0] = '\0';
      send_packet_authentication_reply(&client.conn, &reply);
      set_connection_state(WAITING_TYPE);
    } else {
      king->set_status_bar(_("Passwords don't match, enter password."));
      set_connection_state(NEW_PASSWORD_TYPE);
    }

    return;
  case ENTER_PASSWORD_TYPE:
    ba_bytes = ui.connect_password_edit->text().toLatin1();
    sz_strlcpy(reply.password, ba_bytes.data());
    send_packet_authentication_reply(&client.conn, &reply);
    set_connection_state(WAITING_TYPE);
    return;
  case WAITING_TYPE:
    return;
  }

  log_error("Unsupported connection status: %d", connection_status);
}

/**********************************************************************/ /**

 **************************************************************************/
void page_network::slot_selection_changed(const QItemSelection &selected,
                                          const QItemSelection &deselected)
{

  QModelIndexList indexes = selected.indexes();
  QStringList sl;
  QModelIndex index;
  QTableWidgetItem *item;
  QItemSelectionModel *tw;

  const struct server *pserver = NULL;
  int k, col, n;
  struct srv_list *srvrs;

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  ui.connect_host_edit->setText(index.data().toString());
  index = indexes.at(1);
  ui.connect_port_edit->setText(index.data().toString());

  tw = qobject_cast<QItemSelectionModel *>(sender());

  if (tw == ui.lan_widget->selectionModel()) {
    ui.wan_widget->clearSelection();
  } else {
    ui.lan_widget->clearSelection();
  }

  srvrs = server_scan_get_list(meta_scan);
  if (srvrs->servers) {
    pserver = server_list_get(srvrs->servers, index.row());
  }
  if (!pserver || !pserver->players) {
    return;
  }
  n = pserver->nplayers;
  ui.info_widget->clearContents();
  ui.info_widget->setRowCount(0);
  for (k = 0; k < n; k++) {
    ui.info_widget->insertRow(k);
    for (col = 0; col < 4; col++) {
      item = new QTableWidgetItem();
      switch (col) {
      case 0:
        item->setText(pserver->players[k].name);
        break;
      case 1:
        item->setText(pserver->players[k].type);
        break;
      case 2:
        item->setText(pserver->players[k].host);
        break;
      case 3:
        item->setText(pserver->players[k].nation);
        break;
      default:
        break;
      }
      ui.info_widget->setItem(k, col, item);
    }
  }
}
