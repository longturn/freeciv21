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
#include <QAbstractListModel>
#include <QItemDelegate>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QWidget>
// client
#include "climisc.h"
#include "views/view_cities_data.h"

#define CMA_NONE (10000)

class QHBoxLayout;
class QItemSelection;
class QLabel;
class QMenu;
class QMenu;
class QPainter;
class QPoint;
class QPushButton;
class QSortFilterProxyModel;
class QTableWidget;
class QVBoxLayout;
class QVBoxLayout;
class city_report;
class city_report;
struct city;
template <class Key, class T> class QMap;

class city_sort_model : public QSortFilterProxyModel {
  bool lessThan(const QModelIndex &left,
                const QModelIndex &right) const override;
};

/***************************************************************************
  Item delegate for painting in model of city table
***************************************************************************/
class city_item_delegate : public QItemDelegate {
  Q_OBJECT

public:
  city_item_delegate(QObject *parent);
  ~city_item_delegate() override = default;
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;

private:
  int item_height;
};

/***************************************************************************
  Single item in model of city view table
***************************************************************************/
class city_item : public QObject {
  Q_OBJECT

public:
  city_item(struct city *pcity);
  inline int columnCount() const { return NUM_CREPORT_COLS; }
  QVariant data(int column, int role = Qt::DisplayRole) const;
  bool setData(int column, const QVariant &value,
               int role = Qt::DisplayRole);
  struct city *get_city();

private:
  struct city *i_city;
};

/***************************************************************************
  City model
***************************************************************************/
class city_model : public QAbstractListModel {
  Q_OBJECT

public:
  city_model(QObject *parent = 0);
  ~city_model() override;
  inline int
  rowCount(const QModelIndex &index = QModelIndex()) const override
  {
    Q_UNUSED(index);
    return city_list.size();
  }
  int columnCount(const QModelIndex &parent = QModelIndex()) const override
  {
    Q_UNUSED(parent);
    return NUM_CREPORT_COLS;
  }
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::DisplayRole) override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  QVariant menu_data(int section) const;
  QVariant hide_data(int section) const;
  void populate();
  void city_changed(struct city *pcity);
  void all_changed();
private slots:
  void notify_city_changed(int row);

private:
  QList<city_item *> city_list;
};

/***************************************************************************
  City widget to show city model
***************************************************************************/
class city_widget : public QTreeView {
  Q_OBJECT
  city_model *list_model;
  QSortFilterProxyModel *filter_model;
  city_item_delegate *c_i_d;
  city_report *cr;
  enum menu_labels {
    CHANGE_PROD_NOW = 1,
    CHANGE_PROD_NEXT,
    CHANGE_PROD_LAST,
    CHANGE_PROD_BEF_LAST,
    CMA,
    SELL,
    WORKLIST_ADD,
    WORKLIST_CHANGE,
    SELECT_IMPR,
    SELECT_WONDERS,
    SELECT_SUPP_UNITS,
    SELECT_PRES_UNITS,
    SELECT_AVAIL_UNITS,
    SELECT_AVAIL_IMPR,
    SELECT_AVAIL_WONDERS
  };

public:
  city_widget(city_report *ctr);
  ~city_widget() override;
  QList<city *> selected_cities;
  void update_model();
  void update_city(struct city *pcity);
public slots:
  void display_header_menu(const QPoint);
  void hide_columns();
  void city_doubleclick(const QModelIndex &index);
  void city_view();
  void clear_worlist();
  void cities_selected(const QItemSelection &sl, const QItemSelection &ds);
  void display_list_menu(const QPoint);
  void buy();
  void center();
  void select_all();
  void select_none();
  void invert_selection();
  void select_coastal();
  void select_building_something();
  void select_same_island();

private:
  void restore_selection();
  void select_city(struct city *pcity);
  void gen_cma_labels(QMap<QString, int> &list);
  void gen_select_labels(QMenu *menu);
  void gen_worklist_labels(QMap<QString, int> &list);
  void gen_production_labels(menu_labels which, QMap<QString, cid> &list,
                             bool append_units, bool append_wonders,
                             TestCityFunc test_func, bool global = false);
  void fill_data(menu_labels which, QMap<QString, cid> &custom_labels,
                 QMenu *menu);
  void fill_production_menus(city_widget::menu_labels what,
                             QMap<QString, cid> &custom_labels,
                             TestCityFunc test_func, QMenu *menu);
  void sell(const struct impr_type *building);
};

/***************************************************************************
  Widget to show as tab widget in cities view.
***************************************************************************/
class city_report : public QWidget {
  Q_OBJECT
  city_widget *city_wdg;
  QVBoxLayout *layout;

public:
  city_report();
  ~city_report() override;
  void update_report();
  void update_city(struct city *pcity);
  void init();

private:
  int index;
};

void popdown_city_report();
