/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__MPGUI_QT_H
#define FC__MPGUI_QT_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QMainWindow>
#include <QObject>

// tools
#include "modinst.h"

class QLabel;
class QLineEdit;
class QProgressBar;
class QTableWidget;

class mpgui_main : public QMainWindow {
  Q_OBJECT

public:
  mpgui_main(QApplication *qapp_in, QWidget *central_in);

private:
  void popup_quit_dialog();
  QApplication *qapp;
  QWidget *central;

protected:
  void closeEvent(QCloseEvent *event);
};

class mpgui : public QObject {
  Q_OBJECT

public:
  void setup(QWidget *central, struct fcmp_params *fcmp);
  void display_msg_thr(const QString &msg);
  void progress_thr(int downloaded, int max);
  void setup_list(const char *name, const char *URL, const char *version,
                  const char *license, enum modpack_type type,
                  const char *subtype, const char *notes);
  void refresh_list_versions_thr();

signals:
  void display_msg_thr_signal(QString msg);
  void progress_thr_signal(int downloaded, int max);
  void refresh_list_versions_thr_signal();

public slots:
  void display_msg(const QString &msg);
  void progress(int downloaded, int max);
  void refresh_list_versions();

private slots:
  void URL_given();
  void row_selected(int, int);
  void row_download(const QModelIndex &);

private:
  QLineEdit *URLedit;
  QLabel *msg_dspl;
  QProgressBar *bar;
  QTableWidget *mplist_table;
};

#endif // FC__MPGUI_QT_H
