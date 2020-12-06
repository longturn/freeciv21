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
#include <QWidget>

enum { SW_STD = 0, SW_TAX = 1, SW_INDICATORS = 2 };

class QPixmap;
class QVBoxLayout;

typedef void (*pfcn_bool)(bool);
typedef void (*pfcn)(void);

void sidebarBlinkEndturn(bool do_restore);
void sidebarCenterUnit();
void sidebarDisableEndturn(bool do_restore);
void sidebarFinishTurn(bool nothing);
void sidebarIndicatorsMenu();
void sidebarRatesWdg(bool nothing);
void sidebarRightClickDiplomacy();
void sidebarRightClickScience();
void sidebarLeftClickScience(bool nothing);
void sidebarShowMap(bool nothing);

/***************************************************************************
  Class representing single widget(icon) on sidebar
***************************************************************************/
class sidebarWidget : public QWidget {
  Q_OBJECT
public:
  sidebarWidget(QPixmap *pix, const QString &label, const QString &pg,
                pfcn_bool func, int type = SW_STD);
  ~sidebarWidget();
  int getPriority();
  QPixmap *get_pixmap();
  void paint(QPainter *painter, QPaintEvent *event);
  void resizePixmap(int width, int height);
  void setCustomLabels(const QString &);
  void setLabel(const QString &str);
  void setLeftClick(pfcn_bool func);
  void setPixmap(QPixmap *pm);
  void setRightClick(pfcn func);
  void setTooltip(const QString &tooltip);
  void setWheelDown(pfcn func);
  void setWheelUp(pfcn func);
  void updateFinalPixmap();

  bool blink;
  bool keep_blinking;
  bool disabled;
  int standard;
  QString page;
public slots:
  void sblink();
  void someSlot();

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
class sidebar : public QWidget {
  Q_OBJECT
public:
  sidebar();
  ~sidebar();
  void addWidget(sidebarWidget *fsw);
  void paint(QPainter *painter, QPaintEvent *event);
  void resizeMe(int height, bool force = false);
  QList<sidebarWidget *> objects;

protected:
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);

private:
  QVBoxLayout *layout;
};
