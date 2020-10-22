/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>

// client
#include "repodlgs_common.h"
#include "sprite.h"

// gui-qt
#include "fc_client.h"

#include "repodlgs.h"


/************************************************************************/ /**
   Constructor for endgame report
 ****************************************************************************/
endgame_report::endgame_report(const struct packet_endgame_report *packet)
    : QWidget()
{
  QGridLayout *end_layout = new QGridLayout;
  end_widget = new QTableWidget;
  unsigned int i;

  players = 0;
  const size_t col_num = packet->category_num + 3;
  QStringList slist;
  slist << _("Player") << _("Nation") << _("Score");
  for (i = 0; i < col_num - 3; i++) {
    slist << Q_(packet->category_name[i]);
  }
  end_widget->setColumnCount(slist.count());
  end_widget->setHorizontalHeaderLabels(slist);
  end_widget->setProperty("showGrid", "false");
  end_widget->setProperty("selectionBehavior", "SelectRows");
  end_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  end_widget->verticalHeader()->setVisible(false);
  end_widget->setSelectionMode(QAbstractItemView::SingleSelection);
  end_widget->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  end_layout->addWidget(end_widget, 1, 0, 5, 5);
  setLayout(end_layout);
}

/************************************************************************/ /**
   Destructor for endgame report
 ****************************************************************************/
endgame_report::~endgame_report() { gui()->remove_repo_dlg("END"); }

/************************************************************************/ /**
   Initializes place in tab for endgame report
 ****************************************************************************/
void endgame_report::init()
{
  gui()->gimme_place(this, "END");
  index = gui()->add_game_tab(this);
  gui()->game_tab_widget->setCurrentIndex(index);
}

/************************************************************************/ /**
   Refresh all widgets for economy report
 ****************************************************************************/
void endgame_report::update_report(
    const struct packet_endgame_player *packet)
{
  QTableWidgetItem *item;
  QPixmap *pix;
  unsigned int i;
  const struct player *pplayer = player_by_number(packet->player_id);
  const size_t col_num = packet->category_num + 3;
  end_widget->insertRow(players);
  for (i = 0; i < col_num; i++) {
    item = new QTableWidgetItem;
    switch (i) {
    case 0:
      item->setText(player_name(pplayer));
      break;
    case 1:
      pix = get_nation_flag_sprite(tileset, nation_of_player(pplayer))->pm;
      if (pix != NULL) {
        item->setData(Qt::DecorationRole, *pix);
      }
      break;
    case 2:
      item->setText(QString::number(packet->score));
      item->setTextAlignment(Qt::AlignHCenter);
      break;
    default:
      item->setText(QString::number(packet->category_score[i - 3]));
      item->setTextAlignment(Qt::AlignHCenter);
      break;
    }
    end_widget->setItem(players, i, item);
  }
  players++;
  end_widget->resizeRowsToContents();
}

/************************************************************************/ /**
   Show a dialog with player statistics at endgame.
 ****************************************************************************/
void endgame_report_dialog_start(const struct packet_endgame_report *packet)
{
  endgame_report *end_rep;
  end_rep = new endgame_report(packet);
  end_rep->init();
}

/************************************************************************/ /**
   Removes endgame report
 ****************************************************************************/
void popdown_endgame_report()
{
  int i;
  if (gui()->is_repo_dlg_open("END")) {
    i = gui()->gimme_index_of("END");
    fc_assert(i != -1);
    delete gui()->game_tab_widget->widget(i);
  }
}

/************************************************************************/ /**
   Popups endgame report to front if exists
 ****************************************************************************/
void popup_endgame_report()
{
  int i;
  if (gui()->is_repo_dlg_open("END")) {
    i = gui()->gimme_index_of("END");
    gui()->game_tab_widget->setCurrentIndex(i);
  }
}

/************************************************************************/ /**
   Received endgame report information about single player.
 ****************************************************************************/
void endgame_report_dialog_player(const struct packet_endgame_player *packet)
{
  int i;
  endgame_report *end_rep;
  QWidget *w;

  if (gui()->is_repo_dlg_open("END")) {
    i = gui()->gimme_index_of("END");
    fc_assert(i != -1);
    w = gui()->game_tab_widget->widget(i);
    end_rep = reinterpret_cast<endgame_report *>(w);
    end_rep->update_report(packet);
  }
}




