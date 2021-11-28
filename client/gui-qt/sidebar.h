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

class QPixmap;
class QVBoxLayout;

typedef void (*pfcn_bool)(bool);
typedef void (*pfcn)();

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
  enum standards { SW_STD, SW_TAX, SW_INDICATORS };

  sidebarWidget(QPixmap *pix, const QString &label, const QString &pg,
                pfcn_bool func, standards type = SW_STD);
  ~sidebarWidget() override;
  int getPriority();
  QPixmap *get_pixmap();
  int heightForWidth(int width) const override;
  bool hasHeightForWidth() const override;
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
  standards standard;
  QString page;
public slots:
  void sblink();
  void someSlot();

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void enterEvent(QEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void paint();
  bool hover;
  pfcn right_click;
  pfcn wheel_down;
  pfcn wheel_up;
  pfcn_bool left_click;
  QFont sfont;
  QFont info_font;
  QPixmap *def_pixmap;
  QPixmap *final_pixmap;
  QPixmap *scaled_pixmap;
  QString custom_label;
  QString desc;
  QTimer *timer;
};

/***************************************************************************
  Freeciv21 sidebar
***************************************************************************/
class sidebar : public QWidget {
  Q_OBJECT

public:
  sidebar();
  ~sidebar() override;
  void addWidget(sidebarWidget *fsw);
  void addSpacer();
  void paint(QPainter *painter, QPaintEvent *event);
  void resizeMe();
  QList<sidebarWidget *> objects;

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  QVBoxLayout *layout;
};
