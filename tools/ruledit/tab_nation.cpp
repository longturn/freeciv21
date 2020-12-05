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
#include <QLineEdit>
#include <QPushButton>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// common
#include "game.h"

// ruledit
#include "ruledit_qt.h"

#include "tab_nation.h"

/**********************************************************************/ /**
   Setup tab_nation object
 **************************************************************************/
tab_nation::tab_nation(ruledit_gui *ui_in) : QWidget()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QLabel *nationlist_label;
  int row = 0;

  ui = ui_in;

  main_layout->setSizeConstraint(QLayout::SetMaximumSize);

  via_include = new QRadioButton(QString::fromUtf8(R__("Use nationlist")));
  main_layout->addWidget(via_include, row++, 0);
  connect(via_include, &QAbstractButton::toggled, this,
          &tab_nation::nationlist_toggle);

  nationlist_label = new QLabel(QString::fromUtf8(R__("Nationlist")));
  nationlist_label->setParent(this);
  main_layout->addWidget(nationlist_label, row, 0);
  nationlist = new QLineEdit(this);
  main_layout->addWidget(nationlist, row++, 1);

  refresh();

  setLayout(main_layout);
}

/**********************************************************************/ /**
   Refresh the information.
 **************************************************************************/
void tab_nation::refresh()
{
  if (ui->data.nationlist == NULL) {
    via_include->setChecked(false);
    nationlist->setEnabled(false);
  } else {
    via_include->setChecked(true);
    nationlist->setText(ui->data.nationlist);
    nationlist->setEnabled(true);
  }
}

/**********************************************************************/ /**
   Flush information from widgets to stores where it can be saved from.
 **************************************************************************/
void tab_nation::flush_widgets()
{
  FC_FREE(ui->data.nationlist);

  if (via_include->isChecked()) {
    QByteArray nln_bytes;

    nln_bytes = nationlist->text().toUtf8();
    ui->data.nationlist = fc_strdup(nln_bytes.data());
  } else {
    ui->data.nationlist = NULL;
  }
}

/**********************************************************************/ /**
   Toggled nationlist include setting
 **************************************************************************/
void tab_nation::nationlist_toggle(bool checked)
{
  if (checked) {
    if (ui->data.nationlist_saved != NULL) {
      ui->data.nationlist = ui->data.nationlist_saved;
    } else {
      ui->data.nationlist = fc_strdup("default/nationlist.ruleset");
    }
  } else {
    QByteArray nln_bytes;

    FC_FREE(ui->data.nationlist_saved);
    nln_bytes = nationlist->text().toUtf8();
    ui->data.nationlist_saved = fc_strdup(nln_bytes.data());
    ui->data.nationlist = NULL;
  }

  refresh();
}
