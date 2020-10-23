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

#include "page_network.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>

// utility
#include "fcintl.h"

// common



// client
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"


// gui-qt
#include "dialogs.h"
#include "fc_client.h"
#include "pages.h"
#include "qtg_cxxside.h"

static enum connection_state connection_status;
static struct server_scan *meta_scan, *lan_scan;
static bool holding_srv_list_mutex = false;
static struct terrain *char2terrain(char ch);

/**********************************************************************/ /**
   Helper function for drawing map of savegames. Converts stored map char in
   savefile to proper terrain.
 **************************************************************************/
static struct terrain *char2terrain(char ch)
{
  if (ch == TERRAIN_UNKNOWN_IDENTIFIER) {
    return T_UNKNOWN;
  }
  terrain_type_iterate(pterrain)
  {
    if (pterrain->identifier_load == ch) {
      return pterrain;
    }
  }
  terrain_type_iterate_end;
  return nullptr;
}

/**********************************************************************/ /**
   Creates buttons and layouts for network page.
 **************************************************************************/
void fc_client::create_network_page(void)
{
  QHeaderView *header;
  QLabel *connect_msg;
  QLabel *lan_label;
  QPushButton *network_button;

  pages_layout[PAGE_NETWORK] = new QGridLayout;
  QVBoxLayout *page_network_layout = new QVBoxLayout;
  QGridLayout *page_network_grid_layout = new QGridLayout;
  QHBoxLayout *page_network_lan_layout = new QHBoxLayout;
  QHBoxLayout *page_network_wan_layout = new QHBoxLayout;

  connect_host_edit = new QLineEdit;
  connect_port_edit = new QLineEdit;
  connect_login_edit = new QLineEdit;
  connect_password_edit = new QLineEdit;
  connect_confirm_password_edit = new QLineEdit;

  connect_password_edit->setEchoMode(QLineEdit::Password);
  connect_confirm_password_edit->setEchoMode(QLineEdit::Password);

  connect_password_edit->setDisabled(true);
  connect_confirm_password_edit->setDisabled(true);
  connect_lan = new QWidget;
  connect_metaserver = new QWidget;
  lan_widget = new QTableWidget;
  wan_widget = new QTableWidget;
  info_widget = new QTableWidget;

  QStringList servers_list;
  servers_list << _("Server Name") << _("Port") << _("Version")
               << _("Status") << Q_("?count:Players") << _("Comment");
  QStringList server_info;
  server_info << _("Name") << _("Type") << _("Host") << _("Nation");

  lan_widget->setRowCount(0);
  lan_widget->setColumnCount(servers_list.count());
  lan_widget->verticalHeader()->setVisible(false);
  lan_widget->setAutoScroll(false);

  wan_widget->setRowCount(0);
  wan_widget->setColumnCount(servers_list.count());
  wan_widget->verticalHeader()->setVisible(false);
  wan_widget->setAutoScroll(false);

  info_widget->setRowCount(0);
  info_widget->setColumnCount(server_info.count());
  info_widget->verticalHeader()->setVisible(false);

  lan_widget->setHorizontalHeaderLabels(servers_list);
  lan_widget->setProperty("showGrid", "false");
  lan_widget->setProperty("selectionBehavior", "SelectRows");
  lan_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  lan_widget->setSelectionMode(QAbstractItemView::SingleSelection);

  wan_widget->setHorizontalHeaderLabels(servers_list);
  wan_widget->setProperty("showGrid", "false");
  wan_widget->setProperty("selectionBehavior", "SelectRows");
  wan_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  wan_widget->setSelectionMode(QAbstractItemView::SingleSelection);

  connect(wan_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &fc_client::slot_selection_changed);

  connect(lan_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &fc_client::slot_selection_changed);
  connect(wan_widget, &QTableWidget::itemDoubleClicked, this,
          &fc_client::slot_connect);
  connect(lan_widget, &QTableWidget::itemDoubleClicked, this,
          &fc_client::slot_connect);

  info_widget->setHorizontalHeaderLabels(server_info);
  info_widget->setProperty("selectionBehavior", "SelectRows");
  info_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  info_widget->setSelectionMode(QAbstractItemView::SingleSelection);
  info_widget->setProperty("showGrid", "false");
  info_widget->setAlternatingRowColors(true);

  header = lan_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  header = wan_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  header = info_widget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);

  QStringList label_names;
  label_names << _("Connect") << _("Port") << _("Login") << _("Password")
              << _("Confirm Password");

  for (int i = 0; i < label_names.count(); i++) {
    connect_msg = new QLabel;
    connect_msg->setText(label_names[i]);
    page_network_grid_layout->addWidget(connect_msg, i, 0, Qt::AlignHCenter);
  }

  page_network_grid_layout->addWidget(connect_host_edit, 0, 1);
  page_network_grid_layout->addWidget(connect_port_edit, 1, 1);
  page_network_grid_layout->addWidget(connect_login_edit, 2, 1);
  page_network_grid_layout->addWidget(connect_password_edit, 3, 1);
  page_network_grid_layout->addWidget(connect_confirm_password_edit, 4, 1);
  page_network_grid_layout->addWidget(info_widget, 0, 2, 5, 4);

  network_button = new QPushButton(_("Refresh"));
  QObject::connect(network_button, &QAbstractButton::clicked, this,
                   &fc_client::update_network_lists);
  page_network_grid_layout->addWidget(network_button, 5, 0);

  network_button = new QPushButton(_("Cancel"));
  QObject::connect(network_button, &QPushButton::clicked,
                   [this]() { switch_page(PAGE_MAIN); });
  page_network_grid_layout->addWidget(network_button, 5, 2, 1, 1);

  network_button = new QPushButton(_("Connect"));
  page_network_grid_layout->addWidget(network_button, 5, 5, 1, 1);
  connect(network_button, &QAbstractButton::clicked, this,
          &fc_client::slot_connect);
  connect(connect_login_edit, &QLineEdit::returnPressed, this,
          &fc_client::slot_connect);
  connect(connect_password_edit, &QLineEdit::returnPressed, this,
          &fc_client::slot_connect);
  connect(connect_confirm_password_edit, &QLineEdit::returnPressed, this,
          &fc_client::slot_connect);

  connect_lan->setLayout(page_network_lan_layout);
  connect_metaserver->setLayout(page_network_wan_layout);
  page_network_lan_layout->addWidget(lan_widget, 0);
  page_network_wan_layout->addWidget(wan_widget, 1);
  lan_label = new QLabel(_("Internet servers:"));
  page_network_layout->addWidget(lan_label, 1);
  page_network_layout->addWidget(wan_widget, 10);
  lan_label = new QLabel(_("Local servers:"));
  page_network_layout->addWidget(lan_label, 1);
  page_network_layout->addWidget(lan_widget, 1);
  page_network_grid_layout->setColumnStretch(3, 4);
  pages_layout[PAGE_NETWORK]->addLayout(page_network_layout, 1, 1);
  pages_layout[PAGE_NETWORK]->addLayout(page_network_grid_layout, 2, 1);
}

/**********************************************************************/ /**
   Update network page connection state.
 **************************************************************************/
void fc_client::set_connection_state(enum connection_state state)
{
  switch (state) {
  case LOGIN_TYPE:
    set_status_bar("");
    connect_password_edit->setText("");
    connect_password_edit->setDisabled(true);
    connect_confirm_password_edit->setText("");
    connect_confirm_password_edit->setDisabled(true);
    break;
  case NEW_PASSWORD_TYPE:
    connect_password_edit->setText("");
    connect_confirm_password_edit->setText("");
    connect_password_edit->setDisabled(false);
    connect_confirm_password_edit->setDisabled(false);
    connect_password_edit->setFocus(Qt::OtherFocusReason);
    break;
  case ENTER_PASSWORD_TYPE:
    connect_password_edit->setText("");
    connect_confirm_password_edit->setText("");
    connect_password_edit->setDisabled(false);
    connect_confirm_password_edit->setDisabled(true);
    connect_password_edit->setFocus(Qt::OtherFocusReason);

    break;
  case WAITING_TYPE:
    set_status_bar("");
    break;
  }

  connection_status = state;
}

/**********************************************************************/ /**
   Updates list of servers in network page in proper QTableViews
 **************************************************************************/
void fc_client::update_server_list(enum server_scan_type sstype,
                                   const struct server_list *list)
{
  QTableWidget *sel = NULL;
  QString host, portstr;
  int port;
  int row;
  int old_row_count;

  switch (sstype) {
  case SERVER_SCAN_LOCAL:
    sel = lan_widget;
    break;
  case SERVER_SCAN_GLOBAL:
    sel = wan_widget;
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

  host = connect_host_edit->text();
  portstr = connect_port_edit->text();
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

  /* Main thread will finalize the scan later (or even concurrently) -
   * do not do anything here to cause double free or raze condition. */
}

/**********************************************************************/ /**
   Free the server scans.
 **************************************************************************/
void fc_client::destroy_server_scans(void)
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
void fc_client::update_network_lists(void)
{
  destroy_server_scans();

  lan_scan_timer = new QTimer(this);
  lan_scan = server_scan_begin(SERVER_SCAN_LOCAL, server_scan_error);
  connect(lan_scan_timer, &QTimer::timeout, this, &fc_client::slot_lan_scan);
  lan_scan_timer->start(500);

  meta_scan_timer = new QTimer(this);
  meta_scan = server_scan_begin(SERVER_SCAN_GLOBAL, server_scan_error);
  connect(meta_scan_timer, &QTimer::timeout, this,
          &fc_client::slot_meta_scan);
  meta_scan_timer->start(800);
}

/**********************************************************************/ /**
   This function updates the list of servers every so often.
 **************************************************************************/
bool fc_client::check_server_scan(server_scan *scan_data)
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
    fc_allocate_mutex(&srvrs->mutex);
    holding_srv_list_mutex = true;
    update_server_list(type, srvrs->servers);
    holding_srv_list_mutex = false;
    fc_release_mutex(&srvrs->mutex);
  }

  if (stat == SCAN_STATUS_ERROR || stat == SCAN_STATUS_DONE) {
    return false;
  }

  return true;
}

/**********************************************************************/ /**
   Executes lan scan network
 **************************************************************************/
void fc_client::slot_lan_scan()
{
  if (lan_scan_timer == NULL) {
    return;
  }
  check_server_scan(lan_scan);
}

/**********************************************************************/ /**
   Executes metaserver scan network
 **************************************************************************/
void fc_client::slot_meta_scan()
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
void fc_client::handle_authentication_req(enum authentication_type type,
                                          const char *message)
{
  set_status_bar(QString::fromUtf8(message));
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
void fc_client::slot_connect()
{
  char errbuf[512];
  struct packet_authentication_reply reply;
  QByteArray ba_bytes;

  switch (connection_status) {
  case LOGIN_TYPE:
    ba_bytes = connect_login_edit->text().toLocal8Bit();
    sz_strlcpy(user_name, ba_bytes.data());
    ba_bytes = connect_host_edit->text().toLocal8Bit();
    sz_strlcpy(server_host, ba_bytes.data());
    server_port = connect_port_edit->text().toInt();

    if (connect_to_server(user_name, server_host, server_port, errbuf,
                          sizeof(errbuf))
        != -1) {
    } else {
      set_status_bar(QString::fromUtf8(errbuf));
      output_window_append(ftc_client, errbuf);
    }

    return;
  case NEW_PASSWORD_TYPE:
    ba_bytes = connect_password_edit->text().toLatin1();
    sz_strlcpy(password, ba_bytes.data());
    ba_bytes = connect_confirm_password_edit->text().toLatin1();
    sz_strlcpy(reply.password, ba_bytes.data());

    if (strncmp(reply.password, password, MAX_LEN_NAME) == 0) {
      password[0] = '\0';
      send_packet_authentication_reply(&client.conn, &reply);
      set_connection_state(WAITING_TYPE);
    } else {
      set_status_bar(_("Passwords don't match, enter password."));
      set_connection_state(NEW_PASSWORD_TYPE);
    }

    return;
  case ENTER_PASSWORD_TYPE:
    ba_bytes = connect_password_edit->text().toLatin1();
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
   TODO SPLIT TO SCENARIO AND LOAD
 **************************************************************************/
void fc_client::slot_selection_changed(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{

  QModelIndexList indexes = selected.indexes();
  QStringList sl;
  QModelIndex index;
  QTableWidgetItem *item;
  QItemSelectionModel *tw;
  QVariant qvar;
  QString str_pixmap;

  client_pages i = current_page();
  const char *terr_name;
  const struct server *pserver = NULL;
  int ii = 0;
  int k, col, n, nat_y, nat_x;
  struct section_file *sf;
  struct srv_list *srvrs;
  QByteArray fn_bytes;

  if (indexes.isEmpty()) {
    return;
  }

  switch (i) {
  case PAGE_NETWORK:
    index = indexes.at(0);
    connect_host_edit->setText(index.data().toString());
    index = indexes.at(1);
    connect_port_edit->setText(index.data().toString());

    tw = qobject_cast<QItemSelectionModel *>(sender());

    if (tw == lan_widget->selectionModel()) {
      wan_widget->clearSelection();
    } else {
      lan_widget->clearSelection();
    }

    srvrs = server_scan_get_list(meta_scan);
    if (!holding_srv_list_mutex) {
      fc_allocate_mutex(&srvrs->mutex);
    }
    if (srvrs->servers) {
      pserver = server_list_get(srvrs->servers, index.row());
    }
    if (!holding_srv_list_mutex) {
      fc_release_mutex(&srvrs->mutex);
    }
    if (!pserver || !pserver->players) {
      return;
    }
    n = pserver->nplayers;
    info_widget->clearContents();
    info_widget->setRowCount(0);
    for (k = 0; k < n; k++) {
      info_widget->insertRow(k);
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
        info_widget->setItem(k, col, item);
      }
    }
    break;
  case PAGE_SCENARIO:
    index = indexes.at(0);
    qvar = index.data(Qt::UserRole);
    sl = qvar.toStringList();
    scenarios_text->setText(sl.at(0));
    if (sl.count() > 1) {
      scenarios_view->setText(sl.at(2));
      current_file = sl.at(1);
    }
    break;
  case PAGE_LOAD:
    index = indexes.at(0);
    qvar = index.data(Qt::UserRole);
    current_file = qvar.toString();
    if (show_preview->checkState() == Qt::Unchecked) {
      load_pix->setPixmap(*(new QPixmap));
      load_save_text->setText("");
      break;
    }
    fn_bytes = current_file.toLocal8Bit();
    if ((sf = secfile_load_section(fn_bytes.data(), "game", TRUE))) {
      const char *sname;
      bool sbool;
      int integer;
      QString final_str;
      QString pl_str = nullptr;
      int num_players = 0;
      int curr_player = 0;
      QByteArray pl_bytes;

      integer = secfile_lookup_int_default(sf, -1, "game.turn");
      if (integer >= 0) {
        final_str = QString("<b>") + _("Turn") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      if ((sf = secfile_load_section(fn_bytes.data(), "players", TRUE))) {
        integer = secfile_lookup_int_default(sf, -1, "players.nplayers");
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Players") + ":</b>" + " "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        num_players = integer;
      }
      for (int i = 0; i < num_players; i++) {
        pl_str = QString("player") + QString::number(i);
        pl_bytes = pl_str.toLocal8Bit();
        if ((sf = secfile_load_section(fn_bytes.data(), pl_bytes.data(),
                                       true))) {
          if (!(sbool = secfile_lookup_bool_default(
                    sf, true, "player%d.unassigned_user", i))) {
            curr_player = i;
            break;
          }
        }
      }
      /* Break case (and return) if no human player found */
      if (pl_str == nullptr) {
        load_save_text->setText(final_str);
        break;
      }

      /* Information about human player */
      pl_bytes = pl_str.toLocal8Bit();
      if ((sf = secfile_load_section(fn_bytes.data(), pl_bytes.data(),
                                     true))) {
        sname = secfile_lookup_str_default(sf, nullptr, "player%d.nation",
                                           curr_player);
        if (sname) {
          final_str = final_str + "<b>" + _("Nation") + ":</b> "
                      + QString(sname).toHtmlEscaped() + "<br>";
        }
        integer = secfile_lookup_int_default(sf, -1, "player%d.ncities",
                                             curr_player);
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Cities") + ":</b> "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        integer = secfile_lookup_int_default(sf, -1, "player%d.nunits",
                                             curr_player);
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Units") + ":</b> "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        integer =
            secfile_lookup_int_default(sf, -1, "player%d.gold", curr_player);
        if (integer >= 0) {
          final_str = final_str + "<b>" + _("Gold") + ":</b> "
                      + QString::number(integer).toHtmlEscaped() + "<br>";
        }
        nat_x = 0;
        for (nat_y = 0; nat_y > -1; nat_y++) {
          const char *line = secfile_lookup_str_default(
              sf, nullptr, "player%d.map_t%04d", curr_player, nat_y);
          if (line == nullptr) {
            break;
          }
          nat_x = strlen(line);
          str_pixmap = str_pixmap + line;
        }

        /* Reset terrain information */
        terrain_type_iterate(pterr) { pterr->identifier_load = '\0'; }
        terrain_type_iterate_end;

        /* Load possible terrains and their identifiers (chars) */
        if ((sf = secfile_load_section(fn_bytes.data(), "savefile", true)))
          while ((terr_name = secfile_lookup_str_default(
                      sf, NULL, "savefile.terrident%d.name", ii))
                 != NULL) {
            struct terrain *pterr = terrain_by_rule_name(terr_name);
            if (pterr != NULL) {
              const char *iptr = secfile_lookup_str_default(
                  sf, NULL, "savefile.terrident%d.identifier", ii);
              pterr->identifier_load = *iptr;
            }
            ii++;
          }

        /* Create image */
        QImage img(nat_x, nat_y, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::black);
        for (int a = 0; a < nat_x; a++) {
          for (int b = 0; b < nat_y; b++) {
            struct terrain *tr;
            struct rgbcolor *rgb;
            tr = char2terrain(str_pixmap.at(b * nat_x + a).toLatin1());
            if (tr != nullptr) {
              rgb = tr->rgb;
              QColor col;
              col.setRgb(rgb->r, rgb->g, rgb->b);
              img.setPixel(a, b, col.rgb());
            }
          }
        }
        if (img.width() > 1) {
          load_pix->setPixmap(QPixmap::fromImage(img).scaledToHeight(200));
        } else {
          load_pix->setPixmap(*(new QPixmap));
        }
        load_pix->setFixedSize(load_pix->pixmap()->width(),
                               load_pix->pixmap()->height());
        if ((sf = secfile_load_section(fn_bytes.data(), "research", TRUE))) {
          sname = secfile_lookup_str_default(
              sf, nullptr, "research.r%d.now_name", curr_player);
          if (sname) {
            final_str = final_str + "<b>" + _("Researching") + ":</b> "
                        + QString(sname).toHtmlEscaped();
          }
        }
      }
      load_save_text->setText(final_str);
    }
    break;
  default:
    break;
  }
}