/*
 Copyright (c) 1996-2022 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "fonts.h"
// Qt
#include <QDirIterator>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>

// client
#include "gui_main.h"
#include "options.h"

/**
   Font provider constructor
 */
fcFont::fcFont() {}

/**
   Returns instance of fc_font
 */
fcFont *fcFont::instance()
{
  if (!m_instance) {
    m_instance = new fcFont;
  }
  return m_instance;
}

/**
   Deletes fc_icons instance
 */
void fcFont::drop()
{
  if (m_instance) {
    m_instance->releaseFonts();
    delete m_instance;
    m_instance = 0;
  }
}

/**
   Returns desired font
 */
QFont fcFont::getFont(const QString &name, double zoom) const
{
  /**
   * example: get_font("gui_qt_font_notify_label")
   */

  if (font_map.contains(name)) {
    auto font = font_map.value(name);
    if (gui_options.zoom_scale_fonts) {
      font.setPointSizeF(font.pointSizeF() * zoom);
    }
    return font;
  } else {
    return QFont();
  }
}

/**
   Initiazlizes fonts from client options
 */
void fcFont::initFonts()
{
  /**
   * default font names are:
   * gui_qt_font_notify_label and so on.
   * (full list is in options.c in client dir)
   */

  options_iterate(client_optset, poption)
  {
    if (option_type(poption) == OT_FONT) {
      auto f = QFont();
      auto s = option_font_get(poption);
      if (f.fromString(s)) {
        s = option_name(poption);
        setFont(s, f);
      }
    }
  }
  options_iterate_end;
}

/**
   Increases/decreases all fonts sizes
 */
void fcFont::setSizeAll(int new_size)
{
  options_iterate(client_optset, poption)
  {
    if (option_type(poption) == OT_FONT) {
      QFont font;
      font.fromString(option_font_get(poption));
      int old_size = font.pointSize();
      font.setPointSize(old_size + (new_size * old_size) / 100);
      QString s = font.toString();
      QByteArray ba = s.toLocal8Bit();
      option_font_set(poption, ba.data());
      gui_qt_apply_font(poption);
    }
  }
  options_iterate_end;
}

/**
   Deletes all fonts
 */
void fcFont::releaseFonts() { font_map.clear(); }

/**
   Adds new font or overwrite old one
 */
void fcFont::setFont(const QString &name, const QFont &qf)
{
  font_map.insert(name, qf);
}

/**
 *   Returns if a font is installed
 */
bool isFontInstalled(const QString &font_name)
{
  QFontDatabase database;

  return database.families().contains(font_name);
}

/**
 * Loads the fonts into the font database.
 */
void load_fonts()
{
  const auto il =
      find_files_in_path(get_data_dirs(), QStringLiteral("fonts"), false);
  if (!il.isEmpty()) {
    for (const auto &info : qAsConst(il)) {
      QDirIterator iterator(
          info.absolutePath(),
          {QStringLiteral("*.otf"), QStringLiteral("*.ttf")}, QDir::Files,
          QDirIterator::Subdirectories);
      while (iterator.hasNext()) {
        QFontDatabase::addApplicationFont(iterator.next());
      }
    }
  }
}

/**
 * Tries to choose good fonts for Freeciv21
 */
void configure_fonts()
{
  QStringList sl;
  QFont font;

  const int max = 16;
  const int default_size = 12;

  if (!isFontInstalled(QStringLiteral("Libertinus Sans"))
      && !isFontInstalled(QStringLiteral("Linux Libertine"))) {
    load_fonts();
  }

  /* Sans Serif List */
  sl << QStringLiteral("Libertinus Sans")
     << QStringLiteral("Linux Biolinum O")
     << QStringLiteral("Linux Biolinum");

  font = configure_font(fonts::default_font, sl, QFont::SansSerif, max);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_default,
               qUtf8Printable(font.toString()), 512);
  }
  font = configure_font(fonts::notify_label, sl, QFont::SansSerif,
                        default_size);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_notify_label,
               qUtf8Printable(font.toString()), 512);
  }
  font = configure_font(fonts::city_names, sl, QFont::SansSerif, max, true);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_city_names,
               qUtf8Printable(font.toString()), 512);
  }
  font = configure_font(fonts::city_productions, sl, QFont::SansSerif,
                        default_size, true);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_city_productions,
               qUtf8Printable(font.toString()), 512);
  }

  /* Monospace List */
  sl.clear();
  sl << QStringLiteral("Libertinus Mono")
     << QStringLiteral("Linux Libertine Mono O")
     << QStringLiteral("Linux Libertine Mono");

  font =
      configure_font(fonts::help_label, sl, QFont::Monospace, default_size);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_help_label,
               qUtf8Printable(font.toString()), 512);
  }
  font =
      configure_font(fonts::help_text, sl, QFont::Monospace, default_size);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_help_text,
               qUtf8Printable(font.toString()), 512);
  }
  font = configure_font(fonts::chatline, sl, QFont::Monospace, default_size);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_chatline,
               qUtf8Printable(font.toString()), 512);
  }

  /* Serif List */
  sl.clear();
  sl << QStringLiteral("Libertinus Display")
     << QStringLiteral("Linux Libertine Display O")
     << QStringLiteral("Linux Libertine Display");

  font = configure_font(fonts::reqtree_text, sl, QFont::Serif, max, true);
  if (font.exactMatch()) {
    fc_strlcpy(gui_options.gui_qt_font_reqtree_text,
               qUtf8Printable(font.toString()), 512);
  }
}

/**
   Returns long font name, sets given for for use
 */
QFont configure_font(const QString &font_name, const QStringList &sl,
                     QFont::StyleHint hint, int size, bool bold)
{
  // FIXME Qt 6: Use QFont(QStringList...)
  QFontDatabase database;

  for (auto const &str : sl) {
    if (database.families().contains(str)) {
      auto font = QFont(str, size, bold ? QFont::Bold : QFont::Normal);
      font.setStyleHint(hint);
      fcFont::instance()->setFont(font_name, font);
      return font;
    }
  }

  auto font = QFont();
  font.setStyleHint(hint);
  fcFont::instance()->setFont(font_name, font);
  return font;
}
