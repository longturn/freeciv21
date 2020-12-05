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

#include "requirers_dlg.h"

/**********************************************************************/ /**
   Setup requirers_dlg object
 **************************************************************************/
requirers_dlg::requirers_dlg(ruledit_gui *ui_in) : QDialog()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QPushButton *close_button;
  int row = 0;

  ui = ui_in;

  area = new QTextEdit();
  area->setParent(this);
  area->setReadOnly(true);
  main_layout->addWidget(area, row++, 0);

  close_button = new QPushButton(QString::fromUtf8(R__("Close")), this);
  connect(close_button, &QAbstractButton::pressed, this,
          &requirers_dlg::close_now);
  main_layout->addWidget(close_button, row++, 0);

  setLayout(main_layout);
}

/**********************************************************************/ /**
   Clear text area
 **************************************************************************/
void requirers_dlg::clear(const char *title)
{
  char buffer[256];

  fc_snprintf(buffer, sizeof(buffer), R__("Removing %s"), title);

  setWindowTitle(QString::fromUtf8(buffer));
  area->clear();
}

/**********************************************************************/ /**
   Add requirer entry
 **************************************************************************/
void requirers_dlg::add(const char *msg)
{
  char buffer[2048];

  /* TRANS: %s could be any of a number of ruleset items (e.g., tech,
   * unit type, ... */
  fc_snprintf(buffer, sizeof(buffer), R__("Needed by %s"), msg);

  area->append(QString::fromUtf8(buffer));
}

/**********************************************************************/ /**
   User pushed close button
 **************************************************************************/
void requirers_dlg::close_now() { done(0); }
