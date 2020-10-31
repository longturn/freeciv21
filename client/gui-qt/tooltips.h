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

#include <QObject>

class QString;
class QVariant;

QString get_tooltip(QVariant qvar);
QString get_tooltip_improvement(const impr_type *building,
                                struct city *pcity = nullptr,
                                bool ext = false);
QString get_tooltip_unit(const struct unit_type *utype, bool ext = false);
QString bold(const QString &text);

class fc_tooltip : public QObject {
  Q_OBJECT
public:
  explicit fc_tooltip(QObject *parent = NULL) : QObject(parent) {}

protected:
  bool eventFilter(QObject *obj, QEvent *event);
};

