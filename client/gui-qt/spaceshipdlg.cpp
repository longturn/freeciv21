/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
// common
#include "game.h"
#include "victory.h"
// client
#include "client_main.h"
#include "mapview_common.h"
#include "spaceshipdlg_g.h"
// gui-qt
#include "canvas.h"
#include "fc_client.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "spaceshipdlg.h"

class QGridLayout;

/************************************************************************/ /**
   Constructor for spaceship report
 ****************************************************************************/
ss_report::ss_report(struct player *pplayer)
{
  int w, h;

  setAttribute(Qt::WA_DeleteOnClose);
  player = pplayer;
  get_spaceship_dimensions(&w, &h);
  can = qtg_canvas_create(w, h);

  QGridLayout *layout = new QGridLayout;
  ss_pix_label = new QLabel;
  ss_pix_label->setPixmap(can->map_pixmap);
  layout->addWidget(ss_pix_label, 0, 0, 3, 3);
  ss_label = new QLabel;
  layout->addWidget(ss_label, 0, 3, 3, 1);
  launch_button = new QPushButton(_("Launch"));
  connect(launch_button, &QAbstractButton::clicked, this,
          &ss_report::launch);
  layout->addWidget(launch_button, 4, 3, 1, 1);
  setLayout(layout);
  update_report();
}

/************************************************************************/ /**
   Destructor for spaceship report
 ****************************************************************************/
ss_report::~ss_report()
{
  queen()->remove_repo_dlg(QStringLiteral("SPS"));
  qtg_canvas_free(can);
}

/************************************************************************/ /**
   Initializes widget on game_tab_widget
 ****************************************************************************/
void ss_report::init()
{
  int index;
  queen()->gimme_place(this, QStringLiteral("SPS"));
  index = queen()->add_game_tab(this);
  queen()->game_tab_widget->setCurrentIndex(index);
  update_report();
}

/************************************************************************/ /**
   Updates spaceship report
 ****************************************************************************/
void ss_report::update_report()
{
  const char *ch;
  struct player_spaceship *pship;

  pship = &(player->spaceship);

  if (victory_enabled(VC_SPACERACE) && player == client.conn.playing
      && pship->state == SSHIP_STARTED && pship->success_rate > 0.0) {
    launch_button->setEnabled(true);
  } else {
    launch_button->setEnabled(false);
  }
  ch = get_spaceship_descr(&player->spaceship);
  ss_label->setText(ch);
  put_spaceship(can, 0, 0, player);
  ss_pix_label->setPixmap(can->map_pixmap);
  update();
}

/************************************************************************/ /**
   Launch spaceship
 ****************************************************************************/
void ss_report::launch() { send_packet_spaceship_launch(&client.conn); }

/************************************************************************/ /**
   Popup (or raise) the spaceship dialog for the given player.
 ****************************************************************************/
void popup_spaceship_dialog(struct player *pplayer)
{
  ss_report *ss_rep;
  int i;
  QWidget *w;

  if (client_is_global_observer()) {
    return;
  }
  if (!queen()->is_repo_dlg_open(QStringLiteral("SPS"))) {
    ss_rep = new ss_report(pplayer);
    ss_rep->init();
  } else {
    i = queen()->gimme_index_of(QStringLiteral("SPS"));
    fc_assert(i != -1);
    if (queen()->game_tab_widget->currentIndex() == i) {
      return;
    }
    w = queen()->game_tab_widget->widget(i);
    ss_rep = reinterpret_cast<ss_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(ss_rep);
  }
}

/************************************************************************/ /**
   Close the spaceship dialog for the given player.
 ****************************************************************************/
void popdown_spaceship_dialog(struct player *pplayer)
{ /* PORTME */
}

/************************************************************************/ /**
   Refresh (update) the spaceship dialog for the given player.
 ****************************************************************************/
void refresh_spaceship_dialog(struct player *pplayer)
{
  int i;
  ss_report *ss_rep;
  QWidget *w;

  if (!queen()->is_repo_dlg_open(QStringLiteral("SPS"))) {
    return;
  } else {
    i = queen()->gimme_index_of(QStringLiteral("SPS"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    ss_rep = reinterpret_cast<ss_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(ss_rep);
    ss_rep->update_report();
  }
}

/************************************************************************/ /**
   Close all spaceships dialogs
 ****************************************************************************/
void popdown_all_spaceships_dialogs()
{
  int i;
  ss_report *ss_rep;
  QWidget *w;

  if (!queen()->is_repo_dlg_open(QStringLiteral("SPS"))) {
    return;
  } else {
    i = queen()->gimme_index_of(QStringLiteral("SPS"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    ss_rep = reinterpret_cast<ss_report *>(w);
    ss_rep->deleteLater();
  }
}
