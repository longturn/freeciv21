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

#ifndef FC__RULEDIT_QT_H
#define FC__RULEDIT_QT_H

// Qt
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QTabWidget>

// ruledit
#include "effect_edit.h"
#include "rulesave.h"

class QLineEdit;
class QStackedLayout;

class requirers_dlg;
class tab_building;
class tab_good;
class tab_gov;
class tab_misc;
class tab_multiplier;
class tab_tech;
class tab_unit;
class tab_nation;
class tab_enabler;
class tab_extras;
class tab_terrains;
class req_edit;
class req_vec_fix;
class req_vec_fix_item;

class ruledit_main : public QMainWindow {
  Q_OBJECT

public:
  ruledit_main();

protected:
  void closeEvent(QCloseEvent *cevent) override;
};

/* get 'struct req_edit_list' and related functions: */
#define SPECLIST_TAG req_edit
#define SPECLIST_TYPE class req_edit
#include "speclist.h"

#define req_edit_list_iterate(reqeditlist, preqedit)                        \
  TYPED_LIST_ITERATE(class req_edit, reqeditlist, preqedit)
#define req_edit_list_iterate_end LIST_ITERATE_END

/* get 'struct effect_edit_list' and related functions: */
#define SPECLIST_TAG effect_edit
#define SPECLIST_TYPE class effect_edit
#include "speclist.h"

#define effect_edit_list_iterate(effecteditlist, peffectedit)               \
  TYPED_LIST_ITERATE(class effect_edit, effecteditlist, peffectedit)
#define effect_edit_list_iterate_end LIST_ITERATE_END

/* get 'struct req_vec_fix_list' and related functions: */
#define SPECLIST_TAG req_vec_fix
#define SPECLIST_TYPE class req_vec_fix
#include "speclist.h"

#define req_vec_fix_list_iterate(reqvecfixlist, preqvecfix)                 \
  TYPED_LIST_ITERATE(class req_vec_fix, reqvecfixlist, preqvecfix)
#define req_vec_fix_list_iterate_end LIST_ITERATE_END

class ruledit_gui : public QObject {
  Q_OBJECT

public:
  ruledit_gui(ruledit_main *main);
  ~ruledit_gui() override;

  void display_msg(const char *msg);
  requirers_dlg *create_requirers(const char *title);
  void show_required(requirers_dlg *requirers, const char *msg);
  void flush_widgets();

  void open_req_edit(const QString &target,
                     struct requirement_vector *preqs);
  void unregister_req_edit(class req_edit *redit);

  void open_req_vec_fix(req_vec_fix_item *item_info);
  void unregister_req_vec_fix(req_vec_fix *fixer);

  void open_effect_edit(const QString &target, struct universal *uni,
                        enum effect_filter_main_class efmc);
  void unregister_effect_edit(class effect_edit *e_edit);
  void refresh_effect_edits();

  struct rule_data data;

signals:
  /********************************************************************/ /**
     A requirement vector may have been changed.
     @param vec the requirement vector that was changed.
   ************************************************************************/
  void rec_vec_may_have_changed(const requirement_vector *vec);

private:
  QLabel *msg_dspl;
  QTabWidget *stack;
  QLineEdit *ruleset_select;
  QStackedLayout *main_layout;

  tab_building *bldg;
  tab_misc *misc;
  tab_tech *tech;
  tab_unit *unit;
  tab_good *good;
  tab_gov *gov;
  tab_enabler *enablers;
  tab_extras *extras;
  tab_multiplier *multipliers;
  tab_terrains *terrains;
  tab_nation *nation;

  struct req_edit_list *req_edits;
  struct req_vec_fix_list *req_vec_fixers;
  struct effect_edit_list *effect_edits;

private slots:
  void launch_now();
  void incoming_rec_vec_change(const requirement_vector *vec);
};

void ruledit_qt_display_requirers(const char *msg, void *data);

#endif // FC__RULEDIT_QT_H
