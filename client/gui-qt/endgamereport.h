/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

#ifndef FC__ENDGAMEREPORT_H
#define FC__ENDGAMEREPORT_H

// Qt
#include <QObject>
#include <QString>
#include <QWidget>
// client
#include "repodlgs_g.h"
class QObject;
class QTableWidget;

/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class endgame_report : public QWidget {
  Q_OBJECT
  QTableWidget *end_widget;

public:
  endgame_report(const struct packet_endgame_report *packet);
  ~endgame_report();
  void update_report(const struct packet_endgame_player *packet);
  void init();

private:
  int index;
  int players;
};

void popdown_endgame_report();
void popup_endgame_report();

#endif /* FC__ENDGAMEREPORT_H */
