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

#ifndef FC__CONVERSION_LOG_H
#define FC__CONVERSION_LOG_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QTextEdit>

class conversion_log : public QDialog {
  Q_OBJECT

public:
  explicit conversion_log();
  void add(const char *msg);

private:
  QTextEdit *area;

private slots:
  void close_now();
};

#endif // FC__CONVERSION_LOG_H
