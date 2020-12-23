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

#include <QPixmapCache>

/****************************************************************************
  Class helping reading icons/pixmaps from themes/gui-qt/icons folder
****************************************************************************/
class fcIcons {
  Q_DISABLE_COPY(fcIcons);

private:
  explicit fcIcons();
  static fcIcons *m_instance;

public:
  static fcIcons *instance();
  static void drop();
  QIcon getIcon(const QString &id);
  QPixmap *getPixmap(const QString &id);
  QString getPath(const QString &id);
};

// header city icons
class hIcon {
  Q_DISABLE_COPY(hIcon);

private:
  explicit hIcon(){};
  static hIcon *m_instance;
  QHash<QString, QIcon> hash;

public:
  static hIcon *i();
  static void drop();
  void createIcons();
  QIcon get(const QString &id);
};
