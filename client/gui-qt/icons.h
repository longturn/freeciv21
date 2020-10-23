/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__ICONS_H
#define FC__ICONS_H

#include <QPixmapCache>

/****************************************************************************
  Class helping reading icons/pixmaps from themes/gui-qt/icons folder
****************************************************************************/
class fc_icons {
  Q_DISABLE_COPY(fc_icons);

private:
  explicit fc_icons();
  static fc_icons *m_instance;

public:
  static fc_icons *instance();
  static void drop();
  QIcon get_icon(const QString &id);
  QPixmap *get_pixmap(const QString &id);
  QString get_path(const QString &id);
};

#endif /* FC__ICONS_H */
