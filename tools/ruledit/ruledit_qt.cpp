/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2022 Freeciv21 and Freeciv
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

#include <fc_config.h>

// Qt
#include <QApplication>
#include <QCloseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "version.h"

// common
#include "game.h"
#include "version.h"

// server
#include "ruleset.h"

// ruledit
#include "conversion_log.h"
#include "effect_edit.h"
#include "req_edit.h"
#include "req_vec_fix.h"
#include "requirers_dlg.h"
#include "ruledit.h"
#include "tab_building.h"
#include "tab_enablers.h"
#include "tab_extras.h"
#include "tab_good.h"
#include "tab_gov.h"
#include "tab_misc.h"
#include "tab_multiplier.h"
#include "tab_nation.h"
#include "tab_tech.h"
#include "tab_terrains.h"
#include "tab_unit.h"

#include "ruledit_qt.h"

static ruledit_gui *gui;
static conversion_log *convlog;

/**
   Display requirer list.
 */
void ruledit_qt_display_requirers(const char *msg, void *data)
{
  requirers_dlg *requirers = (requirers_dlg *) data;

  gui->show_required(requirers, msg);
}

/**
   Setup GUI object
 */
ruledit_gui::ruledit_gui(ruledit_main *main) : QObject(main)
{
  QVBoxLayout *full_layout = new QVBoxLayout();
  QVBoxLayout *preload_layout = new QVBoxLayout();
  QWidget *preload_widget = new QWidget();
  QVBoxLayout *edit_layout = new QVBoxLayout();
  QWidget *edit_widget = new QWidget();
  QPushButton *ruleset_accept;
  QLabel *rs_label;

  data.nationlist = nullptr;
  data.nationlist_saved = nullptr;

  auto *central = new QWidget;
  main->setCentralWidget(central);

  main_layout = new QStackedLayout();

  preload_layout->setSizeConstraint(QLayout::SetMaximumSize);

  auto version_label =
      new QLabel(QString(_("Version %1")).arg(freeciv21_version()));
  version_label->setAlignment(Qt::AlignHCenter);
  version_label->setParent(central);
  preload_layout->addWidget(version_label);
  rs_label = new QLabel(
      QString::fromUtf8(R__("Give ruleset to use as starting point.")));
  rs_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  preload_layout->addWidget(rs_label);
  ruleset_select = new QLineEdit(central);
  if (reargs.ruleset.isEmpty()) {
    ruleset_select->setText(reargs.ruleset);
  } else {
    ruleset_select->setText(GAME_DEFAULT_RULESETDIR);
  }
  connect(ruleset_select, &QLineEdit::returnPressed, this,
          &ruledit_gui::launch_now);
  preload_layout->addWidget(ruleset_select);
  ruleset_accept = new QPushButton(QString::fromUtf8(R__("Start editing")));
  connect(ruleset_accept, &QAbstractButton::pressed, this,
          &ruledit_gui::launch_now);
  preload_layout->addWidget(ruleset_accept);

  preload_widget->setLayout(preload_layout);
  main_layout->addWidget(preload_widget);

  stack = new QTabWidget(central);

  misc = new tab_misc(this);
  stack->addTab(misc, QString::fromUtf8(R__("Misc")));
  tech = new tab_tech(this);
  stack->addTab(tech, QString::fromUtf8(R__("Tech")));
  bldg = new tab_building(this);
  stack->addTab(bldg, QString::fromUtf8(R__("Buildings")));
  unit = new tab_unit(this);
  stack->addTab(unit, QString::fromUtf8(R__("Units")));
  good = new tab_good(this);
  stack->addTab(good, QString::fromUtf8(R__("Goods")));
  gov = new tab_gov(this);
  stack->addTab(gov, QString::fromUtf8(R__("Governments")));
  enablers = new tab_enabler(this);
  stack->addTab(enablers, QString::fromUtf8(R__("Enablers")));
  extras = new tab_extras(this);
  stack->addTab(extras, QString::fromUtf8(R__("Extras")));
  terrains = new tab_terrains(this);
  stack->addTab(terrains, QString::fromUtf8(R__("Terrains")));
  multipliers = new tab_multiplier(this);
  stack->addTab(multipliers, QString::fromUtf8(R__("Multipliers")));
  nation = new tab_nation(this);
  stack->addTab(nation, QString::fromUtf8(R__("Nations")));

  edit_layout->addWidget(stack);

  edit_widget->setLayout(edit_layout);
  main_layout->addWidget(edit_widget);

  full_layout->addLayout(main_layout);

  msg_dspl =
      new QLabel(QString::fromUtf8(R__("Welcome to freeciv21-ruledit")));
  msg_dspl->setParent(central);

  msg_dspl->setAlignment(Qt::AlignHCenter);

  full_layout->addWidget(msg_dspl);

  central->setLayout(full_layout);

  req_edits = req_edit_list_new();
  this->req_vec_fixers = req_vec_fix_list_new();
  effect_edits = effect_edit_list_new();

  // FIXME Should get rid of the static variable.
  gui = this;
}

/**
   Destructor
 */
ruledit_gui::~ruledit_gui()
{
  req_edit_list_destroy(req_edits);
  req_vec_fix_list_destroy(req_vec_fixers);
  effect_edit_list_destroy(effect_edits);
}

/**
   Ruleset conversion log callback
 */
static void conversion_log_cb(const char *msg) { convlog->add(msg); }

/**
   User entered savedir
 */
void ruledit_gui::launch_now()
{
  QByteArray rn_bytes;

  convlog = new conversion_log();

  rn_bytes = ruleset_select->text().toUtf8();
  sz_strlcpy(game.server.rulesetdir, rn_bytes.data());

  if (load_rulesets(nullptr, nullptr, true, conversion_log_cb, false, true,
                    true)) {
    display_msg(R__("Ruleset loaded"));

    // Make freeable copy
    if (game.server.ruledit.nationlist != nullptr) {
      data.nationlist = fc_strdup(game.server.ruledit.nationlist);
    } else {
      data.nationlist = nullptr;
    }

    bldg->refresh();
    misc->refresh();
    nation->refresh();
    tech->refresh();
    unit->refresh();
    good->refresh();
    gov->refresh();
    enablers->refresh();
    extras->refresh();
    multipliers->refresh();
    terrains->refresh();
    main_layout->setCurrentIndex(1);
  } else {
    display_msg(R__("Ruleset loading failed!"));
  }
}

/**
   A requirement vector may have been changed.
   @param vec the requirement vector that may have been changed.
 */
void ruledit_gui::incoming_rec_vec_change(const requirement_vector *vec)
{
  emit rec_vec_may_have_changed(vec);
}

/**
   Display status message
 */
void ruledit_gui::display_msg(const char *msg)
{
  msg_dspl->setText(QString::fromUtf8(msg));
}

/**
   Create requirers dlg.
 */
requirers_dlg *ruledit_gui::create_requirers(const char *title)
{
  requirers_dlg *requirers;

  requirers = new requirers_dlg(this);

  requirers->clear(title);

  return requirers;
}

/**
   Add entry to requirers dlg.
 */
void ruledit_gui::show_required(requirers_dlg *requirers, const char *msg)
{
  requirers->add(msg);

  // Show dialog if not already visible
  requirers->show();
}

/**
   Flush information from widgets to stores where it can be saved from.
 */
void ruledit_gui::flush_widgets() { nation->flush_widgets(); }

/**
   Open req_edit dialog
 */
void ruledit_gui::open_req_edit(const QString &target,
                                struct requirement_vector *preqs)
{
  req_edit *redit;

  req_edit_list_iterate(req_edits, old_edit)
  {
    if (old_edit->req_vector == preqs) {
      // Already open
      return;
    }
  }
  req_edit_list_iterate_end;

  redit = new req_edit(this, target, preqs);

  redit->show();

  connect(redit, &req_edit::rec_vec_may_have_changed, this,
          &ruledit_gui::incoming_rec_vec_change);

  req_edit_list_append(req_edits, redit);
}

/**
   Unregisted closed req_edit dialog
 */
void ruledit_gui::unregister_req_edit(class req_edit *redit)
{
  req_edit_list_remove(req_edits, redit);
}

/**
   Open req_vec_fix dialog.
 */
void ruledit_gui::open_req_vec_fix(req_vec_fix_item *item_info)
{
  req_vec_fix *fixer;

  req_vec_fix_list_iterate(req_vec_fixers, old_fixer)
  {
    if (old_fixer->item() == item_info->item()) {
      item_info->close();

      // Already open
      return;
    }
  }
  req_vec_fix_list_iterate_end;

  fixer = new req_vec_fix(this, item_info);

  fixer->refresh();
  fixer->show();

  connect(fixer, &req_vec_fix::rec_vec_may_have_changed, this,
          &ruledit_gui::incoming_rec_vec_change);

  req_vec_fix_list_append(req_vec_fixers, fixer);
}

/**
   Unregister closed req_vec_fix dialog.
 */
void ruledit_gui::unregister_req_vec_fix(req_vec_fix *fixer)
{
  req_vec_fix_list_remove(req_vec_fixers, fixer);
}

/**
   Open effect_edit dialog
 */
void ruledit_gui::open_effect_edit(const QString &target,
                                   struct universal *uni,
                                   enum effect_filter_main_class efmc)
{
  effect_edit *e_edit;

  effect_edit_list_iterate(effect_edits, old_edit)
  {
    struct universal *old = old_edit->filter_get();

    if (uni != nullptr) {
      if (are_universals_equal(old, uni)) {
        // Already open
        return;
      }
    } else if (old->kind == VUT_NONE && old_edit->efmc == efmc) {
      // Already open
      return;
    }
  }
  effect_edit_list_iterate_end;

  e_edit = new effect_edit(this, target, uni, efmc);

  e_edit->show();

  effect_edit_list_append(effect_edits, e_edit);
}

/**
   Unregisted closed effect_edit dialog
 */
void ruledit_gui::unregister_effect_edit(class effect_edit *e_edit)
{
  effect_edit_list_remove(effect_edits, e_edit);
}

/**
   Refresh all effect edit dialogs
 */
void ruledit_gui::refresh_effect_edits()
{
  effect_edit_list_iterate(effect_edits, e_edit) { e_edit->refresh(); }
  effect_edit_list_iterate_end;
}

/**
   Main window constructor
 */
ruledit_main::ruledit_main() : QMainWindow()
{
  const QString title = QString::fromUtf8(R__("Freeciv21 Ruleset Editor"));

  setWindowTitle(title);
}

/**
   User clicked windows close button.
 */
void ruledit_main::closeEvent(QCloseEvent *cevent)
{
  // Ask for confirmation
  QMessageBox ask(centralWidget());
  int ret;

  ask.setText(R__("Are you sure you want to quit?"));
  ask.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask.setDefaultButton(QMessageBox::Cancel);
  ask.setIcon(QMessageBox::Warning);
  ask.setWindowTitle(R__("Quit?"));
  ret = ask.exec();

  switch (ret) {
  case QMessageBox::Cancel:
    // Cancelled by user
    cevent->ignore();
    break;
  case QMessageBox::Ok:
    close();
    break;
  }
}
