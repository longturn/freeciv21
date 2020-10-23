/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

#ifndef FC__ECONOMYREPORT_H
#define FC__ECONOMYREPORT_H

// Qt
#include <QLabel>
#include <QPushButton>
#include <QWidget>
// client
#include "climisc.h"
#include "repodlgs_g.h"

class QItemSelection;
class QLabel;
class QObject;
class QPushButton;
class QTableWidget;

/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class eco_report : public QWidget {
  Q_OBJECT
  QPushButton *disband_button;
  QPushButton *sell_button;
  QPushButton *sell_redun_button;
  QTableWidget *eco_widget;
  QLabel *eco_label;

public:
  eco_report();
  ~eco_report();
  void update_report();
  void init();

private:
  int index;
  int curr_row;
  int max_row;
  cid uid;
  int counter;

private slots:
  void disband_units();
  void sell_buildings();
  void sell_redundant();
  void selection_changed(const QItemSelection &sl, const QItemSelection &ds);
};

void popdown_economy_report();

#endif /* FC__ECONOMYREPORT_H */
