/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QApplication>
#include <QDir>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>
/* utility */
#include "shared.h"
#include "string_vector.h"
/* client */
#include "page_game.h"
#include "qtg_cxxside.h"
#include "themes_common.h"
// gui-qt
#include "fc_client.h"

extern QApplication *current_app();
extern QApplication *qapp;
extern QString current_theme;
Q_GLOBAL_STATIC(QString, def_app_style)
Q_GLOBAL_STATIC(QString, stylestring)

/*************************************************************************/ /**
   Loads a qt theme directory/theme_name
 *****************************************************************************/
void qtg_gui_load_theme(const char *directory, const char *theme_name)
{
  QString name;
  QString path;
  QString fake_dir;
  QString data_dir;
  QFile f;
  QString lnb = QStringLiteral("LittleFinger");
  QPalette pal;

  if (def_app_style->isEmpty()) {
    *def_app_style = QApplication::style()->objectName();
  }

  data_dir = QString(directory);

  path = data_dir + DIR_SEPARATOR + theme_name + DIR_SEPARATOR;
  name = path + "resource.qss";
  f.setFileName(name);

  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (QString(theme_name) != QStringLiteral(FC_QT_DEFAULT_THEME_NAME)) {
      qtg_gui_clear_theme();
    }
    return;
  }
  /* Stylesheet uses UNIX separators */
  fake_dir = data_dir;
  fake_dir.replace(QStringLiteral(DIR_SEPARATOR), QLatin1String("/"));
  QTextStream in(&f);
  *stylestring = in.readAll();
  stylestring->replace(lnb, fake_dir + "/" + theme_name + "/");

  if (QString(theme_name) == QStringLiteral("System")) {
    QApplication::setStyle(QStyleFactory::create(*def_app_style));
  } else {
    QStyle *fstyle = QStyleFactory::create(QStringLiteral("Fusion"));

    if (fstyle != nullptr) {
      QApplication::setStyle(fstyle);
    } else {
      QApplication::setStyle(QStyleFactory::create(*def_app_style));
    }
  }

  current_theme = theme_name;
  QPixmapCache::clear();
  current_app()->setStyleSheet(*stylestring);
  if (king()) {
    queen()->reloadSidebarIcons();
  }
  pal.setBrush(QPalette::Link, QColor(92, 170, 229));
  pal.setBrush(QPalette::LinkVisited, QColor(54, 150, 229));
  QApplication::setPalette(pal);
}

/*************************************************************************/ /**
   Clears a theme (sets default system theme)
 *****************************************************************************/
void qtg_gui_clear_theme()
{
  if (!load_theme(FC_QT_DEFAULT_THEME_NAME)) {
    /* TRANS: No full stop after the URL, could cause confusion. */
    qFatal(_("No Qt-client theme was found. For instructions on how to "
             "get one, please visit %s"),
           WIKI_URL);
    exit(EXIT_FAILURE);
  }
}

/*************************************************************************/ /**
   Each gui has its own themes directories.

   Returns an array containing these strings and sets array size in count.
   The caller is responsible for freeing the array and the paths.
 *****************************************************************************/
char **qtg_get_gui_specific_themes_directories(int *count)
{
  const struct strvec *data_dirs = get_data_dirs();
  char **directories = new char *[strvec_size(data_dirs)];
  int i = 0;

  *count = strvec_size(data_dirs);
  strvec_iterate(data_dirs, data_dir)
  {
    char buf[strlen(data_dir) + strlen("/themes/gui-qt") + 1];

    fc_snprintf(buf, sizeof(buf), "%s/themes/gui-qt", data_dir);

    directories[i++] = fc_strdup(buf);
  }
  strvec_iterate_end;

  return directories;
}

/*************************************************************************/ /**
   Return an array of names of usable themes in the given directory.
   Array size is stored in count.
   The caller is responsible for freeing the array and the names
 *****************************************************************************/
char **qtg_get_useable_themes_in_directory(const char *directory, int *count)
{
  QStringList sl, theme_list;
  char **array;
  char *data;
  QByteArray qba;
  QString name;
  QString qtheme_name;
  QDir dir;
  QFile f;

  dir.setPath(directory);
  sl << dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
  name = QString(directory);

  for (auto const &str : qAsConst(sl)) {
    f.setFileName(name + DIR_SEPARATOR + str + DIR_SEPARATOR
                  + "resource.qss");
    if (!f.exists()) {
      continue;
    }
    theme_list << str;
  }

  qtheme_name = gui_options.gui_qt_default_theme_name;
  /* move current theme on first position */
  if (theme_list.contains(qtheme_name)) {
    theme_list.removeAll(qtheme_name);
    theme_list.prepend(qtheme_name);
  }
  array = new char *[theme_list.count()];
  *count = theme_list.count();

  for (int i = 0; i < *count; i++) {
    QByteArray tn_bytes;

    qba = theme_list[i].toLocal8Bit();
    data = new char[theme_list[i].toLocal8Bit().count() + 1];
    tn_bytes = theme_list[i].toLocal8Bit();
    strcpy(data, tn_bytes.data());
    array[i] = data;
  }

  return array;
}
