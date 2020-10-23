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
#include <QAction>
#include <QGridLayout>
#include <QHeaderView>
#include <QPainter>
#include <QSplitter>
#include <QTreeWidget>
// utility
#include "fcintl.h"
// common
#include "colors_common.h"
#include "connectdlg_common.h"
#include "game.h"
// client
#include "client_main.h"
// gui-qt
#include "canvas.h"
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"
#include "icons.h"
#include "sprite.h"
#include "voteinfo_bar.h"

/**********************************************************************/ /**
   Creates buttons and layouts for start page.
 **************************************************************************/
void fc_client::create_start_page()
{
  QPushButton *but;
  QSplitter *splitter;
  QGridLayout *up_layout;
  QGridLayout *down_layout;
  QWidget *up_widget;
  QWidget *down_widget;
  QFont f;

  QStringList player_widget_list;
  pages_layout[PAGE_START] = new QGridLayout;
  up_layout = new QGridLayout;
  down_layout = new QGridLayout;
  start_players_tree = new QTreeWidget;
  pr_options = new pregame_options(this);
  chat_line = new chat_input;
  chat_line->setProperty("doomchat", true);
  output_window = new QTextEdit;
  output_window->setReadOnly(false);
  f.setBold(true);
  output_window->setFont(f);

  pr_options->init();
  player_widget_list << _("Name") << _("Ready") << Q_("?player:Leader")
                     << _("Flag") << _("Border") << _("Nation") << _("Team")
                     << _("Host");

  start_players_tree->setColumnCount(player_widget_list.count());
  start_players_tree->setHeaderLabels(player_widget_list);
  start_players_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  start_players_tree->setProperty("selectionBehavior", "SelectRows");
  start_players_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  start_players_tree->setRootIsDecorated(false);

  connect(start_players_tree,
          SIGNAL(customContextMenuRequested(const QPoint &)),
          SLOT(start_page_menu(QPoint)));

  up_layout->addWidget(start_players_tree, 0, 0, 3, 6);
  up_layout->addWidget(pr_options, 0, 6, 3, 2);
  but = new QPushButton;
  but->setText(_("Disconnect"));
  but->setIcon(style()->standardPixmap(QStyle::SP_DialogCancelButton));
  QObject::connect(but, &QAbstractButton::clicked, this,
                   &fc_client::slot_disconnect);
  down_layout->addWidget(but, 5, 4);
  nation_button = new QPushButton;
  nation_button->setText(_("Pick Nation"));
  nation_button->setIcon(fc_icons::instance()->get_icon("flag"));
  down_layout->addWidget(nation_button, 5, 5);
  QObject::connect(nation_button, &QAbstractButton::clicked, this,
                   &fc_client::slot_pick_nation);

  obs_button = new QPushButton;
  obs_button->setText(_("Observe"));
  obs_button->setIcon(fc_icons::instance()->get_icon("meeting-observer"));
  down_layout->addWidget(obs_button, 5, 6);
  QObject::connect(obs_button, &QAbstractButton::clicked, this,
                   &fc_client::slot_pregame_observe);
  start_button = new QPushButton;
  start_button->setText(_("Start"));
  start_button->setIcon(style()->standardPixmap(QStyle::SP_DialogOkButton));
  down_layout->addWidget(start_button, 5, 7);
  QObject::connect(start_button, &QAbstractButton::clicked, this,
                   &fc_client::slot_pregame_start);
  pre_vote = new pregamevote;

  down_layout->addWidget(pre_vote, 4, 0, 1, 4);
  down_layout->addWidget(chat_line, 5, 0, 1, 4);
  down_layout->addWidget(output_window, 3, 0, 1, 8);
  splitter = new QSplitter;
  up_widget = new QWidget();
  down_widget = new QWidget();
  up_widget->setLayout(up_layout);
  down_widget->setLayout(down_layout);
  splitter->addWidget(up_widget);
  splitter->addWidget(down_widget);
  splitter->setOrientation(Qt::Vertical);
  pages_layout[PAGE_START]->addWidget(splitter);
}

/**********************************************************************/ /**
   Updates start page (start page = client connected to server, but game not
   started)
 **************************************************************************/
void fc_client::update_start_page()
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

  start_players_tree->clear();
  qvar2 = 0;

  player_item = new QTreeWidgetItem();
  player_item->setText(0, Q_("?header:Players"));
  player_item->setData(0, Qt::UserRole, qvar2);

  i = 0;
  players_iterate(pplayer) { i++; }
  players_iterate_end;
  gui()->pr_options->set_aifill(i);
  /**
   * Inserts playing players, observing custom players, and AI)
   */

  players_iterate(pplayer)
  {
    host = "";
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
          leader = "";
        }
      } else {
        nation = nation_adjective_for_player(pplayer);
        leader = player_name(pplayer);
      }

      if (pplayer->team) {
        team = team_name_translation(pplayer->team);
      } else {
        team = "";
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
            item->setIcon(col, fc_icons::instance()->get_icon("ai"));
          } else {
            item->setIcon(col, fc_icons::instance()->get_icon("human"));
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
              start_players_tree->header()->sectionSizeHint(col), 16);
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
  start_players_tree->insertTopLevelItem(0, player_item);

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
  start_players_tree->insertTopLevelItem(1, global_item);
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
  start_players_tree->insertTopLevelItem(2, detach_item);
  start_players_tree->header()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  start_players_tree->expandAll();
  update_buttons();
}


/**********************************************************************/ /**
   Updates observe button in case user started observing manually
 **************************************************************************/
void fc_client::update_buttons()
{
  bool sensitive;
  QString text;

  /* Observe button */
  if (client_is_observer() || client_is_global_observer()) {
    obs_button->setText(_("Don't Observe"));
  } else {
    obs_button->setText(_("Observe"));
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
  start_button->setEnabled(sensitive);
  start_button->setText(text);

  /* Nation button */
  sensitive = game.info.is_new_game && can_client_control();
  nation_button->setEnabled(sensitive);

  sensitive = game.info.is_new_game;
  pr_options->setEnabled(sensitive);

  gui()->pr_options->update_buttons();
  gui()->pr_options->update_ai_level();
}

/**********************************************************************/ /**
   Context menu on some player, arg Qpoint specifies some pixel on screen
 **************************************************************************/
void fc_client::start_page_menu(QPoint pos)
{
  QAction *action;
  QMenu *menu, *submenu_AI, *submenu_team;
  QPoint global_pos = start_players_tree->mapToGlobal(pos);
  QString me, splayer, str, sp;
  bool need_empty_team;
  const char *level_cmd, *level_name;
  int level, count;
  player *selected_player;
  QVariant qvar, qvar2;

  me = client.conn.username;
  QTreeWidgetItem *item = start_players_tree->itemAt(pos);

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
        action = new QAction(str, start_players_tree);
        str = "/observe " + sp;
        QObject::connect(action, &QAction::triggered,
                         [this, str]() { send_fake_chat_message(str); });
        menu->addAction(action);

        if (ALLOW_CTRL <= client.conn.access_level) {
          str = QString(_("Remove player"));
          action = new QAction(str, start_players_tree);
          str = "/remove " + sp;
          QObject::connect(action, &QAction::triggered,
                           [this, str]() { send_fake_chat_message(str); });
          menu->addAction(action);
        }
        str = QString(_("Take this player"));
        action = new QAction(str, start_players_tree);
        str = "/take " + sp;
        QObject::connect(action, &QAction::triggered,
                         [this, str]() { send_fake_chat_message(str); });
        menu->addAction(action);
      }

      if (can_conn_edit_players_nation(&client.conn, pplayer)) {
        str = QString(_("Pick nation"));
        action = new QAction(str, start_players_tree);
        str = "PICK:" + QString(player_name(pplayer)); /* PICK is a key */
        QObject::connect(action, &QAction::triggered,
                         [this, str]() { send_fake_chat_message(str); });
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
              action = new QAction(QString(level_name), start_players_tree);
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
          action = new QAction(str, start_players_tree);
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
        action = new QAction(str, start_players_tree);
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

/**********************************************************************/ /**
   Calls dialg selecting nations
 **************************************************************************/
void fc_client::slot_pick_nation() { popup_races_dialog(client_player()); }

