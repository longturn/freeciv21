/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "fonts.h"
// Qt
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
// client
#include "gui_main.h"
#include "options.h"

/************************************************************************/ /**
   Font provider constructor
 ****************************************************************************/
fcFont::fcFont() : city_fontsize(12), prod_fontsize(12) {}

/************************************************************************/ /**
   Returns instance of fc_font
 ****************************************************************************/
fcFont *fcFont::instance()
{
  if (!m_instance) {
    m_instance = new fcFont;
}
  return m_instance;
}

/************************************************************************/ /**
   Deletes fc_icons instance
 ****************************************************************************/
void fcFont::drop()
{
  if (m_instance) {
    m_instance->releaseFonts();
    delete m_instance;
    m_instance = 0;
  }
}

/************************************************************************/ /**
   Returns desired font
 ****************************************************************************/
QFont *fcFont::getFont(const QString &name)
{
  /**
   * example: get_font("gui_qt_font_notify_label")
   */

  if (font_map.contains(name)) {
    return font_map.value(name);
  } else {
    return nullptr;
  }
}

/************************************************************************/ /**
   Initiazlizes fonts from client options
 ****************************************************************************/
void fcFont::initFonts()
{
  QFont *f;
  QString s;

  /**
   * default font names are:
   * gui_qt_font_notify_label and so on.
   * (full list is in options.c in client dir)
   */

  options_iterate(client_optset, poption)
  {
    if (option_type(poption) == OT_FONT) {
      f = new QFont;
      s = option_font_get(poption);
      if (f->fromString(s)) {
        s = option_name(poption);
        setFont(s, f);
      } else {
        delete f;
      }
    }
  }
  options_iterate_end;
  getMapfontSize();
}

/*****************************************************************************
   Increases/decreases all fonts sizes
 ****************************************************************************/
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

/************************************************************************/ /**
   Deletes all fonts
 ****************************************************************************/
void fcFont::releaseFonts()
{
  for (QFont *f : qAsConst(font_map)) {
    delete f;
  }
}

/************************************************************************/ /**
   Stores default font sizes
 ****************************************************************************/
void fcFont::getMapfontSize()
{
  city_fontsize = getFont(fonts::city_names)->pointSize();
  prod_fontsize = getFont(fonts::city_productions)->pointSize();
}

/************************************************************************/ /**
   Adds new font or overwrite old one
 ****************************************************************************/
void fcFont::setFont(const QString &name, QFont *qf)
{
  font_map.insert(name, qf);
}

/************************************************************************/ /**
   Tries to choose good fonts for freeciv-qt
 ****************************************************************************/
void configure_fonts()
{
  int max, default_size;
  QStringList sl;
  QString font_name;
  const QList<QScreen *> screens = QGuiApplication::screens();
  const QScreen *screen = screens.at(0);
  qreal logical_dpi = screen->logicalDotsPerInchX();
  qreal physical_dpi = screen->physicalDotsPerInchX();
  qreal screen_size = screen->geometry().width() / physical_dpi + 5;
  qreal scale = (physical_dpi * screen_size / (logical_dpi * 27))
                / screen->devicePixelRatio();
  QByteArray fn_bytes;

  max = qRound(scale * 16);
  default_size = qRound(scale * 14);

  /* default and help label*/
  sl << QStringLiteral("Segoe UI") << QStringLiteral("Cousine")
     << QStringLiteral("Liberation Sans") << QStringLiteral("Droid Sans")
     << QStringLiteral("Ubuntu") << QStringLiteral("Noto Sans")
     << QStringLiteral("DejaVu Sans") << QStringLiteral("Luxi Sans")
     << QStringLiteral("Lucida Sans") << QStringLiteral("Trebuchet MS")
     << QStringLiteral("Times New Roman");
  font_name = configure_font(fonts::default_font, sl, max);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_default, fn_bytes.data(), 512);
  }
  font_name = configure_font(fonts::city_names, sl, max, true);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_city_names, fn_bytes.data(), 512);
  }
  /* default for help text */
  font_name = configure_font(fonts::help_text, sl, default_size);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_help_text, fn_bytes.data(), 512);
  }
  sl.clear();

  /* notify */
  sl << QStringLiteral("Cousine") << QStringLiteral("Liberation Mono")
     << QStringLiteral("Source Code Pro")
     << QStringLiteral("Source Code Pro [ADBO]")
     << QStringLiteral("Noto Mono") << QStringLiteral("Ubuntu Mono")
     << QStringLiteral("Courier New");
  font_name = configure_font(fonts::notify_label, sl, default_size);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_notify_label, fn_bytes.data(), 512);
  }

  /* standard for chat */
  font_name = configure_font(fonts::chatline, sl, default_size);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_chatline, fn_bytes.data(), 512);
  }

  /* City production */
  sl.clear();
  sl << QStringLiteral("Arimo") << QStringLiteral("Play")
     << QStringLiteral("Tinos") << QStringLiteral("Ubuntu")
     << QStringLiteral("Times New Roman") << QStringLiteral("Droid Sans")
     << QStringLiteral("Noto Sans");
  font_name =
      configure_font(fonts::city_productions, sl, default_size, true);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_city_productions, fn_bytes.data(),
               512);
  }
  /* Reqtree */
  sl.clear();
  sl << QStringLiteral("Papyrus") << QStringLiteral("Segoe Script")
     << QStringLiteral("Comic Sans MS") << QStringLiteral("Droid Sans")
     << QStringLiteral("Noto Sans") << QStringLiteral("Ubuntu");
  font_name = configure_font(fonts::reqtree_text, sl, max, true);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_reqtree_text, fn_bytes.data(), 512);
  }
}

/************************************************************************/ /**
   Returns long font name, sets given for for use
 ****************************************************************************/
QString configure_font(const QString &font_name, const QStringList &sl,
                       int size, bool bold)
{
  QFontDatabase database;
  QFont *f;

  for (auto const &str : sl) {
    if (database.families().contains(str)) {
      QByteArray fn_bytes;

      f = new QFont(str, size);
      if (bold) {
        f->setBold(true);
      }
      fcFont::instance()->setFont(font_name, f);
      fn_bytes = f->toString().toLocal8Bit();

      return fn_bytes.data();
    }
  }
  return QString();
}
