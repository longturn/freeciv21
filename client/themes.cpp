/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// Qt
#include <QApplication>
#include <QDir>
#include <QMetaEnum>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>

// utility
#include "shared.h"

// client
#include "chatline.h"
#include "colors_common.h"
#include "fc_client.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "themes_common.h"
#include <array>

extern QString current_theme;
Q_GLOBAL_STATIC(QString, def_app_style)
Q_GLOBAL_STATIC(QString, stylestring)

namespace {

/**
   Loads chat color substitutions from theme settings.
 */
void load_chat_colors(QSettings &settings)
{
  settings.beginGroup("color_mapping");
  QHash<QString, QString> colors;

  for (auto k : settings.childKeys()) {
    auto val = settings.value(k).toString();
    if (!QColor::isValidColor(val)) {
      qWarning() << "color invalid: " << val;
      continue;
    }
    colors[QStringLiteral("#") + k] = val;
  }

  set_chat_colors(colors);
  settings.endGroup();
}

static std::array<QColor, COLOR_LAST> diag_colors = {};

/**
 * Loads diagram (tech tree) colors from theme settings
 */
void load_diag_colors(QSettings &settings)
{
  settings.beginGroup("diagram_colors");

  for (int i = 0; i < COLOR_LAST; ++i) {
    const auto name = color_std_name(static_cast<color_std>(i));
    if (settings.contains(name)) {
      diag_colors[i].setNamedColor(settings.value(name).toString());
    } else {
      diag_colors[i] = QColor(); // Invalid, fetch from tileset
    }
  }

  settings.endGroup();
}

/**
   Loads a palette from theme settings.
 */
QPalette load_palette(QSettings &settings)
{
  settings.beginGroup("general_colors");
  QPalette pal;

  auto meta = QMetaEnum::fromType<QPalette::ColorRole>();
  for (int i = 0; i < meta.keyCount(); ++i) {
    const auto name = meta.key(i);
    const auto role = static_cast<QPalette::ColorRole>(meta.value(i));
    if (role == QPalette::NColorRoles || role == QPalette::NoRole) {
      continue;
    }

    if (!settings.contains(name)) {
      qWarning() << "missing color" << name;
      continue;
    }

    const auto val = settings.value(name).toString();
    if (!QColor::isValidColor(val)) {
      qWarning() << "color invalid:" << val;
      continue;
    }

    pal.setBrush(role, QColor(val));
  }

  settings.endGroup();
  return pal;
}

} // namespace

/**
   Loads a qt theme directory/theme_name
 */
void gui_load_theme(const QString &directory, const QString &theme_name)
{
  QString fake_dir;
  QString data_dir;
  QFile f;
  QString lnb = QStringLiteral("LittleFinger");

  if (def_app_style->isEmpty()) {
    *def_app_style = QApplication::style()->objectName();
  }

  data_dir = QString(directory);

  f.setFileName(data_dir + "/" + theme_name + "/resource.qss");

  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (QString(theme_name) != QStringLiteral(FC_QT_DEFAULT_THEME_NAME)) {
      gui_clear_theme();
    }
    return;
  }
  // Stylesheet uses UNIX separators
  fake_dir = data_dir;
  QTextStream in(&f);
  *stylestring = in.readAll();
  stylestring->replace(lnb, fake_dir + "/" + theme_name + "/");

  if (theme_name == QStringLiteral("System")) {
    QApplication::setStyle(QStyleFactory::create(*def_app_style));
  } else {
    QStyle *fstyle = QStyleFactory::create(QStringLiteral("Fusion"));

    if (fstyle != nullptr) {
      QApplication::setStyle(fstyle);
    } else {
      QApplication::setStyle(QStyleFactory::create(*def_app_style));
    }
  }

  QSettings settings(data_dir + "/" + theme_name + "/theme.conf",
                     QSettings::IniFormat);
  load_chat_colors(settings);
  load_diag_colors(settings);

  current_theme = theme_name;
  QPixmapCache::clear();
  if (theme_name != QStringLiteral("System")) {
    // Need to do this *before* changing the stylesheet.
    // FIXME How to reset to the system palette?
    QApplication::setPalette(load_palette(settings));
  }
  qApp->setStyleSheet(*stylestring);
  if (king()) {
    queen()->reloadSidebarIcons();
  }
}

/**
   Clears a theme (sets default system theme)
 */
void gui_clear_theme()
{
  if (!load_theme(FC_QT_DEFAULT_THEME_NAME)) {
    // TRANS: No full stop after the URL, could cause confusion.
    qFatal(_("No Qt-client theme was found. For instructions on how to "
             "get one, please visit %s"),
           WIKI_URL);
    exit(EXIT_FAILURE);
  }
}

/**
   Each gui has its own themes directories.

   Returns an array containing these strings and sets array size in count.
   The caller is responsible for freeing the array and the paths.
 */
QStringList get_gui_specific_themes_directories(int *count)
{
  auto data_dirs = get_data_dirs();
  QStringList directories;

  *count = data_dirs.size();
  for (const auto &data_dir : data_dirs) {
    directories.append(QStringLiteral("%1/themes").arg(data_dir));
  }

  return directories;
}

/**
   Return an array of names of usable themes in the given directory.
   Array size is stored in count.
   The caller is responsible for freeing the array and the names
 */
QStringList get_useable_themes_in_directory(QString &directory)
{
  QStringList sl, theme_list, array;
  QByteArray qba;
  QString name;
  QString qtheme_name;
  QDir dir;
  QFile f;

  dir.setPath(directory);
  sl << dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
  name = QString(directory);

  for (auto const &str : qAsConst(sl)) {
#ifdef Q_OS_WIN
    // "System" creates endless problems on Windows, see #1287 #756
    if (str == QStringLiteral("System")) {
      continue;
    }
#endif
    f.setFileName(name + "/" + str + "/resource.qss");
    if (!f.exists()) {
      continue;
    }
    theme_list << str;
  }

  qtheme_name = gui_options->gui_qt_default_theme_name;
  // move current theme on first position
  if (theme_list.contains(qtheme_name)) {
    theme_list.removeAll(qtheme_name);
    theme_list.prepend(qtheme_name);
  }
  return theme_list;
}

/**
 * Gets a diagram color. They come from the tileset, but the theme can
 * override them.
 */
QColor get_diag_color(color_std c)
{
  return diag_colors[c].isValid() ? diag_colors[c] : get_color(tileset, c);
}
