/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// Qt
#include <QWidget>
// client
#include "voteinfo_bar_g.h"

class QGridLayout;
class QLabel;
class QPushButton;

/***************************************************************************
  pregamevote class used for displaying vote bar in PAGE START
***************************************************************************/
class pregamevote : public QWidget {
  Q_OBJECT
public:
  explicit pregamevote(QWidget *parent = NULL);
  ~pregamevote();
  void update_vote();
  QLabel *label_text;
  QLabel *label_vote_text;
  QPushButton *vote_yes;
  QPushButton *vote_no;
  QPushButton *vote_abstain;
  QLabel *lab_yes;
  QLabel *lab_no;
  QLabel *lab_abstain;
  QLabel *voters;
  QGridLayout *layout;
public slots:
  void v_yes();
  void v_no();
  void v_abstain();
};

/***************************************************************************
  xvote class used for displaying vote bar in PAGE GAME
***************************************************************************/
class xvote : public pregamevote {
  Q_OBJECT
public:
  xvote(QWidget *parent);

protected:
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);
};
