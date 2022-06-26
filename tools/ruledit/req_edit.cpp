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
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

// utility
#include "fcintl.h"

// common
#include "reqtext.h"
#include "requirements.h"

// ruledit
#include "ruledit.h"
#include "ruledit_qt.h"
#include "univ_value.h"

#include "req_edit.h"

/**
   Setup req_edit object
 */
req_edit::req_edit(ruledit_gui *ui_in, const QString &target,
                   struct requirement_vector *preqs)
    : QDialog()
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  QGridLayout *reqedit_layout = new QGridLayout();
  QHBoxLayout *active_layout = new QHBoxLayout();
  QPushButton *close_button;
  QPushButton *add_button;
  QPushButton *delete_button;
  QMenu *menu;
  QLabel *lbl;

  ui = ui_in;
  connect(ui, &ruledit_gui::rec_vec_may_have_changed, this,
          &req_edit::incoming_rec_vec_change);

  clear_selected();
  req_vector = preqs;

  req_list = new QListWidget(this);

  connect(req_list, &QListWidget::itemSelectionChanged, this,
          &req_edit::select_req);
  main_layout->addWidget(req_list);

  lbl = new QLabel(R__("Type:"));
  active_layout->addWidget(lbl, 0);
  edit_type_button = new QToolButton();
  menu = new QMenu();
  edit_type_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  edit_type_button->setPopupMode(QToolButton::MenuButtonPopup);
  connect(menu, &QMenu::triggered, this, &req_edit::req_type_menu);
  edit_type_button->setMenu(menu);
  universals_iterate(univ_id)
  {
    struct universal dummy;

    dummy.kind = univ_id;
    if (universal_value_initial(&dummy)) {
      menu->addAction(universals_n_name(univ_id));
    }
  }
  universals_iterate_end;
  active_layout->addWidget(edit_type_button, 1);

  lbl = new QLabel(R__("Value:"));
  active_layout->addWidget(lbl, 2);
  edit_value_enum_button = new QToolButton();
  edit_value_enum_menu = new QMenu();
  edit_value_enum_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  edit_value_enum_button->setPopupMode(QToolButton::MenuButtonPopup);
  connect(edit_value_enum_menu, &QMenu::triggered, this,
          &req_edit::univ_value_enum_menu);
  edit_value_enum_button->setMenu(edit_value_enum_menu);
  edit_value_enum_menu->setVisible(false);
  active_layout->addWidget(edit_value_enum_button, 3);
  edit_value_nbr_field = new QLineEdit();
  edit_value_nbr_field->setVisible(false);
  connect(edit_value_nbr_field, &QLineEdit::returnPressed, this,
          &req_edit::univ_value_edit);
  active_layout->addWidget(edit_value_nbr_field, 4);

  lbl = new QLabel(R__("Range:"));
  active_layout->addWidget(lbl, 5);
  edit_range_button = new QToolButton();
  menu = new QMenu();
  edit_range_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  edit_range_button->setPopupMode(QToolButton::MenuButtonPopup);
  connect(menu, &QMenu::triggered, this, &req_edit::req_range_menu);
  edit_range_button->setMenu(menu);
  req_range_iterate(range_id) { menu->addAction(req_range_name(range_id)); }
  req_range_iterate_end;
  active_layout->addWidget(edit_range_button, 6);

  edit_present_button = new QToolButton();
  menu = new QMenu();
  edit_present_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  edit_present_button->setPopupMode(QToolButton::MenuButtonPopup);
  connect(menu, &QMenu::triggered, this, &req_edit::req_present_menu);
  edit_present_button->setMenu(menu);
  menu->addAction(QStringLiteral("Allows"));
  menu->addAction(QStringLiteral("Prevents"));
  active_layout->addWidget(edit_present_button, 7);

  main_layout->addLayout(active_layout);

  add_button =
      new QPushButton(QString::fromUtf8(R__("Add Requirement")), this);
  connect(add_button, &QAbstractButton::pressed, this, &req_edit::add_now);
  reqedit_layout->addWidget(add_button, 0, 0);

  delete_button =
      new QPushButton(QString::fromUtf8(R__("Delete Requirement")), this);
  connect(delete_button, &QAbstractButton::pressed, this,
          &req_edit::delete_now);
  reqedit_layout->addWidget(delete_button, 1, 0);

  close_button = new QPushButton(QString::fromUtf8(R__("Close")), this);
  connect(close_button, &QAbstractButton::pressed, this,
          &req_edit::close_now);
  reqedit_layout->addWidget(close_button, 2, 0);

  refresh();

  main_layout->addLayout(reqedit_layout);

  setLayout(main_layout);
  setWindowTitle(target);
}

/**
   Refresh the information.
 */
void req_edit::refresh()
{
  int i = 0;

  req_list->clear();

  requirement_vector_iterate(req_vector, preq)
  {
    char buf[512];
    QListWidgetItem *item;

    buf[0] = '\0';
    if (!req_text_insert(buf, sizeof(buf), nullptr, preq, VERB_ACTUAL, "")) {
      if (preq->present) {
        universal_name_translation(&preq->source, buf, sizeof(buf));
      } else {
        char buf2[256];

        universal_name_translation(&preq->source, buf2, sizeof(buf2));
        fc_snprintf(buf, sizeof(buf), "%s prevents", buf2);
      }
    }
    item = new QListWidgetItem(QString::fromUtf8(buf));
    req_list->insertItem(i++, item);
    if (selected == preq) {
      item->setSelected(true);
    }
  }
  requirement_vector_iterate_end;

  fill_active();
}

/**
   The selected requirement has changed.
 */
void req_edit::update_selected()
{
  if (selected != nullptr) {
    selected_values = *selected;
  }
}

/**
   Unselect the currently selected requirement.
 */
void req_edit::clear_selected()
{
  selected = nullptr;

  selected_values.source.kind = VUT_NONE;
  selected_values.source.value.advance = nullptr;
  selected_values.range = REQ_RANGE_LOCAL;
  selected_values.present = true;
  selected_values.survives = false;
  selected_values.quiet = false;
}

/**
   User pushed close button
 */
void req_edit::close_now()
{
  ui->unregister_req_edit(this);
  done(0);
}

/**
   User selected requirement from the list.
 */
void req_edit::select_req()
{
  int i = 0;

  requirement_vector_iterate(req_vector, preq)
  {
    QListWidgetItem *item = req_list->item(i++);

    if (item != nullptr && item->isSelected()) {
      selected = preq;
      update_selected();
      fill_active();
      return;
    }
  }
  requirement_vector_iterate_end;
}

struct uvb_data {
  QLineEdit *number;
  QToolButton *enum_button;
  QMenu *menu;
  struct universal *univ;
};

/**
   Callback for filling menu values
 */
static void universal_value_cb(const char *value, bool current, void *cbdata)
{
  struct uvb_data *data = (struct uvb_data *) cbdata;

  if (value == nullptr) {
    int kind, val;

    universal_extraction(data->univ, &kind, &val);
    data->number->setText(QString::number(val));
    data->number->setVisible(true);
  } else {
    data->enum_button->setVisible(true);
    data->menu->addAction(value);
    if (current) {
      data->enum_button->setText(value);
    }
  }
}

/**
   Fill active menus from selected req.
 */
void req_edit::fill_active()
{
  if (selected != nullptr) {
    struct uvb_data data;

    edit_type_button->setText(universals_n_name(selected->source.kind));
    data.number = edit_value_nbr_field;
    data.enum_button = edit_value_enum_button;
    data.menu = edit_value_enum_menu;
    data.univ = &selected->source;
    edit_value_enum_menu->clear();
    edit_value_enum_button->setVisible(false);
    edit_value_nbr_field->setVisible(false);
    universal_kind_values(&selected->source, universal_value_cb, &data);
    edit_range_button->setText(req_range_name(selected->range));
    if (selected->present) {
      edit_present_button->setText(QStringLiteral("Allows"));
    } else {
      edit_present_button->setText(QStringLiteral("Prevents"));
    }
  }
}

/**
   User selected type for the requirement.
 */
void req_edit::req_type_menu(QAction *action)
{
  QByteArray un_bytes = action->text().toUtf8();
  enum universals_n univ =
      universals_n_by_name(un_bytes.data(), fc_strcasecmp);

  if (selected != nullptr) {
    selected->source.kind = univ;
    universal_value_initial(&selected->source);
    update_selected();
  }

  refresh();

  emit rec_vec_may_have_changed(req_vector);
}

/**
   User selected range for the requirement.
 */
void req_edit::req_range_menu(QAction *action)
{
  QByteArray un_bytes = action->text().toUtf8();
  enum req_range range = req_range_by_name(un_bytes.data(), fc_strcasecmp);

  if (selected != nullptr) {
    selected->range = range;
    update_selected();
  }

  refresh();

  emit rec_vec_may_have_changed(req_vector);
}

/**
   User selected 'present' value for the requirement.
 */
void req_edit::req_present_menu(QAction *action)
{
  if (selected != nullptr) {
    selected->present = action->text() != QLatin1String("Prevents");
    update_selected();
  }

  refresh();

  emit rec_vec_may_have_changed(req_vector);
}

/**
   User selected value for the requirement.
 */
void req_edit::univ_value_enum_menu(QAction *action)
{
  if (selected != nullptr) {
    QByteArray un_bytes = action->text().toUtf8();

    universal_value_from_str(&selected->source, un_bytes.data());

    update_selected();
    refresh();

    emit rec_vec_may_have_changed(req_vector);
  }
}

/**
   User entered numerical requirement value.
 */
void req_edit::univ_value_edit()
{
  if (selected != nullptr) {
    QByteArray num_bytes = edit_value_nbr_field->text().toUtf8();

    universal_value_from_str(&selected->source, num_bytes.data());

    update_selected();
    refresh();

    emit rec_vec_may_have_changed(req_vector);
  }
}

/**
   User requested new requirement
 */
void req_edit::add_now()
{
  struct requirement new_req;

  new_req =
      req_from_values(VUT_NONE, REQ_RANGE_LOCAL, false, true, false, 0);

  requirement_vector_append(req_vector, new_req);

  refresh();

  emit rec_vec_may_have_changed(req_vector);
}

/**
   User requested requirement deletion
 */
void req_edit::delete_now()
{
  if (selected != nullptr) {
    size_t i;

    for (i = 0; i < requirement_vector_size(req_vector); i++) {
      if (requirement_vector_get(req_vector, i) == selected) {
        requirement_vector_remove(req_vector, i);
        break;
      }
    }

    clear_selected();

    refresh();

    emit rec_vec_may_have_changed(req_vector);
  }
}

/**
   The requirement vector may have been changed.
   @param vec the requirement vector that may have been changed.
 */
void req_edit::incoming_rec_vec_change(const requirement_vector *vec)
{
  if (req_vector == vec) {
    // The selected requirement may be gone

    selected = nullptr;
    requirement_vector_iterate(req_vector, preq)
    {
      if (are_requirements_equal(preq, &selected_values)) {
        selected = preq;
        break;
      }
    }
    requirement_vector_iterate_end;

    if (selected == nullptr) {
      // The currently selected requirement was deleted.
      clear_selected();
    }

    refresh();
  }
}

/**
   User clicked windows close button.
 */
void req_edit::closeEvent(QCloseEvent *event)
{
  ui->unregister_req_edit(this);
}
