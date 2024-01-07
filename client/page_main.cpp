/*
 Copyright (c) 1996-2022 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "page_main.h"
// utility
#include "fcintl.h"
// common
#include "version.h"
// gui-qt
#include "fc_client.h"

page_main::page_main(QWidget *parent, fc_client *gui) : QWidget(parent)
{
  QString msgbuf;
  QString beta;
  ui.setupUi(this);
  ui.btut->setText(_("Tutorial"));
  ui.bstart->setText(_("Start new game"));
  ui.bscenario->setText(_("Start scenario game"));
  ui.boptions->setText(_("Options"));
  ui.bload->setText(_("Load saved game"));
  ui.bconnect->setText(_("Connect to network game"));
  ui.bquit->setText(_("Quit"));
  ui.baddon->setText(_("Add Ons"));

  connect(ui.btut, &QAbstractButton::clicked, gui,
          &fc_client::start_tutorial);
  connect(ui.bstart, &QAbstractButton::clicked, gui,
          &fc_client::start_new_game);
  connect(ui.baddon, &QAbstractButton::clicked, gui,
          &fc_client::load_modpack);
  connect(ui.bscenario, &QPushButton::clicked,
          [gui]() { gui->switch_page(PAGE_SCENARIO); });
  connect(ui.boptions, &QAbstractButton::clicked,
          [=]() { popup_client_options(); });
  connect(ui.bquit, &QAbstractButton::clicked, qApp, &QApplication::quit);
  connect(ui.bconnect, &QPushButton::clicked,
          [gui]() { gui->switch_page(PAGE_NETWORK); });
  connect(ui.bload, &QPushButton::clicked,
          [gui]() { gui->switch_page(PAGE_LOAD); });

  // TRANS: "version 2.6.0, Qt client"
  msgbuf = QString(_("%1%2, Qt client"))
               .arg(word_version())
               .arg(freeciv21_version());
  ui.lversion->setText(QString(msgbuf) + beta);
  setLayout(ui.gridLayout);
}

page_main::~page_main() = default;
