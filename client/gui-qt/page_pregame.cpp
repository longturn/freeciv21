/**************************************************************************
             ____             Copyright (c) 1996-2020 Freeciv21 and Freeciv
            /    \__          contributors. This file is part of Freeciv21.
|\         /    @   \   Freeciv21 is free software: you can redistribute it
\ \_______|    \  .:|>         and/or modify it under the terms of the GNU
 \      ##|    | \__/     General Public License  as published by the Free
  |    ####\__/   \   Software Foundation, either version 3 of the License,
  /  /  ##       \|                  or (at your option) any later version.
 /  /__________\  \                 You should have received a copy of the
 L_JJ           \__JJ      GNU General Public License along with Freeciv21.
                                 If not, see https://www.gnu.org/licenses/.
**************************************************************************/

#include "page_pregame.h"
// Qt
#include <QPainter>
#include <QScrollBar>
// utility
#include "fcintl.h"
// common
#include "chatline_common.h"
#include "colors_common.h"
#include "connectdlg_common.h"
#include "game.h"
// client
#include "client_main.h"
// gui-qt
#include "canvas.h"
#include "chatline.h"
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"
#include "icons.h"
#include "pregameoptions.h"
#include "sprite.h"
#include "voteinfo_bar.h"

page_pregame::page_pregame(QWidget *parent, fc_client *gui) : QWidget(parent)
{
  king = gui;
  ui.setupUi(this);

  QFont f;
  QStringList player_widget_list;

  ui.chat_line->setProperty("doomchat", true);

  ui.output_window->setReadOnly(false);
  f.setBold(true);
  ui.output_window->setFont(f);
  player_widget_list << _("Name") << _("Ready") << Q_("?player:Leader")
                     << _("Flag") << _("Border") << _("Nation") << _("Team")
                     << _("Host");
  ui.start_players_tree->setColumnCount(player_widget_list.count());
  ui.start_players_tree->setHeaderLabels(player_widget_list);
  ui.start_players_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  ui.start_players_tree->setProperty("selectionBehavior", "SelectRows");
  ui.start_players_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.start_players_tree->setRootIsDecorated(false);
  connect(ui.start_players_tree, &QWidget::customContextMenuRequested, this,
          &page_pregame::start_page_menu);
  ui.bdisc->setText(_("Disconnect"));
  ui.bdisc->setIcon(style()->standardPixmap(QStyle::SP_DialogCancelButton));
  connect(ui.bdisc, &QAbstractButton::clicked, gui,
          &fc_client::slot_disconnect);
  ui.bpick->setText(_("Pick Nation"));
  ui.bpick->setIcon(fc_icons::instance()->get_icon(QStringLiteral("flag")));
  connect(ui.bpick, &QAbstractButton::clicked, this,
          &page_pregame::slot_pick_nation);
  ui.bops->setText(_("Observe"));
  ui.bops->setIcon(
      fc_icons::instance()->get_icon(QStringLiteral("meeting-observer")));
  connect(ui.bops, &QAbstractButton::clicked, this,
          &page_pregame::slot_pregame_observe);
  ui.bstart->setText(_("Start"));
  ui.bstart->setIcon(style()->standardPixmap(QStyle::SP_DialogOkButton));
  connect(ui.bstart, &QAbstractButton::clicked, this,
          &page_pregame::slot_pregame_start);
  pre_vote = new pregamevote;
  // down_layout->addWidget(pre_vote, 4, 0, 1, 4);
  pre_vote->hide();
  chat_listener::listen();
  setLayout(ui.gridLayout);
}

page_pregame::~page_pregame() {}

void page_pregame::set_rulesets(int num_rulesets, char **rulesets)
{
  ui.pr_options->set_rulesets(num_rulesets, rulesets);
}

/**********************************************************************/ /**
   Updates start page (start page = client connected to server, but game not
   started)
 **************************************************************************/
void page_pregame::update_start_page()
{
  int conn_num, i;
  QVariant qvar, qvar2;
  bool is_ready;
  QString host, nation, leader, team, str;
  QPixmap *pixmap;
  QPainter p;
  struct sprite *psprite;
  QTreeWidgetItem *item;
  QTreeWidgetItem *item_r;
  QList<QTreeWidgetItem *> items;
  QList<QTreeWidgetItem *> recursed_items;
  QTreeWidgetItem *player_item;
  QTreeWidgetItem *global_item;
  QTreeWidgetItem *detach_item;
  int conn_id;
  conn_num = conn_list_size(game.est_connections);

  if (conn_num == 0) {
    return;
  }

  ui.start_players_tree->clear();
  qvar2 = 0;

  player_item = new QTreeWidgetItem();
  player_item->setText(0, Q_("?header:Players"));
  player_item->setData(0, Qt::UserRole, qvar2);

  i = 0;
  players_iterate(pplayer) { i++; }
  players_iterate_end;
  ui.pr_options->set_aifill(i);
  /**
   * Inserts playing players, observing custom players, and AI)
   */

  players_iterate(pplayer)
  {
    host = QLatin1String("");
    if (!player_has_flag(pplayer, PLRF_SCENARIO_RESERVED)) {
      conn_id = -1;
      conn_list_iterate(pplayer->connections, pconn)
      {
        if (pconn->playing == pplayer && !pconn->observer) {
          conn_id = pconn->id;
          host = pconn->addr;
          break;
        }
      }
      conn_list_iterate_end;
      if (is_barbarian(pplayer)) {
        continue;
      }
      if (is_ai(pplayer)) {
        is_ready = true;
      } else {
        is_ready = pplayer->is_ready;
      }

      if (pplayer->nation == NO_NATION_SELECTED) {
        nation = _("Random");

        if (pplayer->was_created) {
          leader = player_name(pplayer);
        } else {
          leader = QLatin1String("");
        }
      } else {
        nation = nation_adjective_for_player(pplayer);
        leader = player_name(pplayer);
      }

      if (pplayer->team) {
        team = team_name_translation(pplayer->team);
      } else {
        team = QLatin1String("");
      }

      item = new QTreeWidgetItem();
      for (int col = 0; col < 8; col++) {
        switch (col) {
        case 0:
          str = pplayer->username;

          if (is_ai(pplayer)) {
            str =
                str + " <"
                + (ai_level_translated_name(pplayer->ai_common.skill_level))
                + ">";
            item->setIcon(
                col, fc_icons::instance()->get_icon(QStringLiteral("ai")));
          } else {
            item->setIcon(col, fc_icons::instance()->get_icon(
                                   QStringLiteral("human")));
          }

          item->setText(col, str);
          qvar = QVariant::fromValue((void *) pplayer);
          qvar2 = 1;
          item->setData(0, Qt::UserRole, qvar2);
          item->setData(1, Qt::UserRole, qvar);
          break;
        case 1:
          if (is_ready) {
            item->setText(col, _("Yes"));
          } else {
            item->setText(col, _("No"));
          }
          break;
        case 2:
          item->setText(col, leader);
          break;
        case 3:
          if (!pplayer->nation) {
            break;
          }
          psprite = get_nation_flag_sprite(tileset, pplayer->nation);
          pixmap = psprite->pm;
          item->setData(col, Qt::DecorationRole, *pixmap);
          break;
        case 4:
          if (!player_has_color(tileset, pplayer)) {
            break;
          }
          pixmap = new QPixmap(
              ui.start_players_tree->header()->sectionSizeHint(col), 16);
          pixmap->fill(Qt::transparent);
          p.begin(pixmap);
          p.fillRect(pixmap->width() / 2 - 8, 0, 16, 16, Qt::black);
          p.fillRect(pixmap->width() / 2 - 7, 1, 14, 14,
                     get_player_color(tileset, pplayer)->qcolor);
          p.end();
          item->setData(col, Qt::DecorationRole, *pixmap);
          delete pixmap;
          break;
        case 5:
          item->setText(col, nation);
          break;
        case 6:
          item->setText(col, team);
          break;
        case 7:
          item->setText(col, host);
          break;
        default:
          break;
        }
      }

      /**
       * find any custom observers
       */
      recursed_items.clear();
      conn_list_iterate(pplayer->connections, pconn)
      {
        if (pconn->id == conn_id) {
          continue;
        }
        item_r = new QTreeWidgetItem();
        item_r->setText(0, pconn->username);
        item_r->setText(5, _("Observer"));
        item_r->setText(7, pconn->addr);
        recursed_items.append(item_r);
        item->addChildren(recursed_items);
      }
      conn_list_iterate_end;
      items.append(item);
    }
  }
  players_iterate_end;

  player_item->addChildren(items);
  ui.start_players_tree->insertTopLevelItem(0, player_item);

  /**
   * Insert global observers
   */
  items.clear();
  global_item = new QTreeWidgetItem();
  global_item->setText(0, _("Global observers"));
  qvar2 = 0;
  global_item->setData(0, Qt::UserRole, qvar2);

  conn_list_iterate(game.est_connections, pconn)
  {
    if (NULL != pconn->playing || !pconn->observer) {
      continue;
    }
    item = new QTreeWidgetItem();
    for (int col = 0; col < 8; col++) {
      switch (col) {
      case 0:
        item->setText(col, pconn->username);
        break;
      case 5:
        item->setText(col, _("Observer"));
        break;
      case 7:
        item->setText(col, pconn->addr);
        break;
      default:
        break;
      }
      items.append(item);
    }
  }
  conn_list_iterate_end;

  global_item->addChildren(items);
  ui.start_players_tree->insertTopLevelItem(1, global_item);
  items.clear();

  /**
   * Insert detached
   */
  detach_item = new QTreeWidgetItem();
  detach_item->setText(0, _("Detached"));
  qvar2 = 0;
  detach_item->setData(0, Qt::UserRole, qvar2);

  conn_list_iterate(game.all_connections, pconn)
  {
    if (NULL != pconn->playing || pconn->observer) {
      continue;
    }
    item = new QTreeWidgetItem();
    item->setText(0, pconn->username);
    item->setText(7, pconn->addr);
    items.append(item);
  }
  conn_list_iterate_end;

  detach_item->addChildren(items);
  ui.start_players_tree->insertTopLevelItem(2, detach_item);
  ui.start_players_tree->header()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  ui.start_players_tree->expandAll();
  update_buttons();
}

/**********************************************************************/ /**
   Updates observe button in case user started observing manually
 **************************************************************************/
void page_pregame::update_buttons()
{
  bool sensitive;
  QString text;

  /* Observe button */
  if (client_is_observer() || client_is_global_observer()) {
    ui.bops->setText(_("Don't Observe"));
  } else {
    ui.bops->setText(_("Observe"));
  }

  /* Ready button */
  if (can_client_control()) {
    sensitive = true;
    if (client_player()->is_ready) {
      text = _("Not ready");
    } else {
      int num_unready = 0;

      players_iterate(pplayer)
      {
        if (is_human(pplayer) && !pplayer->is_ready) {
          num_unready++;
        }
      }
      players_iterate_end;

      if (num_unready > 1) {
        text = _("Ready");
      } else {
        /* We are the last unready player so clicking here will
         * immediately start the game. */
        text = ("Start");
      }
    }
  } else {
    text = _("Start");
    if (can_client_access_hack() && client.conn.observer) {
      sensitive = true;
      players_iterate(plr)
      {
        if (is_human(plr)) {
          /* There's human controlled player(s) in game, so it's their
           * job to start the game. */
          sensitive = false;
          break;
        }
      }
      players_iterate_end;
    } else {
      sensitive = false;
    }
  }
  ui.bstart->setEnabled(sensitive);
  ui.bstart->setText(text);

  /* Nation button */
  sensitive = game.info.is_new_game && can_client_control();
  ui.bpick->setEnabled(sensitive);

  sensitive = game.info.is_new_game;
  ui.pr_options->setEnabled(sensitive);
  ui.pr_options->update_buttons();
  ui.pr_options->update_ai_level();
}

/**********************************************************************/ /**
   Context menu on some player, arg Qpoint specifies some pixel on screen
 **************************************************************************/
void page_pregame::start_page_menu(QPoint pos)
{
  QAction *action;
  QMenu *menu, *submenu_AI, *submenu_team;
  QPoint global_pos = ui.start_players_tree->mapToGlobal(pos);
  QString me, splayer, str, sp;
  bool need_empty_team;
  const char *level_cmd, *level_name;
  int level, count;
  player *selected_player;
  QVariant qvar, qvar2;

  me = client.conn.username;
  QTreeWidgetItem *item = ui.start_players_tree->itemAt(pos);

  menu = new QMenu(this);
  submenu_AI = new QMenu(this);
  submenu_team = new QMenu(this);
  if (!item) {
    return;
  }

  qvar = item->data(0, Qt::UserRole);
  qvar2 = item->data(1, Qt::UserRole);

  /**
   * qvar = 0 -> selected label -> do nothing
   * qvar = 1 -> selected player (stored in qvar2)
   */

  selected_player = NULL;
  if (qvar == 0) {
    return;
  }
  if (qvar == 1) {
    selected_player = (player *) qvar2.value<void *>();
  }

  players_iterate(pplayer)
  {
    if (selected_player && selected_player == pplayer) {
      splayer = QString(pplayer->name);
      sp = "\"" + splayer + "\"";
      if (me != splayer) {
        str = QString(_("Observe"));
        action = new QAction(str, ui.start_players_tree);
        str = "/observe " + sp;
        QObject::connect(action, &QAction::triggered,
                         [this, str]() { send_fake_chat_message(str); });
        menu->addAction(action);

        if (ALLOW_CTRL <= client.conn.access_level) {
          str = QString(_("Remove player"));
          action = new QAction(str, ui.start_players_tree);
          str = "/remove " + sp;
          QObject::connect(action, &QAction::triggered,
                           [this, str]() { send_fake_chat_message(str); });
          menu->addAction(action);
        }
        str = QString(_("Take this player"));
        action = new QAction(str, ui.start_players_tree);
        str = "/take " + sp;
        QObject::connect(action, &QAction::triggered,
                         [this, str]() { send_fake_chat_message(str); });
        menu->addAction(action);
      }

      if (can_conn_edit_players_nation(&client.conn, pplayer)) {
        str = QString(_("Pick nation"));
        action = new QAction(str, ui.start_players_tree);
        str = QString(player_name(pplayer)); /* PICK is a key */
        QObject::connect(action, &QAction::triggered, [str]() {
          QString splayer;
          players_iterate(pplayer)
          {
            splayer = QString(pplayer->name);
            if (!splayer.compare(str)) {
              popup_races_dialog(pplayer);
            }
          }
          players_iterate_end;
        });
        menu->addAction(action);
      }

      if (is_ai(pplayer)) {
        /**
         * Set AI difficulty submenu
         */
        if (ALLOW_CTRL <= client.conn.access_level) {
          submenu_AI->setTitle(_("Set difficulty"));
          menu->addMenu(submenu_AI);

          for (level = 0; level < AI_LEVEL_COUNT; level++) {
            if (is_settable_ai_level(static_cast<ai_level>(level))) {
              level_name =
                  ai_level_translated_name(static_cast<ai_level>(level));
              level_cmd = ai_level_cmd(static_cast<ai_level>(level));
              action =
                  new QAction(QString(level_name), ui.start_players_tree);
              str = "/" + QString(level_cmd) + " " + sp;
              QObject::connect(action, &QAction::triggered, [this, str]() {
                send_fake_chat_message(str);
              });
              submenu_AI->addAction(action);
            }
          }
        }
      }

      /**
       * Put to Team X submenu
       */
      if (pplayer && game.info.is_new_game) {
        menu->addMenu(submenu_team);
        submenu_team->setTitle(_("Put on team"));
        menu->addMenu(submenu_team);
        count = pplayer->team ? player_list_size(team_members(pplayer->team))
                              : 0;
        need_empty_team = (count != 1);
        team_slots_iterate(tslot)
        {
          if (!team_slot_is_used(tslot)) {
            if (!need_empty_team) {
              continue;
            }
            need_empty_team = false;
          }
          str = team_slot_name_translation(tslot);
          action = new QAction(str, ui.start_players_tree);
          str = "/team" + sp + " \"" + QString(team_slot_rule_name(tslot))
                + "\"";
          QObject::connect(action, &QAction::triggered,
                           [this, str]() { send_fake_chat_message(str); });
          submenu_team->addAction(action);
        }
        team_slots_iterate_end;
      }

      if (ALLOW_CTRL <= client.conn.access_level && NULL != pplayer) {
        str = QString(_("Aitoggle player"));
        action = new QAction(str, ui.start_players_tree);
        str = "/aitoggle " + sp;
        QObject::connect(action, &QAction::triggered,
                         [this, str]() { send_fake_chat_message(str); });
        menu->addAction(action);
      }

      menu->popup(global_pos);
      return;
    }
  }
  players_iterate_end;
}

/************************************************************************/ /**
   Slot to send fake chat messages. Do not use in new code.
 ****************************************************************************/
void page_pregame::send_fake_chat_message(const QString &message)
{
  send_chat_message(message);
}

/************************************************************************/ /**
   Appends text to chat window
 ****************************************************************************/
void page_pregame::chat_message_received(const QString &message,
                                         const struct text_tag_list *tags)
{
  QColor col = ui.output_window->palette().color(QPalette::Text);
  QString str = apply_tags(message, tags, col);

  if (ui.output_window != NULL) {
    ui.output_window->append(str);
    ui.output_window->verticalScrollBar()->setSliderPosition(
        ui.output_window->verticalScrollBar()->maximum());
  }
}

/************************************************************************/ /**
   User clicked Observe button in START_PAGE
 ****************************************************************************/
void page_pregame::slot_pregame_observe()
{
  if (client_is_observer() || client_is_global_observer()) {
    if (game.info.is_new_game) {
      send_chat("/take -");
    } else {
      send_chat("/detach");
    }
    ui.bops->setText(_("Don't Observe"));
  } else {
    send_chat("/observe");
    ui.bops->setText(_("Observe"));
  }
}

/************************************************************************/ /**
   User clicked Start in START_PAGE
 ****************************************************************************/
void page_pregame::slot_pregame_start()
{
  if (can_client_control()) {
    dsend_packet_player_ready(&client.conn, player_number(client_player()),
                              !client_player()->is_ready);
  } else {
    dsend_packet_player_ready(&client.conn, 0, TRUE);
  }
}

/**********************************************************************/ /**
   Calls dialg selecting nations
 **************************************************************************/
void page_pregame::slot_pick_nation()
{
  popup_races_dialog(client_player());
}
