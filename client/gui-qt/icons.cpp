/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

#include "icons.h"
// Qt
#include <QIcon>
// utility
#include "shared.h"

QString current_theme;
fc_icons *fc_icons::m_instance = 0;

/************************************************************************/ /**
   Icon provider constructor
 ****************************************************************************/
fc_icons::fc_icons() {}

/************************************************************************/ /**
   Returns instance of fc_icons
 ****************************************************************************/
fc_icons *fc_icons::instance()
{
  if (!m_instance) {
    m_instance = new fc_icons;
  }
  return m_instance;
}

/************************************************************************/ /**
   Deletes fc_icons instance
 ****************************************************************************/
void fc_icons::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

/************************************************************************/ /**
   Returns icon by given name
 ****************************************************************************/
QIcon fc_icons::get_icon(const QString &id)
{
  QIcon icon;
  QString str;
  QByteArray pn_bytes;
  QByteArray png_bytes;

  str = QString("themes") + DIR_SEPARATOR + "gui-qt" + DIR_SEPARATOR;
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
QPixmap *fc_icons::get_pixmap(const QString &id)
{
  QPixmap *pm;
  bool status;
  QString str;
  QByteArray png_bytes;

  pm = new QPixmap;
  if (QPixmapCache::find(id, pm)) {
    return pm;
  }
  str = QString("themes") + DIR_SEPARATOR + "gui-qt" + DIR_SEPARATOR;
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
QString fc_icons::get_path(const QString &id)
{
  QString str;
  QByteArray png_bytes;

  str = QString("themes") + DIR_SEPARATOR + "gui-qt" + DIR_SEPARATOR
        + "icons" + DIR_SEPARATOR;
  png_bytes = QString(str + id + ".png").toLocal8Bit();

  return fileinfoname(get_data_dirs(), png_bytes.data());
}
