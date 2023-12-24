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

// client
#include "utils/collated_sort_proxy.h"
#include "views/view_nations_data.h"

// Qt
#include <QAbstractListModel>
#include <QItemDelegate>
#include <QTableView>
#include <QWidget>

class QHBoxLayout;
class QItemSelection;
class QLabel;
class QMouseEvent;
class QPainter;
class QPoint;
class QPushButton;
class QSortFilterProxyModel;
class QSplitter;
class QVBoxLayout;
class plr_report;

/***************************************************************************
  Item delegate for painting in model of nations view table
***************************************************************************/
class plr_item_delegate : public QItemDelegate {
  Q_OBJECT

public:
  plr_item_delegate(QObject *parent) : QItemDelegate(parent) {}
  ~plr_item_delegate() override = default;
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;
};

/***************************************************************************
  Single item in model of nations view table
***************************************************************************/
class plr_item : public QObject {
  Q_OBJECT

public:
  plr_item(struct player *pplayer);
  inline int columnCount() const { return num_player_dlg_columns; }
  QVariant data(int column, int role = Qt::DisplayRole) const;
  bool setData(int column, const QVariant &value,
               int role = Qt::DisplayRole);

private:
  struct player *ipplayer;
};

/***************************************************************************
  Nation/Player model
***************************************************************************/
class plr_model : public QAbstractListModel {
  Q_OBJECT

public:
  plr_model(QObject *parent = 0);
  ~plr_model() override;
  inline int
  rowCount(const QModelIndex &index = QModelIndex()) const override
  {
    Q_UNUSED(index);
    return plr_list.size();
  }
  int columnCount(const QModelIndex &parent = QModelIndex()) const override
  {
    Q_UNUSED(parent);
    return num_player_dlg_columns;
  }
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::DisplayRole) override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  QVariant hide_data(int section) const;
  void populate();
private slots:
  void notify_plr_changed(int row);

private:
  QList<plr_item *> plr_list;
};

/***************************************************************************
  Player widget to show player/nation model
***************************************************************************/
class plr_widget : public QTableView {
  Q_OBJECT
  plr_model *list_model;
  freeciv::collated_sort_filter_proxy_model *filter_model;
  plr_item_delegate *pid;
  plr_report *plr;
  QString techs_known;
  QString techs_unknown;
  struct player *selected_player;

public:
  plr_widget(QWidget *);
  ~plr_widget() override;
  void set_pr_rep(plr_report *pr);
  void restore_selection();
  plr_model *get_model() const;
  QString intel_str;
  QString ally_str;
  QString tech_str;
  struct player *other_player;
public slots:
  void display_header_menu(const QPoint);
  void nation_selected(const QItemSelection &sl, const QItemSelection &ds);

private:
  void mousePressEvent(QMouseEvent *event) override;
  void hide_columns();
};

#include "ui_view_nations.h"
/***************************************************************************
  Widget to show as tab widget in players view.
***************************************************************************/
class plr_report : public QWidget {
  Q_OBJECT

public:
  plr_report();
  ~plr_report() override;
  void update_report(bool update_selection = true);
  void init();
  void call_meeting();

private:
  Ui::FormPlrDlg ui;
  struct player *other_player;
  int index;
private slots:
  void req_meeeting();
  void plr_cancel_threaty();
  void plr_withdraw_vision();
  void toggle_ai_mode();
  void plr_diplomacy();
};

void popdown_players_report();
void update_top_bar_diplomacy_status(bool notify);
