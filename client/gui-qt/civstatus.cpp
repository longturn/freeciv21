/**************************************************************************
                  Copyright (c) 2021 Freeciv21 contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#include "civstatus.h"

#include "client_main.h"
#include "goto.h"
#include "player.h"
#include "text.h"
#include "tilespec.h"
#include "unitlist.h"
#include "canvas.h"
#include "mapview.h"
#include "fonts.h"
#include "fc_client.h"
#include "page_game.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

civstatus::civstatus(QWidget *parent) : fcwidget()
{
  QPixmap *spr;
  QLabel *label;
  QImage img, cropped_img;
  QPixmap pix;
  QRect crop;
  int icon_size;
  QFontMetrics *fm;
  QFont f;

  setParent(parent);
  layout = new QHBoxLayout;
  layout->setSizeConstraint(QLayout::SetMinimumSize);

  f = *fcFont::instance()->getFont(fonts::default_font);
  fm = new QFontMetrics(f);
  icon_size = fm->height() * 7 / 8;

  spr = get_tax_sprite(tileset, O_GOLD);
  label = new QLabel();
  label->setAlignment(Qt::AlignVCenter);
  img = spr->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  pix = QPixmap::fromImage(cropped_img.scaledToHeight(icon_size));
  label->setPixmap(pix);

  layout->addWidget(label);
  economyLabel.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  layout->addWidget(&economyLabel);

  spr = get_tax_sprite(tileset, O_SCIENCE);
  label = new QLabel();
  label->setAlignment(Qt::AlignBottom);
  img = spr->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  pix = QPixmap::fromImage(cropped_img.scaledToHeight(icon_size));
  label->setPixmap(pix);
  layout->addWidget(label);
  scienceLabel.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  layout->addWidget(&scienceLabel);
  unitLabel.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  layout->addWidget(&unitLabel);

  setLayout(layout);

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &civstatus::updateInfo);
  mw = new move_widget(this);
  timer->start(1000);
  updateInfo();
}

void civstatus::moveEvent(QMoveEvent *event)
{
  QPoint p;

  p = pos();
  king()->qt_settings.civstatus_x =
      static_cast<float>(p.x()) / queen()->mapview_wdg->width();
  king()->qt_settings.civstatus_y =
      static_cast<float>(p.y()) / queen()->mapview_wdg->height();
}

void civstatus::update_menu() {}

void civstatus::updateInfo()
{
  int a, c;
  bool b;

  if (!client_has_player()) {
    return;
  }

  // gold
  QString economyText =
      QString::number(client.conn.playing->economic.gold) + "("
      + ((player_get_expected_income(client.conn.playing) > 0) ? "+" : "")
      + QString::number(player_get_expected_income(client.conn.playing))
      + ")";

  // science
  int perturn = get_bulbs_per_turn(&a, &b, &c);
  int upkeep = client_player()->client.tech_upkeep;
  QString scienceText = QString::number(perturn - upkeep);

  // units waiting
  int unitsWaiting = 0;
  unit_list_iterate(client_player()->units, punit)
  {
    if (!can_unit_move_now(punit)) {
      ++unitsWaiting;
    }
  }
  unit_list_iterate_end;
  QString unitText = "";
  if (unitsWaiting) {
    unitText = QString("✋") + QString::number(unitsWaiting) + " "
               + _("units waiting");
  }

  economyLabel.setText(economyText);
  scienceLabel.setText(scienceText);
  unitLabel.setText(unitText);
  update();
}

civstatus::~civstatus() {}
