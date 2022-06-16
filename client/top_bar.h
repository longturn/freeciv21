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
#include <QToolButton>
#include <QWidget>

class QHBoxLayout;
class QPixmap;

typedef void (*pfcn)();

void top_bar_center_unit();
void top_bar_finish_turn();
void top_bar_indicators_menu();
void top_bar_rates_wdg();
void top_bar_right_click_diplomacy();
void top_bar_right_click_science();
void top_bar_left_click_science();
void top_bar_show_map();

/**
 * Top bar widget for tax rates.
 */
class tax_rates_widget : public QToolButton {
  Q_OBJECT

public:
  tax_rates_widget();
  ~tax_rates_widget() override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
};

/**
 * Top bar widget for indicators (global warming/nuclear winter/science/
 * government).
 */
class indicators_widget : public QToolButton {
  Q_OBJECT

public:
  indicators_widget();
  ~indicators_widget() override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
};

/***************************************************************************
  Class representing single widget(icon) on top_bar
***************************************************************************/
class top_bar_widget : public QToolButton {
  Q_OBJECT

public:
  top_bar_widget(const QString &label, const QString &pg, pfcn func);
  ~top_bar_widget() override;
  int getPriority();
  QPixmap *get_pixmap();
  void paint(QPainter *painter, QPaintEvent *event);
  void setCustomLabels(const QString &);
  void setLabel(const QString &str);
  void setLeftClick(pfcn func);
  void setRightClick(pfcn func);
  void setTooltip(const QString &tooltip);
  void setWheelDown(pfcn func);
  void setWheelUp(pfcn func);

  bool blink;
  bool keep_blinking;
  QString page;
public slots:
  void sblink();
  void someSlot();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  pfcn right_click;
  pfcn wheel_down;
  pfcn wheel_up;
  pfcn left_click;
  QTimer *timer;
};

/**
 * Top bar widget that shows the amount of gold owned by the current player,
 * and their income.
 *
 * The top bar widget displays a warning when the player has negative income
 * or will go bankrupt.
 */
class gold_widget : public top_bar_widget {
  Q_OBJECT
  Q_PROPERTY(warning current_warning READ current_warning)

public:
  /// Types of warnings displayed by gold_widget
  enum class warning {
    no_warning = 0,   ///< Used when no warning is shown
    losing_money = 1, ///< The current player has negative gold income
    low_on_funds = 2, ///< The current player will soon go bankrupt
  };
  Q_ENUM(warning)

  gold_widget();
  ~gold_widget() override;

  /// Returns the incom as currently shown.
  int income() const { return m_gold; }

  /// Changes the gold amount shown by the widget.
  void set_income(int income)
  {
    m_income = income;
    update_contents();
  }

  /// Returns the gold amount as currently shown.
  int gold() const { return m_gold; }

  /// Changes the gold amount shown by the widget.
  void set_gold(int gold)
  {
    m_gold = gold;
    update_contents();
  }

  /// Retrieves the current warning.
  warning current_warning() const { return m_warning; }

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  void update_contents();

  int m_gold = 0, m_income = 0;
  warning m_warning = warning::no_warning;
};

/***************************************************************************
  Freeciv21 top_bar
***************************************************************************/
class top_bar : public QWidget {
  Q_OBJECT

public:
  top_bar();
  ~top_bar() override;
  void addWidget(QWidget *fsw);
  QList<QWidget *> objects;

private:
  QHBoxLayout *layout;
};
