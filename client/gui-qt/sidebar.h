/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__SIDEBAR_H
#define FC__SIDEBAR_H

// Qt
#include <QWidget>

enum { SW_STD = 0, SW_TAX = 1, SW_INDICATORS = 2 };

class QPixmap;
class QVBoxLayout;

typedef void (*pfcn_bool)(bool);
typedef void (*pfcn)(void);

void side_blink_endturn(bool do_restore);
void side_center_unit();
void side_disable_endturn(bool do_restore);
void side_finish_turn(bool nothing);
void side_indicators_menu();
void side_rates_wdg(bool nothing);
void side_right_click_diplomacy();
void side_right_click_science();
void side_left_click_science(bool nothing);
void side_show_map(bool nothing);

/***************************************************************************
  Class representing single widget(icon) on sidebar
***************************************************************************/
class fc_sidewidget : public QWidget {
  Q_OBJECT
public:
  fc_sidewidget(QPixmap *pix, const QString &label, const QString &pg,
                pfcn_bool func, int type = SW_STD);
  ~fc_sidewidget();
  int get_priority();
  QPixmap *get_pixmap();
  void paint(QPainter *painter, QPaintEvent *event);
  void resize_pixmap(int width, int height);
  void set_custom_labels(const QString &);
  void set_label(const QString &str);
  void set_left_click(pfcn_bool func);
  void set_pixmap(QPixmap *pm);
  void set_right_click(pfcn func);
  void set_tooltip(const QString &tooltip);
  void set_wheel_down(pfcn func);
  void set_wheel_up(pfcn func);
  void update_final_pixmap();

  bool blink;
  bool keep_blinking;
  bool disabled;
  int standard;
  QString page;
public slots:
  void sblink();
  void some_slot();

protected:
  void contextMenuEvent(QContextMenuEvent *event);
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void paintEvent(QPaintEvent *event);
  void wheelEvent(QWheelEvent *event);

private:
  void paint();
  bool hover;
  pfcn right_click;
  pfcn wheel_down;
  pfcn wheel_up;
  pfcn_bool left_click;
  QFont *sfont;
  QFont *info_font;
  QPixmap *def_pixmap;
  QPixmap *final_pixmap;
  QPixmap *scaled_pixmap;
  QString custom_label;
  QString desc;
  QTimer *timer;
};

/***************************************************************************
  Freeciv sidebar
***************************************************************************/
class fc_sidebar : public QWidget {
  Q_OBJECT
public:
  fc_sidebar();
  ~fc_sidebar();
  void add_widget(fc_sidewidget *fsw);
  void paint(QPainter *painter, QPaintEvent *event);
  void resize_me(int height, bool force = false);
  QList<fc_sidewidget *> objects;

protected:
  void paintEvent(QPaintEvent *event);

private:
  QVBoxLayout *layout;
};

#endif /* FC__SIDEBAR_H */
