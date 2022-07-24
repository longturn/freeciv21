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

// Qt
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>

// utility
#include "fcintl.h"

// common
#include "achievements.h"
#include "game.h"
#include "government.h"
#include "multipliers.h"
#include "specialist.h"
#include "style.h"

// server
#include "rssanity.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "rulesave.h"

#include "tab_misc.h"

/**
   Setup tab_misc object
 */
tab_misc::tab_misc(ruledit_gui *ui_in) : QWidget()
{
  QGridLayout *main_layout = new QGridLayout(this);
  QLabel *save_label;
  QLabel *save_ver_label;
  QLabel *name_label;
  QLabel *version_label;
  QPushButton *save_button;
  QPushButton *always_active_effects;
  QPushButton *all_effects;
  QPushButton *refresh_button;
  int row = 0;
  QTableWidgetItem *item;

  ui = ui_in;

  main_layout->setSizeConstraint(QLayout::SetMaximumSize);

  name_label = new QLabel(QString::fromUtf8(R__("Ruleset name")));
  name_label->setParent(this);
  main_layout->addWidget(name_label, row, 0);
  name = new QLineEdit(this);
  main_layout->addWidget(name, row++, 1);
  version_label = new QLabel(QString::fromUtf8(R__("Ruleset version")));
  version_label->setParent(this);
  main_layout->addWidget(version_label, row, 0);
  version = new QLineEdit(this);
  main_layout->addWidget(version, row++, 1);
  save_label = new QLabel(QString::fromUtf8(R__("Save to directory")));
  save_label->setParent(this);
  main_layout->addWidget(save_label, row, 0);
  savedir = new QLineEdit(this);
  savedir->setText(QStringLiteral("ruledit-tmp"));
  savedir->setFocus();
  main_layout->addWidget(savedir, row++, 1);
  save_ver_label =
      new QLabel(QString::fromUtf8(R__("Version suffix to directory name")));
  save_ver_label->setParent(this);
  main_layout->addWidget(save_ver_label, row, 0);
  savedir_version = new QRadioButton(this);
  main_layout->addWidget(savedir_version, row++, 1);
  save_button = new QPushButton(QString::fromUtf8(R__("Save now")), this);
  connect(save_button, &QAbstractButton::pressed, this, &tab_misc::save_now);
  main_layout->addWidget(save_button, row++, 1);
  always_active_effects =
      new QPushButton(QString::fromUtf8(R__("Always active Effects")), this);
  connect(always_active_effects, &QAbstractButton::pressed, this,
          &tab_misc::edit_aae_effects);
  main_layout->addWidget(always_active_effects, row++, 1);
  all_effects = new QPushButton(QString::fromUtf8(R__("All Effects")), this);
  connect(all_effects, &QAbstractButton::pressed, this,
          &tab_misc::edit_all_effects);
  main_layout->addWidget(all_effects, row++, 1);

  stats = new QTableWidget(this);
  stats->setColumnCount(8);
  stats->setRowCount(6);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Terrains")));
  stats->setItem(0, 0, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(0, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Resources")));
  stats->setItem(1, 0, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(1, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Tech Classes")));
  stats->setItem(2, 0, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(2, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Techs")));
  stats->setItem(3, 0, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(3, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Unit Classes")));
  stats->setItem(4, 0, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(4, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Unit Types")));
  stats->setItem(5, 0, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(5, 1, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Buildings")));
  stats->setItem(0, 3, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(0, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Nations")));
  stats->setItem(1, 3, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(1, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Styles")));
  stats->setItem(2, 3, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(2, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Specialists")));
  stats->setItem(3, 3, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(3, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Governments")));
  stats->setItem(4, 3, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(4, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Disasters")));
  stats->setItem(5, 3, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(5, 4, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Achievements")));
  stats->setItem(0, 6, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(0, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Extras")));
  stats->setItem(1, 6, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(1, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Bases")));
  stats->setItem(2, 6, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(2, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Roads")));
  stats->setItem(3, 6, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(3, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Goods")));
  stats->setItem(4, 6, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(4, 7, item);
  item = new QTableWidgetItem(QString::fromUtf8(RQ_("?stat:Multipliers")));
  stats->setItem(5, 6, item);
  item = new QTableWidgetItem(QStringLiteral("-"));
  stats->setItem(5, 7, item);
  stats->verticalHeader()->setVisible(false);
  stats->horizontalHeader()->setVisible(false);
  stats->setEditTriggers(QAbstractItemView::NoEditTriggers);
  main_layout->addWidget(stats, row++, 0, 1, 2);
  refresh_button =
      new QPushButton(QString::fromUtf8(R__("Refresh Stats")), this);
  connect(refresh_button, &QAbstractButton::pressed, this,
          &tab_misc::refresh_stats);
  main_layout->addWidget(refresh_button, row++, 0, 1, 2);

  // Stats never change except with experimental features. Hide useless
  // button.
  show_experimental(refresh_button);

  refresh();

  setLayout(main_layout);
}

/**
   Refresh the information.
 */
void tab_misc::refresh()
{
  name->setText(game.control.name);
  version->setText(game.control.version);
  refresh_stats();
}

/**
   User entered savedir
 */
void tab_misc::save_now()
{
  char nameUTF8[MAX_LEN_NAME];
  QString full_dir;
  QByteArray ba_bytes;

  ui->flush_widgets();

  ba_bytes = name->text().toUtf8();
  qstrncpy(nameUTF8, ba_bytes.data(), sizeof(nameUTF8) - 1);

  if (nameUTF8[0] != '\0') {
    qstrncpy(game.control.name, nameUTF8, sizeof(game.control.name));
  }

  ba_bytes = version->text().toUtf8();
  qstrncpy(game.control.version, ba_bytes.data(),
           sizeof(game.control.version) - 1);

  if (!autoadjust_ruleset_data() || !sanity_check_ruleset_data(false)) {
    QMessageBox *box = new QMessageBox();

    box->setText(
        QStringLiteral("Current data fails sanity checks. Save anyway?"));
    box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box->exec();

    if (box->result() != QMessageBox::Yes) {
      return;
    }
  }

  if (savedir_version->isChecked()) {
    full_dir = savedir->text() + "-" + version->text();
  } else {
    full_dir = savedir->text();
  }

  ba_bytes = full_dir.toUtf8();
  save_ruleset(ba_bytes.data(), nameUTF8, &(ui->data));

  ui->display_msg(R__("Ruleset saved"));
}

/**
   Recalculate stats
 */
void tab_misc::refresh_stats()
{
  int row = 0;
  int count;
  int base_count;
  int road_count;

  count = 0;
  terrain_re_active_iterate(pterr) { count++; }
  terrain_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  stats->item(row++, 1)->setText(
      QString::number(game.control.num_resource_types));

  count = 0;
  tech_class_re_active_iterate(ptclass) { count++; }
  tech_class_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  count = 0;
  advance_re_active_iterate(padv) { count++; }
  advance_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  count = 0;
  unit_class_re_active_iterate(pclass) { count++; }
  unit_class_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  count = 0;
  unit_type_re_active_iterate(ptype) { count++; }
  unit_type_re_active_iterate_end;
  stats->item(row++, 1)->setText(QString::number(count));

  // Second column
  row = 0;
  count = 0;
  improvement_re_active_iterate(pimpr) { count++; }
  improvement_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  stats->item(row++, 4)->setText(QString::number(game.control.nation_count));

  count = 0;
  styles_re_active_iterate(pstyle) { count++; }
  styles_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  count = 0;
  specialist_type_re_active_iterate(pspe) { count++; }
  specialist_type_re_active_iterate_end;
  stats->item(row++, 4)->setText(QString::number(count));

  count = std::count_if(governments.begin(), governments.end(),
                        [](government &p) { return !p.ruledit_disabled; });

  stats->item(row++, 4)->setText(QString::number(count));

  stats->item(row++, 4)->setText(
      QString::number(game.control.num_disaster_types));

  // Third column
  row = 0;

  count = 0;
  achievements_re_active_iterate(pach) { count++; }
  achievements_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));

  count = 0;
  base_count = 0;
  road_count = 0;
  extra_type_re_active_iterate(pextra)
  {
    count++;
    if (is_extra_caused_by(pextra, EC_BASE)) {
      base_count++;
    }
    if (is_extra_caused_by(pextra, EC_ROAD)) {
      road_count++;
    }
  }
  extra_type_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));
  stats->item(row++, 7)->setText(QString::number(base_count));
  stats->item(row++, 7)->setText(QString::number(road_count));

  count = 0;
  goods_type_re_active_iterate(pg) { count++; }
  goods_type_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));

  count = 0;
  multipliers_re_active_iterate(pmul) { count++; }
  multipliers_re_active_iterate_end;
  stats->item(row++, 7)->setText(QString::number(count));

  stats->resizeColumnsToContents();
}

/**
   User wants to edit always active effects
 */
void tab_misc::edit_aae_effects()
{
  ui->open_effect_edit(QString::fromUtf8(R__("Always active")), nullptr,
                       EFMC_NONE);
}

/**
   User wants to edit effects from full list
 */
void tab_misc::edit_all_effects()
{
  ui->open_effect_edit(QString::fromUtf8(R__("All effects")), nullptr,
                       EFMC_ALL);
}
