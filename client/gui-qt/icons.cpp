/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "icons.h"
// Qt
#include <QIcon>
// utility
#include "shared.h"

QString current_theme;
fcIcons *fcIcons::m_instance = 0;

/************************************************************************/ /**
   Icon provider constructor
 ****************************************************************************/
fcIcons::fcIcons() {}

/************************************************************************/ /**
   Returns instance of fc_icons
 ****************************************************************************/
fcIcons *fcIcons::instance()
{
  if (!m_instance) {
    m_instance = new fcIcons;
  }
  return m_instance;
}

/************************************************************************/ /**
   Deletes fc_icons instance
 ****************************************************************************/
void fcIcons::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

/************************************************************************/ /**
   Returns icon by given name
 ****************************************************************************/
QIcon fcIcons::getIcon(const QString &id)
{
  QIcon icon;
  QString str;
  QByteArray pn_bytes;
  QByteArray png_bytes;

  str = QStringLiteral("themes") + DIR_SEPARATOR + "gui-qt" + DIR_SEPARATOR;
  /* Try custom icon from theme */
  pn_bytes = str.toLocal8Bit();
  png_bytes =
      QString(pn_bytes.data() + current_theme + DIR_SEPARATOR + id + ".png")
          .toLocal8Bit();
  icon.addFile(fileinfoname(get_data_dirs(), png_bytes.data()));
  str = str + "icons" + DIR_SEPARATOR;
  /* Try icon from icons dir */
  if (icon.isNull()) {
    pn_bytes = str.toLocal8Bit();
    png_bytes = QString(pn_bytes.data() + id + ".png").toLocal8Bit();
    icon.addFile(fileinfoname(get_data_dirs(), png_bytes.data()));
  }

  return QIcon(icon);
}

/************************************************************************/ /**
   Returns pixmap by given name, pixmap needs to be deleted by someone else
 ****************************************************************************/
QPixmap *fcIcons::getPixmap(const QString &id)
{
  QPixmap *pm;
  bool status;
  QString str;
  QByteArray png_bytes;

  pm = new QPixmap;
  if (QPixmapCache::find(id, pm)) {
    return pm;
  }
  str = QStringLiteral("themes") + DIR_SEPARATOR + "gui-qt" + DIR_SEPARATOR;
  png_bytes = QString(str + current_theme + DIR_SEPARATOR + id + ".png")
                  .toLocal8Bit();
  status = pm->load(fileinfoname(get_data_dirs(), png_bytes.data()));

  if (!status) {
    str = str + "icons" + DIR_SEPARATOR;
    png_bytes = QString(str + id + ".png").toLocal8Bit();
    pm->load(fileinfoname(get_data_dirs(), png_bytes.data()));
  }
  QPixmapCache::insert(id, *pm);

  return pm;
}

/************************************************************************/ /**
   Returns path for icon
 ****************************************************************************/
QString fcIcons::getPath(const QString &id)
{
  QString str;
  QByteArray png_bytes;

  str = QStringLiteral("themes") + DIR_SEPARATOR + "gui-qt" + DIR_SEPARATOR
        + "icons" + DIR_SEPARATOR;
  png_bytes = QString(str + id + ".png").toLocal8Bit();

  return fileinfoname(get_data_dirs(), png_bytes.data());
}
