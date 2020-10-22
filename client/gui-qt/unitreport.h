/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

#ifndef FC__UNITREPORT_H
#define FC__UNITREPORT_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "repodlgs_g.h"

// gui-qt
#include "mapview.h"

// Qt
#include <QLabel>
#include <QPushButton>
#include <QWidget>

class QGridLayout;
class QHBoxLayout;
class QItemSelection;
class QTableWidget;
class QTableWidgetItem;

class unittype_item : public QFrame {

  Q_OBJECT

  bool entered;
  int unit_scroll;
  QLabel label_pix;

public:
  unittype_item(QWidget *parent, unit_type *ut);
  ~unittype_item();
  void init_img();
  QLabel food_upkeep;
  QLabel gold_upkeep;
  QLabel label_info_active;
  QLabel label_info_inbuild;
  QLabel label_info_unit;
  QLabel shield_upkeep;
  QPushButton upgrade_button;

private:
  unit_type *utype;

private slots:
  void upgrade_units();

protected:
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void paintEvent(QPaintEvent *event);
  void wheelEvent(QWheelEvent *event);
};

class units_reports : public fcwidget {

  Q_DISABLE_COPY(units_reports);
  Q_OBJECT

  close_widget *cw;
  explicit units_reports();
  QHBoxLayout *scroll_layout;
  QScrollArea *scroll;
  QWidget scroll_widget;
  static units_reports *m_instance;

public:
  ~units_reports();
  static units_reports *instance();
  static void drop();
  void clear_layout();
  void init_layout();
  void update_units(bool show = false);
  void add_item(unittype_item *item);
  virtual void update_menu();
  QHBoxLayout *layout;
  QList<unittype_item *> unittype_list;

protected:
  void paintEvent(QPaintEvent *event);
};

void popdown_units_report();
void toggle_units_report(bool);

#endif /* FC__UNITREPORT_H */
