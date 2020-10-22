/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

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
