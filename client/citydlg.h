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

#include "fc_types.h"

// Qt
#include <QDialog>
#include <QElapsedTimer>
#include <QGroupBox>
#include <QItemDelegate>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QTableWidget>
#include <QToolTip>
#include <QtMath>
// gui-qt
#include "dialogs.h"

class QAction;
class QCheckBox;
class QCloseEvent;
class QContextMenuEvent;
class QEvent;
class QFont;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QHideEvent;
class QItemSelection;
class QMenu;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPushButton;
class QRadioButton;
class QRect;
class QResizeEvent;
class QShowEvent;
class QSlider;
class QSplitter;
class QTableWidget;
class QTableWidgetItem;
class QTimerEvent;
class QVBoxLayout;
class QVariant;
class fc_tooltip;
class QPixmap;

/****************************************************************************
  A list widget that sets its size hint to the size of its contents.
****************************************************************************/
class icon_list : public QListWidget {
public:
  explicit icon_list(QWidget *parent = nullptr);
  QSize viewportSizeHint() const override;
  bool oneliner;
};

#define NUM_INFO_FIELDS 15
/****************************************************************************
  Custom progressbar with animated progress and right click event
****************************************************************************/
class progress_bar : public QProgressBar {
  Q_OBJECT
  QElapsedTimer m_timer;
signals:
  void clicked();

public:
  progress_bar(QWidget *parent);
  ~progress_bar() override;
  void mousePressEvent(QMouseEvent *event) override
  {
    Q_UNUSED(event);
    emit clicked();
  }
  void set_pixmap(struct universal *target);
  void set_pixmap(int n);

protected:
  void paintEvent(QPaintEvent *event) override;
  void timerEvent(QTimerEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void create_region();
  int m_animate_step;
  QPixmap *pix;
  QRegion reg;
  QFont *sfont;
};

/****************************************************************************
  Single item on unit_info in city dialog representing one unit
****************************************************************************/
class unit_list_item : public QObject, public QListWidgetItem {
  Q_OBJECT

public:
  unit_list_item(unit *punit);

  bool can_issue_orders() const;
  QMenu *menu() { return m_menu; }

  // Will hopefully become slots in unit class one day
  void disband();
  void change_homecity();
  void activate_and_close_dialog();
  void sentry();
  void fortify();
  void upgrade();
  void load();
  void unload();
  void unload_all();

private:
  void create_menu();

  QMenu *m_menu = nullptr;
  struct unit *m_unit;
};

/****************************************************************************
  Single item on unit_info in city dialog representing one unit
****************************************************************************/
class impr_item : public QLabel {
  Q_OBJECT

public:
  impr_item(QWidget *parent, const struct impr_type *building,
            struct city *pcity);
  ~impr_item() override;
  void init_pix();

private:
  const struct impr_type *impr;
  QPixmap *impr_pixmap;
  struct city *pcity;

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void enterEvent(QEvent *event) override;
};

/****************************************************************************
  Shows list of improvemrnts
****************************************************************************/
class impr_info : public QFrame {
  Q_OBJECT

public:
  impr_info();
  ~impr_info() override;
  void add_item(impr_item *item);
  void init_layout();
  void update_buildings();
  void clear_layout();
  QHBoxLayout *layout;
  QList<impr_item *> impr_list;
};

/****************************************************************************
  Item delegate for production popup
****************************************************************************/
class city_production_delegate : public QItemDelegate {
  Q_OBJECT

public:
  city_production_delegate(QPoint sh, QObject *parent, struct city *city);
  ~city_production_delegate() override = default;
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;

private:
  int item_height;
  QPoint pd;
  struct city *pcity;

protected:
  void drawFocus(QPainter *painter, const QStyleOptionViewItem &option,
                 const QRect &rect) const override;
};

/****************************************************************************
  Single item in production popup
****************************************************************************/
class production_item : public QObject {
  Q_OBJECT

public:
  production_item(struct universal *ptarget, QObject *parent);
  ~production_item() override;
  inline int columnCount() const { return 1; }
  QVariant data() const;

private:
  struct universal *target;
};

/***************************************************************************
  City production model
***************************************************************************/
class city_production_model : public QAbstractListModel {
  Q_OBJECT

public:
  city_production_model(struct city *pcity, bool f, bool su, bool sw,
                        bool sb, QObject *parent = 0);
  ~city_production_model() override;
  inline int
  rowCount(const QModelIndex &index = QModelIndex()) const override
  {
    Q_UNUSED(index);
    return (qCeil(static_cast<float>(city_target_list.size()) / 3));
  }
  int columnCount(const QModelIndex &parent = QModelIndex()) const override
  {
    Q_UNUSED(parent);
    return 3;
  }
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  void populate();
  QPoint sh;

private:
  QList<production_item *> city_target_list;
  struct city *mcity;
  bool future_t;
  bool show_units;
  bool show_buildings;
  bool show_wonders;
};

/****************************************************************************
  Class for popup avaialable production
****************************************************************************/
class production_widget : public QTableView {
  Q_OBJECT

  city_production_model *list_model;
  city_production_delegate *c_p_d;

public:
  production_widget(QWidget *parent, struct city *pcity, bool future,
                    int when, int curr, bool show_units, bool buy = false,
                    bool show_wonders = true, bool show_buildings = true);
  ~production_widget() override;

public slots:
  void prod_selected(const QItemSelection &sl, const QItemSelection &ds);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *ev) override;

private:
  struct city *pw_city;
  int when_change;
  int curr_selection;
  bool buy_it;
  fc_tooltip *fc_tt;
};

class cityIconInfoLabel : public QWidget {
  Q_OBJECT

public:
  cityIconInfoLabel(QWidget *parent = 0);
  void setCity(struct city *pcity);
  void updateText();
  void updateTooltip(int, const QString &);

private:
  void initLayout();
  struct city *pcity{nullptr};
  QLabel labs[12];
  int pixHeight;
};

/****************************************************************************
  city_label is used only for showing citizens icons
  and was created to catch mouse events
****************************************************************************/
class city_label : public QLabel {
  Q_OBJECT

public:
  city_label(QWidget *parent = 0);
  void set_city(struct city *pcity);
  void set_type(int);

private:
  struct city *pcity{nullptr};
  int type;

protected:
  void mousePressEvent(QMouseEvent *event) override;
};

class city_info : public QWidget {
  Q_OBJECT

public:
  city_info(QWidget *parent = 0);
  void update_labels(struct city *ci_city, cityIconInfoLabel *);

private:
  QLabel *qlt[NUM_INFO_FIELDS];
};

class governor_sliders : public QGroupBox {
  Q_OBJECT

public:
  governor_sliders(QWidget *parent = 0);
  void update_sliders(struct cm_parameter &param);
  QCheckBox *cma_celeb_checkbox{nullptr};
  QSlider *slider_tab[2 * O_LAST + 2]{nullptr};
private slots:
  void cma_slider(int val);
  void cma_celebrate_changed(int val);
};

#include "ui_citydlg.h"
/****************************************************************************
  City dialog
****************************************************************************/
class city_dialog : public QWidget {

  Q_OBJECT
  Q_DISABLE_COPY(city_dialog);
  Ui::FormCityDlg ui;
  QPixmap *citizen_pixmap;
  bool future_targets{false}, show_units{true}, show_wonders{true},
      show_buildings{true};
  int selected_row_p;
  city_label *lab_table[6];

public:
  city_dialog(QWidget *parent = 0);

  ~city_dialog() override;
  void setup_ui(struct city *qcity);
  void refresh();
  void cma_check_agent();
  struct city *pcity;
  bool dont_focus{false};

private:
  void update_title();
  void update_building();
  void update_info_label();
  void update_buy_button();
  void update_citizens();
  void update_improvements();
  void update_units();
  void update_nation_table();
  void update_cma_tab();
  void update_disabled();
  void update_sliders();
  void update_prod_buttons();
  void change_production(bool next);

private slots:
  void next_city();
  void prev_city();
  void get_city(bool next);
  void show_targets();
  void show_targets_worklist();
  void buy();
  void dbl_click_p(QTableWidgetItem *item);
  void item_selected(const QItemSelection &sl, const QItemSelection &ds);
  void clear_worklist();
  void save_worklist();
  void worklist_up();
  void worklist_down();
  void worklist_del();
  void display_worklist_menu(const QPoint);
  void disband_state_changed(bool allow_disband);
  void cma_remove();
  void cma_enable();
  void cma_changed();
  void cma_selected(const QItemSelection &sl, const QItemSelection &ds);
  void cma_double_clicked(int row, int column);
  void cma_context_menu(const QPoint p);
  void save_cma();
  void city_rename();

protected:
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;
};

void destroy_city_dialog();
void city_font_update();
