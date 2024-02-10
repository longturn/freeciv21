/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
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
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRubberBand>
#include <QTableWidget>
// utility
#include "fc_types.h"
#include "shortcuts.h"

class QComboBox;
class QFontMetrics;
class QHBoxLayout;
class QIcon;
class QItemSelection;
class QKeyEvent;
class QMouseEvent;
class QMoveEvent;
class QObject;
class QPaintEvent;
class QPushButton;
class QRadioButton;
class QTimerEvent;
class QVBoxLayout;
class move_widget;
class scale_widget;
class close_widget;

struct tile;
struct unit;
struct unit_list;

void show_new_turn_info();
bool has_player_unit_type(Unit_type_id utype);

/****************************************************************************
  Custom message box with animated background
****************************************************************************/
class hud_message_box : public QMessageBox {
  Q_OBJECT
  QElapsedTimer m_timer;

public:
  hud_message_box(QWidget *parent);
  ~hud_message_box() override;
  void set_text_title(const QString &s1, const QString &s2);

protected:
  void paintEvent(QPaintEvent *event) override;
  void timerEvent(QTimerEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  int m_animate_step;
  QString text;
  QString title;
  QFontMetrics *fm_text;
  QFontMetrics *fm_title;
  QFont f_text;
  QString cs1, cs2;
  QFont f_title;
  int top;
  int mult;
};

/****************************************************************************
  Class for showing text on screen
****************************************************************************/
class hud_text : public QWidget {
  Q_OBJECT

public:
  hud_text(const QString &s, int time_secs, QWidget *parent);
  ~hud_text() override;
  void show_me();

protected:
  void paintEvent(QPaintEvent *event) override;
  void timerEvent(QTimerEvent *event) override;

private:
  void center_me();
  QRect bound_rect;
  int timeout;
  int m_animate_step;
  QString text;
  QElapsedTimer m_timer;
  QFontMetrics *fm_text;
  QFont f_text;
  QFont f_title;
};

/****************************************************************************
  Custom input box with animated background
****************************************************************************/
class hud_input_box : public QDialog {
  Q_OBJECT
  QElapsedTimer m_timer;

public:
  hud_input_box(QWidget *parent);
  ~hud_input_box() override;
  void set_text_title_definput(const QString &s1, const QString &s2,
                               const QString &def_input);
  QLineEdit input_edit;

protected:
  void paintEvent(QPaintEvent *event) override;
  void timerEvent(QTimerEvent *event) override;

private:
  int m_animate_step;
  QString text;
  QString title;
  QFontMetrics *fm_text;
  QFontMetrics *fm_title;
  QFont f_text;
  QString cs1, cs2;
  QFont f_title;
  int top;
  int mult;
};

/****************************************************************************
  Custom label to center on current unit
****************************************************************************/
class click_label : public QLabel {
  Q_OBJECT

public:
  click_label();
signals:
  void left_clicked();
private slots:
  void mouse_clicked();

protected:
  void mousePressEvent(QMouseEvent *e) override;
};

/****************************************************************************
  Single action on unit actions
****************************************************************************/
class hud_action : public QWidget {
  Q_OBJECT
  const QIcon icon;
  bool focus;

public:
  hud_action(QWidget *parent, const QIcon &icon, shortcut_id shortcut);
  ~hud_action() override;
  shortcut_id action_shortcut;
signals:
  void left_clicked();
  void right_clicked();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void enterEvent(QEvent *event) override;
private slots:
  void mouse_clicked();
  void mouse_right_clicked();
};

/****************************************************************************
  List of unit actions
****************************************************************************/
class unit_actions : public QWidget {
  Q_OBJECT

public:
  unit_actions(QWidget *parent, unit *punit);
  ~unit_actions() override;
  void init_layout();
  int update_actions();
  void clear_layout();
  QHBoxLayout *layout;
  QList<hud_action *> actions;

private:
  unit *current_unit;
};

/****************************************************************************
  Widget showing current unit, tile and possible actions
****************************************************************************/
class hud_units : public QFrame {
  Q_OBJECT
  click_label unit_label;
  click_label tile_label;
  QLabel text_label;
  QFont *ufont;
  QHBoxLayout *main_layout;
  unit_actions *unit_icons;

public:
  hud_units(QWidget *parent);
  ~hud_units() override;
  void update_actions();

protected:
  void moveEvent(QMoveEvent *event) override;

private:
  move_widget *mw;
  unit_list *ul_units;
  tile *current_tile;
};

/****************************************************************************
  Widget allowing load units on transport
****************************************************************************/
class hud_unit_loader : public QTableWidget {
  QList<unit *> transports;
  Q_OBJECT
public:
  hud_unit_loader(struct unit *pcargo, struct tile *ptile);
  ~hud_unit_loader() override;
  void show_me();
protected slots:
  void selection_changed(const QItemSelection &, const QItemSelection &);

private:
  struct unit *cargo;
  struct tile *qtile;
};

/****************************************************************************
  Widget showing one combat result
****************************************************************************/
class hud_unit_combat : public QWidget {
  Q_OBJECT

public:
  hud_unit_combat(int attacker_unit_id, int defender_unit_id,
                  int attacker_hp, int defender_hp, bool make_att_veteran,
                  bool make_def_veteran, float scale, QWidget *parent);
  ~hud_unit_combat() override;
  bool get_focus();
  void set_fading(float fade);
  void set_scale(float scale);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *event) override;
  void enterEvent(QEvent *event) override;

private:
  void init_images(bool redraw = false);
  int att_hp = 0;
  int def_hp = 0;
  int att_hp_loss = 0;
  int def_hp_loss = 0;
  bool att_veteran = false;
  bool def_veteran = false;
  struct unit *attacker = nullptr;
  struct unit *defender = nullptr;
  const struct unit_type *type_attacker = nullptr;
  const struct unit_type *type_defender = nullptr;
  struct tile *center_tile = nullptr;
  bool focus = false;
  float fading = 0.0f;
  float hud_scale;
  QImage dimg, aimg;
  bool att_win = false;
  bool def_win = false;
};

/****************************************************************************
  Widget showing combat log
****************************************************************************/
class hud_battle_log : public QFrame {
  Q_OBJECT
  QVBoxLayout *main_layout;
  QList<hud_unit_combat *> lhuc;

public:
  hud_battle_log(QWidget *parent);
  ~hud_battle_log() override;
  void add_combat_info(hud_unit_combat *huc);
  void set_scale(float s);
  float scale;

protected:
  void paintEvent(QPaintEvent *event) override;
  void moveEvent(QMoveEvent *event) override;
  void timerEvent(QTimerEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  void update_size();
  scale_widget *sw;
  close_widget *clw;
  move_widget *mw;
  QElapsedTimer m_timer;
};
