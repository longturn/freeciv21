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
#include "terrain.h"

// ruledit
#include "req_edit.h"
#include "ruledit.h"
#include "ruledit_qt.h"
#include "validity.h"

#include "tab_terrains.h"

/**
   Setup tab_terrains object
 */
tab_terrains::tab_terrains(ruledit_gui *ui_in) : QWidget()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *terrains_layout = new QGridLayout();
  QLabel *label;
  QPushButton *effects_button;
  QPushButton *add_button;
  QPushButton *delete_button;

  ui = ui_in;
  selected = 0;

  terrain_list = new QListWidget(this);

  connect(terrain_list, &QListWidget::itemSelectionChanged, this,
          &tab_terrains::select_terrain);
  main_layout->addWidget(terrain_list);

  terrains_layout->setSizeConstraint(QLayout::SetMaximumSize);

  label = new QLabel(QString::fromUtf8(R__("Rule Name")));
  label->setParent(this);
  rname = new QLineEdit(this);
  rname->setText(QStringLiteral("None"));
  connect(rname, &QLineEdit::returnPressed, this, &tab_terrains::name_given);
  terrains_layout->addWidget(label, 0, 0);
  terrains_layout->addWidget(rname, 0, 2);

  label = new QLabel(QString::fromUtf8(R__("Name")));
  label->setParent(this);
  same_name = new QRadioButton();
  connect(same_name, &QAbstractButton::toggled, this,
          &tab_terrains::same_name_toggle);
  name = new QLineEdit(this);
  name->setText(QStringLiteral("None"));
  connect(name, &QLineEdit::returnPressed, this, &tab_terrains::name_given);
  terrains_layout->addWidget(label, 1, 0);
  terrains_layout->addWidget(same_name, 1, 1);
  terrains_layout->addWidget(name, 1, 2);

  effects_button = new QPushButton(QString::fromUtf8(R__("Effects")), this);
  connect(effects_button, &QAbstractButton::pressed, this,
          &tab_terrains::edit_effects);
  terrains_layout->addWidget(effects_button, 2, 2);

  add_button = new QPushButton(QString::fromUtf8(R__("Add Terrain")), this);
  connect(add_button, &QAbstractButton::pressed, this,
          &tab_terrains::add_now);
  terrains_layout->addWidget(add_button, 3, 0);
  show_experimental(add_button);

  delete_button =
      new QPushButton(QString::fromUtf8(R__("Remove this Terrain")), this);
  connect(delete_button, &QAbstractButton::pressed, this,
          &tab_terrains::delete_now);
  terrains_layout->addWidget(delete_button, 3, 2);
  show_experimental(delete_button);

  refresh();

  main_layout->addLayout(terrains_layout);

  setLayout(main_layout);
}

/**
   Refresh the information.
 */
void tab_terrains::refresh()
{
  terrain_list->clear();

  terrain_type_iterate(pterr)
  {
    if (!pterr->ruledit_disabled) {
      QListWidgetItem *item =
          new QListWidgetItem(QString::fromUtf8(terrain_rule_name(pterr)));

      terrain_list->insertItem(terrain_index(pterr), item);
    }
  }
  terrain_type_iterate_end;
}

/**
   Update info of the terrain
 */
void tab_terrains::update_terrain_info(struct terrain *pterr)
{
  selected = pterr;

  if (selected != nullptr) {
    QString dispn = QString::fromUtf8(untranslated_name(&(pterr->name)));
    QString rulen = QString::fromUtf8(terrain_rule_name(pterr));

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
   User selected terrain from the list.
 */
void tab_terrains::select_terrain()
{
  QList<QListWidgetItem *> select_list = terrain_list->selectedItems();

  if (!select_list.isEmpty()) {
    QByteArray tn_bytes;

    tn_bytes = select_list.at(0)->text().toUtf8();
    update_terrain_info(terrain_by_rule_name(tn_bytes.data()));
  }
}

/**
   User entered name for the terrain
 */
void tab_terrains::name_given()
{
  if (selected != nullptr) {
    QByteArray name_bytes;
    QByteArray rname_bytes;

    terrain_type_iterate(pterr)
    {
      if (pterr != selected && !pterr->ruledit_disabled) {
        rname_bytes = rname->text().toUtf8();
        if (!strcmp(terrain_rule_name(pterr), rname_bytes.data())) {
          ui->display_msg(
              R__("A terrain with that rule name already exists!"));
          return;
        }
      }
    }
    terrain_type_iterate_end;

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
   User requested terrain deletion
 */
void tab_terrains::delete_now()
{
  if (selected != nullptr) {
    requirers_dlg *requirers;

    requirers = ui->create_requirers(terrain_rule_name(selected));
    if (is_terrain_needed(selected, &ruledit_qt_display_requirers,
                          requirers)) {
      return;
    }

    selected->ruledit_disabled = true;

    refresh();
    update_terrain_info(nullptr);
  }
}

/**
   Initialize new terrain for use.
 */
bool tab_terrains::initialize_new_terrain(struct terrain *pterr)
{
  if (terrain_by_rule_name("New Terrain") != nullptr) {
    return false;
  }

  name_set(&(pterr->name), 0, "New Terrain");

  return true;
}

/**
   User requested new terrain
 */
void tab_terrains::add_now()
{
  struct terrain *new_terr;

  // Try to reuse freed terrain slot
  terrain_type_iterate(pterr)
  {
    if (pterr->ruledit_disabled) {
      if (initialize_new_terrain(pterr)) {
        pterr->ruledit_disabled = false;
        update_terrain_info(pterr);
        refresh();
      }
      return;
    }
  }
  terrain_type_iterate_end;

  // Try to add completely new terrain
  if (game.control.terrain_count >= MAX_NUM_TERRAINS) {
    return;
  }

  // terrain_count must be big enough to hold new extra or
  // terrain_by_number() fails.
  game.control.terrain_count++;
  new_terr = terrain_by_number(game.control.terrain_count - 1);
  if (initialize_new_terrain(new_terr)) {
    update_terrain_info(new_terr);

    refresh();
  } else {
    game.control.terrain_count--; // Restore
  }
}

/**
   Toggled whether rule_name and name should be kept identical
 */
void tab_terrains::same_name_toggle(bool checked)
{
  name->setEnabled(!checked);
  if (checked) {
    name->setText(rname->text());
  }
}

/**
   User wants to edit effects
 */
void tab_terrains::edit_effects()
{
  if (selected != nullptr) {
    struct universal uni;

    uni.value.terrain = selected;
    uni.kind = VUT_TERRAIN;

    ui->open_effect_edit(QString::fromUtf8(terrain_rule_name(selected)),
                         &uni, EFMC_NORMAL);
  }
}
