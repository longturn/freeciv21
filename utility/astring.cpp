/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include "astring.h"

#include "fcintl.h"

QString strvec_to_or_list(const QVector<QString> &psv)
{
  if (psv.size() == 1) {
    // TRANS: "or"-separated string list with one single item.
    return QString(Q_("?or-list-single:%1")).arg(psv[0]);
  } else if (psv.size() == 2) {
    // TRANS: "or"-separated string list with 2 items.
    return QString(Q_("?or-list:%1 or %2")).arg(psv[0], psv[1]);
  } else {
    /* TRANS: start of an "or"-separated string list with more than two
     * items. */
    auto result = QString(Q_("?or-list:%1")).arg(psv[0]);
    for (int i = 1; i < psv.size() - 1; ++i) {
      /* TRANS: next elements of an "or"-separated string list with more
       * than two items. */
      result += QString(Q_("?or-list:, %1")).arg(psv[i]);
    }
    /* TRANS: end of an "or"-separated string list with more than two
     * items. */
    return result + QString(Q_("?or-list:, or %1")).arg(psv.back());
  }
}

QString strvec_to_and_list(const QVector<QString> &psv)
{
  if (psv.size() == 1) {
    // TRANS: "and"-separated string list with one single item.
    return QString(Q_("?and-list-single:%1")).arg(psv[0]);
  } else if (psv.size() == 2) {
    // TRANS: "and"-separated string list with 2 items.
    return QString(Q_("?and-list:%1 and %2")).arg(psv[0], psv[1]);
  } else {
    // TRANS: start of an "and"-separated string list with more than two
    // items.
    auto result = QString(Q_("?and-list:%1")).arg(psv[0]);
    for (int i = 1; i < psv.size() - 1; ++i) {
      // TRANS: next elements of an "and"-separated string list with more
      // than two items.
      result += QString(Q_("?and-list:, %1")).arg(psv[i]);
    }
    // TRANS: end of an "and"-separated string list with more than two items.
    return result + QString(Q_("?and-list:, and %1")).arg(psv.back());
  }
}

QString qendl() { return QStringLiteral("\n"); }

// break line after after n-th char
QString break_lines(const QString &src, int after)
{
  QStringList broken =
      src.split(QStringLiteral(" "), Qt::SkipEmptyParts);
  QString dst;

  int clen = 0;
  while (!broken.isEmpty()) {
    QString s = broken.takeFirst();
    dst += s + " ";
    clen += s.length();
    if (s.contains('\n')) {
      clen = 0;
      continue;
    }
    if (clen > after) {
      dst += qendl();
      clen = 0;
    }
  }
  return dst;
}
