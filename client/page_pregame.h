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
#pragma once

#include <QWidget>

class fc_client;
class pregame_options;
class pregamevote;
#include "pregameoptions.h" // before ui
#include "ui_page_pregame.h"

class page_pregame : public QWidget, private chat_listener {
  Q_OBJECT
public:
  page_pregame(QWidget *, fc_client *);
  ~page_pregame() override;
  void update_start_page();
  void set_rulesets(int num_rulesets, QStringList rulesets);
  void update_vote();
private slots:
  void start_page_menu(QPoint pos);
  void send_fake_chat_message(const QString &message);

private:
  void update_buttons();
  void chat_message_received(const QString &message,
                             const struct text_tag_list *tags) override;
  void slot_pregame_observe();
  void slot_pregame_start();
  Ui::FormPagePregame ui;
  fc_client *king; // serve the King
};
