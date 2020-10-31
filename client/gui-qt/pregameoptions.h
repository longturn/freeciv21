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

#include "ui_pregameoptions.h"

class pregame_options : public QWidget {
  Q_OBJECT

public:
  pregame_options(QWidget *parent);

  void set_rulesets(int num_rulesets, char **rulesets);
  void set_aifill(int aifill);
  void update_ai_level();
  void update_buttons();
private slots:
  void max_players_change(int i);
  void ailevel_change(int i);
  void ruleset_change(int i);
  void pick_nation();

private:
  Ui::FormPregameOptions ui;
};
