/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */
#include "unithudselector.h"
#include "client_main.h"
#include "control.h"
#include "dialogs.h"
#include "hudwidget.h"
#include "tilespec.h"

// Qt
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QRadioButton>
#include <QVBoxLayout>

/**
   Constructor for unit_hud_selector
 */
unit_hud_selector::unit_hud_selector(QWidget *parent) : qfc_dialog(parent)
{
  QHBoxLayout *hbox, *hibox;
  Unit_type_id utype_id;
  QGroupBox *no_name;
  QVBoxLayout *groupbox_layout;

  hide();
  struct unit *punit = head_of_units_in_focus();

  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog
                 | Qt::FramelessWindowHint);
  main_layout = new QVBoxLayout(this);

  unit_sel_type = new QComboBox();

  unit_type_iterate(utype)
  {
    utype_id = utype_index(utype);
    if (has_player_unit_type(utype_id)) {
      unit_sel_type->addItem(utype_name_translation(utype), utype_id);
    }
  }
  unit_type_iterate_end;

  if (punit) {
    int i;
    i = unit_sel_type->findText(utype_name_translation(punit->utype));
    unit_sel_type->setCurrentIndex(i);
  }
  no_name = new QGroupBox();
  no_name->setTitle(_("Unit type"));
  this_type = new QRadioButton(_("Selected type"), no_name);
  this_type->setChecked(true);
  any_type = new QRadioButton(_("All types"), no_name);
  connect(unit_sel_type, qOverload<int>(&QComboBox::currentIndexChanged),
          this, qOverload<int>(&unit_hud_selector::select_units));
  connect(this_type, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(any_type, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  groupbox_layout = new QVBoxLayout;
  groupbox_layout->addWidget(unit_sel_type);
  groupbox_layout->addWidget(this_type);
  groupbox_layout->addWidget(any_type);
  no_name->setLayout(groupbox_layout);
  hibox = new QHBoxLayout;
  hibox->addWidget(no_name);

  no_name = new QGroupBox();
  no_name->setTitle(_("Unit activity"));
  any_activity = new QRadioButton(_("Any activity"), no_name);
  any_activity->setChecked(true);
  fortified = new QRadioButton(_("Fortified"), no_name);
  idle = new QRadioButton(_("Idle"), no_name);
  sentried = new QRadioButton(_("Sentried"), no_name);
  connect(any_activity, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(idle, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(fortified, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(sentried, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  groupbox_layout = new QVBoxLayout;
  groupbox_layout->addWidget(any_activity);
  groupbox_layout->addWidget(idle);
  groupbox_layout->addWidget(fortified);
  groupbox_layout->addWidget(sentried);
  no_name->setLayout(groupbox_layout);
  hibox->addWidget(no_name);
  main_layout->addLayout(hibox);

  no_name = new QGroupBox();
  no_name->setTitle(_("Unit HP and MP"));
  any = new QRadioButton(_("Any unit"), no_name);
  full_hp = new QRadioButton(_("Full HP"), no_name);
  full_mp = new QRadioButton(_("Full MP"), no_name);
  full_hp_mp = new QRadioButton(_("Full HP and MP"), no_name);
  full_hp_mp->setChecked(true);
  connect(any, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(full_hp, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(full_mp, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(full_hp_mp, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  groupbox_layout = new QVBoxLayout;
  groupbox_layout->addWidget(any);
  groupbox_layout->addWidget(full_hp);
  groupbox_layout->addWidget(full_mp);
  groupbox_layout->addWidget(full_hp_mp);
  no_name->setLayout(groupbox_layout);
  hibox = new QHBoxLayout;
  hibox->addWidget(no_name);

  no_name = new QGroupBox();
  no_name->setTitle(_("Location"));
  anywhere = new QRadioButton(_("Anywhere"), no_name);
  this_tile = new QRadioButton(_("Current tile"), no_name);
  this_continent = new QRadioButton(_("Current continent"), no_name);
  main_continent = new QRadioButton(_("Main continent"), no_name);
  groupbox_layout = new QVBoxLayout;

  if (punit) {
    this_tile->setChecked(true);
  } else {
    this_tile->setDisabled(true);
    this_continent->setDisabled(true);
    main_continent->setChecked(true);
  }

  groupbox_layout->addWidget(this_tile);
  groupbox_layout->addWidget(this_continent);
  groupbox_layout->addWidget(main_continent);
  groupbox_layout->addWidget(anywhere);

  no_name->setLayout(groupbox_layout);
  hibox->addWidget(no_name);
  main_layout->addLayout(hibox);

  select = new QPushButton(_("Select"));
  cancel = new QPushButton(_("Cancel"));
  connect(anywhere, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(this_tile, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(this_continent, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(main_continent, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(select, &QAbstractButton::clicked, this,
          &unit_hud_selector::uhs_select);
  connect(cancel, &QAbstractButton::clicked, this,
          &unit_hud_selector::uhs_cancel);
  hbox = new QHBoxLayout;
  hbox->addWidget(cancel);
  hbox->addWidget(select);

  result_label.setAlignment(Qt::AlignCenter);
  main_layout->addWidget(&result_label, Qt::AlignHCenter);
  main_layout->addLayout(hbox);
  setLayout(main_layout);
}

/**
   Shows and moves to center unit_hud_selector
 */
void unit_hud_selector::show_me()
{
  QPoint p;

  p = QPoint((parentWidget()->width() - sizeHint().width()) / 2,
             (parentWidget()->height() - sizeHint().height()) / 2);
  p = parentWidget()->mapToGlobal(p);
  move(p);
  setVisible(true);
  show();
  select_units();
}

/**
   Unit_hud_selector destructor
 */
unit_hud_selector::~unit_hud_selector() = default;

/**
   Selects and closes widget
 */
void unit_hud_selector::uhs_select()
{
  const struct player *pplayer;

  pplayer = client_player();

  unit_list_iterate(pplayer->units, punit)
  {
    if (activity_filter(punit) && hp_filter(punit) && island_filter(punit)
        && type_filter(punit)) {
      unit_focus_add(punit);
    }
  }
  unit_list_iterate_end;
  close();
}

/**
   Closes current widget
 */
void unit_hud_selector::uhs_cancel() { close(); }

/**
   Shows number of selected units on label
 */
void unit_hud_selector::select_units(int x)
{
  int num = 0;
  const struct player *pplayer;

  pplayer = client_player();

  unit_list_iterate(pplayer->units, punit)
  {
    if (activity_filter(punit) && hp_filter(punit) && island_filter(punit)
        && type_filter(punit)) {
      num++;
    }
  }
  unit_list_iterate_end;
  result_label.setText(QString(PL_("%1 unit", "%1 units", num)).arg(num));
}

/**
   Convinient slot for ez connect
 */
void unit_hud_selector::select_units(bool x) { select_units(0); }

/**
   Key press event for unit_hud_selector
 */
void unit_hud_selector::keyPressEvent(QKeyEvent *event)
{
  if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter)) {
    uhs_select();
  }
  if (event->key() == Qt::Key_Escape) {
    close();
    event->accept();
  }
  QWidget::keyPressEvent(event);
}

/**
   Filter by activity
 */
bool unit_hud_selector::activity_filter(struct unit *punit)
{
  return (punit->activity == ACTIVITY_FORTIFIED && fortified->isChecked())
         || (punit->activity == ACTIVITY_SENTRY && sentried->isChecked())
         || (punit->activity == ACTIVITY_IDLE && idle->isChecked())
         || any_activity->isChecked();
}

/**
   Filter by hp/mp
 */
bool unit_hud_selector::hp_filter(struct unit *punit)
{
  return any->isChecked()
         || (full_mp->isChecked()
             && punit->moves_left >= punit->utype->move_rate)
         || (full_hp->isChecked() && punit->hp >= punit->utype->hp)
         || (full_hp_mp->isChecked() && punit->hp >= punit->utype->hp
             && punit->moves_left >= punit->utype->move_rate);
}

/**
   Filter by location
 */
bool unit_hud_selector::island_filter(struct unit *punit)
{
  int island = -1;
  struct unit *cunit = head_of_units_in_focus();

  if (this_tile->isChecked() && cunit) {
    if (punit->tile == cunit->tile) {
      return true;
    }
  }

  if (main_continent->isChecked()
      && player_primary_capital(client_player())) {
    island = player_primary_capital(client_player())->tile->continent;
  } else if (this_continent->isChecked() && cunit) {
    island = cunit->tile->continent;
  }

  if (island > -1) {
    if (punit->tile->continent == island) {
      return true;
    }
  }

  return anywhere->isChecked();
}

/**
   Filter by type
 */
bool unit_hud_selector::type_filter(struct unit *punit)
{
  QVariant qvar;
  Unit_type_id utype_id;

  if (this_type->isChecked()) {
    qvar = unit_sel_type->currentData();
    utype_id = qvar.toInt();
    return utype_id == utype_index(punit->utype);
  }
  return any_type->isChecked();
}
