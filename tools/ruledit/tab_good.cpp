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
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>

// utility
#include "fcintl.h"

// common
#include "game.h"
#include "improvement.h"

// ruledit
#include "req_edit.h"
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_good.h"

/**
   Setup tab_good object
 */
tab_good::tab_good(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *good_layout = new QGridLayout();
  QLabel *label;
  QPushButton *effects_button;
  QPushButton *add_button;
  QPushButton *delete_button;
  QPushButton *reqs_button;

  ui = ui_in;
  selected = 0;

  good_list = new QListWidget(this);

  connect(good_list, &QListWidget::itemSelectionChanged, this,
          &tab_good::select_good);
  main_layout->addWidget(good_list);

  good_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Rule Name")));
  label->setParent(this);
  rname = new QLineEdit(this);
  rname->setText(QStringLiteral("None"));
  connect(rname, &QLineEdit::returnPressed, this, &tab_good::name_given);
  good_layout->addWidget(label, 0, 0);
  good_layout->addWidget(rname, 0, 2);

  label = new QLabel(QString::fromUtf8(R__("Name")));
  label->setParent(this);
  same_name = new QRadioButton();
  connect(same_name, &QAbstractButton::toggled, this,
          &tab_good::same_name_toggle);
  name = new QLineEdit(this);
  name->setText(QStringLiteral("None"));
  connect(name, &QLineEdit::returnPressed, this, &tab_good::name_given);
  good_layout->addWidget(label, 1, 0);
  good_layout->addWidget(same_name, 1, 1);
  good_layout->addWidget(name, 1, 2);

  reqs_button =
      new QPushButton(QString::fromUtf8(R__("Requirements")), this);
  connect(reqs_button, &QAbstractButton::pressed, this,
          &tab_good::edit_reqs);
  good_layout->addWidget(reqs_button, 2, 2);

  effects_button = new QPushButton(QString::fromUtf8(R__("Effects")), this);
  connect(effects_button, &QAbstractButton::pressed, this,
          &tab_good::edit_effects);
  good_layout->addWidget(effects_button, 3, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Good")), this);
  connect(add_button, &QAbstractButton::pressed, this, &tab_good::add_now);
  good_layout->addWidget(add_button, 4, 0);
  show_experimental(add_button);

  delete_button =
      new QPushButton(QString::fromUtf8(R__("Remove this Good")), this);
  connect(delete_button, &QAbstractButton::pressed, this,
          &tab_good::delete_now);
  good_layout->addWidget(delete_button, 4, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(good_layout);

  setLayout(main_layout);
}

/**
   Refresh the information.
 */
void tab_good::refresh()
{
  good_list->clear();

  goods_type_iterate(pgood)
  {
    if (!pgood->ruledit_disabled) {
      QListWidgetItem *item =
          new QListWidgetItem(QString::fromUtf8(goods_rule_name(pgood)));

      good_list->insertItem(goods_index(pgood), item);
    }
  }
  goods_type_iterate_end;
}

/**
   Update info of the good
 */
void tab_good::update_good_info(struct goods_type *pgood)
{
  selected = pgood;

  if (selected != 0) {
    QString dispn = QString::fromUtf8(untranslated_name(&(pgood->name)));
    QString rulen = QString::fromUtf8(goods_rule_name(pgood));

    name->setText(dispn);
    rname->setText(rulen);
    if (dispn == rulen) {
      name->setEnabled(false);
      same_name->setChecked(true);
    } else {
      same_name->setChecked(false);
      name->setEnabled(true);
    }
  } else {
    name->setText(QStringLiteral("None"));
    rname->setText(QStringLiteral("None"));
    same_name->setChecked(true);
    name->setEnabled(false);
  }
}

/**
   User selected good from the list.
 */
void tab_good::select_good()
{
  QList<QListWidgetItem *> select_list = good_list->selectedItems();

  if (!select_list.isEmpty()) {
    QByteArray gn_bytes;

    gn_bytes = select_list.at(0)->text().toUtf8();
    update_good_info(goods_by_rule_name(gn_bytes.data()));
  }
}

/**
   User entered name for the good
 */
void tab_good::name_given()
{
  if (selected != nullptr) {
    QByteArray name_bytes;
    QByteArray rname_bytes;

    goods_type_iterate(pgood)
    {
      if (pgood != selected && !pgood->ruledit_disabled) {
        rname_bytes = rname->text().toUtf8();
        if (!strcmp(goods_rule_name(pgood), rname_bytes.data())) {
          ui->display_msg(R__("A good with that rule name already exists!"));
          return;
        }
      }
    }
    goods_type_iterate_end;

    if (same_name->isChecked()) {
      name->setText(rname->text());
    }

    name_bytes = name->text().toUtf8();
    rname_bytes = rname->text().toUtf8();
    names_set(&(selected->name), 0, name_bytes.data(), rname_bytes.data());
    refresh();
  }
}

/**
   User requested good deletion
 */
void tab_good::delete_now()
{
  if (selected != 0) {
    requirers_dlg *requirers;

    requirers = ui->create_requirers(goods_rule_name(selected));
    if (is_good_needed(selected, &ruledit_qt_display_requirers, requirers)) {
      return;
    }

    selected->ruledit_disabled = true;

    refresh();
    update_good_info(nullptr);
  }
}

/**
   Initialize new good for use.
 */
bool tab_good::initialize_new_good(struct goods_type *pgood)
{
  if (goods_by_rule_name("New Good") != nullptr) {
    return false;
  }

  name_set(&(pgood->name), 0, "New Good");

  return true;
}

/**
   User requested new good
 */
void tab_good::add_now()
{
  struct goods_type *new_good;

  // Try to reuse freed good slot
  goods_type_iterate(pgood)
  {
    if (pgood->ruledit_disabled) {
      if (initialize_new_good(pgood)) {
        pgood->ruledit_disabled = false;
        update_good_info(pgood);
        refresh();
      }
      return;
    }
  }
  goods_type_iterate_end;

  // Try to add completely new good
  if (game.control.num_goods_types >= MAX_GOODS_TYPES) {
    return;
  }

  // num_good_types must be big enough to hold new good or
  // good_by_number() fails.
  game.control.num_goods_types++;
  new_good = goods_by_number(game.control.num_goods_types - 1);
  if (initialize_new_good(new_good)) {
    update_good_info(new_good);

    refresh();
  } else {
    game.control.num_goods_types--; // Restore
  }
}

/**
   Toggled whether rule_name and name should be kept identical
 */
void tab_good::same_name_toggle(bool checked)
{
  name->setEnabled(!checked);
  if (checked) {
    name->setText(rname->text());
  }
}

/**
   User wants to edit reqs
 */
void tab_good::edit_reqs()
{
  if (selected != nullptr) {
    req_edit *redit = new req_edit(
        ui, QString::fromUtf8(goods_rule_name(selected)), &selected->reqs);

    redit->show();
  }
}

/**
   User wants to edit effects
 */
void tab_good::edit_effects()
{
  if (selected != nullptr) {
    struct universal uni;

    uni.value.good = selected;
    uni.kind = VUT_GOOD;

    ui->open_effect_edit(QString::fromUtf8(goods_rule_name(selected)), &uni,
                         EFMC_NORMAL);
  }
}
