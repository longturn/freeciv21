// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
#include <QVector>
class QString;

QString strvec_to_or_list(const QVector<QString> &psv);
QString strvec_to_and_list(const QVector<QString> &psv);
QString break_lines(const QString &src, int after);
QString qendl();
