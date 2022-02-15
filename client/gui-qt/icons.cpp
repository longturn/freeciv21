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
#include <QApplication>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPalette>
#include <QSvgRenderer>
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

namespace {

/**
   Custom icon engine that accepts paths to SVG files and renders them,
   substituting the string "#000" (including quotes) for the current global
   palette's ButtonText color.
 */
class icon_engine : public QIconEngine {
public:
  explicit icon_engine(const QString &path) : path(path) {}

  icon_engine *clone() const override { return new icon_engine(path); }

  void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
             QIcon::State state) override
  {
    // For now, only use one color variant for icons.
    Q_UNUSED(mode)
    Q_UNUSED(state)

    auto col = QApplication::palette().color(QPalette::ButtonText);
    QString key = path + "-" + QString::number(rect.width()) + "-"
                  + QString::number(rect.height()) + "-" + col.name();
    QPixmap pix;
    if (!QPixmapCache::find(key, &pix)) {
      pix = QPixmap(rect.width(), rect.height());
      pix.fill(Qt::transparent);
      if (!path.isEmpty()) {
        QSvgRenderer renderer;
        QFile f(path);
        QByteArray bytes;
        if (f.open(QIODevice::ReadOnly)) {
          bytes = f.readAll();
        }
        if (!bytes.isEmpty()) {
          bytes.replace("\"#000\"",
                        QStringLiteral("\"%1\"").arg(col.name()).toLatin1());
        }
        renderer.load(bytes);
        QPainter p(&pix);
        renderer.render(&p, QRect(0, 0, rect.width(), rect.height()));
      }
      QPixmapCache::insert(key, pix);
    }
    painter->drawPixmap(rect.topLeft(), pix);
  }

  QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                 QIcon::State state) override
  {
    QPixmap pix(size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
    return pix;
  }

private:
  QString path;
};
} // namespace

/**
   Returns icon by given name
 */
QIcon fcIcons::getIcon(const QString &id)
{
  QIcon icon;

  // Try custom icon from theme.
  icon.addFile(
      fileinfoname(get_data_dirs(),
                   qUtf8Printable(QStringLiteral("themes/gui-qt/%1/%2.svg")
                                      .arg(current_theme, id))));
  if (!icon.isNull()) {
    return icon;
  }

  // Try SVG from icons dir.
  QString path = fileinfoname(
      get_data_dirs(),
      qUtf8Printable(QStringLiteral("themes/gui-qt/icons/%1.svg").arg(id)));
  if (!path.isEmpty()) {
    // Special icon engine for theme colorization.
    return QIcon(new icon_engine(path));
  }

  // Fall back to PNG.
  icon.addFile(
      fileinfoname(get_data_dirs(),
                   qUtf8Printable(QStringLiteral("themes/gui-qt/%1/%2.png")
                                      .arg(current_theme, id))));
  if (!icon.isNull()) {
    return icon;
  }

  icon.addFile(fileinfoname(
      get_data_dirs(),
      qUtf8Printable(QStringLiteral("themes/gui-qt/icons/%1.png").arg(id))));

  return icon;
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
