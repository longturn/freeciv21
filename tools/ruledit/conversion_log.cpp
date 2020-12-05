/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QGridLayout>
#include <QPushButton>

// utility
#include "fcintl.h"

#include "conversion_log.h"

/**********************************************************************/ /**
   Setup conversion_log object
 **************************************************************************/
conversion_log::conversion_log() : QDialog()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QPushButton *close_button;
  int row = 0;

  area = new QTextEdit();
  area->setParent(this);
  area->setReadOnly(true);
  main_layout->addWidget(area, row++, 0);

  close_button = new QPushButton(QString::fromUtf8(R__("Close")), this);
  connect(close_button, &QAbstractButton::pressed, this,
          &conversion_log::close_now);
  main_layout->addWidget(close_button, row++, 0);

  setLayout(main_layout);
  setWindowTitle(QString::fromUtf8(R__("Old ruleset to a new format")));

  setVisible(false);
}

/**********************************************************************/ /**
   Add entry
 **************************************************************************/
void conversion_log::add(const char *msg)
{
  area->append(QString::fromUtf8(msg));
  setVisible(true);
}

/**********************************************************************/ /**
   User pushed close button
 **************************************************************************/
void conversion_log::close_now() { done(0); }
