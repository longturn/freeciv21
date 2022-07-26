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
#include <QPainter>
#include <QPushButton>

// common
#include "game.h"
#include "victory.h"
// client
#include "client_main.h"
#include "colors_common.h"
#include "spaceshipdlg_g.h"
// gui-qt
#include "fc_client.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "spaceshipdlg.h"

class QGridLayout;

namespace /* anonymous */ {
/**
 * Return the desired size of the spaceship canvas.
 */
QSize get_spaceship_dimensions()
{
  return 7 * get_spaceship_sprite(tileset, SPACESHIP_HABITATION)->size();
}

/**
 * Draw the spaceship onto the canvas.
 */
void put_spaceship(QPixmap *pcanvas, const struct player *pplayer)
{
  int i, x, y;
  const struct player_spaceship *ship = &pplayer->spaceship;
  const QPixmap *spr;
  struct tileset *t = tileset;

  spr = get_spaceship_sprite(t, SPACESHIP_HABITATION);
  const int w = spr->width(), h = spr->height();

  QPainter p(pcanvas);
  p.fillRect(0, 0, w * 7, h * 7,
             get_color(tileset, COLOR_SPACESHIP_BACKGROUND));

  for (i = 0; i < NUM_SS_MODULES; i++) {
    const int j = i / 3;
    const int k = i % 3;

    if ((k == 0 && j >= ship->habitation)
        || (k == 1 && j >= ship->life_support)
        || (k == 2 && j >= ship->solar_panels)) {
      continue;
    }
    x = modules_info[i].x * w / 4 - w / 2;
    y = modules_info[i].y * h / 4 - h / 2;

    spr = (k == 0   ? get_spaceship_sprite(t, SPACESHIP_HABITATION)
           : k == 1 ? get_spaceship_sprite(t, SPACESHIP_LIFE_SUPPORT)
                    : get_spaceship_sprite(t, SPACESHIP_SOLAR_PANEL));
    p.drawPixmap(x, y, *spr);
  }

  for (i = 0; i < NUM_SS_COMPONENTS; i++) {
    const int j = i / 2;
    const int k = i % 2;

    if ((k == 0 && j >= ship->fuel) || (k == 1 && j >= ship->propulsion)) {
      continue;
    }
    x = components_info[i].x * w / 4 - w / 2;
    y = components_info[i].y * h / 4 - h / 2;

    spr = ((k == 0) ? get_spaceship_sprite(t, SPACESHIP_FUEL)
                    : get_spaceship_sprite(t, SPACESHIP_PROPULSION));

    p.drawPixmap(x, y, *spr);

    if (k && ship->state == SSHIP_LAUNCHED) {
      spr = get_spaceship_sprite(t, SPACESHIP_EXHAUST);
      p.drawPixmap(x + w, y, *spr);
    }
  }

  for (i = 0; i < NUM_SS_STRUCTURALS; i++) {
    if (!BV_ISSET(ship->structure, i)) {
      continue;
    }
    x = structurals_info[i].x * w / 4 - w / 2;
    y = structurals_info[i].y * h / 4 - h / 2;

    spr = get_spaceship_sprite(t, SPACESHIP_STRUCTURAL);
    p.drawPixmap(x, y, *spr);
  }

  p.end();
}
} // anonymous namespace

/**
   Constructor for spaceship report
 */
ss_report::ss_report(struct player *pplayer)
{
  setAttribute(Qt::WA_DeleteOnClose);
  player = pplayer;
  can = new QPixmap(get_spaceship_dimensions());

  QGridLayout *layout = new QGridLayout;
  ss_pix_label = new QLabel;
  ss_pix_label->setPixmap(*can);
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

/**
   Destructor for spaceship report
 */
ss_report::~ss_report()
{
  queen()->removeRepoDlg(QStringLiteral("SPS"));
  delete can;
}

/**
   Initializes widget on game_tab_widget
 */
void ss_report::init()
{
  int index;
  queen()->gimmePlace(this, QStringLiteral("SPS"));
  index = queen()->addGameTab(this);
  queen()->game_tab_widget->setCurrentIndex(index);
  update_report();
}

/**
   Updates spaceship report
 */
void ss_report::update_report()
{
  QString ch;
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
  put_spaceship(can, player);
  ss_pix_label->setPixmap(*can);
  update();
}

/**
   Launch spaceship
 */
void ss_report::launch() { send_packet_spaceship_launch(&client.conn); }

/**
   Popup (or raise) the spaceship dialog for the given player.
 */
void popup_spaceship_dialog(struct player *pplayer)
{
  ss_report *ss_rep;
  int i;
  QWidget *w;

  if (client_is_global_observer()) {
    return;
  }
  if (!queen()->isRepoDlgOpen(QStringLiteral("SPS"))) {
    ss_rep = new ss_report(pplayer);
    ss_rep->init();
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("SPS"));
    fc_assert(i != -1);
    if (queen()->game_tab_widget->currentIndex() == i) {
      return;
    }
    w = queen()->game_tab_widget->widget(i);
    ss_rep = reinterpret_cast<ss_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(ss_rep);
  }
}

/**
   Close the spaceship dialog for the given player.
 */
void popdown_spaceship_dialog(struct player *pplayer)
{ // PORTME
}

/**
   Refresh (update) the spaceship dialog for the given player.
 */
void refresh_spaceship_dialog(struct player *pplayer)
{
  int i;
  ss_report *ss_rep;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("SPS"))) {
    return;
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("SPS"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    ss_rep = reinterpret_cast<ss_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(ss_rep);
    ss_rep->update_report();
  }
}

/**
   Close all spaceships dialogs
 */
void popdown_all_spaceships_dialogs()
{
  int i;
  ss_report *ss_rep;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("SPS"))) {
    return;
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("SPS"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    ss_rep = reinterpret_cast<ss_report *>(w);
    ss_rep->deleteLater();
  }
}
