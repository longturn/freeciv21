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
fcIcons *fcIcons::m_instance = nullptr;
hIcon *hIcon::m_instance = nullptr;

/**
   Icon provider constructor
 */
fcIcons::fcIcons() = default;

/**
   Returns instance of fc_icons
 */
fcIcons *fcIcons::instance()
{
  if (!m_instance) {
    m_instance = new fcIcons;
  }
  return m_instance;
}

/**
   Deletes fc_icons instance
 */
void fcIcons::drop() { NFCN_FREE(m_instance); }

/**
   Returns icon by given name
 */
QIcon fcIcons::getIcon(const QString &id)
{
  QIcon icon;
  QString str;
  QByteArray pn_bytes;
  QByteArray png_bytes;

  str = QStringLiteral("themes/gui-qt/");
  // Try custom icon from theme
  pn_bytes = str.toLocal8Bit();
  png_bytes = QString(pn_bytes.data() + current_theme + "/" + id + ".png")
                  .toLocal8Bit();
  icon.addFile(fileinfoname(get_data_dirs(), png_bytes.data()));
  str = str + "icons/";
  // Try icon from icons dir
  if (icon.isNull()) {
    pn_bytes = str.toLocal8Bit();
    png_bytes = QString(pn_bytes.data() + id + ".png").toLocal8Bit();
    icon.addFile(fileinfoname(get_data_dirs(), png_bytes.data()));
  }

  return QIcon(icon);
}

/**
   Returns pixmap by given name, pixmap needs to be deleted by someone else
 */
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
  str = QStringLiteral("themes/gui-qt/");
  png_bytes = QString(str + current_theme + "/" + id + ".png").toLocal8Bit();
  status = pm->load(fileinfoname(get_data_dirs(), png_bytes.data()));

  if (!status) {
    str = str + "icons/";
    png_bytes = QString(str + id + ".png").toLocal8Bit();
    pm->load(fileinfoname(get_data_dirs(), png_bytes.data()));
  }
  QPixmapCache::insert(id, *pm);

  return pm;
}

/**
   Returns path for icon
 */
QString fcIcons::getPath(const QString &id)
{
  QString str;
  QByteArray png_bytes;

  str = QStringLiteral("themes/gui-qt/icons/");
  png_bytes = QString(str + id + ".png").toLocal8Bit();

  return fileinfoname(get_data_dirs(), png_bytes.data());
}

hIcon *hIcon::i()
{
  if (!m_instance) {
    m_instance = new hIcon;
    m_instance->createIcons();
  }
  return m_instance;
}

void hIcon::drop() { NFCN_FREE(m_instance); }

void hIcon::createIcons()
{
  hash.insert(QStringLiteral("prodplus"),
              fcIcons::instance()->getIcon(QStringLiteral("hprod")));
  hash.insert(QStringLiteral("foodplus"),
              fcIcons::instance()->getIcon(QStringLiteral("hfood")));
  hash.insert(QStringLiteral("tradeplus"),
              fcIcons::instance()->getIcon(QStringLiteral("htrade")));
  hash.insert(QStringLiteral("gold"),
              fcIcons::instance()->getIcon(QStringLiteral("hgold")));
  hash.insert(QStringLiteral("science"),
              fcIcons::instance()->getIcon(QStringLiteral("hsci")));
  hash.insert(QStringLiteral("resize"),
              fcIcons::instance()->getIcon(QStringLiteral("resize")));
}

QIcon hIcon::get(const QString &id) { return hash.value(id, QIcon()); }
