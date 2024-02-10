/**************************************************************************
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// Qt
#include <QDialog>
#include <QMessageBox>
#include <QVariant>
// utility
#include "fc_types.h"
// client
#include "dialogs_g.h"
#include "hudwidget.h"
#include "widgets/decorations.h"

class QCloseEvent;
class QComboBox;
class QGridLayout;
class QHBoxLayout;
class QItemSelection;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;
class QRadioButton;
class QTableWidget;
class QTextEdit;
class QVBoxLayout;
class QWheelEvent;
struct tile;
struct unit;

typedef void (*pfcn_void)(QVariant, QVariant);
void update_nationset_combo();
void popup_races_dialog(struct player *pplayer);

class qdef_act {
  Q_DISABLE_COPY(qdef_act);

private:
  explicit qdef_act();
  static qdef_act *m_instance;
  action_id vs_city{-1};
  action_id vs_unit{-1};

public:
  static qdef_act *action();
  static void drop();
  void vs_city_set(int i);
  void vs_unit_set(int i);
  action_id vs_city_get();
  action_id vs_unit_get();
};

/***************************************************************************
  Dialog with themed titlebar
***************************************************************************/
class qfc_dialog : public QDialog {
  Q_OBJECT
public:
  qfc_dialog(QWidget *parent);
  ~qfc_dialog() override;

private:
  int titlebar_height;
  QPoint point;
  bool moving_now;
  QPixmap *close_pix;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
};

/***************************************************************************
  Nonmodal message box for disbanding units
***************************************************************************/
class disband_box : public hud_message_box {
  Q_OBJECT
  const std::vector<unit *> cpunits;

public:
  explicit disband_box(const std::vector<unit *> &punits,
                       QWidget *parent = 0);
  ~disband_box() override;
private slots:
  void disband_clicked();
};

/***************************************************************************
 Dialog for goto popup
***************************************************************************/
class notify_goto : public QMessageBox {
  Q_OBJECT
  QPushButton *goto_but;
  QPushButton *inspect_but;
  QPushButton *close_but;
  struct tile *gtile;

public:
  notify_goto(const char *headline, const char *lines,
              const struct text_tag_list *tags, struct tile *ptile,
              QWidget *parent);

private slots:
  void goto_tile();
  void inspect_city();
};

/***************************************************************************
 Dialog for selecting nation, style and leader leader
***************************************************************************/
class races_dialog : public qfc_dialog {
  Q_OBJECT
  QGridLayout *main_layout;
  QTableWidget *nation_tabs;
  QList<QWidget *> *nations_tabs_list;
  QTableWidget *selected_nation_tabs;
  QComboBox *leader_name;
  QComboBox *qnation_set;
  QRadioButton *is_male;
  QRadioButton *is_female;
  QTableWidget *styles;
  QTextEdit *description;
  QPushButton *ok_button;
  QPushButton *random_button;

public:
  races_dialog(struct player *pplayer, QWidget *parent = 0);
  ~races_dialog() override;
  void refresh();
  void update_nationset_combo();

private slots:
  void set_index(int index);
  void nation_selected(const QItemSelection &sl, const QItemSelection &ds);
  void style_selected(const QItemSelection &sl, const QItemSelection &ds);
  void group_selected(const QItemSelection &sl, const QItemSelection &ds);
  void nationset_changed(int index);
  void leader_selected(int index);
  void ok_pressed();
  void cancel_pressed();
  void random_pressed();

private:
  int selected_nation;
  int selected_style;
  struct player *tplayer;
  int last_index;
};

/**************************************************************************
  A QPushButton that includes data like function to call and parmeters
**************************************************************************/
class Choice_dialog_button : public QPushButton {
  Q_OBJECT
  pfcn_void func;
  QVariant data1, data2;

public:
  Choice_dialog_button(const QString title, pfcn_void func_in,
                       QVariant data1_in, QVariant data2_in);
  pfcn_void getFunc();
  QVariant getData1();
  QVariant getData2();
  void setData1(QVariant wariat);
  void setData2(QVariant wariat);
};

/***************************************************************************
  Simple choice dialog, allowing choosing one of set actions
***************************************************************************/
class choice_dialog : public QWidget {
  Q_OBJECT
  QPushButton *target_unit_button;
  QVBoxLayout *layout;
  QHBoxLayout *unit_skip;
  QList<Choice_dialog_button *> buttons_list;
  QList<Choice_dialog_button *> last_buttons_stack;
  QList<Choice_dialog_button *> action_button_map;
  void (*run_on_close)(int);
  void switch_target();

public:
  choice_dialog(const QString title, const QString text,
                QWidget *parent = nullptr,
                void (*run_on_close_in)(int) = nullptr);
  ~choice_dialog() override;
  void set_layout();
  void add_item(QString title, pfcn_void func, QVariant data1,
                QVariant data2, QString tool_tip, const int button_id);
  void show_me();
  void stack_button(Choice_dialog_button *button);
  void unstack_all_buttons();
  QVBoxLayout *get_layout();
  Choice_dialog_button *get_identified_button(const int id);
  int unit_id;
  int target_id[ATK_COUNT];
  int sub_target_id[ASTK_COUNT];
  struct unit *targeted_unit;
  void update_dialog(const struct act_prob *act_probs);
public slots:
  void execute_action(const int action);
private slots:
  void prev_unit();
  void next_unit();
};

void popup_revolution_dialog(struct government *government = nullptr);
void revolution_response(struct government *government);
void popup_upgrade_dialog(const std::vector<unit *> &punits);
void popup_disband_dialog(const std::vector<unit *> &punits);
bool try_default_unit_action(QVariant q1, QVariant q2);
bool try_default_city_action(QVariant q1, QVariant q2);
